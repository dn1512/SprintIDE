#ifndef PROJECT_EXECUTOR_H
#define PROJECT_EXECUTOR_H

#include <QObject>
#include <QJsonArray>
#include "ai/executor/i_executor.h"

namespace SprintIDE {

// 前向声明必须在 SprintIDE 命名空间内！
// 否则会和 using namespace SprintIDE 后的 SprintIDE::MainWindow 冲突
class MainWindow;

namespace Protocol {

/**
 * @brief 处理 project.* 域
 *
 * 已实现：
 *  - project.info       返回项目基本信息 + targets
 *  - project.targets    只返回 targets
 *  - project.open       打开指定文件夹作为项目（异步；若无 .lightcpproj 会自动生成）
 *  - project.register   只注册项目，不创建任何文件（同步）
 *
 * projectId 采用项目路径的 SHA1 前 16 个十六进制字符（稳定、唯一）。
 */
class ProjectExecutor : public QObject, public IExecutor
{
    Q_OBJECT
public:
    explicit ProjectExecutor(QObject* parent = nullptr);

    QString domain() const override { return QStringLiteral("project"); }
    ExecutionResult execute(const QJsonObject& item) override;

signals:
    /**
     * @brief 异步操作完成后通过此信号把结果交还 OperationBus
     */
    void deferredResultReady(const QString& itemId,
                             const SprintIDE::Protocol::ExecutionResult& result);

private:
    ExecutionResult handleInfo(const QJsonObject& item);
    ExecutionResult handleTargets(const QJsonObject& item);
    ExecutionResult handleOpen(const QJsonObject& item);
    ExecutionResult handleRegister(const QJsonObject& item);

    QJsonObject collectBasicInfo() const;
    QJsonArray  collectTargets() const;

    ExecutionResult handleCreate(const QJsonObject& item);

    MainWindow* m_mainWindow = nullptr;
};

} // namespace Protocol
} // namespace SprintIDE

#endif // PROJECT_EXECUTOR_H