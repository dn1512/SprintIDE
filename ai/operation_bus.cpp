#include "ai/operation_bus.h"
#include "ai/ai_connection_manager.h"
#include "ai/executor/i_executor.h"
#include "ai/protocol/validator.h"
#include "ai/protocol/protocol_types.h"
#include "ai/protocol/result_builder.h"
#include "ai/policy/policy_engine.h"

#include <QJsonDocument>
#include <QDebug>
#include <QTimer>

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
    const auto envRes = Validator::validateEnvelope(message);
    if (!envRes.ok) {
        const QString msg = QStringLiteral("Envelope invalid: %1 (%2)")
                                .arg(envRes.errorMessage, envRes.errorCode);
        qWarning() << "[OperationBus]" << msg;
        emit logErrorMessage(msg);
        return;
    }

    const QString sessionId = message.value("sessionId").toString();
    const QString requestId = message.value("requestId").toString();
    const QJsonArray items  = message.value("items").toArray();

    if (items.isEmpty()) {
        emit allItemsExecuted(requestId, 0, 0);
        return;
    }

    // ---- 预扫描 chat.confirm：targetId prompt / defaultApprove ----
    QMap<QString, QString> confirmPrompts;
    QMap<QString, QString> confirmDefaults;
    for (const QJsonValue& v : items) {
        if (!v.isObject()) continue;
        const QJsonObject item = v.toObject();
        if (item.value("type").toString() != QStringLiteral("chat.confirm")) continue;
        const QString targetId = item.value("targetId").toString();
        if (targetId.isEmpty()) continue;
        confirmPrompts[targetId]  = item.value("prompt").toString();
        confirmDefaults[targetId] = item.value("defaultApprove").toString();
    }

    processItems(sessionId, requestId, items, 0, items.size(), 0,
                 {}, {}, confirmPrompts, confirmDefaults);
}

void OperationBus::processItems(const QString& sessionId,
                                const QString& requestId,
                                const QJsonArray& items,
                                int startIndex,
                                int totalCount,
                                int failCount,
                                const QSet<QString>& rejectedTargets,
                                const QSet<QString>& policyApprovedTargets,
                                const QMap<QString, QString>& confirmPrompts,
                                const QMap<QString, QString>& confirmDefaults)
{
    for (int i = startIndex; i < items.size(); ++i) {
        if (!items[i].isObject()) continue;
        const QJsonObject item = items[i].toObject();
        const QString type   = item.value("type").toString();
        const QString itemId = item.value("id").toString();

        // ---- chat.confirm 自身：跳过（已在预扫描处理） ----
        if (type == QStringLiteral("chat.confirm")) {
            continue;
        }

        // ---- 被拒绝的 item：跳过 ----
        if (rejectedTargets.contains(itemId)) {
            ExecutionResult r = ExecutionResult::skipped(
                itemId, type, QStringLiteral("Rejected by user"));
            r.errorCode = ErrorCode::UserRejected;
            emit itemExecuted(itemId, r);
            sendResultForItem(sessionId, requestId, type, r);
            continue;
        }

        // ---- chat.confirm 门禁（Agent 显式要求） ----
        if (confirmPrompts.contains(itemId)
            && !policyApprovedTargets.contains(itemId)) {

            m_paused.kind                  = PausedState::Kind::Confirm;
            m_paused.sessionId             = sessionId;
            m_paused.requestId             = requestId;
            m_paused.items                 = items;
            m_paused.nextIndex             = i;
            m_paused.totalCount            = totalCount;
            m_paused.failCount             = failCount;
            m_paused.rejectedTargets       = rejectedTargets;
            m_paused.policyApprovedTargets = policyApprovedTargets;
            m_paused.confirmPrompts        = confirmPrompts;
            m_paused.confirmDefaults       = confirmDefaults;
            m_paused.pausedItemId          = itemId;
            m_paused.pausedTargetId        = itemId;
            m_paused.pausedPrompt          = confirmPrompts.value(itemId);
            m_paused.pausedDefaultApprove  = confirmDefaults.value(itemId);
            m_paused.pausedByPolicy        = false;

            qDebug() << "[OperationBus] confirm required (chat.confirm) for:" << itemId;
            emit confirmRequired(itemId, itemId,
                                 m_paused.pausedPrompt,
                                 m_paused.pausedDefaultApprove);
            return;
        }

        // ---- PolicyEngine 检查 ----
        if (!policyApprovedTargets.contains(itemId)) {
            const auto dec = PolicyEngine::instance().checkItem(item);

            if (dec == PolicyEngine::Decision::Deny) {
                ++failCount;
                ExecutionResult r = ExecutionResult::fail(
                    itemId, type,
                    ErrorCode::PermissionDenied,
                    QStringLiteral("Denied by policy"));
                emit itemExecuted(itemId, r);
                sendResultForItem(sessionId, requestId, type, r);
                continue;
            }

            if (dec == PolicyEngine::Decision::Confirm) {
                m_paused.kind                  = PausedState::Kind::Confirm;
                m_paused.sessionId             = sessionId;
                m_paused.requestId             = requestId;
                m_paused.items                 = items;
                m_paused.nextIndex             = i;
                m_paused.totalCount            = totalCount;
                m_paused.failCount             = failCount;
                m_paused.rejectedTargets       = rejectedTargets;
                m_paused.policyApprovedTargets = policyApprovedTargets;
                m_paused.confirmPrompts        = confirmPrompts;
                m_paused.confirmDefaults       = confirmDefaults;
                m_paused.pausedItemId          = itemId;
                m_paused.pausedTargetId        = itemId;
                m_paused.pausedPrompt          =
                    QStringLiteral("AI 请求执行「%1」，是否允许？").arg(type);
                m_paused.pausedDefaultApprove  = QStringLiteral("reject");
                m_paused.pausedByPolicy        = true;

                qDebug() << "[OperationBus] confirm required (policy) for:"
                         << itemId << " type:" << type;
                emit confirmRequired(itemId, itemId,
                                     m_paused.pausedPrompt,
                                     m_paused.pausedDefaultApprove);
                return;
            }
        }

        // ---- 校验 ----
        const auto itemRes = Validator::validateItem(item);
        if (!itemRes.ok) {
            if (itemRes.isUnknownType) {
                const QString msg = QStringLiteral("Unknown item type: %1")
                                        .arg(type);
                qWarning() << "[OperationBus]" << msg;
                emit logWarnMessage(msg);
                continue;
            }
            const QString msg = QStringLiteral("Protocol error: %1 (type: %2)")
                                    .arg(itemRes.errorMessage, type);
            qWarning() << "[OperationBus]" << msg;
            emit logErrorMessage(msg);
            ++failCount;
            const ExecutionResult r = ExecutionResult::fail(
                itemId, type, itemRes.errorCode, itemRes.errorMessage);
            emit itemExecuted(itemId, r);
            sendResultForItem(sessionId, requestId, type, r);
            continue;
        }

        if (type == QStringLiteral("chat.ask")) {
            m_paused.kind        = PausedState::Kind::Ask;
            m_paused.sessionId   = sessionId;
            m_paused.requestId   = requestId;
            m_paused.items       = items;
            m_paused.nextIndex   = i;
            m_paused.totalCount  = totalCount;
            m_paused.failCount   = failCount;
            m_paused.askItemId   = itemId;
            m_paused.askQuestion = item.value(QStringLiteral("question")).toString();

            qDebug() << "[OperationBus] chat.ask paused, itemId:" << itemId;
            emit askRequired(itemId,
                             m_paused.askQuestion,
                             item.value(QStringLiteral("choices")).toArray(),
                             item.value(QStringLiteral("defaultAnswer")).toString());
            return;
        }

        // ---- 执行 ----
        const ExecutionResult result = executeItem(item);

        if (result.status == ExecutionResult::Status::Pending) {
            PendingState state;
            state.itemId          = result.itemId;
            state.sessionId       = sessionId;
            state.requestId       = requestId;
            state.items           = items;
            state.nextIndex       = i + 1;
            state.totalCount      = totalCount;
            state.failCount       = failCount;
            state.rejectedTargets = rejectedTargets;
            state.confirmPrompts  = confirmPrompts;
            state.confirmDefaults = confirmDefaults;

            m_pendingByRequest.insert(requestId, state);
            m_itemToRequest.insert(result.itemId, requestId);
            qDebug() << "[OperationBus] item pending:" << result.itemId
                     << "requestId:" << requestId;
            return;
        }

        if (result.status == ExecutionResult::Status::Failed) ++failCount;
        emit itemExecuted(itemId, result);

        if (type == QStringLiteral("meta.batch")) {
            sendBatchResult(sessionId, requestId, item, result);
        } else {
            sendResultForItem(sessionId, requestId, type, result);
        }
    }

    emit allItemsExecuted(requestId, totalCount, failCount);
}

void OperationBus::onDeferredResultReady(const QString& itemId,
                                         const ExecutionResult& result)
{
    if (!m_itemToRequest.contains(itemId)) {
        qWarning() << "[OperationBus] unexpected deferred result:" << itemId;
        return;
    }

    const QString requestId = m_itemToRequest.take(itemId);
    if (!m_pendingByRequest.contains(requestId)) {
        qWarning() << "[OperationBus] no pending state for requestId:"
                   << requestId;
        return;
    }

    const PendingState state = m_pendingByRequest.take(requestId);

    int failCount = state.failCount;
    if (result.status == ExecutionResult::Status::Failed) ++failCount;

    emit itemExecuted(itemId, result);
    sendResultForItem(state.sessionId, state.requestId,
                      result.itemType, result);

    processItems(state.sessionId, state.requestId, state.items,
                 state.nextIndex, state.totalCount, failCount,
                 state.rejectedTargets,
                 QSet<QString>(),   // 异步回执恢复时不带 policyApprovedTargets（罕见场景）
                 state.confirmPrompts, state.confirmDefaults);
}

void OperationBus::resumeAfterConfirm(bool approved)
{
    if (m_paused.pausedTargetId.isEmpty()) {
        qWarning() << "[OperationBus] no paused confirm to resume";
        return;
    }

    const QString sessionId = m_paused.sessionId;
    const QString requestId = m_paused.requestId;
    const QJsonArray items  = m_paused.items;
    const int nextIndex     = m_paused.nextIndex;
    const int totalCount    = m_paused.totalCount;
    const int failCount     = m_paused.failCount;
    QSet<QString> rejected         = m_paused.rejectedTargets;
    QSet<QString> policyApproved   = m_paused.policyApprovedTargets;
    QMap<QString, QString> prompts  = m_paused.confirmPrompts;
    QMap<QString, QString> defaults = m_paused.confirmDefaults;
    const QString targetId  = m_paused.pausedTargetId;
    const bool byPolicy     = m_paused.pausedByPolicy;

    m_paused = PausedState();

    if (!approved) {
        rejected.insert(targetId);
    } else {
        policyApproved.insert(targetId);   // 无论来源，都记入 approved
        if (!byPolicy) {
            prompts.remove(targetId);
            defaults.remove(targetId);
        }
    }

    qDebug() << "[OperationBus] resume confirm, targetId:" << targetId
             << " approved:" << approved << " byPolicy:" << byPolicy;

    processItems(sessionId, requestId, items, nextIndex, totalCount, failCount,
                 rejected, policyApproved, prompts, defaults);
}

void OperationBus::resumeAfterAsk(const QString& userAnswer)
{
    if (m_paused.kind != PausedState::Kind::Ask) {
        qWarning() << "[OperationBus] resumeAfterAsk: no ask in progress";
        return;
    }

    const QString sessionId  = m_paused.sessionId;
    const QString requestId  = m_paused.requestId;
    const QString itemId     = m_paused.askItemId;
    const QString question   = m_paused.askQuestion;
    const int     totalCount = m_paused.totalCount;
    const int     failCount  = m_paused.failCount;

    m_paused = PausedState();

    // 发 chatResult 回 Agent
    if (m_connMgr && m_connMgr->isReady()) {
        const QJsonObject msg = ResultBuilder::buildChatResult(
            sessionId, requestId, itemId, question, userAnswer);
        m_connMgr->sendRaw(msg);
    }

    // 关键：不恢复队列。chat.ask 之后的所有 items 全部不执行。
    // Agent 收到 chatResult 后会重新发一条新消息。
    emit allItemsExecuted(requestId, totalCount, failCount);

    qDebug() << "[OperationBus] chat.ask answered:" << userAnswer;
}

ExecutionResult OperationBus::executeItem(const QJsonObject& item)
{
    const QString type   = item.value("type").toString();
    const int     dotPos = type.indexOf('.');
    const QString domain = (dotPos > 0) ? type.left(dotPos) : QString();
    const QString itemId = item.value("id").toString();

    IExecutor* executor = m_executors.value(domain, nullptr);
    if (!executor) {
        const QString msg = QStringLiteral("No executor registered for domain '%1' (type: %2)")
                                .arg(domain, type);
        qWarning() << "[OperationBus]" << msg;
        emit logErrorMessage(msg);
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
               itemType.startsWith(QStringLiteral("system.")) ||
               itemType == QStringLiteral("env.query")) {
        msg = ResultBuilder::buildQueryResult(sessionId, requestId,
                                              itemType, result);

    } else if (itemType == QStringLiteral("file.read")) {
        msg = ResultBuilder::buildFileContent(sessionId, requestId, result);

    } else if (itemType.startsWith(QStringLiteral("file."))) {
        msg = ResultBuilder::buildFileResult(sessionId, requestId,
                                             itemType, result);

    } else {
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
