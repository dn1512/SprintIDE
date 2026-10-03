#include "ai/ai_connection_manager.h"
#include "ai/transport/tcp_transport.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QUuid>
#include <QDebug>

namespace SprintIDE {

AiConnectionManager::AiConnectionManager(QObject* parent)
    : QObject(parent)
    , m_transport(new TcpTransport(this))
    , m_reconnectTimer(new QTimer(this))
{
    m_reconnectTimer->setSingleShot(true);

    connect(m_transport, &AiTransport::connected,
            this, &AiConnectionManager::onTransportConnected);
    connect(m_transport, &AiTransport::disconnected,
            this, &AiConnectionManager::onTransportDisconnected);
    connect(m_transport, &AiTransport::messageReceived,
            this, &AiConnectionManager::onTransportMessage);
    connect(m_transport, &AiTransport::errorOccurred,
            this, &AiConnectionManager::onTransportError);
    connect(m_reconnectTimer, &QTimer::timeout,
            this, &AiConnectionManager::onReconnectTimer);
}

AiConnectionManager::~AiConnectionManager() = default;

// ------------------------------------------------------------
// 生命周期
// ------------------------------------------------------------
void AiConnectionManager::start(const QString& host, quint16 port,
                                const QString& token, const QString& clientId)
{
    m_host        = host;
    m_port        = port;
    m_token       = token;
    m_clientId    = clientId;
    m_userStopped = false;
    m_reconnectAttempt = 0;
    m_reconnectTimer->stop();

    setState(State::Connecting);
    m_transport->connectToServer(m_host, m_port);
}

void AiConnectionManager::stop()
{
    m_userStopped = true;
    m_reconnectTimer->stop();
    m_reconnectAttempt = 0;
    clearSession();
    m_transport->disconnectFromServer();
    // disconnected 信号会触发 setState(Disconnected)
}

void AiConnectionManager::reconnectNow()
{
    if (m_host.isEmpty() || m_port == 0) return;
    m_userStopped = false;
    m_reconnectTimer->stop();
    m_reconnectAttempt = 0;

    setState(State::Connecting);
    m_transport->disconnectFromServer();
    m_transport->connectToServer(m_host, m_port);
}

// ------------------------------------------------------------
// 发送
// ------------------------------------------------------------
bool AiConnectionManager::sendMethod(const QString& method,
                                     const QJsonObject& payload,
                                     const QString& requestId)
{
    if (m_state != State::Ready) {
        qWarning() << "[AI] sendMethod rejected: not Ready, state="
                   << static_cast<int>(m_state);
        return false;
    }

    QJsonObject msg;
    msg["method"]          = method;
    msg["sessionId"]       = m_sessionId;
    msg["requestId"]       = requestId;
    msg["protocolVersion"] = QStringLiteral("1.1");
    msg["payload"]         = payload;

    const QByteArray body = QJsonDocument(msg).toJson(QJsonDocument::Compact);
    return m_transport->sendMessage(body);
}

bool AiConnectionManager::sendRaw(const QJsonObject& message)
{
    if (m_state != State::Ready) {
        qWarning() << "[AI] sendRaw rejected: not Ready, state="
                   << static_cast<int>(m_state);
        return false;
    }
    const QByteArray body = QJsonDocument(message).toJson(QJsonDocument::Compact);
    return m_transport->sendMessage(body);
}

// ------------------------------------------------------------
// 传输层回调
// ------------------------------------------------------------
void AiConnectionManager::onTransportConnected()
{
    setState(State::Handshaking);
    sendHandshake();
}

void AiConnectionManager::onTransportDisconnected()
{
    clearSession();

    if (m_userStopped) {
        setState(State::Disconnected);
        emit disconnected();
        return;
    }

    setState(State::Reconnecting);
    emit disconnected();
    scheduleReconnect();
}

void AiConnectionManager::onTransportMessage(const QByteArray& jsonBody)
{
    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonBody, &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[AI] Invalid JSON from agent:"
                   << parseErr.errorString();
        return;
    }
    const QJsonObject msg = doc.object();

    // ---- Handshaking：只接受握手响应 ----
    if (m_state == State::Handshaking) {
        const QString reqId = msg.value("requestId").toString();
        if (reqId != m_handshakeRequestId) {
            qWarning() << "[AI] Unexpected message during handshake";
            return;
        }

        const bool accepted = msg.value("accepted").toBool(false);
        if (!accepted) {
            const QString reason = msg.value("reason").toString();
            m_userStopped = true;   // 认证失败不重连
            setState(State::Failed);
            emit handshakeRejected(reason);
            return;
        }

        m_sessionId = msg.value("sessionId").toString();
        const QJsonArray caps = msg.value("serverCapabilities").toArray();
        m_reconnectAttempt = 0;
        setState(State::Ready);
        emit connected();
        emit handshakeAccepted(m_sessionId, caps);
        return;
    }

    // ---- Ready：转发业务消息 ----
    if (m_state == State::Ready) {
        emit agentMessageReceived(msg);
        return;
    }

    // 其他状态（Disconnected / Failed / Connecting / Reconnecting）忽略
}

void AiConnectionManager::onTransportError(const QString& error)
{
    qWarning() << "[AI] Transport error:" << error;

    // 连接失败时（Connecting / Reconnecting 状态）主动调度下一次重连。
    // 注意：TCP 连接被拒时只发 errorOccurred，不发 disconnected，
    // 所以不能指望 onTransportDisconnected 来兜底。
    if (m_userStopped) return;
    if (m_state == State::Ready) return;
    if (m_reconnectTimer->isActive()) return;   // 已经在等下次，不重复调度

    setState(State::Reconnecting);
    scheduleReconnect();
}

void AiConnectionManager::onReconnectTimer()
{
    if (m_userStopped) return;
    if (m_host.isEmpty() || m_port == 0) return;

    qDebug() << "[AI] Reconnect attempt" << m_reconnectAttempt;
    setState(State::Connecting);
    m_transport->connectToServer(m_host, m_port);
}

// ------------------------------------------------------------
// 内部辅助
// ------------------------------------------------------------
void AiConnectionManager::setState(State s)
{
    if (m_state == s) return;
    m_state = s;
    emit stateChanged(s);
}

void AiConnectionManager::sendHandshake()
{
    m_handshakeRequestId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    QJsonArray caps;
    caps.append(QStringLiteral("file.read"));
    caps.append(QStringLiteral("file.write"));
    caps.append(QStringLiteral("file.rename"));
    caps.append(QStringLiteral("ide.build"));
    caps.append(QStringLiteral("ide.run"));
    caps.append(QStringLiteral("command.run"));
    caps.append(QStringLiteral("command.runShell"));
    caps.append(QStringLiteral("env.query"));
    caps.append(QStringLiteral("meta.batch"));
    caps.append(QStringLiteral("chat.confirm"));
    caps.append(QStringLiteral("chat.ask"));

    QJsonObject payload;
    payload["clientId"]        = m_clientId;
    payload["clientName"]      = m_clientName;
    payload["token"]           = m_token;
    payload["clientVersion"]   = QStringLiteral("1.0.0");
    payload["projectId"]       = m_projectId;
    payload["projectName"]     = m_projectName;
    payload["capabilities"]    = caps;

    QJsonObject msg;
    msg["method"]          = QStringLiteral("handshake");
    msg["requestId"]       = m_handshakeRequestId;
    msg["protocolVersion"] = QStringLiteral("1.1");
    msg["payload"]         = payload;

    const QByteArray body = QJsonDocument(msg).toJson(QJsonDocument::Compact);
    m_transport->sendMessage(body);
}

void AiConnectionManager::scheduleReconnect()
{
    ++m_reconnectAttempt;
    // 1s, 2s, 4s, 8s, 16s, 30s（封顶）
    const int shift = qMin(m_reconnectAttempt - 1, 20);
    const int delay = qMin(kBaseBackoffMs * (1 << shift), kMaxBackoffMs);
    qDebug() << "[AI] Reconnect in" << delay << "ms";
    m_reconnectTimer->start(delay);
}

void AiConnectionManager::clearSession()
{
    m_sessionId.clear();
    m_handshakeRequestId.clear();
}

} // namespace SprintIDE
