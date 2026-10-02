#ifndef IDE_EXECUTOR_H
#define IDE_EXECUTOR_H

#include <QObject>
#include "ai/executor/i_executor.h"

namespace SprintIDE {

class MainWindow;

namespace Protocol {

/**
 * @brief 处理 ide.* 域（骨架，阶段 2 填充实现）
 *
 * 依赖 MainWindow::aiRequest* 系列方法。
 */
class IdeExecutor : public QObject, public IExecutor
{
    Q_OBJECT
public:
    explicit IdeExecutor(MainWindow* mw, QObject* parent = nullptr);

    QString domain() const override { return QStringLiteral("ide"); }
    ExecutionResult execute(const QJsonObject& item) override;

private:
    MainWindow* m_mw = nullptr;
};

} // namespace Protocol
} // namespace SprintIDE

#endif // IDE_EXECUTOR_H
