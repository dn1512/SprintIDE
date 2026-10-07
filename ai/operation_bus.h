#ifndef OPERATION_BUS_H
#define OPERATION_BUS_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QSet>

#include "ai/protocol/execution_result.h"

namespace SprintIDE {

class AiConnectionManager;

namespace Protocol {
class IExecutor;
}

/**
 * @brief 操作总线
 *
 * 职责：
 *  1. 接收 Agent 消息，校验信封 + items
 *  2. 按 domain 路由到对应 Executor
 *  3. 收集每个 item 的结果
 *  4. 通过 itemExecuted / allItemsExecuted 信号广播
 *
 * 阶段 1 骨架：只做同步路由，异步挂起、policy 判断、回执发送留后续步骤。
 */
class OperationBus : public QObject
{
    Q_OBJECT
public:
    explicit OperationBus(AiConnectionManager* connMgr,
                          QObject* parent = nullptr);
    ~OperationBus() override;

    /// 注册 executor（同 domain 重复注册会覆盖旧值）
    void registerExecutor(Protocol::IExecutor* executor);

    /// 入口：处理来自 Agent 的消息（统一信封）
    void handleAgentMessage(const QJsonObject& message);

    /// 执行单个 item（供 MetaExecutor 递归调用）
    Protocol::ExecutionResult executeItem(const QJsonObject& item);

signals:
    /// 单个 item 执行完毕
    void itemExecuted(const QString& itemId,
                      const SprintIDE::Protocol::ExecutionResult& result);

    /// 一条消息中所有 item 执行完毕
    void allItemsExecuted(const QString& requestId,
                          int totalCount,
                          int failCount);

    /// 需要用户确认（chat.confirm 门禁）
    void confirmRequired(const QString& itemId,
                         const QString& targetId,
                         const QString& prompt,
                         const QString& defaultApprove);

    /// 需要用户回答（chat.ask）
    void askRequired(const QString& itemId,
                     const QString& question,
                     const QJsonArray& choices,
                     const QString& defaultAnswer);

public slots:
    /// 异步 Executor 完成时回调
    void onDeferredResultReady(const QString& itemId,
                               const SprintIDE::Protocol::ExecutionResult& result);

    /// 用户对 chat.confirm 的回答
    void resumeAfterConfirm(bool approved);

    /// 用户对 chat.ask 的回答
    void resumeAfterAsk(const QString& userAnswer);

private:
    /// 顺序处理 items（支持遇到 Pending 时挂起）
    void processItems(const QString& sessionId,
                      const QString& requestId,
                      const QJsonArray& items,
                      int startIndex,
                      int totalCount,
                      int failCount);

    /// 把一条 item 的执行结果发回 Agent（根据 type 选方法）
    void sendResultForItem(const QString& sessionId,
                           const QString& requestId,
                           const QString& itemType,
                           const Protocol::ExecutionResult& result);

    /// 把整个 batch 的结果发回 Agent
    void sendBatchResult(const QString& sessionId,
                         const QString& requestId,
                         const QJsonObject& batchItem,
                         const Protocol::ExecutionResult& result);

    struct PendingState {
        QString    itemId;
        QString    sessionId;
        QString    requestId;
        QJsonArray items;
        int        nextIndex  = 0;
        int        totalCount = 0;
        int        failCount  = 0;
        QSet<QString>          rejectedTargets;
        QMap<QString, QString> confirmPrompts;    // targetId → prompt
        QMap<QString, QString> confirmDefaults;   // targetId → defaultApprove
    };

    struct PausedState {
        enum class Kind { None, Confirm, Ask };

        Kind       kind = Kind::None;

        QString    sessionId;
        QString    requestId;
        QJsonArray items;
        int        nextIndex  = 0;
        int        totalCount = 0;
        int        failCount  = 0;
        QSet<QString>          rejectedTargets;
        QMap<QString, QString> confirmPrompts;
        QMap<QString, QString> confirmDefaults;
        QSet<QString>          policyApprovedTargets;   // 用户通过 policy 检查批准过的
        bool                   pausedByPolicy = false;  // 本次暂停是 policy 触发还是 chat.confirm 触发

        // chat.confirm 用
        QString    pausedItemId;
        QString    pausedTargetId;
        QString    pausedPrompt;
        QString    pausedDefaultApprove;

        // chat.ask 用
        QString    askItemId;
        QString    askQuestion;
    };

    void processItems(const QString& sessionId,
                      const QString& requestId,
                      const QJsonArray& items,
                      int startIndex,
                      int totalCount,
                      int failCount,
                      const QSet<QString>& rejectedTargets,
                      const QSet<QString>& policyApprovedTargets,
                      const QMap<QString, QString>& confirmPrompts,
                      const QMap<QString, QString>& confirmDefaults);

    AiConnectionManager* m_connMgr = nullptr;
    QMap<QString, Protocol::IExecutor*> m_executors;

    QMap<QString, PendingState> m_pendingByRequest;
    QMap<QString, QString>      m_itemToRequest;
    PausedState                 m_paused;
};

} // namespace SprintIDE

#endif // OPERATION_BUS_H
