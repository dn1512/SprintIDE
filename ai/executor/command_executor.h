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
#ifndef COMMAND_EXECUTOR_H
#define COMMAND_EXECUTOR_H

#include <QObject>
#include "ai/executor/i_executor.h"

namespace SprintIDE {
namespace Protocol {

/**
 * @brief 处理 command.* 域
 *
 * 阶段 1 实现：command.run（同步阻塞）
 * 阶段 3 实现：runShell / installDep（异步 + TerminalPanel 集成）
 */
class CommandExecutor : public QObject, public IExecutor
{
    Q_OBJECT
public:
    explicit CommandExecutor(QObject* parent = nullptr);

    QString domain() const override { return QStringLiteral("command"); }
    ExecutionResult execute(const QJsonObject& item) override;

private:
    ExecutionResult handleRun(const QJsonObject& item);
    ExecutionResult handleRunShell(const QJsonObject& item);   // 骨架
    ExecutionResult handleInstallDep(const QJsonObject& item); // 骨架
};

} // namespace Protocol
} // namespace SprintIDE

#endif // COMMAND_EXECUTOR_H
