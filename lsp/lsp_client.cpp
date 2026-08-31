#include "lsp_client.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#include "utils/java_utils.h"

namespace SprintIDE {

LSPClient::LSPClient(QObject* parent)
    : QObject(parent)
    , m_process(nullptr)
    , m_messageId(1)
    , m_initialized(false)
    , m_serverPath("clangd")
    , m_enableClangTidy(true)
{
    m_reparseTimer = new QTimer(this);
    m_reparseTimer->setSingleShot(true);
    m_reparseTimer->setInterval(200);
    m_initTimer = new QTimer(this);
    m_initTimer->setSingleShot(true);
    connect(m_initTimer, &QTimer::timeout, this, &LSPClient::onInitializeTimeout);
    connect(m_reparseTimer, &QTimer::timeout, this, &LSPClient::onReparseTimer);
}

LSPClient::~LSPClient()
{
    stopServer();
}

bool LSPClient::startServer(const QString& serverPath)
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        return true;
    }

    if (!serverPath.isEmpty()) {
        m_serverPath = serverPath;
    }

    bool isJdtls = m_serverPath.contains("jdtls", Qt::CaseInsensitive) ||
                   m_serverPath.contains("jdtls_wrapper", Qt::CaseInsensitive);

    // ----- 1. 检查可执行文件 -----
    QString executable = m_serverPath;
    if (!QFile::exists(executable)) {
        // 尝试从 PATH 查找（仅当不是绝对路径时）
        if (!QFileInfo(executable).isAbsolute()) {
            QString found = QStandardPaths::findExecutable(executable);
            if (!found.isEmpty()) {
                executable = found;
            }
        }
        if (!QFile::exists(executable)) {
            emit serverError("可执行文件不存在: " + m_serverPath);
            return false;
        }
    }

    // ----- 2. 构建启动参数 -----
    QStringList args;
    if (isJdtls) {
        // 从 m_serverPath 中提取项目根目录（即 wrapper 所在目录的上一级？）
        // 更可靠：从 m_projectRoot 获取项目根目录（已在 LSPManager 中设置）
        QDir projectDir(m_projectRoot);
        QString jdtlsBase = projectDir.absoluteFilePath(".SprintIDE/jdtls");

        // 配置目录
        QString configDir = jdtlsBase + "/config";
        if (!JavaUtils::createMinimalConfig(configDir)) {
            emit serverError("无法创建 config.ini");
            return false;
        }

        // 工作区目录（每个项目独立）
        QString workspaceRoot = QDir::homePath() + "/.SprintIDE/workspaces";
        QDir().mkpath(workspaceRoot);
        QString workspaceDir = workspaceRoot + "/" + QString::number(qHash(m_projectRoot));
        QDir().mkpath(workspaceDir);

        args << "-configuration" << configDir;
        args << "-data" << workspaceDir;
        args << "-consoleLog";
    } else {
        // clangd 参数不变
        args << "--background-index";
        if (m_enableClangTidy) args << "--clang-tidy";
        if (!m_projectRoot.isEmpty()) args << "--compile-commands-dir=" + m_projectRoot;
    }

    // ----- 3. 启动进程 -----
    m_process = new QProcess(this);
    m_process->setProgram(executable);
    m_process->setArguments(args);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &LSPClient::onServerReadyRead);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &LSPClient::onServerErrorRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &LSPClient::onServerFinished);

    m_process->start();
    if (!m_process->waitForStarted(3000)) {
        emit serverError("Failed to start LSP server (process not started)");
        return false;
    }

    // ----- 4. 构造 initialize 请求（与之前相同，包含 Maven/Gradle 配置） -----
    QJsonObject initParams;
    initParams["processId"] = QCoreApplication::applicationPid();
    initParams["rootUri"] = uriFromPath(m_projectRoot.isEmpty() ? QDir::currentPath() : m_projectRoot);

    QJsonArray workspaceFolders;
    QJsonObject folder;
    folder["uri"] = uriFromPath(m_projectRoot);
    folder["name"] = QFileInfo(m_projectRoot).fileName();
    workspaceFolders.append(folder);
    initParams["workspaceFolders"] = workspaceFolders;

    QString rootUri = uriFromPath(m_projectRoot.isEmpty() ? QDir::currentPath() : m_projectRoot);
    initParams["rootUri"] = rootUri;

    // ---- initializationOptions（仅 JDTLS） ----
    QJsonObject initializationOptions;
    if (isJdtls) {
        // Maven 配置
        QJsonObject mavenOffline;
        mavenOffline["enabled"] = false;
        QJsonObject maven;
        maven["enabled"] = true;
        maven["offline"] = mavenOffline;
        maven["downloadSources"] = true;
        maven["downloadJavadoc"] = true;

        // Gradle 配置
        QJsonObject gradle;
        gradle["enabled"] = true;
        QJsonObject wrapper;
        wrapper["enabled"] = true;
        gradle["wrapper"] = wrapper;

        QJsonObject importObj;
        importObj["maven"] = maven;
        importObj["gradle"] = gradle;

        QJsonObject config;
        config["updateBuildConfiguration"] = "automatic";

        QJsonObject javaSettings;
        javaSettings["autobuild"] = "on";
        javaSettings["import"] = importObj;
        javaSettings["configuration"] = config;

        QJsonObject settings;
        settings["java"] = javaSettings;
        initializationOptions["settings"] = settings;
    }
    initParams["initializationOptions"] = initializationOptions;

    // ---- capabilities ----
    QJsonObject capabilities;
    QJsonObject textDocument;
    textDocument["synchronization"] = QJsonObject{{"didSave", true}};
    textDocument["completion"] = QJsonObject{{"dynamicRegistration", true}};
    textDocument["definition"] = QJsonObject{};
    textDocument["hover"] = QJsonObject{};
    textDocument["publishDiagnostics"] = QJsonObject{};
    capabilities["textDocument"] = textDocument;

    QJsonObject workspace;
    workspace["didChangeWatchedFiles"] = QJsonObject{{"dynamicRegistration", true}};
    workspace["configuration"] = true;
    capabilities["workspace"] = workspace;
    initParams["capabilities"] = capabilities;

    // ---- 发送请求 ----
    QJsonObject request;
    request["jsonrpc"] = "2.0";
    request["id"] = m_messageId++;
    request["method"] = "initialize";
    request["params"] = initParams;

    m_pendingRequests[request["id"].toInt()] = "initialize";
    sendMessage(request);

    m_initTimer->start(30000); // 30 秒超时

    return true;
}

void LSPClient::stopServer()
{
    if (m_process) {
        m_process->disconnect();
        if (m_process->state() != QProcess::NotRunning) {
            m_process->terminate();
            m_process->waitForFinished(3000);
            if (m_process->state() != QProcess::NotRunning) {
                m_process->kill();
                m_process->waitForFinished(1000);
            }
        }
        m_process->deleteLater();
        m_process = nullptr;
    }

    m_initialized = false;
    m_buffer.clear();
    m_pendingRequests.clear();
    m_reparseTimer->stop();
    emit serverStopped();
}

bool LSPClient::isReady() const
{
    return m_initialized && m_process && m_process->state() == QProcess::Running;
}

bool LSPClient::isServerRunning() const
{
    return m_process && m_process->state() == QProcess::Running;
}

void LSPClient::requestDefinition(const QString& filePath, int line, int column)
{
    if (!isReady()) return;

    QJsonObject params;
    QJsonObject textDocument;
    textDocument["uri"] = uriFromPath(filePath);
    params["textDocument"] = textDocument;

    QJsonObject position;
    position["line"] = line;
    position["character"] = column;
    params["position"] = position;

    QJsonObject request;
    request["jsonrpc"] = "2.0";
    request["id"] = m_messageId++;
    request["method"] = "textDocument/definition";
    request["params"] = params;

    m_pendingRequests[request["id"].toInt()] = "definition";
    sendMessage(request);
}

void LSPClient::handleDefinitionResponse(const QJsonObject& response)
{
    // 1. 获取 result
    QJsonValue result = response["result"];
    if (result.isNull()) {
        qWarning() << "[LSP] definition result is null";
        return;
    }

    // 2. 提取目标位置
    QString targetFile;
    int line = 0, column = 0;

    if (result.isObject()) {
        QJsonObject loc = result.toObject();
        targetFile = QUrl(loc["uri"].toString()).toLocalFile();
        QJsonObject range = loc["range"].toObject();
        QJsonObject start = range["start"].toObject();
        line = start["line"].toInt() + 1;
        column = start["character"].toInt() + 1;
    }
    else if (result.isArray()) {
        QJsonArray arr = result.toArray();
        if (!arr.isEmpty()) {
            QJsonObject loc = arr.first().toObject();
            targetFile = QUrl(loc["uri"].toString()).toLocalFile();
            QJsonObject range = loc["range"].toObject();
            QJsonObject start = range["start"].toObject();
            line = start["line"].toInt() + 1;
            column = start["character"].toInt() + 1;
        }
    } else {
        return;
    }

    // 3. 发射信号（如果解析成功）
    if (!targetFile.isEmpty()) {
        emit definitionReceived(targetFile, line, column);
    } else {
        qWarning() << "[LSP] targetFile is empty, NOT emitting definitionReceived";
    }
}

// ========== 文档管理 ==========

void LSPClient::openDocument(const QString& filePath, const QString& content)
{
    if (!isReady()) {
        return;
    }

    QString uri = uriFromPath(filePath);

    QJsonObject params;
    QJsonObject textDocument;
    textDocument["uri"] = uri;
    textDocument["languageId"] = "cpp";
    textDocument["version"] = 1;
    textDocument["text"] = content;

    params["textDocument"] = textDocument;

    QJsonObject notification;
    notification["jsonrpc"] = "2.0";
    notification["method"] = "textDocument/didOpen";
    notification["params"] = params;

    sendMessage(notification);
}

void LSPClient::changeDocument(const QString& filePath, const QString& content)
{
    if (!isReady()) {
        return;
    }

    m_pendingFilePath = filePath;
    m_pendingContent = content;
    m_reparseTimer->start();
}

void LSPClient::onReparseTimer()
{
    if (m_pendingFilePath.isEmpty() || m_pendingContent.isEmpty()) {
        return;
    }

    QString uri = uriFromPath(m_pendingFilePath);
    static int version = 1;

    QJsonObject params;
    QJsonObject textDocument;
    textDocument["uri"] = uri;
    textDocument["version"] = ++version;

    QJsonObject contentChange;
    contentChange["text"] = m_pendingContent;

    QJsonArray changes;
    changes.append(contentChange);
    params["textDocument"] = textDocument;
    params["contentChanges"] = changes;

    QJsonObject notification;
    notification["jsonrpc"] = "2.0";
    notification["method"] = "textDocument/didChange";
    notification["params"] = params;

    sendMessage(notification);
}

void LSPClient::onInitializeTimeout()
{
    if (!m_initialized) {
        emit serverError("JDTLS 初始化超时，请检查 Java 环境或日志文件。");
        // 可选：重启服务器
    }
}

void LSPClient::closeDocument(const QString& filePath)
{
    if (!isReady()) return;

    QString uri = uriFromPath(filePath);

    QJsonObject params;
    QJsonObject textDocument;
    textDocument["uri"] = uri;

    params["textDocument"] = textDocument;

    QJsonObject notification;
    notification["jsonrpc"] = "2.0";
    notification["method"] = "textDocument/didClose";
    notification["params"] = params;

    sendMessage(notification);
}

void LSPClient::saveDocument(const QString& filePath)
{
    if (!isReady()) return;

    QString uri = uriFromPath(filePath);

    QJsonObject params;
    QJsonObject textDocument;
    textDocument["uri"] = uri;

    params["textDocument"] = textDocument;

    QJsonObject notification;
    notification["jsonrpc"] = "2.0";
    notification["method"] = "textDocument/didSave";
    notification["params"] = params;

    sendMessage(notification);
}

// ========== 消息发送与接收 ==========

void LSPClient::sendMessage(const QJsonObject& message)
{
    QByteArray jsonData = QJsonDocument(message).toJson(QJsonDocument::Compact);
    QByteArray header = QString("Content-Length: %1\r\n\r\n").arg(jsonData.size()).toUtf8();
    m_process->write(header + jsonData);
}

void LSPClient::onServerReadyRead()
{
    m_buffer += m_process->readAllStandardOutput();

    // ---- 调试：打印原始数据 ----

    while (true) {
        // 如果缓冲区不以 "Content-Length:" 开头，说明有非 LSP 输出，丢弃该行
        if (!m_buffer.startsWith("Content-Length:")) {
            int newline = m_buffer.indexOf('\n');
            if (newline == -1) {
                // 没有换行，但可能是后续消息的一部分？暂时保留，但为避免死循环，若超长则丢弃
                if (m_buffer.size() > 4096) {
                    qWarning() << "LSP: discarding large non-header buffer";
                    m_buffer.clear();
                }
                break;
            }
            QByteArray line = m_buffer.left(newline + 1);
            m_buffer.remove(0, newline + 1);
            continue; // 继续检查头部
        }

        // 正常解析头部
        int headerEnd = m_buffer.indexOf("\r\n\r\n");
        if (headerEnd == -1) {
            break; // 头部不完整，等待更多数据
        }

        // 解析 Content-Length
        QByteArray headerData = m_buffer.left(headerEnd);
        int contentLength = 0;
        QRegularExpression regex("Content-Length:\\s*(\\d+)");
        QRegularExpressionMatch match = regex.match(headerData);
        if (match.hasMatch()) {
            contentLength = match.captured(1).toInt();
        } else {
            // 没有 Content-Length，丢弃这条消息
            m_buffer.remove(0, headerEnd + 4);
            continue;
        }

        int messageStart = headerEnd + 4;
        if (m_buffer.size() < messageStart + contentLength) {
            break; // 消息体不完整
        }

        // 提取消息体
        QByteArray messageData = m_buffer.mid(messageStart, contentLength);
        m_buffer.remove(0, messageStart + contentLength);

        // 解析 JSON
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(messageData, &error);
        if (error.error != QJsonParseError::NoError) {
            qWarning() << "LSP: JSON parse error:" << error.errorString();
            qWarning() << "LSP: raw message data:" << messageData;
            continue;
        }

        QJsonObject obj = doc.object();
        qDebug() << "LSP: parsed message:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);

        if (obj.contains("id")) {
            handleResponse(obj);
        } else if (obj.contains("method")) {
            handleNotification(obj);
        }
    }
}

void LSPClient::onServerErrorRead()
{
    QByteArray data = m_process->readAllStandardError();
    QString error = QString::fromUtf8(data);
    if (!error.isEmpty()) {
        qDebug() << "LSP stderr:" << error;  // 完整输出（调试用）

        QStringList lines = error.split('\n');
        for (const QString& line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed.isEmpty()) continue;

            // ========== 过滤 clangd 的正常 INFO 日志 ==========
            if (trimmed.startsWith("I[")) {
                qDebug() << "clangd INFO:" << trimmed;
                continue;
            }

            // 过滤 clangd 常见的正常输出
            if (trimmed.contains("Loaded compilation database") ||
                trimmed.contains("ASTWorker building file") ||
                trimmed.contains("Indexing c++17 standard library") ||
                trimmed.contains("Built preamble") ||
                trimmed.contains("Indexed c++17 standard library") ||
                trimmed.contains("Enqueueing") ||
                trimmed.contains("File version went from") ||
                (trimmed.contains("-->") && trimmed.contains("textDocument"))) {
                continue;
            }

            // 过滤 publishDiagnostics（正常诊断）
            if (trimmed.contains("publishDiagnostics")) {
                continue;
            }

            // ========== 过滤 JDTLS 的正常启动信息 ==========
            if (trimmed.contains("org.apache.aries.spifly.BaseActivator") ||
                trimmed.contains("Registered provider") ||
                trimmed.contains("logback.classic") ||
                trimmed.contains("SLF4JServiceProvider") ||
                trimmed.contains("!SESSION") ||
                trimmed.contains("!ENTRY") ||
                trimmed.contains("!MESSAGE") ||
                trimmed.contains("!SUBENTRY") ||
                trimmed.contains("!STACK") ||
                trimmed.contains("java.version") ||
                trimmed.contains("java.vendor") ||
                trimmed.contains("BootLoader constants") ||
                trimmed.contains("Command-line arguments")) {
                continue;
            }

            // 过滤 JDTLS 恢复/状态信息
            if (trimmed.contains("Workspace restored") ||
                trimmed.contains("exited with unsaved changes") ||
                trimmed.contains("refreshing workspace") ||
                trimmed.contains("ProjectStatus") ||
                trimmed.contains("ServiceReady") ||
                trimmed.contains("Starting Java Language Server") ||
                trimmed.contains("class org.eclipse.jdt.ls.core.internal.JavaLanguageServerPlugin is started")) {
                continue;
            }

            // 只有真正的错误才发送给用户
            if (trimmed.startsWith("E[") ||
                trimmed.contains("error:") ||
                trimmed.contains("fatal error:") ||
                trimmed.contains("failed") ||
                trimmed.contains("Exception") ||
                trimmed.contains("Error")) {
                emit serverError("JDTLS: " + trimmed);
            }
        }
    }
}

void LSPClient::onServerFinished(int exitCode, QProcess::ExitStatus status)
{
    m_initialized = false;
    emit serverStopped();
}

// ========== 响应处理 ==========

void LSPClient::handleResponse(const QJsonObject& response)
{
    int id = response["id"].toInt();
    QString method = m_pendingRequests.value(id, "");

    if (method == "initialize") {
        // 初始化成功处理
        m_initTimer->stop();
        QJsonObject notification;
        notification["jsonrpc"] = "2.0";
        notification["method"] = "initialized";
        notification["params"] = QJsonObject{};
        sendMessage(notification);
        m_initialized = true;
        emit serverReady();
        emit serverStarted();
    } else if (method == "completion") {
        handleCompletionResponse(response);
    } else if (method == "definition") {   // ⬅️ 新增这一行
        handleDefinitionResponse(response);
    }

    m_pendingRequests.remove(id);
}

void LSPClient::handleNotification(const QJsonObject& notification)
{
    QString method = notification["method"].toString();

    if (method == "textDocument/publishDiagnostics") {
        handlePublishDiagnostics(notification["params"].toObject());
    }
}

void LSPClient::handlePublishDiagnostics(const QJsonObject& params)
{
    qDebug() << "LSP: publishDiagnostics called";
    QString uri = params["uri"].toString();
    QString filePath = pathFromUri(uri);

    QList<LSPDiagnostic> diagnostics;
    QJsonArray diagArray = params["diagnostics"].toArray();
    qDebug() << "LSP: diagnostics count:" << diagArray.size();

    for (const QJsonValue& val : diagArray) {
        QJsonObject diag = val.toObject();

        LSPDiagnostic info;
        info.filePath = filePath;
        info.message = diag["message"].toString();
        info.code = diag["code"].toString();

        // 严重程度: 1=error, 2=warning, 3=info, 4=hint
        int severity = diag["severity"].toInt();
        switch (severity) {
            case 1: info.severity = "error"; break;
            case 2: info.severity = "warning"; break;
            case 3: info.severity = "information"; break;
            default: info.severity = "hint"; break;
        }

        // 位置（LSP 使用 0-based，需要转换为 1-based）
        QJsonObject range = diag["range"].toObject();
        QJsonObject start = range["start"].toObject();
        info.line = start["line"].toInt() + 1;
        info.column = start["character"].toInt() + 1;

        diagnostics.append(info);
    }

    emit diagnosticsReceived(filePath, diagnostics);
}

// ========== 工具方法 ==========

QString LSPClient::uriFromPath(const QString& path) const
{
    QString absPath = QFileInfo(path).absoluteFilePath();
#ifdef Q_OS_WIN
    return "file:///" + absPath.replace("\\", "/");
#else
    return "file://" + absPath;
#endif
}

QString LSPClient::pathFromUri(const QString& uri) const
{
    QString path = uri;
    if (path.startsWith("file://")) {
        path = path.mid(7);
#ifdef Q_OS_WIN
        if (path.startsWith("/")) {
            path = path.mid(1);
        }
        path.replace("/", "\\");
#endif
    }
    return QUrl(uri).toLocalFile();
}

void LSPClient::setProjectRoot(const QString& root)
{
    m_projectRoot = root;
}

void LSPClient::enableClangTidy(bool enable)
{
    m_enableClangTidy = enable;
}

// ========== 代码补全实现 ==========

void LSPClient::requestCompletion(const QString& filePath, int line, int column)
{
    if (!isReady()) {
        qDebug() << "LSP: completion skipped, not ready";
        return;
    }

    QString uri = uriFromPath(filePath);

    QJsonObject params;
    QJsonObject textDocument;
    textDocument["uri"] = uri;
    params["textDocument"] = textDocument;

    QJsonObject position;
    position["line"] = line;
    position["character"] = column;
    params["position"] = position;

    // 可选：上下文（当前触发的字符）
    // params["context"] = QJsonObject{{"triggerKind", 1}}; // 1 = Invoked

    QJsonObject request;
    request["jsonrpc"] = "2.0";
    request["id"] = m_messageId++;
    request["method"] = "textDocument/completion";
    request["params"] = params;

    // 保存当前请求的文件路径，响应回来时使用
    m_completionFilePath = filePath;
    m_completionRequestId = request["id"].toInt();
    m_pendingRequests[m_completionRequestId] = "completion";

    sendMessage(request);
    qDebug() << "LSP: completion requested for" << filePath << "at" << line << ":" << column;
}

void LSPClient::handleCompletionResponse(const QJsonObject& response)
{
    QList<LSPCompletionItem> items;
    QString filePath = m_completionFilePath;

    // 解析响应
    QJsonValue result = response["result"];
    if (result.isNull() || !result.isObject()) {
        // 可能没有补全结果
        emit completionReceived(filePath, items);
        return;
    }

    QJsonObject resultObj = result.toObject();
    QJsonValue itemsVal = resultObj["items"];
    if (!itemsVal.isArray()) {
        // 可能是直接返回的数组
        itemsVal = result;
    }

    if (itemsVal.isArray()) {
        QJsonArray arr = itemsVal.toArray();
        for (const QJsonValue& val : arr) {
            QJsonObject obj = val.toObject();
            LSPCompletionItem item;
            item.label = obj["label"].toString();
            item.kindCode = obj["kind"].toInt(0);
            item.kind = completionKindToString(item.kindCode);

            // 插入文本：优先使用 insertText，否则使用 label
            if (obj.contains("insertText")) {
                item.insertText = obj["insertText"].toString();
            } else {
                item.insertText = item.label;
            }

            // 详细信息（可选）
            if (obj.contains("detail")) {
                item.detail = obj["detail"].toString();
            } else if (obj.contains("documentation")) {
                QJsonValue doc = obj["documentation"];
                if (doc.isString()) {
                    item.detail = doc.toString();
                } else if (doc.isObject()) {
                    item.detail = doc.toObject()["value"].toString();
                }
            }

            items.append(item);
        }
    }

    qDebug() << "LSP: completion received" << items.size() << "items for" << filePath;
    emit completionReceived(filePath, items);
}

QString LSPClient::completionKindToString(int kind) const
{
    switch (kind) {
    case 1: return "text";
    case 2: return "method";
    case 3: return "function";
    case 4: return "constructor";
    case 5: return "field";
    case 6: return "variable";
    case 7: return "class";
    case 8: return "interface";
    case 9: return "module";
    case 10: return "property";
    case 11: return "unit";
    case 12: return "value";
    case 13: return "enum";
    case 14: return "keyword";
    case 15: return "snippet";
    case 16: return "color";
    case 17: return "file";
    case 18: return "reference";
    case 19: return "folder";
    case 20: return "enummember";
    case 21: return "constant";
    case 22: return "struct";
    case 23: return "event";
    case 24: return "operator";
    case 25: return "typeparameter";
    default: return "";
    }
}



} // namespace SprintIDE
