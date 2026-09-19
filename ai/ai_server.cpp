#include "ai/ai_action_executor.h"
#include "ai/ai_integration.h"
#include "ai/ai_server.h"
#include "ai/chat_panel.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QtEndian>     // 用于 qToBigEndian / qFromBigEndian

namespace SprintIDE {

// ============================================================
// 构造函数
// ============================================================
AIServer::AIServer(QObject *parent)
    : QObject(parent)
{
    // 创建 TCP 服务器对象
    m_tcpServer = new QTcpServer(this);

    // 创建心跳定时器，每 30 秒检查一次连接状态
    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimer->setInterval(30000);  // 30 秒

    // 连接心跳超时信号到槽函数
    connect(m_heartbeatTimer, &QTimer::timeout,
            this, &AIServer::onHeartbeatTimeout);

    qDebug() << "[AI Server] 对象已创建";
}

// ============================================================
// 析构函数
// ============================================================
AIServer::~AIServer()
{
    // 停止服务器并释放资源
    stopServer();

    // 注意：m_tcpServer 和 m_heartbeatTimer 是 Qt 子对象，会自动删除
    qDebug() << "[AI Server] 对象已销毁";
}

// ============================================================
// 启动服务器
// ============================================================
bool AIServer::startServer(quint16 port, const QString &bindAddress)
{
    // 如果已经启动，先停止旧的服务器
    if (m_isRunning) {
        qWarning() << "[AI Server] 服务器已在运行，先停止旧实例";
        stopServer();
    }

    // 保存端口和绑定地址
    m_port = port;
    m_bindAddress = bindAddress;

    // 尝试监听指定地址和端口
    // QHostAddress::Any 表示监听所有网卡，但我们限制为 localhost 提高安全性
    QHostAddress address(bindAddress);
    if (address.isNull()) {
        qCritical() << "[AI Server] 非法绑定地址:" << bindAddress;
        return false;
    }
    if (!m_tcpServer->listen(address, port)) {
        QString errorMsg = QString("监听失败: %1").arg(m_tcpServer->errorString());
        qCritical() << "[AI Server]" << errorMsg;
        return false;
    }

    // 连接新连接信号到槽函数
    connect(m_tcpServer, &QTcpServer::newConnection,
            this, &AIServer::onNewConnection);

    m_isRunning = true;

    // 启动心跳定时器
    m_heartbeatTimer->start();

    qDebug() << "[AI Server] 已启动，监听地址:" << bindAddress << "端口:" << port;
    emit serverStarted();

    return true;
}

// ============================================================
// 停止服务器
// ============================================================
void AIServer::stopServer()
{
    // 停止心跳定时器
    if (m_heartbeatTimer) {
        m_heartbeatTimer->stop();
    }

    // 如果有连接的 Agent，断开并清理
    if (m_agentSocket) {
        m_agentSocket->disconnectFromHost();
        m_agentSocket->deleteLater();
        m_agentSocket = nullptr;
        m_isAgentConnected = false;
        emit agentDisconnected();
    }

    // 关闭 TCP 服务器
    if (m_tcpServer) {
        m_tcpServer->close();
        // 注意：不要 delete m_tcpServer，它是 Qt 子对象，会在析构时自动删除
    }

    // 清空接收缓冲区
    m_recvBuffer.clear();

    m_isRunning = false;

    qDebug() << "[AI Server] 已停止";
    emit serverStopped();
}

// ============================================================
// 状态查询方法
// ============================================================
bool AIServer::isRunning() const
{
    return m_isRunning;
}

bool AIServer::isAgentConnected() const
{
    return m_isAgentConnected;
}

// ============================================================
// 设置主窗口指针（用于执行 IDE 操作）
// ============================================================
void AIServer::setMainWindow(MainWindow *mainWindow)
{
    m_mainWindow = mainWindow;
    qDebug() << "[AI Server] 主窗口指针已设置";
}

// ============================================================
// 私有槽：新连接到达
// ============================================================
void AIServer::onNewConnection()
{
    // 如果有 Agent 已经连接，拒绝新连接（只允许一个 Agent）
    if (m_isAgentConnected) {
        QTcpSocket *newSocket = m_tcpServer->nextPendingConnection();
        qWarning() << "[AI Server] 已有 Agent 连接，拒绝新连接:" << newSocket->peerAddress().toString();
        newSocket->disconnectFromHost();
        newSocket->deleteLater();
        return;
    }

    // 接收新连接
    QTcpSocket *newSocket = m_tcpServer->nextPendingConnection();
    if (!newSocket) {
        qWarning() << "[AI Server] 获取新连接失败";
        return;
    }

    // 保存 Agent 的 Socket
    m_agentSocket = newSocket;
    m_isAgentConnected = true;

    // 连接 Socket 信号到槽函数
    connect(m_agentSocket, &QTcpSocket::readyRead,
            this, &AIServer::onReadyRead);
    connect(m_agentSocket, &QTcpSocket::disconnected,
            this, &AIServer::onSocketDisconnected);
    connect(m_agentSocket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &AIServer::onSocketError);

    // 清空接收缓冲区（为新连接做准备）
    m_recvBuffer.clear();

    qDebug() << "[AI Server] Agent 已连接，地址:" << m_agentSocket->peerAddress().toString()
             << "端口:" << m_agentSocket->peerPort();

    // 发送连接成功通知给 Agent（可选）
    QJsonObject params;
    params["status"] = "connected";
    params["message"] = "SprintIDE AI Server 已连接";
    sendNotification("ide.ready", params);

    emit agentConnected();
}

// ============================================================
// 私有槽：有数据可读
// ============================================================
void AIServer::onReadyRead()
{
    if (!m_agentSocket) {
        return;
    }

    // 读取所有可用数据
    QByteArray data = m_agentSocket->readAll();

    // 追加到接收缓冲区（用于处理粘包/半包）
    m_recvBuffer.append(data);

    // 循环解析完整消息
    while (m_recvBuffer.size() >= 4) {
        // 读取 4 字节长度（大端序）
        quint32 messageLength = qFromBigEndian<quint32>(
            reinterpret_cast<const uchar*>(m_recvBuffer.constData())
            );

        // 检查消息长度是否合理（防止恶意数据导致内存溢出）
        if (messageLength > 1024 * 1024 * 10) {  // 最大 10MB
            qWarning() << "[AI Server] 消息过长，可能异常:" << messageLength;
            // 清空缓冲区并断开连接（安全措施）
            m_recvBuffer.clear();
            m_agentSocket->disconnectFromHost();
            return;
        }

        // 检查是否已收到完整消息
        if (static_cast<quint32>(m_recvBuffer.size()) < 4 + messageLength) {
            // 数据不完整，等待更多数据
            break;
        }

        // 提取 JSON 数据（跳过长度头）
        QByteArray jsonData = m_recvBuffer.mid(4, messageLength);

        // 从缓冲区中移除已处理的数据
        m_recvBuffer.remove(0, 4 + messageLength);

        // 处理 JSON 消息
        handleIncomingMessage(QString::fromUtf8(jsonData));
    }
}

// ============================================================
// 私有槽：Socket 断开连接
// ============================================================
void AIServer::onSocketDisconnected()
{
    if (m_agentSocket) {
        qDebug() << "[AI Server] Agent 断开连接";
        m_agentSocket->deleteLater();
        m_agentSocket = nullptr;
    }

    m_isAgentConnected = false;
    m_recvBuffer.clear();

    emit agentDisconnected();
}

// ============================================================
// 私有槽：Socket 发生错误
// ============================================================
void AIServer::onSocketError(QAbstractSocket::SocketError error)
{
    if (!m_agentSocket) {
        return;
    }

    QString errorMsg = QString("Socket 错误: %1").arg(m_agentSocket->errorString());
    qWarning() << "[AI Server]" << errorMsg;

    // 根据错误类型决定是否重连或清理
    if (error == QAbstractSocket::RemoteHostClosedError) {
        // 远程主机主动关闭，正常断开
        m_agentSocket->disconnectFromHost();
    } else {
        // 其他错误，强制断开并清理
        m_agentSocket->abort();
        m_agentSocket->deleteLater();
        m_agentSocket = nullptr;
        m_isAgentConnected = false;
        m_recvBuffer.clear();
        emit agentDisconnected();
    }
}

// ============================================================
// 私有槽：心跳超时检测
// ============================================================
void AIServer::onHeartbeatTimeout()
{
    if (!m_isAgentConnected || !m_agentSocket) {
        return;
    }

    // 检查 Socket 状态
    if (m_agentSocket->state() != QTcpSocket::ConnectedState) {
        qWarning() << "[AI Server] Agent 连接已失效（心跳检测）";
        m_agentSocket->deleteLater();
        m_agentSocket = nullptr;
        m_isAgentConnected = false;
        m_recvBuffer.clear();
        emit agentDisconnected();
        return;
    }

    // 发送心跳 Ping（可选）
    // 我们可以发送一个简单的通知来检测 Agent 是否存活
    // 如果 Agent 实现了心跳响应，可以在这里处理
    // 目前只做状态检测，不额外发送心跳包（简化设计）
}

// ============================================================
// 处理收到的消息（字符串格式）
// ============================================================
void AIServer::handleIncomingMessage(const QString &message)
{
    if (message.isEmpty()) {
        qWarning() << "[AI Server] 收到空消息";
        return;
    }

    qDebug() << "[AI Server] 收到消息:" << message.left(200) << (message.length() > 200 ? "..." : "");

    // 解析 JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        QString errorMsg = QString("JSON 解析失败: %1").arg(parseError.errorString());
        qWarning() << "[AI Server]" << errorMsg;
        // 如果不是 JSON 格式，忽略消息（不回复）
        return;
    }

    if (!doc.isObject()) {
        qWarning() << "[AI Server] 消息不是 JSON 对象";
        return;
    }

    // 处理 JSON-RPC 消息
    processJsonRpcMessage(doc.object());
}

// ============================================================
// 处理 JSON-RPC 消息
// ============================================================
void AIServer::processJsonRpcMessage(const QJsonObject &msg)
{
    // 检查 JSON-RPC 版本
    QString version = msg.value("jsonrpc").toString("2.0");
    if (version != "2.0") {
        qWarning() << "[AI Server] 不支持的 JSON-RPC 版本:" << version;
        return;
    }

    // 获取消息 ID（请求必须有 id，通知可以没有）
    int requestId = msg.value("id").toInt(-1);

    // 获取方法名
    QString method = msg.value("method").toString();
    if (method.isEmpty()) {
        qWarning() << "[AI Server] 消息缺少 method 字段";
        return;
    }

    // 获取参数（可能是对象或数组，我们统一转为对象处理）
    QJsonObject params;
    QJsonValue paramsValue = msg.value("params");
    if (paramsValue.isObject()) {
        params = paramsValue.toObject();
    } else if (paramsValue.isArray()) {
        // 如果参数是数组，包装为对象（Positional 参数，暂不深入支持）
        // 实际使用中，我们约定所有参数都是命名参数（对象形式）
        qWarning() << "[AI Server] 收到数组参数，按位置参数处理（不推荐）";
        // 简单处理：将数组转为 {"args": [...]}
        params["args"] = paramsValue.toArray();
    }

    // ============================================================
    // 根据方法名分类处理
    // ============================================================

    // 1. IDE 方法（Agent 请求 IDE 执行操作）
    if (method.startsWith("ide.")) {
        // 如果有 requestId，需要回复
        if (requestId > 0) {
            emit ideRequestReceived(requestId, method, params);
        } else {
            // 不应该发生，但以防万一
            qWarning() << "[AI Server] IDE 方法缺少 requestId:" << method;
        }
        return;
    }

    // 2. Agent 方法（IDE 请求 Agent 执行操作，这里是 Agent 的响应或通知）
    if (method.startsWith("agent.")) {
        // 检查是否有 id（响应必须带 id）
        if (requestId > 0) {
            // 检查是正常响应还是错误响应
            if (msg.contains("error")) {
                QJsonObject errorObj = msg.value("error").toObject();
                int errorCode = errorObj.value("code").toInt(0);
                QString errorMessage = errorObj.value("message").toString("未知错误");
                emit errorResponseReceived(requestId, errorCode, errorMessage);
            } else if (msg.contains("result")) {
                QJsonObject result = msg.value("result").toObject();

                // 检查是否包含流式数据
                bool isStream = result.value("stream").toBool(false);
                QString streamId = result.value("streamId").toString();
                bool isStart = result.value("start").toBool(false);
                bool isEnd = result.value("end").toBool(false);
                QString chunk = result.value("chunk").toString();
                QString fullContent = result.value("fullContent").toString();

                if (isStream) {
                    // 流式响应
                    emit streamDataReceived(requestId, streamId, chunk, isEnd, fullContent);
                } else {
                    // 普通响应
                    emit responseReceived(requestId, result);
                }
            } else {
                // 既没有 result 也没有 error，可能是不完整的响应
                qWarning() << "[AI Server] 响应缺少 result 或 error 字段";
            }
        } else {
            // 无 id，视为通知
            emit notificationReceived(method, params);
        }
        return;
    }

    // 3. 未知方法
    qWarning() << "[AI Server] 未知方法:" << method;

    // 如果有 requestId，返回错误响应
    if (requestId > 0) {
        sendErrorResponse(requestId, -32601, QString("方法不存在: %1").arg(method));
    }
}

// ============================================================
// 向 Agent 发送原始数据
// ============================================================
bool AIServer::sendMessageToAgent(const QByteArray &data)
{
    if (!m_isAgentConnected || !m_agentSocket) {
        qWarning() << "[AI Server] 无法发送消息: Agent 未连接";
        return false;
    }

    if (m_agentSocket->state() != QTcpSocket::ConnectedState) {
        qWarning() << "[AI Server] 无法发送消息: Socket 未连接";
        return false;
    }

    // 写入数据
    qint64 bytesWritten = m_agentSocket->write(data);
    if (bytesWritten != data.size()) {
        qWarning() << "[AI Server] 发送消息失败，期望:" << data.size()
                   << "实际:" << bytesWritten;
        return false;
    }

    // 刷新缓冲区（确保立即发送）
    m_agentSocket->flush();

    qDebug() << "[AI Server] 消息已发送，大小:" << data.size();
    return true;
}

// ============================================================
// 发送 JSON-RPC 请求（带 id，需要 Agent 回复）
// ============================================================
bool AIServer::sendRequest(const QString &method, const QJsonObject &params, int requestId)
{
    QJsonObject msg;
    msg["jsonrpc"] = "2.0";
    msg["id"] = requestId;
    msg["method"] = method;
    msg["params"] = params;

    QByteArray jsonData = QJsonDocument(msg).toJson(QJsonDocument::Compact);
    return sendMessageToAgent(jsonData);
}

// ============================================================
// 发送 JSON-RPC 通知（不带 id，无需回复）
// ============================================================
bool AIServer::sendNotification(const QString &method, const QJsonObject &params)
{
    QJsonObject msg;
    msg["jsonrpc"] = "2.0";
    msg["method"] = method;
    if (!params.isEmpty()) {
        msg["params"] = params;
    }

    QByteArray jsonData = QJsonDocument(msg).toJson(QJsonDocument::Compact);
    return sendMessageToAgent(jsonData);
}

// ============================================================
// 发送 JSON-RPC 错误响应（回复 Agent 的请求）
// ============================================================
bool AIServer::sendErrorResponse(int requestId, int code, const QString &message)
{
    QJsonObject msg;
    msg["jsonrpc"] = "2.0";
    msg["id"] = requestId;

    QJsonObject error;
    error["code"] = code;
    error["message"] = message;
    msg["error"] = error;

    QByteArray jsonData = QJsonDocument(msg).toJson(QJsonDocument::Compact);
    return sendMessageToAgent(jsonData);
}

// ============================================================
// 发送 JSON-RPC 成功响应（回复 Agent 的请求）
// ============================================================
bool AIServer::sendResultResponse(int requestId, const QJsonObject &result)
{
    QJsonObject msg;
    msg["jsonrpc"] = "2.0";
    msg["id"] = requestId;
    msg["result"] = result;

    QByteArray jsonData = QJsonDocument(msg).toJson(QJsonDocument::Compact);
    return sendMessageToAgent(jsonData);
}

// ============================================================
// 发送流式数据块（用于流式响应）
// ============================================================
bool AIServer::sendStreamChunk(int requestId, const QString &streamId, const QString &chunk)
{
    QJsonObject result;
    result["stream"] = true;
    result["streamId"] = streamId;
    result["chunk"] = chunk;

    return sendResultResponse(requestId, result);
}

// ============================================================
// 发送流结束信号
// ============================================================
bool AIServer::sendStreamEnd(int requestId, const QString &streamId, const QString &fullContent)
{
    QJsonObject result;
    result["stream"] = true;
    result["streamId"] = streamId;
    result["end"] = true;

    if (!fullContent.isEmpty()) {
        result["fullContent"] = fullContent;
    }

    return sendResultResponse(requestId, result);
}

} // namespace SprintIDE
