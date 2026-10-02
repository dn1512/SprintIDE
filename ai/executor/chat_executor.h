#ifndef CHAT_EXECUTOR_H
#define CHAT_EXECUTOR_H

#include <QObject>
#include <QStringList>
#include "ai/executor/i_executor.h"

namespace SprintIDE {
namespace Protocol {

/**
 * @brief 处理 chat.* 域
 *
 * 阶段 1 只广播信号，不真正阻塞。阶段 4 实现 chat.ask / chat.confirm
 * 的队列挂起机制。
 */
class ChatExecutor : public QObject, public IExecutor
{
    Q_OBJECT
public:
    explicit ChatExecutor(QObject* parent = nullptr);

    QString domain() const override { return QStringLiteral("chat"); }
    ExecutionResult execute(const QJsonObject& item) override;

signals:
    void notifyRequested(const QString& level,
                         const QString& title,
                         const QString& message);

    void askRequested(const QString& question,
                      const QStringList& choices,
                      const QString& defaultAnswer);

    void confirmRequested(const QString& targetId,
                          const QString& prompt,
                          const QString& defaultApprove);

private:
    ExecutionResult handleNotify(const QJsonObject& item);
    ExecutionResult handleAsk(const QJsonObject& item);
    ExecutionResult handleConfirm(const QJsonObject& item);
};

} // namespace Protocol
} // namespace SprintIDE

#endif // CHAT_EXECUTOR_H
