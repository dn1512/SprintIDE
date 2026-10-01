#ifndef TCP_TRANSPORT_H
#define TCP_TRANSPORT_H

#include "ai/transport/ai_transport.h"
#include <QTcpSocket>

namespace SprintIDE {

/**
 * @brief TCP 客户端传输实现
 *
 * 分帧格式：[4 字节大端 uint32 长度][JSON body]
 */
class TcpTransport : public AiTransport
{
    Q_OBJECT
public:
    explicit TcpTransport(QObject* parent = nullptr);
    ~TcpTransport() override;

    bool connectToServer(const QString& host, quint16 port) override;
    void disconnectFromServer() override;
    bool isConnected() const override;
    bool sendMessage(const QByteArray& jsonBody) override;

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError err);

private:
    QTcpSocket* m_socket = nullptr;
    QByteArray  m_recvBuffer;

    static constexpr quint32 kMaxMessageSize = 10 * 1024 * 1024; // 10MB
};

} // namespace SprintIDE

#endif // TCP_TRANSPORT_H
