#include "ai/executor/project_executor.h"
#include "ai/protocol/protocol_types.h"

namespace SprintIDE {
namespace Protocol {

ProjectExecutor::ProjectExecutor(QObject* parent)
    : QObject(parent)
{
}

ExecutionResult ProjectExecutor::execute(const QJsonObject& item)
{
    const QString type = item.value("type").toString();
    return ExecutionResult::fail(
        item.value("id").toString(), type,
        ErrorCode::UnsupportedType,
        QStringLiteral("ProjectExecutor not yet implemented (planned for Phase 3): %1").arg(type));
}

} // namespace Protocol
} // namespace SprintIDE
