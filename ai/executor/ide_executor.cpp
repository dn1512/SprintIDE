#include "ai/executor/ide_executor.h"
#include "ai/protocol/protocol_types.h"

namespace SprintIDE {
namespace Protocol {

IdeExecutor::IdeExecutor(MainWindow* mw, QObject* parent)
    : QObject(parent)
    , m_mw(mw)
{
}

ExecutionResult IdeExecutor::execute(const QJsonObject& item)
{
    const QString type = item.value("type").toString();
    return ExecutionResult::fail(
        item.value("id").toString(), type,
        ErrorCode::UnsupportedType,
        QStringLiteral("IdeExecutor not yet implemented (planned for Phase 2): %1").arg(type));
}

} // namespace Protocol
} // namespace SprintIDE
