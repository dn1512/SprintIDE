#ifndef COMMAND_EXECUTOR_H
#define COMMAND_EXECUTOR_H

#include <QObject>
#include "ai/executor/i_executor.h"

namespace SprintIDE {
namespace Protocol {

/**
 * @brief 处理 command.* 域
 *
 * 阶段 1 实现：command.run（同步阻塞）
 * 阶段 3 实现：runShell / installDep（异步 + TerminalPanel 集成）
 */
class CommandExecutor : public QObject, public IExecutor
{
    Q_OBJECT
public:
    explicit CommandExecutor(QObject* parent = nullptr);

    QString domain() const override { return QStringLiteral("command"); }
    ExecutionResult execute(const QJsonObject& item) override;

private:
    ExecutionResult handleRun(const QJsonObject& item);
    ExecutionResult handleRunShell(const QJsonObject& item);   // 骨架
    ExecutionResult handleInstallDep(const QJsonObject& item); // 骨架
};

} // namespace Protocol
} // namespace SprintIDE

#endif // COMMAND_EXECUTOR_H
