#ifndef FILE_EXECUTOR_H
#define FILE_EXECUTOR_H

#include <QObject>
#include "ai/executor/i_executor.h"

namespace SprintIDE {
namespace Protocol {

/**
 * @brief 处理 file.* 域
 *
 * 阶段 1 实现：read / create / delete / rename
 * 阶段 3 实现：edit（需要 TextEditor + LocalHistory）
 */
class FileExecutor : public QObject, public IExecutor
{
    Q_OBJECT
public:
    explicit FileExecutor(QObject* parent = nullptr);

    QString domain() const override { return QStringLiteral("file"); }
    ExecutionResult execute(const QJsonObject& item) override;

private:
    ExecutionResult handleRead(const QJsonObject& item);
    ExecutionResult handleCreate(const QJsonObject& item);
    ExecutionResult handleDelete(const QJsonObject& item);
    ExecutionResult handleRename(const QJsonObject& item);
    ExecutionResult handleEdit(const QJsonObject& item);   // 骨架

    /// 相对路径 → 绝对路径（基于当前项目根；无项目则基于 cwd）
    QString resolvePath(const QString& rawPath) const;
};

} // namespace Protocol
} // namespace SprintIDE

#endif // FILE_EXECUTOR_H
