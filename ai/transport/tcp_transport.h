// SPDX-License-Identifier: GPL-3.0-or-later
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
