#include "ai/executor/ide_executor.h"
#include "ai/protocol/protocol_types.h"
#include "mainwindow.h"

#include <QDebug>

namespace SprintIDE {
namespace Protocol {

IdeExecutor::IdeExecutor(MainWindow* mw, QObject* parent)
    : QObject(parent)
    , m_mw(mw)
{
    if (m_mw) {
        connect(m_mw, &MainWindow::aiBuildFinished,
                this, &IdeExecutor::onBuildFinished);
        connect(m_mw, &MainWindow::aiRunFinished,
                this, &IdeExecutor::onRunFinished);
        connect(m_mw, &MainWindow::aiRunStopped,
                this, &IdeExecutor::onRunStopped);
    }
}

ExecutionResult IdeExecutor::execute(const QJsonObject& item)
{
    const QString type = item.value("type").toString();

    if (type == QStringLiteral("ide.build")) return handleBuild(item);
    if (type == QStringLiteral("ide.run"))   return handleRun(item);
    if (type == QStringLiteral("ide.stopRun")) return handleStopRun(item);
    
    return ExecutionResult::fail(
        item.value("id").toString(), type,
        ErrorCode::UnsupportedType,
        QStringLiteral("IdeExecutor not yet implemented for type: %1").arg(type));
}

ExecutionResult IdeExecutor::handleBuild(const QJsonObject& item)
{
    if (!m_mw) {
        return ExecutionResult::fail(
            item.value("id").toString(), QStringLiteral("ide.build"),
            ErrorCode::InternalError,
            QStringLiteral("MainWindow is null"));
    }

    if (!m_pendingBuildItemId.isEmpty()) {
        return ExecutionResult::fail(
            item.value("id").toString(), QStringLiteral("ide.build"),
            ErrorCode::InternalError,
            QStringLiteral("Another build is already in progress"));
    }

    m_pendingBuildItemId = item.value("id").toString();

    // 触发异步构建
    m_mw->aiRequestBuild();

    return ExecutionResult::pending(m_pendingBuildItemId,
                                    QStringLiteral("ide.build"));
}

void IdeExecutor::onBuildFinished(bool success, int exitCode,
                                  const QString& log, int durationMs)
{
    if (m_pendingBuildItemId.isEmpty()) {
        return;   // 不是 AI 触发的构建
    }

    QJsonObject data;
    data["operation"]  = QStringLiteral("build");
    data["status"]     = success ? QStringLiteral("completed")
                                 : QStringLiteral("failed");
    data["exitCode"]   = exitCode;
    data["log"]        = log;
    data["durationMs"] = durationMs;

    ExecutionResult r = success
        ? ExecutionResult::ok(m_pendingBuildItemId, QStringLiteral("ide.build"), data)
        : ExecutionResult::fail(m_pendingBuildItemId, QStringLiteral("ide.build"),
                                ErrorCode::BuildFailed,
                                QStringLiteral("Build failed"), data);

    const QString itemId = m_pendingBuildItemId;
    m_pendingBuildItemId.clear();

    emit deferredResultReady(itemId, r);
}

ExecutionResult IdeExecutor::handleRun(const QJsonObject& item)
{
    if (!m_mw) {
        return ExecutionResult::fail(
            item.value("id").toString(), QStringLiteral("ide.run"),
            ErrorCode::InternalError,
            QStringLiteral("MainWindow is null"));
    }

    if (!m_pendingRunItemId.isEmpty()) {
        return ExecutionResult::fail(
            item.value("id").toString(), QStringLiteral("ide.run"),
            ErrorCode::InternalError,
            QStringLiteral("Another run is already in progress"));
    }

    m_pendingRunItemId = item.value("id").toString();

    m_mw->aiRequestRun();

    return ExecutionResult::pending(m_pendingRunItemId,
                                    QStringLiteral("ide.run"));
}

void IdeExecutor::onRunFinished(int exitCode, const QString& log)
{
    if (m_pendingRunItemId.isEmpty()) {
        return;
    }

    QJsonObject data;
    data["operation"]  = QStringLiteral("run");
    data["status"]     = QStringLiteral("completed");
    data["exitCode"]   = exitCode;
    data["log"]        = log;

    ExecutionResult r = ExecutionResult::ok(
        m_pendingRunItemId, QStringLiteral("ide.run"), data);

    const QString itemId = m_pendingRunItemId;
    m_pendingRunItemId.clear();

    emit deferredResultReady(itemId, r);
}

ExecutionResult IdeExecutor::handleStopRun(const QJsonObject& item)
{
    if (!m_mw) {
        return ExecutionResult::fail(
            item.value("id").toString(), QStringLiteral("ide.stopRun"),
            ErrorCode::InternalError,
            QStringLiteral("MainWindow is null"));
    }

    if (!m_pendingStopItemId.isEmpty()) {
        return ExecutionResult::fail(
            item.value("id").toString(), QStringLiteral("ide.stopRun"),
            ErrorCode::InternalError,
            QStringLiteral("Another stopRun is already in progress"));
    }

    m_pendingStopItemId = item.value("id").toString();

    // 延后一拍再触发停止：让 handleStopRun 先返回 Pending，
    // 使 OperationBus 建立 m_itemToRequest["s1"] 映射。
    // 否则 waitForFinished 内部的事件循环会提前把 deferredResultReady
    // 投递出去，此时映射还没建好，回执会被拒。
    QTimer::singleShot(0, this, [this]() {
        if (m_mw) m_mw->aiRequestStopRun();
    });

    return ExecutionResult::pending(m_pendingStopItemId,
                                    QStringLiteral("ide.stopRun"));
}

void IdeExecutor::onRunStopped(int exitCode, const QString& log)
{
    // 关键：用 QTimer 延后 emit。原因见 1.x —— stopRunningProcess 里的
    // waitForFinished 会同步触发本函数，此时 handleStopRun 还没返回，
    // OperationBus 的 m_itemToRequest 映射还没建立，emit 会被拒绝。
    if (!m_pendingRunItemId.isEmpty()) {
        QJsonObject data;
        data["operation"]   = QStringLiteral("run");
        data["status"]      = QStringLiteral("completed");
        data["exitCode"]    = exitCode;
        data["stopReason"]  = QStringLiteral("agentStopped");
        data["log"]         = log;

        const QString runItemId = m_pendingRunItemId;
        m_pendingRunItemId.clear();

        QTimer::singleShot(0, this, [this, runItemId, data]() {
            emit deferredResultReady(
                runItemId,
                ExecutionResult::ok(runItemId, QStringLiteral("ide.run"), data));
        });
    }

    if (!m_pendingStopItemId.isEmpty()) {
        QJsonObject data;
        data["operation"]   = QStringLiteral("stopRun");
        data["status"]      = QStringLiteral("completed");
        data["exitCode"]    = exitCode;
        data["stopReason"]  = QStringLiteral("agentStopped");
        data["log"]         = log;

        const QString stopItemId = m_pendingStopItemId;
        m_pendingStopItemId.clear();

        QTimer::singleShot(0, this, [this, stopItemId, data]() {
            emit deferredResultReady(
                stopItemId,
                ExecutionResult::ok(stopItemId, QStringLiteral("ide.stopRun"), data));
        });
    }
}

} // namespace Protocol
} // namespace SprintIDE
