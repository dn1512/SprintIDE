#ifndef PROJECT_EXECUTOR_H
#define PROJECT_EXECUTOR_H

#include <QObject>
#include "ai/executor/i_executor.h"

namespace SprintIDE {
namespace Protocol {

class ProjectExecutor : public QObject, public IExecutor
{
    Q_OBJECT
public:
    explicit ProjectExecutor(QObject* parent = nullptr);

    QString domain() const override { return QStringLiteral("project"); }
    ExecutionResult execute(const QJsonObject& item) override;
};

} // namespace Protocol
} // namespace SprintIDE

#endif // PROJECT_EXECUTOR_H
