#include "lsp_manager.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>

#include "utils/java_utils.h"

namespace SprintIDE {

// ============================================================
// 构造函数
// ============================================================
LSPManager::LSPManager(QObject *parent)
    : QObject(parent)
{
    qDebug() << "[LSP Manager] 已创建";
}

// ============================================================
// 析构函数
// ============================================================
LSPManager::~LSPManager()
{
    shutdownAll();
    qDebug() << "[LSP Manager] 已销毁";
}

// ============================================================
// 初始化
// ============================================================
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

// ============================================================
// 构建内置 LSP 服务器配置
// ============================================================
void LSPManager::buildDefaultConfigs()
{
    // ============================================================
    // 1. C/C++ - clangd
    // ============================================================
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

    // ============================================================
    // 2. Python - pylsp
    // ============================================================
    {
        LSPConfig config;
        config.languageId = "python";
        config.serverName = "pylsp";
        config.command = "pylsp";
        config.arguments = {};
        config.hasCompileCommands = false;
        m_configs[config.languageId] = config;
    }

    // ============================================================
    // 3. JavaScript / TypeScript - typescript-language-server
    // ============================================================
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

    // ============================================================
    // 4. Go - gopls
    // ============================================================
    {
        LSPConfig config;
        config.languageId = "go";
        config.serverName = "gopls";
        config.command = "gopls";
        config.arguments = {};
        config.hasCompileCommands = false;
        m_configs[config.languageId] = config;
    }

    // ============================================================
    // 5. Rust - rust-analyzer
    // ============================================================
    {
        LSPConfig config;
        config.languageId = "rust";
        config.serverName = "rust-analyzer";
        config.command = "rust-analyzer";
        config.arguments = {};
        config.hasCompileCommands = false;
        m_configs[config.languageId] = config;
    }

    // ============================================================
    // 6. Java - jdtls (需要特殊配置，这里仅作占位)
    // ============================================================
    {
        LSPConfig config;
        config.languageId = "java";
        config.serverName = "jdtls";
        config.command = "jdtls";  // 实际可能是脚本，这里简化
        config.arguments = {};
        config.hasCompileCommands = false;
        m_configs[config.languageId] = config;
    }

    // ============================================================
    // 7. C# - omnisharp (需要特殊配置)
    // ============================================================
    {
        LSPConfig config;
        config.languageId = "csharp";
        config.serverName = "omnisharp";
        config.command = "omnisharp";
        config.arguments = {};
        config.hasCompileCommands = false;
        m_configs[config.languageId] = config;
    }

    // ============================================================
    // 8. PHP - intelephense (需要 npm 安装)
    // ============================================================
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
        if (dir.exists("pom.xml") || dir.exists("build.gradle") ||
            dir.exists("settings.gradle") || dir.exists(".git") ||
            dir.exists(".lightcppproj")) {
            return dir.absolutePath();
        }
        dir.cdUp();
    }
    // 没找到，返回原始路径
    return startPath;
}

// ============================================================
// 查找 LSP 服务器可执行文件
// ============================================================
QString LSPManager::findServerExecutable(const QString &command) const
{
    // 如果已经缓存，直接返回
    if (m_serverCache.contains(command)) {
        return m_serverCache[command];
    }

    QString fullPath;

    // 1. 检查是否为绝对路径
    if (QFileInfo(command).isAbsolute() && QFile::exists(command)) {
        fullPath = command;
        m_serverCache[command] = fullPath;
        return fullPath;
    }

    // 2. 检查系统 PATH
    QStringList envPaths = QProcessEnvironment::systemEnvironment()
                               .value("PATH")
                               .split(QDir::listSeparator());

#ifdef Q_OS_WIN
    QStringList exeSuffixes = {".exe", ".bat", ".cmd"};
#else
    QStringList exeSuffixes = {""};
#endif

    for (const QString &path : envPaths) {
        QDir dir(path);
        if (!dir.exists()) {
            continue;
        }

        for (const QString &suffix : exeSuffixes) {
            QString candidate = dir.absoluteFilePath(command + suffix);
            if (QFile::exists(candidate)) {
                fullPath = candidate;
                m_serverCache[command] = fullPath;
                qDebug() << "[LSP Manager] 找到服务器:" << command << "->" << fullPath;
                return fullPath;
            }
        }
    }

#ifdef Q_OS_UNIX
    // 检查 ~/.local/bin
    QString home = QDir::homePath();
    QString localBin = home + "/.local/bin";
    QDir dir(localBin);
    if (dir.exists()) {
        for (const QString &suffix : exeSuffixes) {
            QString candidate = dir.absoluteFilePath(command + suffix);
            if (QFile::exists(candidate)) {
                fullPath = candidate;
                m_serverCache[command] = fullPath;
                qDebug() << "[LSP Manager] 在 ~/.local/bin 找到服务器:" << command << "->" << fullPath;
                return fullPath;
            }
        }
    }
#endif

    // 3. 检查常见安装路径
    QStringList commonPaths;
#ifdef Q_OS_WIN
    commonPaths << "C:/Program Files/LLVM/bin"
                << "C:/Program Files (x86)/LLVM/bin"
                << "C:/mingw64/bin"
                << "C:/mingw/bin";
#elif defined(Q_OS_MAC)
    commonPaths << "/usr/local/bin"
                << "/opt/homebrew/bin"
                << "/usr/bin"
                << "/opt/local/bin"
                << "/Library/Developer/CommandLineTools/usr/bin";
#else
    commonPaths << "/usr/bin"
                << "/usr/local/bin"
                << "/opt/bin"
                << "/snap/bin";
#endif

    for (const QString &path : commonPaths) {
        QDir dir(path);
        if (!dir.exists()) {
            continue;
        }

        for (const QString &suffix : exeSuffixes) {
            QString candidate = dir.absoluteFilePath(command + suffix);
            if (QFile::exists(candidate)) {
                fullPath = candidate;
                m_serverCache[command] = fullPath;
                qDebug() << "[LSP Manager] 在常见路径找到服务器:" << command << "->" << fullPath;
                return fullPath;
            }
        }
    }

    // 未找到
    qDebug() << "[LSP Manager] 未找到 LSP 服务器:" << command;
    m_serverCache[command] = QString();
    return QString();
}

// ============================================================
// 生成客户端的唯一键
// ============================================================
QString LSPManager::makeClientKey(const QString &projectRoot,
                                  const QString &languageId) const
{
    return QString("%1|%2").arg(projectRoot, languageId);
}

// ============================================================
// 获取语言对应的 LSP 配置
// ============================================================
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

// ============================================================
// 获取文件对应的 LSP 客户端
// ============================================================
LSPClient *LSPManager::getClientForFile(const QString &filePath,
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
    LSPConfig config = getConfigForLanguage(lang.id);
    if (config.command.isEmpty()) {
        qDebug() << "[LSP Manager] 语言不支持 LSP:" << lang.id;
        return nullptr;
    }

    // ---- 如果是 JDTLS，确保 wrapper 存在 ----
    if (config.command == "jdtls") {
        QString root = projectRoot;
        if (root.isEmpty()) root = QFileInfo(filePath).absolutePath();
        QString wrapperPath = root + "/.SprintIDE/jdtls/jdtls_wrapper.sh";
#ifdef Q_OS_WIN
        wrapperPath = root + "/.SprintIDE/jdtls/jdtls_wrapper.bat";
#endif

        if (!QFile::exists(wrapperPath)) {
            // 获取 JavaInfo
            JavaInfo javaInfo;
            // 先尝试从配置读取（已在 detectJava 内部处理）
            javaInfo = JavaUtils::detectJava();
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

    // 3. 查找服务器可执行文件
    QString serverPath = findServerExecutable(config.command);
    if (serverPath.isEmpty()) {
        qDebug() << "[LSP Manager] 未安装 LSP 服务器:" << config.command
                 << "语言:" << lang.id;
        emit clientError(lang.id, "未安装服务器: " + config.command);
        return nullptr;
    }

    // 4. 获取项目根目录（如果未指定，尝试从文件路径推断）
    QString root = projectRoot;
    if (root.isEmpty()) {
        root = QFileInfo(filePath).absolutePath();
    }
    // 向上查找项目配置文件
    root = findProjectRoot(root);

    // 5. 生成缓存键
    QString key = makeClientKey(root, lang.id);

    // 6. 检查是否已有缓存的客户端
    if (m_clients.contains(key)) {
        LSPClient *client = m_clients[key];
        if (client && client->isServerRunning()) {
            return client;
        }
        // 客户端已关闭或异常，移除并重新创建
        m_clients.remove(key);
        client->deleteLater();
    }

    // 7. 创建新的 LSP 客户端
    qDebug() << "[LSP Manager] 为语言创建 LSP 客户端:" << lang.id
             << "服务器:" << config.serverName
             << "项目:" << root;

    LSPClient *client = new LSPClient(this);
    client->setProjectRoot(root);

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

    // 8. 启动服务器
    bool started = client->startServer(serverPath);
    if (!started) {
        qDebug() << "[LSP Manager] 启动服务器失败:" << config.serverName;
        client->deleteLater();
        emit clientError(lang.id, "启动失败: " + config.serverName);
        return nullptr;
    }

    // 9. 缓存客户端
    m_clients[key] = client;
    qDebug() << "[LSP Manager] LSP 客户端已创建，当前客户端数:" << m_clients.size();

    return client;
}

// ============================================================
// 获取特定语言和项目的 LSP 客户端
// ============================================================
LSPClient *LSPManager::getClient(const QString &languageId,
                                 const QString &projectRoot) const
{
    QString key = makeClientKey(projectRoot, languageId);
    return m_clients.value(key, nullptr);
}

// ============================================================
// 关闭并清理所有 LSP 客户端
// ============================================================
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

// ============================================================
// 获取支持的 LSP 服务器列表
// ============================================================
QList<LSPConfig> LSPManager::availableServers() const
{
    return m_configs.values();
}

} // namespace SprintIDE
