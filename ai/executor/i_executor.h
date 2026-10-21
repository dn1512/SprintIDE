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
#ifndef I_EXECUTOR_H
#define I_EXECUTOR_H

#include <QString>
#include <QJsonObject>

#include "ai/protocol/execution_result.h"

namespace SprintIDE {
namespace Protocol {

/**
 * @brief Executor 抽象接口
 *
 * 每个 domain（chat / file / command / ide / project / env / meta）
 * 对应一个 Executor，负责执行该域下的具体 type。
 *
 * 阶段 1 先做同步接口；异步 item（ide.* / command.*）由 OperationBus
 * 单独处理队列挂起，Executor 本身仍返回 ExecutionResult。
 */
class IExecutor
{
public:
    virtual ~IExecutor() = default;

    /// 负责的 domain（不带点），如 "chat" / "file"
    virtual QString domain() const = 0;

    /// 执行一个 item，返回结果
    virtual ExecutionResult execute(const QJsonObject& item) = 0;
};

} // namespace Protocol
} // namespace SprintIDE

#endif // I_EXECUTOR_H
