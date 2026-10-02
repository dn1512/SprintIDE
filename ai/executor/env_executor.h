#ifndef ENV_EXECUTOR_H
#define ENV_EXECUTOR_H

#include <QObject>
#include "ai/executor/i_executor.h"

namespace SprintIDE {
namespace Protocol {

class EnvExecutor : public QObject, public IExecutor
{
    Q_OBJECT
public:
    explicit EnvExecutor(QObject* parent = nullptr);

    QString domain() const override { return QStringLiteral("env"); }
    ExecutionResult execute(const QJsonObject& item) override;
};

} // namespace Protocol
} // namespace SprintIDE

#endif // ENV_EXECUTOR_H
