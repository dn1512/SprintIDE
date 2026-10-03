#include "ai/executor/command_executor.h"
#include "ai/protocol/protocol_types.h"
#include "project_manager.h"

#include <QProcess>
#include <QProcessEnvironment>
#include <QElapsedTimer>
#include <QDir>
#include <QJsonArray>
#include <QEventLoop>
#include <QTimer>

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
    proc.setProcessChannelMode(QProcess::SeparateChannels);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QJsonObject envObj = item.value("env").toObject();
    for (auto it = envObj.constBegin(); it != envObj.constEnd(); ++it) {
        env.insert(it.key(), it.value().toString());
    }
    proc.setProcessEnvironment(env);

    // ---- 事件驱动累积 ----
    QString stdoutBuf;
    QString stderrBuf;
    QObject::connect(&proc, &QProcess::readyReadStandardOutput, [&]() {
        stdoutBuf += QString::fromLocal8Bit(proc.readAllStandardOutput());
    });
    QObject::connect(&proc, &QProcess::readyReadStandardError, [&]() {
        stderrBuf += QString::fromLocal8Bit(proc.readAllStandardError());
    });

    QElapsedTimer timer;
    timer.start();

    proc.start();
    if (!proc.waitForStarted(3000)) {
        return ExecutionResult::fail(itemId, QStringLiteral("command.run"),
                                     ErrorCode::InternalError,
                                     QStringLiteral("Failed to start program: %1")
                                         .arg(proc.errorString()));
    }

    // ---- 用 QEventLoop 等 finished（或超时） ----
    QEventLoop loop;
    bool finishedNormally = false;
    QObject::connect(&proc,
                     QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     &loop,
                     [&](int, QProcess::ExitStatus) {
                         finishedNormally = true;
                         loop.quit();
                     });

    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);

    if (timeoutMs > 0) {
        timeoutTimer.start(timeoutMs);
    }
    loop.exec();

    const bool timedOut = !finishedNormally;

    if (timedOut) {
        proc.terminate();
        if (!proc.waitForFinished(1000)) {
            proc.kill();
            proc.waitForFinished(500);
        }
    }

    // ---- 补读 socket 残留 ----
    stdoutBuf += QString::fromLocal8Bit(proc.readAllStandardOutput());
    stderrBuf += QString::fromLocal8Bit(proc.readAllStandardError());

    const int exitCode = finishedNormally ? proc.exitCode() : -1;

    // ---- 截断 ----
    constexpr int kMaxChunk = 64 * 1024;
    bool truncated = false;
    if (stdoutBuf.size() > kMaxChunk) { stdoutBuf = stdoutBuf.left(kMaxChunk); truncated = true; }
    if (stderrBuf.size() > kMaxChunk) { stderrBuf = stderrBuf.left(kMaxChunk); truncated = true; }

    QJsonObject data;
    data["commandType"]      = QStringLiteral("run");
    data["exitCode"]         = exitCode;
    data["stdout"]           = stdoutBuf;
    data["stderr"]           = stderrBuf;
    data["truncated"]        = truncated;
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
    const QString itemId = item.value("id").toString();
    const QString script = item.value("script").toString();

    QString shell = item.value("shell").toString();
    if (shell.isEmpty()) {
#ifdef Q_OS_WIN
        shell = QStringLiteral("cmd.exe");
#else
        shell = QStringLiteral("/bin/sh");
#endif
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
        return ExecutionResult::fail(itemId, QStringLiteral("command.runShell"),
                                     ErrorCode::PathNotFound,
                                     QStringLiteral("Working directory not found: %1").arg(workDir));
    }

    const int timeoutMs = item.value("timeoutMs").toInt(0);

    QProcess proc;
#ifdef Q_OS_WIN
    proc.setProgram(shell);
    proc.setArguments({QStringLiteral("/c"), script});
#else
    proc.setProgram(shell);
    proc.setArguments({QStringLiteral("-c"), script});
#endif
    proc.setWorkingDirectory(workDir);
    proc.setProcessChannelMode(QProcess::SeparateChannels);

    // ---- 事件驱动累积 ----
    QString stdoutBuf;
    QString stderrBuf;
    QObject::connect(&proc, &QProcess::readyReadStandardOutput, [&]() {
        stdoutBuf += QString::fromLocal8Bit(proc.readAllStandardOutput());
    });
    QObject::connect(&proc, &QProcess::readyReadStandardError, [&]() {
        stderrBuf += QString::fromLocal8Bit(proc.readAllStandardError());
    });

    QElapsedTimer timer;
    timer.start();

    proc.start();
    if (!proc.waitForStarted(3000)) {
        return ExecutionResult::fail(itemId, QStringLiteral("command.runShell"),
                                     ErrorCode::InternalError,
                                     QStringLiteral("Failed to start shell: %1")
                                         .arg(proc.errorString()));
    }

    // ---- QEventLoop 等 finished（或超时） ----
    QEventLoop loop;
    bool finishedNormally = false;
    QObject::connect(&proc,
                     QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     &loop,
                     [&](int, QProcess::ExitStatus) {
                         finishedNormally = true;
                         loop.quit();
                     });

    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);

    if (timeoutMs > 0) {
        timeoutTimer.start(timeoutMs);
    }
    loop.exec();

    const bool timedOut = !finishedNormally;

    if (timedOut) {
        proc.terminate();
        if (!proc.waitForFinished(1000)) {
            proc.kill();
            proc.waitForFinished(500);
        }
    }

    // ---- 补读 socket 残留 ----
    stdoutBuf += QString::fromLocal8Bit(proc.readAllStandardOutput());
    stderrBuf += QString::fromLocal8Bit(proc.readAllStandardError());

    const int exitCode = finishedNormally ? proc.exitCode() : -1;

    // ---- 截断 ----
    constexpr int kMaxChunk = 64 * 1024;
    bool truncated = false;
    if (stdoutBuf.size() > kMaxChunk) { stdoutBuf = stdoutBuf.left(kMaxChunk); truncated = true; }
    if (stderrBuf.size() > kMaxChunk) { stderrBuf = stderrBuf.left(kMaxChunk); truncated = true; }

    QJsonObject data;
    data["commandType"]      = QStringLiteral("runShell");
    data["exitCode"]         = exitCode;
    data["stdout"]           = stdoutBuf;
    data["stderr"]           = stderrBuf;
    data["truncated"]        = truncated;
    data["durationMs"]       = static_cast<int>(timer.elapsed());
    data["timedOut"]         = timedOut;
    data["workingDirectory"] = workDir;

    if (timedOut) {
        return ExecutionResult::fail(itemId, QStringLiteral("command.runShell"),
                                     ErrorCode::Timeout,
                                     QStringLiteral("Shell script timed out after %1ms").arg(timeoutMs),
                                     data);
    }
    if (exitCode != 0) {
        return ExecutionResult::fail(itemId, QStringLiteral("command.runShell"),
                                     ErrorCode::ExitCodeNonZero,
                                     QStringLiteral("Exit code: %1").arg(exitCode),
                                     data);
    }
    return ExecutionResult::ok(itemId, QStringLiteral("command.runShell"), data);
}

// ------------------------------------------------------------
// command.installDep（骨架，阶段 3 补）
// ------------------------------------------------------------
ExecutionResult CommandExecutor::handleInstallDep(const QJsonObject& item)
{
    const QString itemId   = item.value("id").toString();
    const QString manager  = item.value("manager").toString();
    const QString target   = item.value("target").toString();

    QStringList packages;
    for (const QJsonValue& v : item.value("packages").toArray()) {
        packages.append(v.toString());
    }

    if (packages.isEmpty()) {
        return ExecutionResult::fail(itemId, QStringLiteral("command.installDep"),
                                     ErrorCode::InvalidFields,
                                     QStringLiteral("No packages specified"));
    }

    // ---- 根据 manager 构造命令行 ----
    QString program;
    QStringList args;

    if (manager == QStringLiteral("pip")) {
        program = QStringLiteral("python3");
        args << QStringLiteral("-m") << QStringLiteral("pip")
             << QStringLiteral("install");
        if (!target.isEmpty()) {
            args << QStringLiteral("--target") << target;
        }
        args << packages;

    } else if (manager == QStringLiteral("poetry")) {
        program = QStringLiteral("poetry");
        args << QStringLiteral("add") << packages;

    } else if (manager == QStringLiteral("npm")) {
        program = QStringLiteral("npm");
        args << QStringLiteral("install") << packages;

    } else if (manager == QStringLiteral("cargo")) {
        program = QStringLiteral("cargo");
        args << QStringLiteral("add") << packages;

    } else if (manager == QStringLiteral("go")) {
        program = QStringLiteral("go");
        args << QStringLiteral("get") << packages;

    } else {
        return ExecutionResult::fail(itemId, QStringLiteral("command.installDep"),
                                     ErrorCode::InvalidFields,
                                     QStringLiteral("Unsupported manager: %1").arg(manager));
    }

    // ---- 工作目录 ----
    QString workDir = ProjectManager::instance()
                          ? ProjectManager::instance()->currentProjectPath()
                          : QString();
    if (workDir.isEmpty()) {
        workDir = QDir::currentPath();
    }

    // ---- 执行 ----
    QProcess proc;
    proc.setProgram(program);
    proc.setArguments(args);
    proc.setWorkingDirectory(workDir);
    proc.setProcessChannelMode(QProcess::SeparateChannels);

    QString stdoutBuf;
    QString stderrBuf;
    QObject::connect(&proc, &QProcess::readyReadStandardOutput, [&]() {
        stdoutBuf += QString::fromLocal8Bit(proc.readAllStandardOutput());
    });
    QObject::connect(&proc, &QProcess::readyReadStandardError, [&]() {
        stderrBuf += QString::fromLocal8Bit(proc.readAllStandardError());
    });

    QElapsedTimer timer;
    timer.start();

    proc.start();
    if (!proc.waitForStarted(3000)) {
        return ExecutionResult::fail(itemId, QStringLiteral("command.installDep"),
                                     ErrorCode::InternalError,
                                     QStringLiteral("Failed to start %1: %2")
                                         .arg(program, proc.errorString()));
    }

    // 包管理器可能很慢，硬上限 5 分钟
    constexpr int kInstallTimeoutMs = 5 * 60 * 1000;

    QEventLoop loop;
    bool finishedNormally = false;
    QObject::connect(&proc,
                     QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     &loop,
                     [&](int, QProcess::ExitStatus) {
                         finishedNormally = true;
                         loop.quit();
                     });

    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeoutTimer.start(kInstallTimeoutMs);
    loop.exec();

    const bool timedOut = !finishedNormally;

    if (timedOut) {
        proc.terminate();
        if (!proc.waitForFinished(2000)) {
            proc.kill();
            proc.waitForFinished(1000);
        }
    }

    stdoutBuf += QString::fromLocal8Bit(proc.readAllStandardOutput());
    stderrBuf += QString::fromLocal8Bit(proc.readAllStandardError());

    const int exitCode = finishedNormally ? proc.exitCode() : -1;

    constexpr int kMaxChunk = 64 * 1024;
    bool truncated = false;
    if (stdoutBuf.size() > kMaxChunk) { stdoutBuf = stdoutBuf.left(kMaxChunk); truncated = true; }
    if (stderrBuf.size() > kMaxChunk) { stderrBuf = stderrBuf.left(kMaxChunk); truncated = true; }

    QJsonObject data;
    data["commandType"] = QStringLiteral("installDep");
    data["manager"]     = manager;
    data["packages"]    = QJsonArray::fromStringList(packages);
    data["exitCode"]    = exitCode;
    data["stdout"]      = stdoutBuf;
    data["stderr"]      = stderrBuf;
    data["truncated"]   = truncated;
    data["durationMs"]  = static_cast<int>(timer.elapsed());
    data["timedOut"]    = timedOut;

    if (timedOut) {
        return ExecutionResult::fail(itemId, QStringLiteral("command.installDep"),
                                     ErrorCode::Timeout,
                                     QStringLiteral("Install timed out"),
                                     data);
    }
    if (exitCode != 0) {
        return ExecutionResult::fail(itemId, QStringLiteral("command.installDep"),
                                     ErrorCode::ExitCodeNonZero,
                                     QStringLiteral("Install failed with exit code %1").arg(exitCode),
                                     data);
    }
    return ExecutionResult::ok(itemId, QStringLiteral("command.installDep"), data);
}

} // namespace Protocol
} // namespace SprintIDE
