#ifndef EXECUTION_RESULT_H
#define EXECUTION_RESULT_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>

namespace SprintIDE {
namespace Protocol {

/**
 * @brief 单个 item 的执行结果
 *
 * Executor 执行完一个 item 后返回这个结构；
 * ResultBuilder 把它序列化成协议回执。
 */
struct ExecutionResult {
    enum class Status {
        Success,
        Failed,
        Skipped,
        Cancelled,
        Pending       // 异步 item 已接受，等待 deferredResultReady
    };

    Status      status      = Status::Success;
    QString     itemId;              // 对应 Agent 发来的 item 的 id
    QString     itemType;            // 例如 "ide.build"
    QString     errorCode;           // 失败时的错误码
    QString     message;             // 人类可读摘要
    QJsonObject data;                // 结构化数据（按方法不同）

    static ExecutionResult ok(const QString& itemId,
                              const QString& itemType,
                              const QJsonObject& data = {})
    {
        ExecutionResult r;
        r.status   = Status::Success;
        r.itemId   = itemId;
        r.itemType = itemType;
        r.data     = data;
        return r;
    }

    static ExecutionResult fail(const QString& itemId,
                                const QString& itemType,
                                const QString& errorCode,
                                const QString& message,
                                const QJsonObject& data = {})
    {
        ExecutionResult r;
        r.status    = Status::Failed;
        r.itemId    = itemId;
        r.itemType  = itemType;
        r.errorCode = errorCode;
        r.message   = message;
        r.data      = data;
        return r;
    }

    static ExecutionResult skipped(const QString& itemId,
                                   const QString& itemType,
                                   const QString& reason = {})
    {
        ExecutionResult r;
        r.status   = Status::Skipped;
        r.itemId   = itemId;
        r.itemType = itemType;
        r.message  = reason;
        return r;
    }

    static ExecutionResult pending(const QString& itemId,
                                   const QString& itemType)
    {
        ExecutionResult r;
        r.status   = Status::Pending;
        r.itemId   = itemId;
        r.itemType = itemType;
        return r;
    }
};

} // namespace Protocol
} // namespace SprintIDE

#endif // EXECUTION_RESULT_H
