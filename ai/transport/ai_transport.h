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
