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
