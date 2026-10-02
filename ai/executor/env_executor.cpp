#include "ai/executor/env_executor.h"
#include "ai/protocol/protocol_types.h"

namespace SprintIDE {
namespace Protocol {

EnvExecutor::EnvExecutor(QObject* parent)
    : QObject(parent)
{
}

ExecutionResult EnvExecutor::execute(const QJsonObject& item)
{
    const QString type = item.value("type").toString();
    return ExecutionResult::fail(
        item.value("id").toString(), type,
        ErrorCode::UnsupportedType,
        QStringLiteral("EnvExecutor not yet implemented (planned for Phase 3): %1").arg(type));
}

} // namespace Protocol
} // namespace SprintIDE
