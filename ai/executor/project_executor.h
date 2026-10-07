#ifndef PROJECT_EXECUTOR_H
#define PROJECT_EXECUTOR_H

#include <QObject>
#include <QJsonArray>
#include "ai/executor/i_executor.h"

namespace SprintIDE {
namespace Protocol {

/**
 * @brief 处理 project.* 域
 *
 * 阶段 3 实现：
 *  - project.info    返回项目基本信息 + targets
 *  - project.targets 只返回 targets
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

private:
    ExecutionResult handleInfo(const QJsonObject& item);
    ExecutionResult handleTargets(const QJsonObject& item);

    /// 收集基本信息：projectId / projectName / projectRoot / language / buildSystem
    QJsonObject collectBasicInfo() const;

    /// 扫描项目根，生成 targets 数组（第一版仅支持 CMake）
    QJsonArray collectTargets() const;
};

} // namespace Protocol
} // namespace SprintIDE

#endif // PROJECT_EXECUTOR_H
