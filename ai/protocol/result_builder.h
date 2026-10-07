#ifndef RESULT_BUILDER_H
#define RESULT_BUILDER_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>

#include "ai/protocol/execution_result.h"

namespace SprintIDE {
namespace Protocol {

/**
 * @brief 把 ExecutionResult 序列化成 v1.1 协议回执
 *
 * 每个方法对应一个 buildXxx：
 *  - buildIdeResult       → IDE → Agent 的 ideResult
 *  - buildCommandResult   → commandResult
 *  - buildQueryResult     → queryResult
 *  - buildFileContent     → fileContent
 *  - buildBatchResult     → batchResult
 *  - buildDiagnose        → diagnose（主动推送，不属于某个 item）
 *
 * 所有方法返回"完整消息"（含 method / sessionId / requestId / payload），
 * 上层（AiConnectionManager）只需 sendRaw() 即可。
 */
class ResultBuilder
{
public:
    /// 统一外壳：构造 { method, sessionId, requestId, protocolVersion, payload }
    static QJsonObject makeMessage(const QString& method,
                                   const QString& sessionId,
                                   const QString& requestId,
                                   const QJsonObject& payload);

    /// ideResult
    static QJsonObject buildIdeResult(const QString& sessionId,
                                      const QString& requestId,
                                      const ExecutionResult& r);

    /// commandResult
    static QJsonObject buildCommandResult(const QString& sessionId,
                                          const QString& requestId,
                                          const ExecutionResult& r);

    /// queryResult
    static QJsonObject buildQueryResult(const QString& sessionId,
                                        const QString& requestId,
                                        const QString& queryType,
                                        const ExecutionResult& r);

    /// fileContent
    static QJsonObject buildFileContent(const QString& sessionId,
                                        const QString& requestId,
                                        const ExecutionResult& r);

    /// fileResult（file.create / file.delete / file.rename / file.edit 的统一回执）
    static QJsonObject buildFileResult(const QString& sessionId,
                                       const QString& requestId,
                                       const QString& itemType,
                                       const ExecutionResult& r);

    /// chatResult（chat.ask 的回答）
    static QJsonObject buildChatResult(const QString& sessionId,
                                       const QString& requestId,
                                       const QString& itemId,
                                       const QString& question,
                                       const QString& userAnswer);

    /// batchResult
    static QJsonObject buildBatchResult(const QString& sessionId,
                                        const QString& requestId,
                                        bool atomic,
                                        const QString& overallStatus,
                                        const QList<ExecutionResult>& childResults);

    /// diagnose（主动推送；无 requestId 关联到具体 item）
    static QJsonObject buildDiagnose(const QString& sessionId,
                                     const QString& requestId,
                                     const QString& source,
                                     const QString& triggeredBy,
                                     const QJsonArray& errors,
                                     const QJsonArray& relatedFiles,
                                     const QString& commandId = QString());
};

} // namespace Protocol
} // namespace SprintIDE

#endif // RESULT_BUILDER_H
