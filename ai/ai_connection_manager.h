/*
* SprintIDE - SprintIDE is a cross-platform IDE built from scratch with Qt 6 and C++20. Its core capability is to provide AI Agents with an executable, verifiable code execution environment.
* Copyright (C) 2026  Liu Tengjun (刘腾骏)
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
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
