#include "lsp_manager.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>

#include "utils/java_utils.h"
#include "builder/cpp_builder.h"
#include "builder/java_builder.h"
#include "python/python_dependency_manager.h"
#include "python/python_project_model.h"
#include "project_manager.h"
#include "python/python_run_config.h"

namespace SprintIDE {

// 构造函数
LSPManager::LSPManager(QObject *parent)
    : QObject(parent)
{
    qDebug() << "[LSP Manager] 已创建";
}

// 析构函数
LSPManager::~LSPManager()
{
    shutdownAll();
    qDebug() << "[LSP Manager] 已销毁";
}

// 初始化
void LSPManager::initialize()
{
    if (m_initialized) {
        return;
    }

    // 构建默认配置
    buildDefaultConfigs();

    m_initialized = true;

    qDebug() << "[LSP Manager] 已初始化，支持" << m_configs.size() << "种语言的 LSP";
}

// 构建内置 LSP 服务器配置
void LSPManager::buildDefaultConfigs()
{
    // 1. C/C++ - clangd
    {
        LSPConfig config;
        config.languageId = "cpp";
        config.serverName = "clangd";
        config.command = "clangd";
        config.arguments = {
            "--background-index",
            "--clang-tidy",
            "--completion-style=detailed",
            "--header-insertion=iwyu"
        };
        config.hasCompileCommands = true;
        m_configs[config.languageId] = config;
    }

    // 2. Python - pylsp
    {
        LSPConfig config;
        config.languageId = "python";
        config.serverName = "pyright";
        config.command = "pyright-langserver";
        config.arguments = {"--stdio"};
        config.hasCompileCommands = false;
        m_configs[config.languageId] = config;
    }

    // 3. JavaScript / TypeScript - typescript-language-server
    {
        LSPConfig config;
        config.languageId = "javascript";
        config.serverName = "typescript-language-server";
        config.command = "typescript-language-server";
        config.arguments = {"--stdio"};
        config.hasCompileCommands = false;
        m_configs[config.languageId] = config;

        // TypeScript 复用同一服务器
        LSPConfig tsConfig = config;
        tsConfig.languageId = "typescript";
        m_configs[tsConfig.languageId] = tsConfig;
    }

    // 4. Go - gopls
    {
        LSPConfig config;
        config.languageId = "go";
        config.serverName = "gopls";
        config.command = "gopls";
        config.arguments = {};
        config.hasCompileCommands = false;
        m_configs[config.languageId] = config;
    }

    // 5. Rust - rust-analyzer
    {
        LSPConfig config;
        config.languageId = "rust";
        config.serverName = "rust-analyzer";
        config.command = "rust-analyzer";
        config.arguments = {};
        config.hasCompileCommands = false;
        m_configs[config.languageId] = config;
    }

    // 6. Java - jdtls (需要特殊配置，这里仅作占位)
    {
        LSPConfig config;
        config.languageId = "java";
        config.serverName = "jdtls";
        config.command = "jdtls";  // 实际可能是脚本，这里简化
        config.arguments = {};
        config.hasCompileCommands = false;
        m_configs[config.languageId] = config;
    }

    // 7. C# - omnisharp (需要特殊配置)
    {
        LSPConfig config;
        config.languageId = "csharp";
        config.serverName = "omnisharp";
        config.command = "omnisharp";
        config.arguments = {};
        config.hasCompileCommands = false;
        m_configs[config.languageId] = config;
    }

    // 8. PHP - intelephense (需要 npm 安装)
    {
        LSPConfig config;
        config.languageId = "php";
        config.serverName = "intelephense";
        config.command = "intelephense";
        config.arguments = {"--stdio"};
        config.hasCompileCommands = false;
        m_configs[config.languageId] = config;
    }

    qDebug() << "[LSP Manager] 已加载" << m_configs.size() << "个 LSP 服务器配置";
}


/**
 * @brief 向上查找项目根目录（包含 pom.xml 或 build.gradle 的目录）
 */
static QString findProjectRoot(const QString& startPath)
{
    QDir dir(startPath);
    while (!dir.isRoot()) {
        if (dir.exists("compile_commands.json") ||
            dir.exists("CMakeLists.txt")        ||
            dir.exists("pom.xml")               ||
            dir.exists("build.gradle")          ||
            dir.exists("settings.gradle")       ||
            dir.exists("package.json")          ||
            dir.exists("pyproject.toml")        ||
            dir.exists("Cargo.toml")            ||
            dir.exists("go.mod")                ||
            dir.exists(".git")                  ||
            dir.exists(".lightcppproj")) {
            return dir.absolutePath();
        }
        dir.cdUp();
    }
    return startPath;
}

// 查找 LSP 服务器可执行文件
// ============================================================================
//  查找 LSP 服务器可执行文件
// ----------------------------------------------------------------------------
//  查找顺序（自上而下，命中即返回）：
//    1. 绝对路径（用户显式传入）
//    2. QStandardPaths —— 使用当前进程 PATH（已在 main.cpp 被采纳为登录 shell 的 PATH）
//    3. 语言版本管理器目录（nvm / fnm / volta / asdf / pyenv / cargo / gopath）
//    4. 系统常见路径（Homebrew / /usr/local / /usr/bin …）
//    5. 未找到 → 缓存空结果 + 输出可诊断日志
//
//  关键设计：
//    • 结果无条件缓存（包括"未找到"），避免重复扫描文件系统
//    • nvm / fnm / asdf 的版本目录按名字倒序，优先使用最新版本
//    • 全部走 QFileInfo::isFile() + isExecutable()，避免把目录误判为可执行文件
//    • 未命中时打印 PATH，方便诊断"环境没采纳"类问题
// ============================================================================
QString LSPManager::findServerExecutable(const QString &command,
    QString* diagnosticOut) const
{
    if (diagnosticOut) diagnosticOut->clear();

    // ---------- 缓存 ----------
    if (m_serverCache.contains(command)) {
        return m_serverCache[command];
    }

    auto cacheAndReturn = [this, &command](const QString& path) -> QString {
        m_serverCache[command] = path;
        return path;
    };

    // ============================================================
    // 辅助 1：验证某个可执行文件是否真正可用
    //
    //   只对"可能是 shim 的命令"做验证，其他命令直接信任（避免每次
    //   LSP 启动都 spawn 一个进程做探测）。
    //
    //   rust-analyzer 的 ~/.cargo/bin/rust-analyzer 是 rustup 的 shim，
    //   组件没装时 --version 会失败。但 PATH 里能找到这个 shim，
    //   被误判为"已安装"。所以必须实际跑一次验证。
    // ============================================================
    auto verifyExecutable = [&command](const QString& path) -> bool {
        // 只有 rust-analyzer 需要验证；其他 LSP 服务器（gopls / clangd /
        // typescript-language-server 等）都是真实二进制，不是 shim。
        if (command != "rust-analyzer") {
            return true;
        }

        QProcess probe;
        probe.setProgram(path);
        probe.setArguments({"--version"});
        probe.setProcessChannelMode(QProcess::MergedChannels);
        probe.start();

        if (!probe.waitForStarted(2000)) {
            qWarning() << "[LSP Manager] 验证失败（启动超时）:" << path;
            return false;
        }
        if (!probe.waitForFinished(2000)) {
            probe.kill();
            probe.waitForFinished(500);
            qWarning() << "[LSP Manager] 验证失败（执行超时）:" << path;
            return false;
        }
        if (probe.exitCode() != 0) {
            const QString err = QString::fromLocal8Bit(probe.readAll()).left(200);
            qWarning() << "[LSP Manager] 验证失败（退出码"
            << probe.exitCode() << "）:" << path
            << "输出:" << err;
            return false;
        }
        return true;
    };

    // ============================================================
    // 辅助 2：从 rustup toolchain 目录找真实 rust-analyzer
    //
    //   shim 不可用时，真身通常在：
    //     ~/.rustup/toolchains/<toolchain>/bin/rust-analyzer
    //   遍历所有 toolchain，取第一个存在的。
    // ============================================================
    auto findRealRustAnalyzer = [](const QString& home) -> QString {
        const QString toolchainRoot = home + "/.rustup/toolchains";
        QDir tcDir(toolchainRoot);
        if (!tcDir.exists()) return QString();

        const QStringList tcList = tcDir.entryList(
            QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

        for (auto it = tcList.crbegin(); it != tcList.crend(); ++it) {
            const QString realBin = tcDir.absoluteFilePath(
                *it + "/bin/rust-analyzer");
            QFileInfo fi(realBin);
            if (fi.isFile() && fi.isExecutable()) {
                return realBin;
            }
        }
        return QString();
    };

    // ---------- 1. 绝对路径 ----------
    if (QFileInfo(command).isAbsolute()) {
        QFileInfo fi(command);
        if (fi.isFile() && fi.isExecutable() && verifyExecutable(command)) {
            qDebug() << "[LSP Manager] 使用绝对路径:" << command;
            return cacheAndReturn(command);
        }
        qWarning() << "[LSP Manager] 绝对路径不可执行或不可用:" << command;
        if (diagnosticOut) *diagnosticOut = installHintFor(command);
        return cacheAndReturn(QString());
    }

    // ---------- 2. PATH（含已被采纳的登录 shell PATH） ----------
    {
        const QString fromPath = QStandardPaths::findExecutable(command);
        if (!fromPath.isEmpty()) {
            if (verifyExecutable(fromPath)) {
                qDebug() << "[LSP Manager] 从 PATH 找到:" << command
                << "->" << fromPath;
                return cacheAndReturn(fromPath);
            }

            // shim 存在但不可用 → 找 rustup toolchain 里的真实二进制
            qWarning() << "[LSP Manager] PATH 里的" << command
            << "不可用（可能是 shim）:" << fromPath;
            const QString home = QDir::homePath();
            const QString real = findRealRustAnalyzer(home);
            if (!real.isEmpty() && verifyExecutable(real)) {
                qDebug() << "[LSP Manager] 改用 rustup toolchain 里的真实路径:"
                << real;
                return cacheAndReturn(real);
            }

            // 真实路径也找不到 → 明确告知用户
            qWarning() << "[LSP Manager] 找不到可用的" << command;
            if (diagnosticOut) *diagnosticOut = installHintFor(command);
            return cacheAndReturn(QString());
        }
    }

    // ---------- 3. 语言包管理器目录 ----------
    //  说明：nvm / fnm / asdf 都是"多版本并存"的目录结构，
    //        用 entryList 拿到全部版本后倒序遍历，优先取最新版本。
    const QString home = QDir::homePath();
    QStringList pmDirs;

    auto collectVersionedDirs = [&pmDirs](const QString& root, const QString& suffix) {
        QDir d(root);
        if (!d.exists()) return;
        const QStringList versions =
        d.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        // 倒序：v22.15.1 排在 v20.x 前面
        for (auto it = versions.crbegin(); it != versions.crend(); ++it) {
            pmDirs << d.absoluteFilePath(*it + suffix);
        }
    };

    #if defined(Q_OS_MAC)
    collectVersionedDirs(home + "/.nvm/versions/node",                        "/bin");
    collectVersionedDirs(home + "/Library/Application Support/fnm/node-versions",
        "/installation/bin");
    collectVersionedDirs(home + "/.asdf/installs/nodejs",                     "/bin");
    pmDirs << home + "/.volta/bin";
    pmDirs << home + "/.cargo/bin";             // rust-analyzer
    pmDirs << home + "/go/bin";                 // gopls（默认 GOPATH/bin）
    pmDirs << home + "/.local/bin";             // 通用用户本地
    #elif defined(Q_OS_LINUX)
    collectVersionedDirs(home + "/.nvm/versions/node",                        "/bin");
    collectVersionedDirs(home + "/.fnm/node-versions",                        "/installation/bin");
    collectVersionedDirs(home + "/.asdf/installs/nodejs",                     "/bin");
    pmDirs << home + "/.volta/bin";
    pmDirs << home + "/.cargo/bin";
    pmDirs << home + "/go/bin";
    pmDirs << home + "/.local/bin";
    pmDirs << home + "/.npm-global/bin";
    #elif defined(Q_OS_WIN)
    const QString appData = qEnvironmentVariable("APPDATA");
    if (!appData.isEmpty()) pmDirs << appData + "/npm";
    pmDirs << home + "/.cargo/bin";
    pmDirs << home + "/go/bin";
    pmDirs << home + "/.volta/bin";
    #endif

    #ifdef Q_OS_WIN
    const QStringList exeSuffixes = {".exe", ".cmd", ".bat", ""};
    #else
    const QStringList exeSuffixes = {""};
    #endif

    // 标记：有没有遇到过"存在但不可用"的候选（用于判断给什么错误提示）
    bool hadUnusableCandidate = false;

    for (const QString& dir : pmDirs) {
        for (const QString& suf : exeSuffixes) {
            const QString candidate = QDir(dir).absoluteFilePath(command + suf);
            QFileInfo fi(candidate);
            if (!fi.isFile() || !fi.isExecutable()) continue;

            if (verifyExecutable(candidate)) {
                qDebug() << "[LSP Manager] 从包管理器目录找到:"
                << command << "->" << candidate;
                return cacheAndReturn(candidate);
            }

            // 候选存在但不可用（典型：~/.cargo/bin/rust-analyzer 的 shim）
            hadUnusableCandidate = true;
            qWarning() << "[LSP Manager] 候选存在但不可用，跳过:" << candidate;

            // 尝试从 rustup toolchain 拿真实二进制（只对 rust-analyzer 有效）
            const QString real = findRealRustAnalyzer(home);
            if (!real.isEmpty() && verifyExecutable(real)) {
                qDebug() << "[LSP Manager] 改用 rustup toolchain 里的真实路径:"
                << real;
                return cacheAndReturn(real);
            }
        }
    }

    // ---------- 4. 系统常见路径 ----------
    QStringList sysPaths;
    #if defined(Q_OS_WIN)
    sysPaths << "C:/Program Files/LLVM/bin"
    << "C:/Program Files (x86)/LLVM/bin"
    << "C:/mingw64/bin"
    << "C:/mingw/bin";
    #elif defined(Q_OS_MAC)
    sysPaths << "/opt/homebrew/bin"                           // Apple Silicon
    << "/usr/local/bin"                              // Intel
    << "/opt/homebrew/opt/llvm/bin"                  // clangd（brew llvm）
    << "/usr/local/opt/llvm/bin"
    << "/usr/bin"
    << "/opt/local/bin"                              // MacPorts
    << "/Library/Developer/CommandLineTools/usr/bin";
    #else
    sysPaths << "/usr/bin"
    << "/usr/local/bin"
    << "/usr/local/go/bin"
    << "/opt/bin"
    << "/snap/bin";
    #endif

    for (const QString& dir : sysPaths) {
        for (const QString& suf : exeSuffixes) {
            const QString candidate = QDir(dir).absoluteFilePath(command + suf);
            QFileInfo fi(candidate);
            if (!fi.isFile() || !fi.isExecutable()) continue;

            if (verifyExecutable(candidate)) {
                qDebug() << "[LSP Manager] 从系统路径找到:"
                << command << "->" << candidate;
                return cacheAndReturn(candidate);
            }

            hadUnusableCandidate = true;
            qWarning() << "[LSP Manager] 系统路径候选存在但不可用:" << candidate;
        }
    }

    // ---------- 5. 未找到 ----------
    qWarning() << "[LSP Manager] ✗ 未找到可用的 LSP 服务器:" << command;
    qWarning() << "  PATH =" << qgetenv("PATH");
    if (hadUnusableCandidate) {
        qWarning() << "  提示：部分候选存在但 --version 验证失败，可能是 shim";
    }

    // 关键：把"如何修复"的信息带回给调用方
    if (diagnosticOut) {
        *diagnosticOut = installHintFor(command);
    }
    return cacheAndReturn(QString());
}

QString LSPManager::installHintFor(const QString& command)
{
    if (command == "rust-analyzer") {
        return QStringLiteral(
            "rust-analyzer 组件未安装或不可用。\n"
            "\n"
            "请运行：\n"
            "    rustup component add rust-analyzer\n"
            "\n"
            "然后重启 SprintIDE。\n"
            "\n"
            "验证安装：\n"
            "    rust-analyzer --version");
    }
    if (command == "gopls") {
        return QStringLiteral(
            "gopls 未安装。\n"
            "\n"
            "请运行：\n"
            "    go install golang.org/x/tools/gopls@latest\n"
            "\n"
            "然后确认 $HOME/go/bin 在 PATH 里（或重启 SprintIDE）。");
    }
    if (command == "typescript-language-server") {
        return QStringLiteral(
            "typescript-language-server 未安装。\n"
            "\n"
            "请运行：\n"
            "    npm install -g typescript-language-server typescript");
    }
    if (command == "clangd") {
        return QStringLiteral(
            "clangd 未安装。\n"
            "\n"
            "macOS：brew install llvm\n"
            "Ubuntu：sudo apt install clangd\n"
            "然后确保 clangd 在 PATH 里。");
    }
    if (command == "pylsp") {
        return QStringLiteral(
            "pylsp 未安装。\n"
            "\n"
            "请运行：\n"
            "    pip install python-lsp-server");
    }
    if (command == "jdtls") {
        return QStringLiteral(
            "JDTLS（Eclipse JDT Language Server）未安装。\n"
            "\n"
            "请从 https://github.com/eclipse/eclipse.jdt.ls 下载，\n"
            "或通过包管理器安装 jdtls。");
    }
    if (command == "omnisharp") {
        return QStringLiteral(
            "OmniSharp 未安装。\n"
            "\n"
            "请从 https://github.com/OmniSharp/omnisharp-roslyn 下载。");
    }
    if (command == "intelephense") {
        return QStringLiteral(
            "intelephense 未安装。\n"
            "\n"
            "请运行：\n"
            "    npm install -g intelephense");
    }
    if (command == "pyright-langserver") {
        return QStringLiteral(
            "pyright 未安装。\n"
            "\n"
            "请运行：\n"
            "    npm install -g pyright");
    }

    // 未知命令：给出通用提示
    return QStringLiteral("LSP 服务器未安装：%1\n"
        "请确认该命令在 PATH 中，或在设置里指定完整路径。")
    .arg(command);
}

// 生成客户端的唯一键
QString LSPManager::makeClientKey(const QString &projectRoot,
                                  const QString &languageId) const
{
    return QString("%1|%2").arg(projectRoot, languageId);
}

// 获取语言对应的 LSP 配置
LSPConfig LSPManager::getConfigForLanguage(const QString &languageId) const
{
    // 精确匹配
    if (m_configs.contains(languageId)) {
        return m_configs[languageId];
    }

    // 检查是否匹配父语言（如 typescript → javascript）
    if (languageId == "typescript" && m_configs.contains("javascript")) {
        return m_configs["javascript"];
    }

    // 返回空配置（表示不支持）
    return LSPConfig();
}

// 获取文件对应的 LSP 客户端
LSPClient* LSPManager::getClientForFile(const QString &filePath,
                                        const QString &projectRoot)
{
    if (!m_initialized) {
        initialize();
    }

    // 1. 检测文件语言
    LanguageInfo lang = LanguageDetector::instance().detectLanguage(filePath);
    if (lang.id == "text" || lang.id.isEmpty()) {
        qDebug() << "[LSP Manager] 无法识别语言:" << filePath;
        return nullptr;
    }

    // 2. 获取语言对应的 LSP 配置
    LSPConfig config;
    if (lang.id == "python") {
        config = getPythonLSPConfig();
    } else {
        config = getConfigForLanguage(lang.id);
    }
    if (config.command.isEmpty()) {
        qDebug() << "[LSP Manager] 语言不支持 LSP:" << lang.id;
        return nullptr;
    }

    // 3. 获取项目根目录
    QString root = projectRoot;
    if (root.isEmpty()) {
        root = QFileInfo(filePath).absolutePath();
    }
    root = findProjectRoot(root);

    // ========== 语言特定的环境初始化 ==========
    if (config.command == "clangd") {
        CppBuilder::ensureCompileCommands(root);
    }
    if (config.command == "jdtls") {
        // 生成 Eclipse 元数据
        JavaUtils::ensureEclipseMetadata(root);

        // 获取项目模型，提取类路径
        ProjectManager* pm = ProjectManager::instance();
        ProjectModel* model = pm->javaProjectModel();
        if (model) {
            // 将类路径保存到 LSP 客户端的初始化参数中
            // 注意：这里需要通过 config 传递，但 LSPConfig 没有额外字段
            // 我们可以将类路径保存到 LSPClient 的成员中，在 startServer 时使用
        }

        // 确保 JDTLS wrapper 存在
        QString wrapperPath = root + "/.SprintIDE/jdtls/jdtls_wrapper.sh";
        #ifdef Q_OS_WIN
        wrapperPath = root + "/.SprintIDE/jdtls/jdtls_wrapper.bat";
        #endif
        if (!QFile::exists(wrapperPath)) {
            JavaInfo javaInfo = JavaUtils::detectJava();
            if (!javaInfo.isValid()) {
                emit clientError(lang.id, "未找到 Java 17+，请安装 Java 21 并在设置中配置 JAVA_HOME。");
                return nullptr;
            }
            if (!JavaUtils::ensureJdtlsWrapper(root, javaInfo, wrapperPath)) {
                emit clientError(lang.id, "无法生成 JDTLS 启动脚本");
                return nullptr;
            }
        }
        config.command = wrapperPath;
    }

    // ========== Python 特殊处理 ==========
    if (lang.id == "python") {
        // 如果是 pylsp，确保已安装
        if (config.serverName == "pylsp") {
            auto model = ProjectManager::instance()->pythonProjectModel();
            QString interpreter = model ? model->interpreterPath() : "python3";
            if (!PythonDependencyManager::ensurePylsp(interpreter)) {
                emit clientError(lang.id, "无法安装 pylsp，请手动安装: pip install python-lsp-server");
                return nullptr;
            }
        }
    }

    // 查找服务器可执行文件（仅对非 Python 或 pyright 全局模式）
    QString serverPath;
    if (lang.id == "python") {
        // Python 特殊处理：不通过 findServerExecutable 查找，因为我们使用自定义命令
        serverPath = config.command;  // 仅作为标识，实际可能不使用
    } else {
        // ============================================================
        // v8.5+：把"为什么没找到"的诊断信息带出来
        //   - 未安装 → "请运行 xxx install"
        //   - 存在但不可用（shim） → 也是安装提示（如 rustup component add）
        // ============================================================
        QString diagnostic;
        serverPath = findServerExecutable(config.command, &diagnostic);

        if (serverPath.isEmpty()) {
            qDebug() << "[LSP Manager] LSP 服务器不可用:" << config.command
            << " 诊断:" << diagnostic.left(200);

            const QString msg = diagnostic.isEmpty()
            ? ("未安装服务器: " + config.command)
            : diagnostic;
            emit clientError(lang.id, msg);
            return nullptr;
        }
    }

    // 生成缓存键
    QString key = makeClientKey(root, lang.id);

    if (m_clients.contains(key)) {
        LSPClient *client = m_clients[key];
        if (client && client->isServerRunning()) {
            return client;
        }
        m_clients.remove(key);
        client->deleteLater();
    }

    // 创建新客户端
    qDebug() << "[LSP Manager] 为语言创建 LSP 客户端:" << lang.id
             << "服务器:" << config.serverName
             << "项目:" << root;

    LSPClient *client = new LSPClient(this);
    client->setProjectRoot(root);

    // ========== v8.5：把 LSPConfig.arguments 透传给 client ==========
    //   这是 clangd 拿到 --header-insertion=iwyu 的唯一路径。
    //   Python 等走 m_customArguments 的语言不受影响（优先级高于 m_arguments）。
    client->setArguments(config.arguments);
    qDebug() << "[LSP] 传给 clangd 的参数:" << config.arguments;

    // ========== Python 特定设置（自定义命令和初始化选项） ==========
    if (lang.id == "python") {
        auto model = ProjectManager::instance()->pythonProjectModel();

        if (config.serverName == "pylsp") {
            QString interpreter = model ? model->interpreterPath() : "python3";
            client->setCustomCommand(interpreter);
            client->setCustomArguments({"-m", "pylsp"});
        } else if (config.command == "npx") {
            client->setCustomCommand("npx");
            client->setCustomArguments({"pyright-langserver", "--stdio"});
        } else if (config.serverName == "pyright (global)") {
            client->setCustomCommand("pyright-langserver");
            client->setCustomArguments({"--stdio"});
        } else {
            // 回退
            client->setCustomCommand(config.command);
            client->setCustomArguments(config.arguments);
        }

        // 虚拟环境
        if (model && !model->venvPath().isEmpty()) {
            QJsonObject initOpts;
            QJsonObject pythonOpts;
            pythonOpts["pythonPath"] = model->interpreterPath();
            pythonOpts["venvPath"] = model->venvPath();
            initOpts["python"] = pythonOpts;
            client->setInitializationOptions(initOpts);
            qDebug() << "[LSP] Python 虚拟环境: " << model->venvPath();
        }
    }

    // 连接信号
    connect(client, &LSPClient::diagnosticsReceived,
            this, &LSPManager::diagnosticsReceived);
    connect(client, &LSPClient::serverStarted,
            this, [this, lang, root]() {
                qDebug() << "[LSP Manager] 服务器已启动:" << lang.id << root;
                emit clientCreated(lang.id, root);
            });
    connect(client, &LSPClient::serverError,
            this, [this, lang](const QString &error) {
                emit clientError(lang.id, error);
            });

    // 启动服务器
    bool started;
    if (lang.id == "python") {
        started = client->startServer();  // 使用自定义命令
    } else {
        started = client->startServer(serverPath);
    }
    if (!started) {
        client->deleteLater();
        emit clientError(lang.id, "启动失败: " + config.serverName);
        return nullptr;
    }

    m_clients[key] = client;
    qDebug() << "[LSP Manager] LSP 客户端已创建，当前客户端数:" << m_clients.size();

    return client;
}

LSPConfig LSPManager::getPythonLSPConfig() const
{
    LSPConfig config;
    config.languageId = "python";
    config.hasCompileCommands = false;

    config.serverName = "pylsp";
    config.command = "pylsp";       // 实际启动时会被替换为 "<interpreter> -m pylsp"
    config.arguments = {};
    return config;
}

LSPConfig LSPManager::getFallbackPythonLSPConfig() const
{
    LSPConfig config;
    config.languageId = "python";
    config.serverName = "pylsp";
    config.command = "pylsp";  // 这个值会在客户端设置时被覆盖为 python -m pylsp
    config.arguments = {};
    config.hasCompileCommands = false;
    return config;
}

// 获取特定语言和项目的 LSP 客户端
LSPClient *LSPManager::getClient(const QString &languageId,
                                 const QString &projectRoot) const
{
    QString key = makeClientKey(projectRoot, languageId);
    return m_clients.value(key, nullptr);
}

// 关闭并清理所有 LSP 客户端
void LSPManager::shutdownAll()
{
    qDebug() << "[LSP Manager] 关闭所有 LSP 客户端，数量:" << m_clients.size();

    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        LSPClient *client = it.value();
        if (client) {
            client->stopServer();
            client->deleteLater();
        }
    }

    m_clients.clear();
    qDebug() << "[LSP Manager] 所有 LSP 客户端已关闭";
}

// 获取支持的 LSP 服务器列表
QList<LSPConfig> LSPManager::availableServers() const
{
    return m_configs.values();
}

} // namespace SprintIDE
