#ifndef AI_CONNECTION_MANAGER_H
#define AI_CONNECTION_MANAGER_H

#include <QObject>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>

namespace SprintIDE {

class AiTransport;

class AiConnectionManager : public QObject
{
    Q_OBJECT
public:
    enum class State {
        Disconnected,
        Connecting,
        Handshaking,
        Ready,
        Reconnecting,
        Failed
    };
    Q_ENUM(State)

    explicit AiConnectionManager(QObject* parent = nullptr);
    ~AiConnectionManager() override;

    void start(const QString& host, quint16 port,
               const QString& token, const QString& clientId);
    void stop();
    void reconnectNow();

    State   state()       const { return m_state; }
    bool    isReady()     const { return m_state == State::Ready; }
    QString sessionId()   const { return m_sessionId; }
    QString currentHost() const { return m_host; }
    quint16 currentPort() const { return m_port; }

    bool sendMethod(const QString& method,
                    const QJsonObject& payload,
                    const QString& requestId);

    /// 发送已经组装好的完整消息（供 ResultBuilder 使用）
    bool sendRaw(const QJsonObject& message);

signals:
    void stateChanged(State newState);
    void connected();
    void disconnected();
    void handshakeAccepted(const QString& sessionId,
                           const QJsonArray& serverCapabilities);
    void handshakeRejected(const QString& reason);
    void agentMessageReceived(const QJsonObject& message);

private slots:
    void onTransportConnected();
    void onTransportDisconnected();
    void onTransportMessage(const QByteArray& jsonBody);
    void onTransportError(const QString& error);
    void onReconnectTimer();

private:
    void setState(State s);
    void sendHandshake();
    void scheduleReconnect();
    void clearSession();

    AiTransport* m_transport      = nullptr;
    QTimer*      m_reconnectTimer = nullptr;

    State   m_state  = State::Disconnected;
    QString m_host;
    quint16 m_port   = 0;
    QString m_token;
    QString m_clientId;
    QString m_clientName;
    QString m_projectId;
    QString m_projectName;
    QString m_sessionId;
    QString m_handshakeRequestId;

    int  m_reconnectAttempt = 0;
    bool m_userStopped      = false;

    static constexpr int kBaseBackoffMs = 1000;
    static constexpr int kMaxBackoffMs  = 30000;
};

} // namespace SprintIDE

#endif // AI_CONNECTION_MANAGER_H
