// src/deps/lldb_dap_provider.cpp
#include "lldb_dap_provider.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QDebug>
#include <algorithm>

namespace SprintIDE {

// ============================================================
// 路径辅助：内嵌工具链 / 用户数据工具链
// ============================================================

/**
 * @brief 计算内嵌工具链的根目录
 *
 * 返回：<bundle>/Contents/Frameworks/lldb（macOS）
 *      <app>/lldb（Windows）
 *      <app>/../lib/lldb（Linux）
 * 若目录不存在（开发期常见），调用方应静默跳过。
 */
static QString bundledToolchainRoot()
{
    QDir appDir(QCoreApplication::applicationDirPath());

#ifdef Q_OS_MAC
    // applicationDirPath() = .../SprintIDE.app/Contents/MacOS
    // 目标：.../SprintIDE.app/Contents/Frameworks/lldb
    QDir macosDir = appDir;
    if (macosDir.dirName() == "MacOS") {
        macosDir.cdUp();          // → Contents
        return macosDir.absoluteFilePath("Frameworks/lldb");
    }
    // 非 bundle 运行（例如 Qt Creator 直接跑）：无内嵌
    return QString();
#elif defined(Q_OS_WIN)
    // applicationDirPath() = .../SprintIDE
    // 目标：.../SprintIDE/lldb
    return appDir.absoluteFilePath("lldb");
#else
    // Linux：applicationDirPath() = .../SprintIDE/bin
    // 目标：.../SprintIDE/lib/lldb
    return appDir.absoluteFilePath("../lib/lldb");
#endif
}

/**
 * @brief 计算用户数据目录下的工具链根目录
 *
 * 这是未来"on-demand 下载"的落地点，现在就被 resolve() 读取，
 * 允许开发者手动放工具链到这里。
 */
static QString userDataToolchainRoot()
{
    QString base;

#ifdef Q_OS_MAC
    base = QDir::homePath() + "/Library/Application Support/SprintIDE";
#elif defined(Q_OS_WIN)
    base = qEnvironmentVariable("APPDATA") + "/SprintIDE";
#else
    base = QDir::homePath() + "/.local/share/SprintIDE";
#endif

    return base + "/toolchains/lldb";
}

// ============================================================
// 内部：检查二进制"看起来像" DAP 适配器（不做 --version 校验）
//   适用场景：codelldb 不支持 --version
// ============================================================
static bool looksLikeBinary(const QString& path, QString* reasonOut = nullptr)
{
    if (path.isEmpty()) {
        if (reasonOut) *reasonOut = "empty path";
        return false;
    }

    // 跳过 macOS GUI shim
    if (path.contains(".app/Contents/MacOS/")) {
        if (reasonOut) *reasonOut = "GUI shim";
        return false;
    }

    QFileInfo fi(path);
    if (!fi.exists())      { if (reasonOut) *reasonOut = "not exists"; return false; }
    if (!fi.isFile())      { if (reasonOut) *reasonOut = "not a file"; return false; }
    if (!fi.isExecutable()){ if (reasonOut) *reasonOut = "not executable"; return false; }
    if (fi.size() < 100 * 1024) {
        if (reasonOut) *reasonOut = QString("too small (%1 bytes)").arg(fi.size());
        return false;
    }
    return true;
}

// ============================================================
// 内部：尝试 --version（部分二进制支持），用于补充 version 字段
//   失败不阻塞，只影响 version 字段内容
// ============================================================
static QString tryGetVersion(const QString& path)
{
    QProcess proc;
    proc.setProgram(path);
    proc.setArguments({"--version"});
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start();
    if (!proc.waitForStarted(1500)) { proc.kill(); proc.waitForFinished(200); return {}; }
    if (!proc.waitForFinished(1500)) { proc.kill(); proc.waitForFinished(200); return {}; }

    QString out = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
    // 过滤明显是错误消息的输出（codelldb 对 --version 返回 "error: unexpected argument"）
    if (out.contains("error", Qt::CaseInsensitive) ||
        out.contains("unexpected", Qt::CaseInsensitive) ||
        out.contains("usage", Qt::CaseInsensitive) ||
        out.contains("invalid", Qt::CaseInsensitive)) {
        return {};
    }
    // codelldb 只回显名字，太短，就认为没版本
    if (out.length() < 5 || out == "codelldb" || out == "lldb-dap") return {};
    return out.split('\n').first();
}

// ============================================================
// 内部：逐个候选尝试，模式 = "宽松"（只要 looksLikeBinary 通过就接受）
// ============================================================
static ToolchainResolution tryCandidatesRelaxed(const QStringList& candidates,
                                                const QString& sourceLabel)
{
    for (const QString& cand : candidates) {
        QString reason;
        if (!looksLikeBinary(cand, &reason)) {
            if (reason != "not exists") {
                qDebug() << "[LldbDapProvider] skip" << cand << "->" << reason;
            }
            continue;
        }

        ToolchainResolution res;
        res.found = true;
        res.executablePath = cand;
        res.version = tryGetVersion(cand);  // 可能为空，可接受
        res.source = sourceLabel;
        return res;
    }
    return {};
}

// ============================================================
// 内部：枚举某个根目录下的候选可执行文件名
//   顺序（重要）：codelldb → lldb-dap → lldb-vscode
//   理由：codelldb 是静态二进制，无需处理 dylib 依赖，最适合内嵌/拷贝
// ============================================================
static QStringList candidatesUnderRoot(const QString& root)
{
    if (root.isEmpty()) return {};

#ifdef Q_OS_WIN
    const QString exe = ".exe";
#else
    const QString exe;
#endif

    return {
        root + "/bin/codelldb"    + exe,
        root + "/bin/lldb-dap"    + exe,
        root + "/bin/lldb-vscode" + exe,
        // 兼容：有些用户直接把 codelldb 放在根目录（无 bin 子目录）
        root + "/codelldb"        + exe,
    };
}

// ============================================================
// 内部：扫描 ~/.vscode/extensions/vadimcn.vscode-lldb-*/
//   按版本号排序（新 → 旧），返回所有候选（不判断是否存在）
// ============================================================
static QStringList enumerateCodeLldbCandidates()
{
    QStringList candidates;

    // 从目录名提取版本号并排序
    struct Ext { QString path; int verKey; };

    QString home = QDir::homePath();
    QStringList bases = {
        home + "/.vscode/extensions",
        home + "/.vscode-insiders/extensions",
        home + "/.vscode-oss/extensions",
        home + "/.cursor/extensions",
        home + "/.windsurf/extensions",
    };

    QList<Ext> exts;

    for (const QString& base : bases) {
        QDir baseDir(base);
        if (!baseDir.exists()) continue;

        QStringList dirs = baseDir.entryList(
            {"vadimcn.vscode-lldb-*"},
            QDir::Dirs | QDir::NoDotAndDotDot);

        for (const QString& dir : dirs) {
            // 解析版本号：目录名形如 vadimcn.vscode-lldb-1.12.3
            QRegularExpression re(R"(-(\d+)\.(\d+)\.(\d+)$)");
            auto m = re.match(dir);
            int verKey = 0;
            if (m.hasMatch()) {
                int major = m.captured(1).toInt();
                int minor = m.captured(2).toInt();
                int patch = m.captured(3).toInt();
                verKey = major * 1000000 + minor * 1000 + patch;
            }
            Ext e;
            e.path = base + "/" + dir;
            e.verKey = verKey;
            exts.append(e);
        }
    }

    // 版本号从大到小排序
    std::sort(exts.begin(), exts.end(),
              [](const Ext& a, const Ext& b) { return a.verKey > b.verKey; });

    // 每个扩展目录下，尝试几个可能的入口
    for (const Ext& e : exts) {
        // 新版本布局
        candidates << e.path + "/adapter/codelldb";
        candidates << e.path + "/adapter/lldb-dap";
        // 旧版本布局
        candidates << e.path + "/lldb/bin/codelldb";
        candidates << e.path + "/lldb/bin/lldb-dap";
        candidates << e.path + "/lldb/bin/lldb-vscode";
    }

    return candidates;
}

// ============================================================
// LldbDapProvider::supportedIds
// ============================================================
QList<ToolchainId> LldbDapProvider::supportedIds() const
{
    return { ToolchainId::LldbDap };
}

// ============================================================
// resolveBundled —— 从 app bundle 内嵌目录解析
// ============================================================
ToolchainResolution LldbDapProvider::resolveBundled() const
{
    const QString root = bundledToolchainRoot();
    if (root.isEmpty()) return {};

    // 内嵌目录不存在（开发期常见）时静默返回，不产生日志噪音
    QFileInfo rootInfo(root);
    if (!rootInfo.exists() || !rootInfo.isDir()) {
        return {};
    }

    QStringList candidates = candidatesUnderRoot(root);
    ToolchainResolution res = tryCandidatesRelaxed(candidates, "bundled");
    if (res.found) {
        qDebug() << "[LldbDapProvider] 使用内嵌工具链:" << res.executablePath
                 << "(root:" << root << ")";
    }
    return res;
}

// ============================================================
// resolveUserData —— 从用户数据目录解析
// ============================================================
ToolchainResolution LldbDapProvider::resolveUserData() const
{
    const QString root = userDataToolchainRoot();
    if (root.isEmpty()) return {};

    QFileInfo rootInfo(root);
    if (!rootInfo.exists() || !rootInfo.isDir()) {
        return {};
    }

    QStringList candidates = candidatesUnderRoot(root);
    ToolchainResolution res = tryCandidatesRelaxed(candidates, "user-data");
    if (res.found) {
        qDebug() << "[LldbDapProvider] 使用用户数据目录工具链:"
                 << res.executablePath << "(root:" << root << ")";
    }
    return res;
}

// ============================================================
// 主逻辑：resolve
// ============================================================
ToolchainResolution LldbDapProvider::resolve(ToolchainId id) const
{
    Q_UNUSED(id);

    // ---------------- 0. 用户显式配置（环境变量） ----------------
    {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        QString explicitPath = env.value("LLDB_DAP_PATH").trimmed();
        if (!explicitPath.isEmpty()) {
            auto res = tryCandidatesRelaxed({ explicitPath }, "user-configured");
            if (res.found) return res;
            qWarning() << "[LldbDapProvider] LLDB_DAP_PATH 无效:" << explicitPath;
        }
    }

    // ---------------- 1. 内嵌工具链（打包后才有） ----------------
    {
        auto res = resolveBundled();
        if (res.found) return res;
    }

    // ---------------- 2. 用户数据目录（为 on-demand 下载预留） ----------------
    {
        auto res = resolveUserData();
        if (res.found) return res;
    }

    // ---------------- 3. 系统 PATH ----------------
    {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        QStringList pathDirs = env.value("PATH").split(QDir::listSeparator(),
                                                       Qt::SkipEmptyParts);
        QStringList names = { "lldb-dap", "lldb-vscode", "codelldb" };
        QStringList candidates;
        for (const QString& dir : pathDirs) {
            for (const QString& n : names) {
                candidates << QDir(dir).absoluteFilePath(n);
            }
        }
        auto res = tryCandidatesRelaxed(candidates, "system");
        if (res.found) return res;
    }

    // ---------------- 4. Homebrew LLVM / LLDB 独立 formula ----------------
#ifdef Q_OS_MAC
    {
        QStringList brewBases = {
            "/opt/homebrew/opt/lldb/bin",   // Apple Silicon（独立 formula）
            "/opt/homebrew/opt/llvm/bin",   // Apple Silicon（旧版）
            "/usr/local/opt/lldb/bin",      // Intel（独立 formula）
            "/usr/local/opt/llvm/bin"       // Intel（旧版）
        };
        QStringList names = { "lldb-dap", "lldb-vscode" };
        QStringList candidates;
        for (const QString& base : brewBases) {
            for (const QString& n : names) {
                candidates << base + "/" + n;
            }
        }
        auto res = tryCandidatesRelaxed(candidates, "homebrew");
        if (res.found) return res;
    }
#endif

    // ---------------- 5. VS Code CodeLLDB 扩展（按版本从新到旧） ----------------
    {
        QStringList candidates = enumerateCodeLldbCandidates();
        auto res = tryCandidatesRelaxed(candidates, "vscode-codelldb");
        if (res.found) {
            qDebug() << "[LldbDapProvider] 从 CodeLLDB 扩展找到:" << res.executablePath;
            return res;
        }
    }

    // ---------------- 6. Xcode / CommandLineTools ----------------
#ifdef Q_OS_MAC
    {
        QStringList candidates = {
            "/Applications/Xcode.app/Contents/SharedFrameworks/LLDB.framework/Resources/lldb-dap",
            "/Applications/Xcode.app/Contents/SharedFrameworks/LLDB.framework/Resources/lldb-vscode",
            "/Library/Developer/CommandLineTools/usr/bin/lldb-dap",
            "/Library/Developer/CommandLineTools/usr/bin/lldb-vscode"
        };
        auto res = tryCandidatesRelaxed(candidates, "xcode");
        if (res.found) return res;
    }
#endif

    // ---------------- 7. Linux 常见位置 ----------------
#ifdef Q_OS_LINUX
    {
        QStringList candidates = {
            "/usr/bin/lldb-dap", "/usr/bin/lldb-vscode",
            "/usr/local/bin/lldb-dap", "/usr/local/bin/lldb-vscode",
            "/snap/bin/lldb-dap"
        };
        auto res = tryCandidatesRelaxed(candidates, "linux-system");
        if (res.found) return res;
    }
#endif

    ToolchainResolution fail;
    fail.found = false;
    fail.errorMessage = "未找到 lldb-dap / lldb-vscode / codelldb";
    return fail;
}

// ============================================================
// installHint
// ============================================================
ToolchainInstallHint LldbDapProvider::installHint(ToolchainId id) const
{
    Q_UNUSED(id);

    ToolchainInstallHint hint;
    hint.title = QStringLiteral("需要 LLDB 调试适配器");

#ifdef Q_OS_MAC
    hint.description = QStringLiteral(
        "SprintIDE 使用 lldb-dap / codelldb 为 C++ 提供调试功能。\n"
        "当前系统未检测到该组件。\n\n"
        "最快的获取方式：安装 VS Code 的 CodeLLDB 扩展。");
    hint.command = QStringLiteral("code --install-extension vadimcn.vscode-lldb");
    hint.detailUrl = QStringLiteral("https://marketplace.visualstudio.com/items?itemName=vadimcn.vscode-lldb");
#elif defined(Q_OS_LINUX)
    hint.description = QStringLiteral(
        "SprintIDE 使用 lldb-dap 为 C++ 提供调试功能。\n"
        "当前系统未检测到该组件。\n\n"
        "请通过你的发行版包管理器安装：");
    hint.command = QStringLiteral("sudo apt install lldb  # 或对应发行版包名");
    hint.detailUrl = QStringLiteral("https://lldb.llvm.org/");
#else
    hint.description = QStringLiteral("SprintIDE 需要 lldb-dap 组件提供 C++ 调试功能。");
    hint.command = QString();
    hint.detailUrl = QStringLiteral("https://llvm.org/");
#endif

    return hint;
}

} // namespace SprintIDE
