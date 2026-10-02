#include "ai/operation_bus.h"
#include "ai/ai_connection_manager.h"
#include "ai/executor/i_executor.h"
#include "ai/protocol/validator.h"
#include "ai/protocol/protocol_types.h"
#include "ai/protocol/result_builder.h"

#include <QJsonDocument>
#include <QDebug>

namespace SprintIDE {

using namespace SprintIDE::Protocol;

OperationBus::OperationBus(AiConnectionManager* connMgr, QObject* parent)
    : QObject(parent)
    , m_connMgr(connMgr)
{
}

OperationBus::~OperationBus() = default;

void OperationBus::registerExecutor(IExecutor* executor)
{
    if (!executor) return;
    m_executors.insert(executor->domain(), executor);
    qDebug() << "[OperationBus] registered executor for domain:"
             << executor->domain();
}

void OperationBus::handleAgentMessage(const QJsonObject& message)
{
    // 1. 校验信封
    const auto envRes = Validator::validateEnvelope(message);
    if (!envRes.ok) {
        qWarning() << "[OperationBus] envelope invalid:"
                   << envRes.errorCode << envRes.errorMessage;
        return;
    }

    const QString sessionId = message.value("sessionId").toString();
    const QString requestId = message.value("requestId").toString();
    const QJsonArray items  = message.value("items").toArray();

    if (items.isEmpty()) {
        emit allItemsExecuted(requestId, 0, 0);
        return;
    }

    // 2. 逐 item 处理
    int failCount = 0;
    for (const QJsonValue& v : items) {
        if (!v.isObject()) continue;
        const QJsonObject item = v.toObject();

        const auto itemRes = Validator::validateItem(item);
        if (!itemRes.ok) {
            if (itemRes.isUnknownType) {
                qWarning() << "[OperationBus] unknown type, skipping:"
                           << item.value("type").toString();
                continue;
            }
            ++failCount;
            const QString itemId = item.value("id").toString();
            const ExecutionResult r = ExecutionResult::fail(
                itemId,
                item.value("type").toString(),
                itemRes.errorCode,
                itemRes.errorMessage);
            emit itemExecuted(itemId, r);
            sendResultForItem(sessionId, requestId,
                              item.value("type").toString(), r);
            continue;
        }

        const ExecutionResult result = executeItem(item);
        if (result.status == ExecutionResult::Status::Failed) {
            ++failCount;
        }
        emit itemExecuted(item.value("id").toString(), result);

        // ---- 回执 ----
        const QString type = item.value("type").toString();
        if (type == QStringLiteral("meta.batch")) {
            sendBatchResult(sessionId, requestId, item, result);
        } else {
            sendResultForItem(sessionId, requestId, type, result);
        }
    }

    // 3. 全部执行完
    emit allItemsExecuted(requestId, items.size(), failCount);
}

ExecutionResult OperationBus::executeItem(const QJsonObject& item)
{
    const QString type   = item.value("type").toString();
    const int     dotPos = type.indexOf('.');
    const QString domain = (dotPos > 0) ? type.left(dotPos) : QString();
    const QString itemId = item.value("id").toString();

    IExecutor* executor = m_executors.value(domain, nullptr);
    if (!executor) {
        qWarning() << "[OperationBus] no executor for domain:" << domain
                   << "type:" << type;
        return ExecutionResult::fail(
            itemId, type,
            ErrorCode::InternalError,
            QStringLiteral("No executor registered for domain '%1'").arg(domain));
    }

    return executor->execute(item);
}

// ------------------------------------------------------------
// 回执分发
// ------------------------------------------------------------
void OperationBus::sendResultForItem(const QString& sessionId,
                                     const QString& requestId,
                                     const QString& itemType,
                                     const ExecutionResult& result)
{
    if (!m_connMgr || !m_connMgr->isReady()) return;
    if (sessionId.isEmpty() || requestId.isEmpty()) return;

    QJsonObject msg;

    if (itemType.startsWith(QStringLiteral("ide."))) {
        msg = ResultBuilder::buildIdeResult(sessionId, requestId, result);

    } else if (itemType.startsWith(QStringLiteral("command."))) {
        msg = ResultBuilder::buildCommandResult(sessionId, requestId, result);

    } else if (itemType.startsWith(QStringLiteral("project.")) ||
               itemType == QStringLiteral("env.query")) {
        msg = ResultBuilder::buildQueryResult(sessionId, requestId,
                                              itemType, result);

    } else if (itemType == QStringLiteral("file.read")) {
        msg = ResultBuilder::buildFileContent(sessionId, requestId, result);

    } else {
        // chat.* / file.create / file.delete / file.rename / env.setSdk / meta.*
        // 目前没有独立的回执方法，阶段 1 先不发
        return;
    }

    m_connMgr->sendRaw(msg);
}

void OperationBus::sendBatchResult(const QString& sessionId,
                                   const QString& requestId,
                                   const QJsonObject& batchItem,
                                   const ExecutionResult& result)
{
    if (!m_connMgr || !m_connMgr->isReady()) return;
    if (sessionId.isEmpty() || requestId.isEmpty()) return;

    const bool atomic = batchItem.value("atomic").toBool(false);

    // 从 result.data 里重建 childResults（MetaExecutor 已放入 data["results"]）
    QList<ExecutionResult> children;
    const QJsonArray arr = result.data.value("results").toArray();
    for (const QJsonValue& v : arr) {
        const QJsonObject o = v.toObject();
        ExecutionResult c;
        c.itemId = o.value("itemId").toString();
        const QString st = o.value("status").toString();
        if (st == QStringLiteral("completed"))      c.status = ExecutionResult::Status::Success;
        else if (st == QStringLiteral("failed"))    c.status = ExecutionResult::Status::Failed;
        else if (st == QStringLiteral("skipped"))   c.status = ExecutionResult::Status::Skipped;
        else if (st == QStringLiteral("cancelled")) c.status = ExecutionResult::Status::Cancelled;
        c.errorCode = o.value("error").toString();
        children.append(c);
    }

    const QString overallStatus = result.data.value("overallStatus").toString(
        result.status == ExecutionResult::Status::Success
            ? QStringLiteral("completed")
            : QStringLiteral("failed"));

    const QJsonObject msg = ResultBuilder::buildBatchResult(
        sessionId, requestId, atomic, overallStatus, children);

    m_connMgr->sendRaw(msg);
}

} // namespace SprintIDE
