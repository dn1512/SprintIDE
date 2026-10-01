#include "ai/transport/tcp_transport.h"
#include <QtEndian>
#include <QDebug>

namespace SprintIDE {

TcpTransport::TcpTransport(QObject* parent)
    : AiTransport(parent)
    , m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::connected,
            this, &TcpTransport::onConnected);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &TcpTransport::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead,
            this, &TcpTransport::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred,
            this, &TcpTransport::onError);
}

TcpTransport::~TcpTransport()
{
    if (m_socket) {
        m_socket->abort();
    }
}

bool TcpTransport::connectToServer(const QString& host, quint16 port)
{
    // 若上次连接还在，先 abort 掉，避免状态混乱
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->abort();
    }
    m_recvBuffer.clear();
    m_socket->connectToHost(host, port);
    return true;   // 立即返回，结果由 connected/errorOccurred 通知
}

void TcpTransport::disconnectFromServer()
{
    if (m_socket->state() == QAbstractSocket::UnconnectedState) {
        return;
    }
    m_socket->disconnectFromHost();
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->waitForDisconnected(1000);
    }
    m_recvBuffer.clear();
}

bool TcpTransport::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

bool TcpTransport::sendMessage(const QByteArray& jsonBody)
{
    if (!isConnected()) {
        return false;
    }
    if (jsonBody.size() > static_cast<int>(kMaxMessageSize)) {
        emit errorOccurred(QStringLiteral("Message too large: %1 bytes")
                               .arg(jsonBody.size()));
        return false;
    }

    QByteArray frame;
    frame.resize(4);
    qToBigEndian<quint32>(static_cast<quint32>(jsonBody.size()),
                          reinterpret_cast<uchar*>(frame.data()));
    frame.append(jsonBody);

    const qint64 written = m_socket->write(frame);
    if (written != frame.size()) {
        emit errorOccurred(QStringLiteral("Write failed: %1/%2")
                               .arg(written).arg(frame.size()));
        return false;
    }
    m_socket->flush();
    return true;
}

void TcpTransport::onConnected()
{
    emit connected();
}

void TcpTransport::onDisconnected()
{
    emit disconnected();
}

void TcpTransport::onReadyRead()
{
    m_recvBuffer.append(m_socket->readAll());

    while (m_recvBuffer.size() >= 4) {
        const quint32 msgLen = qFromBigEndian<quint32>(
            reinterpret_cast<const uchar*>(m_recvBuffer.constData()));

        if (msgLen > kMaxMessageSize) {
            emit errorOccurred(QStringLiteral("Incoming message too large: %1")
                                   .arg(msgLen));
            m_socket->abort();
            m_recvBuffer.clear();
            return;
        }

        if (static_cast<quint32>(m_recvBuffer.size()) < 4 + msgLen) {
            break;   // 半包，等更多数据
        }

        const QByteArray body = m_recvBuffer.mid(4, msgLen);
        m_recvBuffer.remove(0, 4 + msgLen);
        emit messageReceived(body);
    }
}

void TcpTransport::onError(QAbstractSocket::SocketError err)
{
    Q_UNUSED(err);
    emit errorOccurred(m_socket->errorString());
}

} // namespace SprintIDE
