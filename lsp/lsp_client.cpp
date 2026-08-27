#include "lsp_client.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>

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
    connect(m_reparseTimer, &QTimer::timeout, this, &LSPClient::onReparseTimer);
}

LSPClient::~LSPClient()
{
    stopServer();
}

bool LSPClient::startServer(const QString& serverPath)
{
    qDebug() << "LSP: startServer called, serverPath:" << serverPath;
    if (m_process && m_process->state() != QProcess::NotRunning) {
        return true;
    }

    if (!serverPath.isEmpty()) {
        m_serverPath = serverPath;
    }

    // 检查 clangd 是否存在
    QFileInfo clangdInfo(m_serverPath);
    if (!clangdInfo.exists() && m_serverPath == "clangd") {
        // 尝试常见路径
        QStringList possiblePaths = {
            "/usr/bin/clangd",
            "/usr/local/bin/clangd",
            "/opt/homebrew/bin/clangd",
            "/Library/Developer/CommandLineTools/usr/bin/clangd"
        };
        for (const QString& path : possiblePaths) {
            if (QFile::exists(path)) {
                m_serverPath = path;
                break;
            }
        }
    }

    if (!QFile::exists(m_serverPath)) {
        qDebug() << "LSP: clangd not found at" << m_serverPath;
        emit serverError("clangd not found. Please install clangd.");
        return false;
    }
    qDebug() << "LSP: clangd found at" << m_serverPath;

    m_process = new QProcess(this);

    QStringList args = {"--background-index", "--clang-tidy"};

    if (m_enableClangTidy) {
        args << "--clang-tidy";
    }
    if (!m_projectRoot.isEmpty()) {
        args << "--compile-commands-dir=" + m_projectRoot;
        qDebug() << "LSP: using compile_commands_dir:" << m_projectRoot;
    }

    m_process->setProgram(m_serverPath);
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
        emit serverError("Failed to start clangd");
        return false;
    }

    // 发送 initialize 请求
    QJsonObject initParams;
    initParams["processId"] = QCoreApplication::applicationPid();
    initParams["rootUri"] = uriFromPath(m_projectRoot.isEmpty() ? QDir::currentPath() : m_projectRoot);

    QJsonObject capabilities;
    QJsonObject textDocument;
    textDocument["synchronization"] = QJsonObject{{"didSave", true}};
    textDocument["completion"] = QJsonObject{{"dynamicRegistration", true}};
    textDocument["definition"] = QJsonObject{};
    textDocument["hover"] = QJsonObject{};
    textDocument["publishDiagnostics"] = QJsonObject{};
    capabilities["textDocument"] = textDocument;

    // workspace
    QJsonObject workspace;
    workspace["didChangeWatchedFiles"] = QJsonObject{{"dynamicRegistration", true}};
    capabilities["workspace"] = workspace;

    initParams["capabilities"] = capabilities;

    QJsonObject request;
    request["jsonrpc"] = "2.0";
    request["id"] = m_messageId++;
    request["method"] = "initialize";
    request["params"] = initParams;

    m_pendingRequests[request["id"].toInt()] = "initialize";
    sendMessage(request);

    qDebug() << "LSP: process started, waiting for initialize response";
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

// ========== 文档管理 ==========

void LSPClient::openDocument(const QString& filePath, const QString& content)
{
    qDebug() << "LSP: openDocument called for" << filePath;
    if (!isReady()) {
        qDebug() << "LSP: openDocument skipped, not ready";
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
    qDebug() << "LSP: changeDocument called for" << filePath;
    if (!isReady()) {
        qDebug() << "LSP: changeDocument skipped, not ready";
        return;
    }

    m_pendingFilePath = filePath;
    m_pendingContent = content;
    m_reparseTimer->start();
    qDebug() << "LSP: changeDocument timer started";
}

void LSPClient::onReparseTimer()
{
    qDebug() << "LSP: onReparseTimer triggered";
    if (m_pendingFilePath.isEmpty() || m_pendingContent.isEmpty()) {
        qDebug() << "LSP: onReparseTimer skipped, empty";
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
    qDebug() << "LSP: received data, buffer size:" << m_buffer.size();

    while (true) {
        int headerEnd = m_buffer.indexOf("\r\n\r\n");
        if (headerEnd == -1) {
            qDebug() << "LSP: no complete message yet";
            break;
        }

        QByteArray headerData = m_buffer.left(headerEnd);
        int contentLength = 0;
        QRegularExpression regex("Content-Length:\\s*(\\d+)");
        QRegularExpressionMatch match = regex.match(headerData);
        if (match.hasMatch()) {
            contentLength = match.captured(1).toInt();
        }

        int messageStart = headerEnd + 4;
        if (m_buffer.size() < messageStart + contentLength) {
            qDebug() << "LSP: incomplete message, need" << contentLength << "bytes";
            break;
        }

        QByteArray messageData = m_buffer.mid(messageStart, contentLength);
        m_buffer.remove(0, messageStart + contentLength);

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(messageData, &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug() << "LSP: JSON parse error:" << error.errorString();
            qDebug() << "LSP: raw data:" << messageData;
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
        qDebug() << "LSP: stderr raw data:" << error;

        QStringList lines = error.split('\n');
        for (const QString& line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed.isEmpty()) continue;

            // ========== 过滤 clangd 的正常 INFO 日志 ==========
            // 格式: I[04:25:33.195] <-- initialize(1)
            if (trimmed.startsWith("I[")) {
                qDebug() << "clangd INFO:" << trimmed;
                continue;
            }

            // 过滤常见的正常信息
            if (trimmed.contains("Loaded compilation database") ||
                trimmed.contains("ASTWorker building file") ||
                trimmed.contains("Indexing c++17 standard library") ||
                trimmed.contains("Built preamble") ||
                trimmed.contains("Indexed c++17 standard library") ||
                trimmed.contains("Enqueueing") ||
                trimmed.contains("File version went from") ||
                trimmed.contains("-->" + trimmed.contains("textDocument"))) {
                qDebug() << "clangd:" << trimmed;
                continue;
            }

            // 过滤 publishDiagnostics（正常诊断）
            if (trimmed.contains("publishDiagnostics")) {
                qDebug() << "clangd diagnostics:" << trimmed;
                continue;
            }

            // 只有错误才发送给用户
            if (trimmed.startsWith("E[") ||
                trimmed.contains("error:") ||
                trimmed.contains("fatal error:") ||
                trimmed.contains("failed")) {
                emit serverError("clangd: " + trimmed);
            } else {
                // 其他信息只调试输出
                qDebug() << "clangd:" << trimmed;
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
        // 初始化成功，发送 initialized 通知
        QJsonObject notification;
        notification["jsonrpc"] = "2.0";
        notification["method"] = "initialized";
        notification["params"] = QJsonObject{};
        sendMessage(notification);

        m_initialized = true;
        emit serverReady();
        emit serverStarted();
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

    qDebug() << "LSP: diagnostics count:" << diagnostics.size();
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
    return path;
}

void LSPClient::setProjectRoot(const QString& root)
{
    m_projectRoot = root;
}

void LSPClient::enableClangTidy(bool enable)
{
    m_enableClangTidy = enable;
}

} // namespace SprintIDE
