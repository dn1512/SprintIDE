#ifndef SYSTEM_EXECUTOR_H
#define SYSTEM_EXECUTOR_H

#include <QObject>
#include "ai/executor/i_executor.h"

namespace SprintIDE {
// 前向声明
class MainWindow;
namespace Protocol {

/**
 * @brief 处理 system.* 域
 *
 * 已实现：
 *  - system.getServerAddress  查询当前 AI 监听地址 + 端口
 *  - system.setServerAddress  修改监听地址 + 端口（需要重连）
 *
 * 与 meta.* 的区别：
 *   meta.*   是"操作元信息"（batch / abort / done）
 *   system.* 是"IDE 运行时环境"（连接配置、系统信息等）
 */
class SystemExecutor : public QObject, public IExecutor
{
    Q_OBJECT
public:
    explicit SystemExecutor(QObject* parent = nullptr);

    QString domain() const override { return QStringLiteral("system"); }
    ExecutionResult execute(const QJsonObject& item) override;

private:
    ExecutionResult handleGetServerAddress(const QJsonObject& item);
    ExecutionResult handleSetServerAddress(const QJsonObject& item);

    MainWindow* m_mainWindow = nullptr;
};

} // namespace Protocol
} // namespace SprintIDE

#endif // SYSTEM_EXECUTOR_H