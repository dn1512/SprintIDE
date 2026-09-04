#include "process_manager.h"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDateTime>

namespace SprintIDE {

ProcessManager::ProcessManager(QObject* parent)
    : QObject(parent)
    , m_nextProcessId(1)
{
    qDebug() << "进程管理器初始化完成";
}

ProcessManager::~ProcessManager()
{
    // 停止所有进程
    stopProcess();
    clearHistory();
    cleanupAll();
    qDebug() << "进程管理器已销毁";
}

ProcessManager::ProcessResult ProcessManager::executeSync(
    const QString& command,
    const QStringList& arguments,
    const QString& workingDirectory,
    int timeoutMs,
    const QString& encoding)
{
    qDebug() << "同步执行命令:" << command << arguments;

    ProcessResult result;
    result.success = false;
    result.executionTime = 0;

    QElapsedTimer timer;
    timer.start();

    if (!validateCommand(command)) {
        result.errorMessage = QString("命令不可用或不存在: %1").arg(command);
        qWarning() << result.errorMessage;
        return result;
    }

    QProcess process;

    if (!workingDirectory.isEmpty()) {
        process.setWorkingDirectory(workingDirectory);
    }

    if (!m_environment.isEmpty()) {
        process.setProcessEnvironment(m_environment);
    }

    // 【关键修改】不要使用 MergedChannels，分别读取 stdout 和 stderr
    process.setProcessChannelMode(QProcess::SeparateChannels);

    // 连接信号
    QObject::connect(&process, &QProcess::readyReadStandardOutput,
                     [&process, &result]() {
                         QByteArray data = process.readAllStandardOutput();
                         result.standardOutput += QString::fromLocal8Bit(data);
                     });

    QObject::connect(&process, &QProcess::readyReadStandardError,
                     [&process, &result]() {
                         QByteArray data = process.readAllStandardError();
                         result.standardError += QString::fromLocal8Bit(data);
                     });

    process.start(command, arguments);

    if (!process.waitForStarted()) {
        result.errorMessage = QString("无法启动进程: %1").arg(process.errorString());
        qWarning() << result.errorMessage;
        return result;
    }

    bool finished = false;
    if (timeoutMs > 0) {
        finished = process.waitForFinished(timeoutMs);
    } else {
        process.waitForFinished(-1);
        finished = true;
    }

    result.executionTime = timer.elapsed();

    if (!finished) {
        process.terminate();
        if (!process.waitForFinished(5000)) {
            process.kill();
            process.waitForFinished(1000);
        }
        result.errorMessage = timeoutMs > 0 ?
                                  QString("进程执行超时 (%1 毫秒)").arg(timeoutMs) :
                                  "进程被中断";
        result.exitCode = -1;
    } else {
        result.exitCode = process.exitCode();
        result.success = (result.exitCode == 0);
        if (!result.success) {
            result.errorMessage = QString("进程退出码: %1").arg(result.exitCode);
        }
        // 读取剩余输出（确保完整）
        result.standardOutput += QString::fromLocal8Bit(process.readAllStandardOutput());
        result.standardError += QString::fromLocal8Bit(process.readAllStandardError());
    }

    qDebug() << QString("命令执行完成: %1, 耗时: %2ms, 退出码: %3")
                    .arg(command)
                    .arg(result.executionTime)
                    .arg(result.exitCode);

    return result;
}

bool ProcessManager::executeAsync(
    const QString& command,
    const QStringList& arguments,
    const QString& workingDirectory,
    const QString& processId)
{
    QString actualProcessId = processId.isEmpty() ?
        createProcessId(command, arguments) : processId;

    qDebug() << "异步执行命令:" << command << arguments << "进程ID:" << actualProcessId;

    // 清理僵尸进程
    cleanupZombieProcesses();

    // 验证命令
    if (!validateCommand(command)) {
        QString error = QString("命令不可用或不存在: %1").arg(command);
        qWarning() << error;
        emit processError(actualProcessId, error);
        return false;
    }

    // 创建进程数据
    ProcessData data;
    data.process = new QProcess(this);
    data.timeoutTimer = new QTimer(this);
    data.isFinished = false;

    // 初始化结果
    data.result.success = false;
    data.result.exitCode = -1;
    data.result.executionTime = 0;

    // 设置进程
    if (!workingDirectory.isEmpty()) {
        data.process->setWorkingDirectory(workingDirectory);
    }

    if (!m_environment.isEmpty()) {
        data.process->setProcessEnvironment(m_environment);
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    data.process->setProcessChannelMode(QProcess::SeparateChannels);
#else
    data.process->setProcessChannelMode(QProcess::SeparateChannels);
#endif

    // 连接信号到私有槽
    connect(data.process, &QProcess::readyReadStandardOutput,
            this, &ProcessManager::onProcessReadyRead);
    connect(data.process, &QProcess::readyReadStandardError,
            this, &ProcessManager::onProcessReadyReadError);
    connect(data.process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ProcessManager::onProcessFinished);
    connect(data.process, &QProcess::errorOccurred,
            this, &ProcessManager::onProcessError);

    // 设置超时定时器
    data.timeoutTimer->setSingleShot(true);
    connect(data.timeoutTimer, &QTimer::timeout,
            this, &ProcessManager::onTimeout);
    data.timeoutTimer->setProperty("processId", actualProcessId);

    // 启动计时器
    data.elapsedTimer.start();

    // 保存进程数据
    m_processes[actualProcessId] = data;

    // 启动进程
    data.process->start(command, arguments);

    if (!data.process->waitForStarted(5000)) {
        QString error = QString("无法启动进程: %1").arg(data.process->errorString());
        qWarning() << error;

        // 清理资源
        delete data.process;
        delete data.timeoutTimer;
        m_processes.remove(actualProcessId);

        emit processError(actualProcessId, error);
        return false;
    }

    // 启动超时定时器（默认30秒）
    data.timeoutTimer->start(30000);

    qDebug() << "进程启动成功，进程ID:" << actualProcessId;
    emit processStarted(actualProcessId);

    return true;
}

bool ProcessManager::stopProcess(const QString& processId)
{
    if (processId.isEmpty()) {
        // 停止所有进程
        bool anyStopped = false;
        for (auto it = m_processes.begin(); it != m_processes.end(); ++it) {
            if (it->process->state() != QProcess::NotRunning) {
                it->process->terminate();
                anyStopped = true;
            }
        }
        return anyStopped;
    }

    // 停止特定进程
    if (!m_processes.contains(processId)) {
        qWarning() << "停止进程失败: 未找到进程ID" << processId;
        return false;
    }

    ProcessData& data = m_processes[processId];
    if (data.process->state() == QProcess::NotRunning) {
        qWarning() << "进程已停止，进程ID:" << processId;
        return false;
    }

    qDebug() << "正在停止进程，进程ID:" << processId;

    // 停止超时定时器
    if (data.timeoutTimer->isActive()) {
        data.timeoutTimer->stop();
    }

    // 终止进程
    data.process->terminate();
    if (!data.process->waitForFinished(5000)) {
        data.process->kill();
        data.process->waitForFinished(1000);
    }

    return true;
}

QProcess::ProcessState ProcessManager::processState(const QString& processId) const
{
    if (!m_processes.contains(processId)) {
        return QProcess::NotRunning;
    }

    const ProcessData& data = m_processes[processId];
    return data.process->state();
}

ProcessManager::ProcessResult ProcessManager::processResult(const QString& processId) const
{
    if (!m_processes.contains(processId)) {
        ProcessResult result;
        result.success = false;
        result.errorMessage = "进程不存在";
        return result;
    }

    const ProcessData& data = m_processes[processId];
    ProcessResult result = data.result;

    // 如果进程还在运行，更新执行时间
    if (!data.isFinished) {
        result.executionTime = data.elapsedTimer.elapsed();
    }

    return result;
}

void ProcessManager::setEnvironment(const QProcessEnvironment& env)
{
    m_environment = env;
    qDebug() << "设置环境变量，数量:" << env.keys().size();
}

void ProcessManager::clearHistory()
{
    qDebug() << "清理进程历史，当前进程数:" << m_processes.size();

    // 停止所有进程
    stopProcess();

    // 删除所有进程对象
    for (auto it = m_processes.begin(); it != m_processes.end(); ++it) {
        ProcessData& data = it.value();

        if (data.timeoutTimer) {
            data.timeoutTimer->stop();
            data.timeoutTimer->deleteLater();
        }

        if (data.process) {
            if (data.process->state() != QProcess::NotRunning) {
                data.process->terminate();
                data.process->waitForFinished(1000);
            }
            data.process->deleteLater();
        }
    }

    m_processes.clear();
    qDebug() << "进程历史已清理";
}

// ==================== 私有槽函数 ====================

void ProcessManager::onProcessReadyRead()
{
    QProcess* process = qobject_cast<QProcess*>(sender());
    if (!process) return;

    // 查找对应的进程ID
    QString processId;
    for (auto it = m_processes.begin(); it != m_processes.end(); ++it) {
        if (it->process == process) {
            processId = it.key();
            break;
        }
    }

    if (processId.isEmpty()) return;

    // 读取标准输出
    QString output = QString::fromLocal8Bit(process->readAllStandardOutput());
    ProcessData& data = m_processes[processId];
    data.standardOutput += output;

    // 发射公共信号
    emit outputReceived(processId, output, false);
}

void ProcessManager::onProcessReadyReadError()
{
    QProcess* process = qobject_cast<QProcess*>(sender());
    if (!process) return;

    // 查找对应的进程ID
    QString processId;
    for (auto it = m_processes.begin(); it != m_processes.end(); ++it) {
        if (it->process == process) {
            processId = it.key();
            break;
        }
    }

    if (processId.isEmpty()) return;

    // 读取错误输出
    QString error = QString::fromLocal8Bit(process->readAllStandardError());
    ProcessData& data = m_processes[processId];
    data.standardError += error;

    // 发射公共信号
    emit outputReceived(processId, error, true);
}

void ProcessManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QProcess* process = qobject_cast<QProcess*>(sender());
    if (!process) return;

    // 查找对应的进程ID
    QString processId;
    for (auto it = m_processes.begin(); it != m_processes.end(); ++it) {
        if (it->process == process) {
            processId = it.key();
            break;
        }
    }

    if (processId.isEmpty()) return;

    ProcessData& data = m_processes[processId];

    // 停止超时定时器
    if (data.timeoutTimer && data.timeoutTimer->isActive()) {
        data.timeoutTimer->stop();
    }

    // 更新结果
    data.result.exitCode = exitCode;
    data.result.success = (exitStatus == QProcess::NormalExit && exitCode == 0);
    data.result.executionTime = data.elapsedTimer.elapsed();
    data.result.standardOutput = data.standardOutput;
    data.result.standardError = data.standardError;
    data.result.errorMessage = data.result.success ? QString() :
        QString("进程退出码: %1").arg(exitCode);

    data.isFinished = true;

    // 读取可能剩余的输出
    QString remainingOutput = QString::fromLocal8Bit(process->readAllStandardOutput());
    QString remainingError = QString::fromLocal8Bit(process->readAllStandardError());

    if (!remainingOutput.isEmpty()) {
        data.result.standardOutput += remainingOutput;
        emit outputReceived(processId, remainingOutput, false);
    }

    if (!remainingError.isEmpty()) {
        data.result.standardError += remainingError;
        emit outputReceived(processId, remainingError, true);
    }

    qDebug() << QString("进程完成，进程ID: %1, 退出码: %2, 耗时: %3ms")
                .arg(processId)
                .arg(exitCode)
                .arg(data.result.executionTime);

    // 发射公共完成信号
    emit processFinished(processId, data.result);

    // 发射进度更新信号
    emit progressUpdated(processId, data.result.executionTime);
}

void ProcessManager::onProcessError(QProcess::ProcessError error)
{
    QProcess* process = qobject_cast<QProcess*>(sender());
    if (!process) return;

    // 查找对应的进程ID
    QString processId;
    for (auto it = m_processes.begin(); it != m_processes.end(); ++it) {
        if (it->process == process) {
            processId = it.key();
            break;
        }
    }

    if (processId.isEmpty()) return;

    ProcessData& data = m_processes[processId];

    QString errorStr;
    switch (error) {
    case QProcess::FailedToStart:
        errorStr = "进程启动失败";
        break;
    case QProcess::Crashed:
        errorStr = "进程崩溃";
        break;
    case QProcess::Timedout:
        errorStr = "进程超时";
        break;
    case QProcess::ReadError:
        errorStr = "读取错误";
        break;
    case QProcess::WriteError:
        errorStr = "写入错误";
        break;
    default:
        errorStr = "未知进程错误";
        break;
    }

    errorStr += ": " + process->errorString();

    // 更新结果
    data.result.success = false;
    data.result.errorMessage = errorStr;
    data.result.executionTime = data.elapsedTimer.elapsed();
    data.isFinished = true;

    // 停止超时定时器
    if (data.timeoutTimer && data.timeoutTimer->isActive()) {
        data.timeoutTimer->stop();
    }

    qWarning() << "进程错误，进程ID:" << processId << "错误:" << errorStr;

    // 发射公共错误信号
    emit processError(processId, errorStr);
}

void ProcessManager::onTimeout()
{
    QTimer* timer = qobject_cast<QTimer*>(sender());
    if (!timer) return;

    QString processId = timer->property("processId").toString();
    if (processId.isEmpty() || !m_processes.contains(processId)) return;

    ProcessData& data = m_processes[processId];

    if (data.process->state() != QProcess::NotRunning) {
        qWarning() << "进程超时，进程ID:" << processId;

        // 终止进程
        data.process->terminate();
        if (!data.process->waitForFinished(5000)) {
            data.process->kill();
            data.process->waitForFinished(1000);
        }

        // 更新结果
        data.result.success = false;
        data.result.errorMessage = "进程执行超时";
        data.result.executionTime = data.elapsedTimer.elapsed();
        data.isFinished = true;

        // 发射公共错误信号
        emit processError(processId, "进程执行超时");
    }
}

// ==================== 私有方法 ====================

void ProcessManager::cleanupZombieProcesses()
{
    // 清理已完成的进程
    QList<QString> toRemove;

    for (auto it = m_processes.begin(); it != m_processes.end(); ++it) {
        ProcessData& data = it.value();

        if (data.isFinished || data.process->state() == QProcess::NotRunning) {
            // 进程已结束，可以清理
            toRemove.append(it.key());
        }
    }

    // 删除已完成的进程
    for (const QString& processId : toRemove) {
        ProcessData& data = m_processes[processId];

        if (data.timeoutTimer) {
            data.timeoutTimer->deleteLater();
        }

        if (data.process) {
            data.process->deleteLater();
        }

        m_processes.remove(processId);
        qDebug() << "清理僵尸进程:" << processId;
    }
}

bool ProcessManager::validateCommand(const QString& command) const
{
    if (command.isEmpty()) {
        return false;
    }

    // 检查是否为绝对路径
    if (QFileInfo(command).isAbsolute()) {
        return QFileInfo::exists(command);
    }

    // 检查是否在PATH环境变量中
    QStringList paths = QProcessEnvironment::systemEnvironment().value("PATH").split(
#ifdef Q_OS_WIN
        ";"
#else
        ":"
#endif
    );

    for (const QString& path : paths) {
        QDir dir(path);
        if (dir.exists(command)) {
            return true;
        }

        // 在Windows上，检查是否带.exe扩展名
#ifdef Q_OS_WIN
        QString exeCommand = command + ".exe";
        if (dir.exists(exeCommand)) {
            return true;
        }
#endif
    }

    return false;
}

QString ProcessManager::createProcessId(const QString& command, const QStringList& arguments) const
{
    QString baseId = QString("%1_%2_%3")
        .arg(QFileInfo(command).fileName())
        .arg(QDateTime::currentMSecsSinceEpoch())
        .arg(m_nextProcessId + 1);

    // 如果参数非空，添加参数哈希
    if (!arguments.isEmpty()) {
        QString argsStr = arguments.join("_");
        // 简单哈希
        uint hash = qHash(argsStr);
        baseId += QString("_%1").arg(hash, 0, 16);
    }

    return baseId;
}

void ProcessManager::cleanupAll()
{
    // 停止所有进程
    for (auto it = m_processes.begin(); it != m_processes.end(); ++it) {
        ProcessData& data = it.value();
        if (data.process && data.process->state() != QProcess::NotRunning) {
            data.process->terminate();
            data.process->waitForFinished(1000);
            if (data.process->state() != QProcess::NotRunning) {
                data.process->kill();
                data.process->waitForFinished(500);
            }
        }
    }
    clearHistory();
}

} // namespace SprintIDE
