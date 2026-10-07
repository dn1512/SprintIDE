#ifndef ENV_EXECUTOR_H
#define ENV_EXECUTOR_H

#include <QObject>
#include <QString>
#include <QJsonValue>
#include "ai/executor/i_executor.h"

namespace SprintIDE {
namespace Protocol {

/**
 * @brief 处理 env.* 域
 *
 * 阶段 3 实现：
 *  - env.query    查询环境（工具路径 / 版本）
 *
 * 阶段 4 或后续：
 *  - env.setSdk   设置项目级 SDK 环境变量
 */
class EnvExecutor : public QObject, public IExecutor
{
    Q_OBJECT
public:
    explicit EnvExecutor(QObject* parent = nullptr);

    QString domain() const override { return QStringLiteral("env"); }
    ExecutionResult execute(const QJsonObject& item) override;

private:
    ExecutionResult handleQuery(const QJsonObject& item);
    ExecutionResult handleSetSdk(const QJsonObject& item);   // 骨架

    /// 把一个查询键解析为 JSON 值（path 或 version 或 null）
    QJsonValue lookupEnvKey(const QString& key) const;

    /// 查询键 → 可执行文件名（"python.version" → "python3"）
    QString keyToExecutable(const QString& key) const;

    /// 执行 <exe> --version 取版本字符串
    QString execVersion(const QString& exeName) const;
};

} // namespace Protocol
} // namespace SprintIDE

#endif // ENV_EXECUTOR_H
