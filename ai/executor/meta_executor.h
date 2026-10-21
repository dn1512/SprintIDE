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
#ifndef META_EXECUTOR_H
#define META_EXECUTOR_H

#include <QObject>
#include "ai/executor/i_executor.h"

namespace SprintIDE {

class OperationBus;

namespace Protocol {

/**
 * @brief 处理 meta.* 域
 *
 * meta.batch：递归执行子项（阶段 1 不做原子回滚）
 * meta.abort：返回 AGENT_ABORTED，上层停止后续 item
 * meta.done：记录日志，返回成功
 */
class MetaExecutor : public QObject, public IExecutor
{
    Q_OBJECT
public:
    explicit MetaExecutor(OperationBus* bus, QObject* parent = nullptr);

    QString domain() const override { return QStringLiteral("meta"); }
    ExecutionResult execute(const QJsonObject& item) override;

private:
    ExecutionResult handleBatch(const QJsonObject& item);
    ExecutionResult handleAbort(const QJsonObject& item);
    ExecutionResult handleDone(const QJsonObject& item);

    OperationBus* m_bus = nullptr;
};

} // namespace Protocol
} // namespace SprintIDE

#endif // META_EXECUTOR_H
