#include "ai/protocol/result_builder.h"

namespace SprintIDE {
namespace Protocol {

// ------------------------------------------------------------
// 通用外壳
// ------------------------------------------------------------
QJsonObject ResultBuilder::makeMessage(const QString& method,
                                       const QString& sessionId,
                                       const QString& requestId,
                                       const QJsonObject& payload)
{
    QJsonObject msg;
    msg["method"]          = method;
    msg["sessionId"]       = sessionId;
    msg["requestId"]       = requestId;
    msg["protocolVersion"] = QStringLiteral("1.1");
    msg["payload"]         = payload;
    return msg;
}

// ------------------------------------------------------------
// ideResult
// ------------------------------------------------------------
QJsonObject ResultBuilder::buildIdeResult(const QString& sessionId,
                                          const QString& requestId,
                                          const ExecutionResult& r)
{
    QJsonObject payload;
    // data 里应包含：operation / status / exitCode / runId / log / durationMs / stopReason
    payload = r.data;
    // 若 data 未填 status，从 ExecutionResult 兜底
    if (!payload.contains("status")) {
        switch (r.status) {
        case ExecutionResult::Status::Success:   payload["status"] = "completed"; break;
        case ExecutionResult::Status::Failed:    payload["status"] = "failed";    break;
        case ExecutionResult::Status::Skipped:   payload["status"] = "skipped";   break;
        case ExecutionResult::Status::Cancelled: payload["status"] = "killed";    break;
        }
    }
    // 失败时带上 errorCode / message
    if (r.status != ExecutionResult::Status::Success) {
        if (!r.errorCode.isEmpty()) payload["error"]   = r.errorCode;
        if (!r.message.isEmpty())   payload["message"] = r.message;
    }
    return makeMessage("ideResult", sessionId, requestId, payload);
}

// ------------------------------------------------------------
// commandResult
// ------------------------------------------------------------
QJsonObject ResultBuilder::buildCommandResult(const QString& sessionId,
                                              const QString& requestId,
                                              const ExecutionResult& r)
{
    QJsonObject payload = r.data;
    if (!payload.contains("exitCode") && r.status == ExecutionResult::Status::Failed) {
        payload["exitCode"] = -1;
    }
    return makeMessage("commandResult", sessionId, requestId, payload);
}

// ------------------------------------------------------------
// queryResult
// ------------------------------------------------------------
QJsonObject ResultBuilder::buildQueryResult(const QString& sessionId,
                                            const QString& requestId,
                                            const QString& queryType,
                                            const ExecutionResult& r)
{
    QJsonObject payload;
    payload["queryType"] = queryType;

    if (r.status == ExecutionResult::Status::Success) {
        payload["result"] = r.data;
    } else {
        payload["result"] = QJsonObject();
        payload["error"]  = r.errorCode.isEmpty()
                               ? QStringLiteral("INTERNAL_ERROR")
                               : r.errorCode;
    }
    return makeMessage("queryResult", sessionId, requestId, payload);
}

// ------------------------------------------------------------
// fileContent
// ------------------------------------------------------------
QJsonObject ResultBuilder::buildFileContent(const QString& sessionId,
                                            const QString& requestId,
                                            const ExecutionResult& r)
{
    QJsonObject payload = r.data;

    if (r.status != ExecutionResult::Status::Success) {
        // 失败时：只带 path + error
        payload["error"] = r.errorCode.isEmpty()
                               ? QStringLiteral("INTERNAL_ERROR")
                               : r.errorCode;
    }
    return makeMessage("fileContent", sessionId, requestId, payload);
}

// ------------------------------------------------------------
// fileResult
// ------------------------------------------------------------
QJsonObject ResultBuilder::buildFileResult(const QString& sessionId,
                                           const QString& requestId,
                                           const QString& itemType,
                                           const ExecutionResult& r)
{
    QJsonObject payload = r.data;
    payload["itemType"] = itemType;

    if (r.status == ExecutionResult::Status::Success) {
        payload["status"] = "completed";
        if (!payload.contains("applied")) {
            payload["applied"] = true;
        }
    } else if (r.status == ExecutionResult::Status::Skipped) {
        payload["status"] = "skipped";
        if (!r.errorCode.isEmpty()) {
            payload["error"] = r.errorCode;
        }
        if (!r.message.isEmpty()) {
            payload["message"] = r.message;
        }
    } else {
        payload["status"]  = "failed";
        payload["error"]   = r.errorCode.isEmpty()
                               ? QStringLiteral("INTERNAL_ERROR")
                               : r.errorCode;
        payload["message"] = r.message;
    }
    return makeMessage("fileResult", sessionId, requestId, payload);
}

QJsonObject ResultBuilder::buildChatResult(const QString& sessionId,
                                           const QString& requestId,
                                           const QString& itemId,
                                           const QString& question,
                                           const QString& userAnswer)
{
    QJsonObject payload;
    payload["itemId"]     = itemId;
    payload["question"]   = question;
    payload["userAnswer"] = userAnswer;
    return makeMessage("chatResult", sessionId, requestId, payload);
}

// ------------------------------------------------------------
// batchResult
// ------------------------------------------------------------
QJsonObject ResultBuilder::buildBatchResult(const QString& sessionId,
                                            const QString& requestId,
                                            bool atomic,
                                            const QString& overallStatus,
                                            const QList<ExecutionResult>& childResults)
{
    QJsonArray arr;
    for (const ExecutionResult& c : childResults) {
        QJsonObject o;
        o["itemId"] = c.itemId;
        switch (c.status) {
        case ExecutionResult::Status::Success:   o["status"] = "completed"; break;
        case ExecutionResult::Status::Failed:    o["status"] = "failed";    break;
        case ExecutionResult::Status::Skipped:   o["status"] = "skipped";   break;
        case ExecutionResult::Status::Cancelled: o["status"] = "cancelled"; break;
        }
        if (!c.errorCode.isEmpty()) {
            o["error"] = c.errorCode;
        }
        arr.append(o);
    }

    QJsonObject payload;
    payload["atomic"]        = atomic;
    payload["overallStatus"] = overallStatus;   // completed / partial / rolledBack / failed
    payload["results"]       = arr;
    return makeMessage("batchResult", sessionId, requestId, payload);
}

// ------------------------------------------------------------
// diagnose（主动推送）
// ------------------------------------------------------------
QJsonObject ResultBuilder::buildDiagnose(const QString& sessionId,
                                         const QString& requestId,
                                         const QString& source,
                                         const QString& triggeredBy,
                                         const QJsonArray& errors,
                                         const QJsonArray& relatedFiles,
                                         const QString& commandId)
{
    QJsonObject payload;
    payload["source"]       = source;         // compile / run / lsp / debug
    payload["triggeredBy"]  = triggeredBy;    // user / agent
    payload["errors"]       = errors;
    payload["relatedFiles"] = relatedFiles;

    if (!commandId.isEmpty()) {
        payload["commandId"] = commandId;
    }
    return makeMessage("diagnose", sessionId, requestId, payload);
}

} // namespace Protocol
} // namespace SprintIDE
