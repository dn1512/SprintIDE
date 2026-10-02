#include "ai/executor/meta_executor.h"
#include "ai/operation_bus.h"
#include "ai/protocol/protocol_types.h"

#include <QJsonArray>
#include <QDebug>

namespace SprintIDE {
namespace Protocol {

MetaExecutor::MetaExecutor(OperationBus* bus, QObject* parent)
    : QObject(parent)
    , m_bus(bus)
{
}

ExecutionResult MetaExecutor::execute(const QJsonObject& item)
{
    const QString type = item.value("type").toString();

    if (type == QStringLiteral("meta.batch")) return handleBatch(item);
    if (type == QStringLiteral("meta.abort")) return handleAbort(item);
    if (type == QStringLiteral("meta.done"))  return handleDone(item);

    return ExecutionResult::fail(
        item.value("id").toString(), type,
        ErrorCode::UnsupportedType,
        QStringLiteral("MetaExecutor cannot handle type: %1").arg(type));
}

ExecutionResult MetaExecutor::handleBatch(const QJsonObject& item)
{
    const QString itemId = item.value("id").toString();
    const bool    atomic = item.value("atomic").toBool(false);

    if (!m_bus) {
        return ExecutionResult::fail(itemId, QStringLiteral("meta.batch"),
                                     ErrorCode::InternalError,
                                     QStringLiteral("OperationBus is null"));
    }

    const QJsonArray children = item.value("items").toArray();

    QJsonArray childResults;
    int failCount = 0;

    for (const QJsonValue& v : children) {
        if (!v.isObject()) continue;
        const ExecutionResult childRes = m_bus->executeItem(v.toObject());

        QJsonObject entry;
        entry["itemId"] = childRes.itemId;
        switch (childRes.status) {
        case ExecutionResult::Status::Success:   entry["status"] = "completed"; break;
        case ExecutionResult::Status::Failed:    entry["status"] = "failed";    break;
        case ExecutionResult::Status::Skipped:   entry["status"] = "skipped";   break;
        case ExecutionResult::Status::Cancelled: entry["status"] = "cancelled"; break;
        }
        if (!childRes.errorCode.isEmpty()) {
            entry["error"] = childRes.errorCode;
        }
        childResults.append(entry);

        if (childRes.status == ExecutionResult::Status::Failed) {
            ++failCount;
            if (atomic) {
                // 阶段 1：不做真正的回滚；只停止继续执行
                break;
            }
        }
    }

    QJsonObject data;
    data["atomic"]        = atomic;
    data["overallStatus"] = (failCount == 0) ? "completed"
                                             : (atomic ? "failed" : "partial");
    data["results"]       = childResults;

    if (failCount == 0) {
        return ExecutionResult::ok(itemId, QStringLiteral("meta.batch"), data);
    }
    return ExecutionResult::fail(itemId, QStringLiteral("meta.batch"),
                                 ErrorCode::InternalError,
                                 QStringLiteral("%1 child item(s) failed").arg(failCount),
                                 data);
}

ExecutionResult MetaExecutor::handleAbort(const QJsonObject& item)
{
    const QString reason = item.value("reason").toString();
    qWarning() << "[MetaExecutor] meta.abort:" << reason;

    QJsonObject data;
    data["aborted"] = true;
    data["reason"]  = reason;

    // 用 Cancelled + AGENT_ABORTED 让 OperationBus 停止后续 item
    ExecutionResult r;
    r.status    = ExecutionResult::Status::Cancelled;
    r.itemId    = item.value("id").toString();
    r.itemType  = QStringLiteral("meta.abort");
    r.errorCode = ErrorCode::AgentAborted;
    r.message   = reason;
    r.data      = data;
    return r;
}

ExecutionResult MetaExecutor::handleDone(const QJsonObject& item)
{
    const QString summary = item.value("summary").toString();
    const QString outcome = item.value("outcome").toString();

    if (!summary.isEmpty()) {
        qDebug() << "[MetaExecutor] meta.done:" << summary
                 << "outcome:" << outcome;
    }

    QJsonObject data;
    if (!summary.isEmpty()) data["summary"] = summary;
    if (!outcome.isEmpty()) data["outcome"] = outcome;

    return ExecutionResult::ok(item.value("id").toString(),
                               QStringLiteral("meta.done"), data);
}

} // namespace Protocol
} // namespace SprintIDE
