#ifndef IDE_EXECUTOR_H
#define IDE_EXECUTOR_H

#include <QObject>
#include <QString>
#include <QTimer>

#include "ai/executor/i_executor.h"

namespace SprintIDE {

class MainWindow;

namespace Protocol {

class IdeExecutor : public QObject, public IExecutor
{
    Q_OBJECT
public:
    explicit IdeExecutor(MainWindow* mw, QObject* parent = nullptr);

    QString domain() const override { return QStringLiteral("ide"); }
    ExecutionResult execute(const QJsonObject& item) override;

signals:
    void deferredResultReady(const QString& itemId,
                             const SprintIDE::Protocol::ExecutionResult& result);

private slots:
    void onBuildFinished(bool success, int exitCode,
                         const QString& log, int durationMs);
    void onRunFinished(int exitCode, const QString& log);
    void onRunStopped(int exitCode, const QString& log);

private:
    ExecutionResult handleBuild(const QJsonObject& item);
    ExecutionResult handleRun(const QJsonObject& item);
    ExecutionResult handleStopRun(const QJsonObject& item);

    MainWindow* m_mw = nullptr;
    QString     m_pendingBuildItemId;
    QString     m_pendingRunItemId;
    QString     m_pendingStopItemId;
};

} // namespace Protocol
} // namespace SprintIDE

#endif // IDE_EXECUTOR_H
