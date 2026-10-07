#include "ai/executor/chat_executor.h"
#include "ai/protocol/protocol_types.h"

#include <QJsonArray>
#include <QDebug>

namespace SprintIDE {
namespace Protocol {

ChatExecutor::ChatExecutor(QObject* parent)
    : QObject(parent)
{
}

ExecutionResult ChatExecutor::execute(const QJsonObject& item)
{
    const QString type = item.value("type").toString();

    if (type == QStringLiteral("chat.notify"))  return handleNotify(item);
    if (type == QStringLiteral("chat.ask"))     return handleAsk(item);
    if (type == QStringLiteral("chat.confirm")) return handleConfirm(item);

    return ExecutionResult::fail(
        item.value("id").toString(), type,
        ErrorCode::UnsupportedType,
        QStringLiteral("ChatExecutor cannot handle type: %1").arg(type));
}

ExecutionResult ChatExecutor::handleNotify(const QJsonObject& item)
{
    const QString itemId  = item.value("id").toString();
    const QString level   = item.value("level").toString();
    const QString title   = item.value("title").toString();
    const QString message = item.value("message").toString();

    emit notifyRequested(level, title, message);

    QJsonObject data;
    data["level"]   = level;
    data["title"]   = title;
    data["message"] = message;

    return ExecutionResult::ok(itemId, QStringLiteral("chat.notify"), data);
}

ExecutionResult ChatExecutor::handleAsk(const QJsonObject& item)
{
    const QString itemId       = item.value("id").toString();
    const QString question     = item.value("question").toString();
    const QString defaultAns   = item.value("defaultAnswer").toString();

    QStringList choices;
    for (const QJsonValue& v : item.value("choices").toArray()) {
        choices.append(v.toString());
    }

    // 阶段 1：只发信号，不做真正阻塞。
    // 上层收到信号后弹 UI，用户回答后由阶段 4 的恢复机制处理。
    emit askRequested(question, choices, defaultAns);

    // 阶段 1 直接返回 Skipped，等阶段 4 改造成挂起
    return ExecutionResult::skipped(itemId, QStringLiteral("chat.ask"),
                                    QStringLiteral("Blocking not yet implemented"));
}

ExecutionResult ChatExecutor::handleConfirm(const QJsonObject& item)
{
    // 正常情况下，OperationBus 会拦截 chat.confirm（门禁机制），
    // 此函数不应被调用。保留为安全垫。
    return ExecutionResult::skipped(item.value("id").toString(),
                                    QStringLiteral("chat.confirm"),
                                    QStringLiteral("Handled by OperationBus"));
}

} // namespace Protocol
} // namespace SprintIDE
