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
#ifndef CHAT_EXECUTOR_H
#define CHAT_EXECUTOR_H

#include <QObject>
#include <QStringList>
#include "ai/executor/i_executor.h"

namespace SprintIDE {
namespace Protocol {

/**
 * @brief 处理 chat.* 域
 *
 * 阶段 1 只广播信号，不真正阻塞。阶段 4 实现 chat.ask / chat.confirm
 * 的队列挂起机制。
 */
class ChatExecutor : public QObject, public IExecutor
{
    Q_OBJECT
public:
    explicit ChatExecutor(QObject* parent = nullptr);

    QString domain() const override { return QStringLiteral("chat"); }
    ExecutionResult execute(const QJsonObject& item) override;

signals:
    void notifyRequested(const QString& level,
                         const QString& title,
                         const QString& message);

    void askRequested(const QString& question,
                      const QStringList& choices,
                      const QString& defaultAnswer);

    void confirmRequested(const QString& targetId,
                          const QString& prompt,
                          const QString& defaultApprove);

private:
    ExecutionResult handleNotify(const QJsonObject& item);
    ExecutionResult handleAsk(const QJsonObject& item);
    ExecutionResult handleConfirm(const QJsonObject& item);
};

} // namespace Protocol
} // namespace SprintIDE

#endif // CHAT_EXECUTOR_H
