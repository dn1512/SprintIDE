#include "gdb_dap_provider.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QDebug>

namespace SprintIDE {

QList<ToolchainId> GdbDapProvider::supportedIds() const
{
    return { ToolchainId::GdbDap };
}

// ============================================================
// 内部：探测候选 gdb 是否满足"可作为 DAP adapter"的条件
//   1. 路径非空、不是 GUI shim
//   2. 文件存在 / 是普通文件 / 可执行
//   3. gdb --version 输出含 "GNU gdb"，主版本 >= 14
//
// 若 out 非空，成功时填充 executablePath / version / source
// ============================================================
static bool probeGdb(const QString& path, ToolchainResolution* out = nullptr)
{
    if (path.isEmpty()) return false;

    // 跳过 macOS GUI shim（与 CppProjectModel 的规则一致）
    if (path.contains(".app/Contents/MacOS/")) {
        qDebug() << "[GdbDapProvider] 跳过 GUI shim:" << path;
        return false;
    }

    QFileInfo fi(path);
    if (!fi.exists())      { return false; }
    if (!fi.isFile())      { return false; }
    if (!fi.isExecutable()){ return false; }

    QProcess proc;
    proc.setProgram(path);
    proc.setArguments({"--version"});
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start();

    if (!proc.waitForStarted(2000)) {
        proc.kill();
        proc.waitForFinished(200);
        return false;
    }
    if (!proc.waitForFinished(3000)) {
        proc.kill();
        proc.waitForFinished(200);
        return false;
    }

    QString stdoutText = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
    if (stdoutText.isEmpty()) return false;

    // 期望形如：
    //   "GNU gdb (GDB) 14.1"
    //   "GNU gdb (GDB) 15.2 (Apple GNU gdb) ..."
    //   "GNU gdb (Ubuntu 15.0.50...) 15.0.50..."
    // 用宽松正则抓主版本号
    static QRegularExpression verRe(
        R"(GNU\s+gdb[^\r\n]*?(\d+)\.(\d+))",
        QRegularExpression::CaseInsensitiveOption);

    QRegularExpressionMatch m = verRe.match(stdoutText);
    if (!m.hasMatch()) {
        qDebug() << "[GdbDapProvider] 不是 GNU gdb 输出:"
                 << path << "->" << stdoutText.left(80);
        return false;
    }

    const int major = m.captured(1).toInt();
    if (major < 14) {
        qDebug() << "[GdbDapProvider] gdb 版本过低（需要 14+）:"
                 << path << "-> 主版本" << major;
        return false;
    }

    if (out) {
        out->found = true;
        out->executablePath = fi.absoluteFilePath();
        // 用整行最前面的那一段作为 version 摘要
        out->version = stdoutText.split('\n').first().trimmed();
        out->source = "gdb-system";
    }
    return true;
}

// ============================================================
// 内部：按顺序尝试候选列表，返回第一个通过 probeGdb 的结果
// ============================================================
static ToolchainResolution tryCandidates(const QStringList& candidates)
{
    for (const QString& cand : candidates) {
        ToolchainResolution res;
        if (probeGdb(cand, &res)) {
            qDebug() << "[GdbDapProvider] ✓ 使用 gdb:" << cand
                     << "version:" << res.version;
            return res;
        }
    }
    return {};
}

// ============================================================
// 主逻辑：resolve
// ============================================================
ToolchainResolution GdbDapProvider::resolve(ToolchainId id) const
{
    Q_UNUSED(id);

    // ---------------- 1. 用户显式配置（环境变量） ----------------
    {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        const QString explicitPath = env.value("GDB_DAP_PATH").trimmed();
        if (!explicitPath.isEmpty()) {
            ToolchainResolution res;
            if (probeGdb(explicitPath, &res)) {
                res.source = "user-configured";
                return res;
            }
            qWarning() << "[GdbDapProvider] GDB_DAP_PATH 配置无效:" << explicitPath;
            // 继续往下走，让系统 PATH 兜底
        }
    }

    // ---------------- 2. 系统 PATH ----------------
    {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        const QStringList pathDirs =
            env.value("PATH").split(QDir::listSeparator(), Qt::SkipEmptyParts);

        QStringList names;
#ifdef Q_OS_WIN
        names << "gdb.exe" << "gdb";
#else
        names << "gdb";
#endif

        QStringList candidates;
        for (const QString& dir : pathDirs) {
            // 跳过 Qt / IDE 工具链路径，避免误取嵌入式 gdb
            if (dir.contains("Qt", Qt::CaseInsensitive)) continue;
            for (const QString& n : names) {
                candidates << QDir(dir).absoluteFilePath(n);
            }
        }
        auto res = tryCandidates(candidates);
        if (res.found) return res;
    }

    // ---------------- 3. 平台常见路径 ----------------
#ifdef Q_OS_MAC
    {
        QStringList candidates = {
            "/opt/homebrew/bin/gdb",                // Apple Silicon
            "/usr/local/bin/gdb",                   // Intel Mac
            "/opt/homebrew/opt/gdb/bin/gdb",
            "/usr/local/opt/gdb/bin/gdb",
        };
        auto res = tryCandidates(candidates);
        if (res.found) return res;
    }
#elif defined(Q_OS_LINUX)
    {
        QStringList candidates = {
            "/usr/bin/gdb",
            "/usr/local/bin/gdb",
            "/snap/bin/gdb",
        };
        auto res = tryCandidates(candidates);
        if (res.found) return res;
    }
#elif defined(Q_OS_WIN)
    {
        QStringList candidates = {
            "C:/msys64/mingw64/bin/gdb.exe",
            "C:/mingw64/bin/gdb.exe",
            "C:/Program Files (x86)/mingw-w64/bin/gdb.exe",
            "C:/Program Files/mingw-w64/bin/gdb.exe",
        };
        auto res = tryCandidates(candidates);
        if (res.found) return res;
    }
#endif

    // ---------------- 4. 全部失败 ----------------
    ToolchainResolution fail;
    fail.found = false;
    fail.errorMessage =
        QStringLiteral("未找到支持 --interpreter=dap 的 GDB（需要 GDB 14+）");
    qDebug() << "[GdbDapProvider]" << fail.errorMessage;
    return fail;
}

ToolchainInstallHint GdbDapProvider::installHint(ToolchainId id) const
{
    Q_UNUSED(id);

    ToolchainInstallHint hint;
    hint.title = QStringLiteral("需要 GDB 14+ 调试适配器");

#ifdef Q_OS_MAC
    hint.description = QStringLiteral(
        "SprintIDE 在 Linux 上优先使用 GDB 的 DAP 模式调试 C++。\n"
        "macOS 上推荐继续使用 lldb-dap（通常随 VS Code 的 CodeLLDB 扩展提供）。\n\n"
        "如确实需要 GDB，可通过 Homebrew 安装（可能需要手动 codesign）：");
    hint.command = QStringLiteral("brew install gdb");
    hint.detailUrl = QStringLiteral("https://www.sourceware.org/gdb/");

#elif defined(Q_OS_LINUX)
    hint.description = QStringLiteral(
        "SprintIDE 使用 gdb --interpreter=dap 为 C++ 提供调试功能。\n"
        "当前系统未检测到 GDB 14 或以上版本。\n\n"
        "请通过发行版包管理器安装：");
    hint.command = QStringLiteral("sudo apt install gdb  # 或对应发行版包名");
    hint.detailUrl = QStringLiteral("https://www.sourceware.org/gdb/");

#else
    hint.description = QStringLiteral(
        "SprintIDE 需要 GDB 14+ 提供 C++ 调试功能。\n"
        "请从 MinGW-w64 / MSYS2 安装支持 DAP 的 GDB。");
    hint.command = QString();
    hint.detailUrl = QStringLiteral("https://www.sourceware.org/gdb/");
#endif

    return hint;
}

} // namespace SprintIDE
