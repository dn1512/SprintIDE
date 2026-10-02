#include "ai/executor/command_executor.h"
#include "ai/protocol/protocol_types.h"
#include "project_manager.h"

#include <QProcess>
#include <QProcessEnvironment>
#include <QElapsedTimer>
#include <QDir>
#include <QJsonArray>

namespace SprintIDE {
namespace Protocol {

CommandExecutor::CommandExecutor(QObject* parent)
    : QObject(parent)
{
}

ExecutionResult CommandExecutor::execute(const QJsonObject& item)
{
    const QString type = item.value("type").toString();

    if (type == QStringLiteral("command.run"))        return handleRun(item);
    if (type == QStringLiteral("command.runShell"))   return handleRunShell(item);
    if (type == QStringLiteral("command.installDep")) return handleInstallDep(item);

    return ExecutionResult::fail(
        item.value("id").toString(), type,
        ErrorCode::UnsupportedType,
        QStringLiteral("CommandExecutor cannot handle type: %1").arg(type));
}

// ------------------------------------------------------------
// command.run（同步阻塞；阶段 3 改异步 + TerminalPanel）
// ------------------------------------------------------------
ExecutionResult CommandExecutor::handleRun(const QJsonObject& item)
{
    const QString itemId  = item.value("id").toString();
    const QString program = item.value("program").toString();

    QStringList args;
    for (const QJsonValue& v : item.value("arguments").toArray()) {
        args.append(v.toString());
    }

    QString workDir = item.value("workingDirectory").toString();
    if (workDir.isEmpty()) {
        ProjectManager* pm = ProjectManager::instance();
        workDir = pm ? pm->currentProjectPath() : QString();
    }
    if (workDir.isEmpty()) {
        workDir = QDir::currentPath();
    }
    if (!QDir(workDir).exists()) {
        return ExecutionResult::fail(itemId, QStringLiteral("command.run"),
                                     ErrorCode::PathNotFound,
                                     QStringLiteral("Working directory not found: %1").arg(workDir));
    }

    const int timeoutMs = item.value("timeoutMs").toInt(0);

    QProcess proc;
    proc.setProgram(program);
    proc.setArguments(args);
    proc.setWorkingDirectory(workDir);
    proc.setProcessChannelMode(QProcess::MergedChannels);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QJsonObject envObj = item.value("env").toObject();
    for (auto it = envObj.constBegin(); it != envObj.constEnd(); ++it) {
        env.insert(it.key(), it.value().toString());
    }
    proc.setProcessEnvironment(env);

    QElapsedTimer timer;
    timer.start();

    proc.start();

    bool finished = false;
    if (timeoutMs > 0) {
        finished = proc.waitForFinished(timeoutMs);
    } else {
        finished = proc.waitForFinished(-1);
    }

    const QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());
    const int     exitCode = finished ? proc.exitCode() : -1;
    const bool    timedOut = !finished;

    QJsonObject data;
    data["commandType"]      = QStringLiteral("run");
    data["exitCode"]         = exitCode;
    data["stdout"]           = output;
    data["stderr"]           = QString();
    data["truncated"]        = false;
    data["durationMs"]       = static_cast<int>(timer.elapsed());
    data["timedOut"]         = timedOut;
    data["workingDirectory"] = workDir;

    if (timedOut) {
        return ExecutionResult::fail(itemId, QStringLiteral("command.run"),
                                     ErrorCode::Timeout,
                                     QStringLiteral("Command timed out after %1ms").arg(timeoutMs),
                                     data);
    }
    if (exitCode != 0) {
        return ExecutionResult::fail(itemId, QStringLiteral("command.run"),
                                     ErrorCode::ExitCodeNonZero,
                                     QStringLiteral("Exit code: %1").arg(exitCode),
                                     data);
    }
    return ExecutionResult::ok(itemId, QStringLiteral("command.run"), data);
}

// ------------------------------------------------------------
// command.runShell（骨架，阶段 3 补）
// ------------------------------------------------------------
ExecutionResult CommandExecutor::handleRunShell(const QJsonObject& item)
{
    return ExecutionResult::fail(
        item.value("id").toString(), QStringLiteral("command.runShell"),
        ErrorCode::UnsupportedType,
        QStringLiteral("command.runShell not yet implemented (planned for Phase 3)"));
}

// ------------------------------------------------------------
// command.installDep（骨架，阶段 3 补）
// ------------------------------------------------------------
ExecutionResult CommandExecutor::handleInstallDep(const QJsonObject& item)
{
    return ExecutionResult::fail(
        item.value("id").toString(), QStringLiteral("command.installDep"),
        ErrorCode::UnsupportedType,
        QStringLiteral("command.installDep not yet implemented (planned for Phase 3)"));
}

} // namespace Protocol
} // namespace SprintIDE
