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
