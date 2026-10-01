#ifndef AI_TRANSPORT_H
#define AI_TRANSPORT_H

#include <QObject>
#include <QByteArray>
#include <QString>

namespace SprintIDE {

/**
 * @brief 传输层抽象接口
 *
 * 职责：在"一条 TCP 连接"上做收发分帧，不管谁监听。
 * 上层（AiConnectionManager）负责重连、握手、状态机。
 *
 * 数据约定：
 *  - 发送：传未分帧的 JSON body，由实现负责加分帧头
 *  - 接收：emit messageReceived，body 已去掉分帧头
 */
class AiTransport : public QObject
{
    Q_OBJECT
public:
    explicit AiTransport(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~AiTransport() = default;

    /// 主动发起连接（异步，立即返回，结果由 connected/errorOccurred 通知）
    virtual bool connectToServer(const QString& host, quint16 port) = 0;

    /// 断开连接
    virtual void disconnectFromServer() = 0;

    /// 当前是否处于连接状态
    virtual bool isConnected() const = 0;

    /// 发送一条消息（jsonBody 未分帧）
    virtual bool sendMessage(const QByteArray& jsonBody) = 0;

signals:
    void connected();
    void disconnected();
    void messageReceived(const QByteArray& jsonBody);
    void errorOccurred(const QString& error);
};

} // namespace SprintIDE

#endif // AI_TRANSPORT_H
