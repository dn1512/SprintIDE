#include "java_launcher.h"
#include "../path_utils.h"
#include "../process_manager.h"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>

namespace SprintIDE {

JavaLauncher::JavaLauncher(QObject* parent)
    : QObject(parent)
{
}

bool JavaLauncher::run(const RunConfiguration& config,
                       const QStringList& classpath,
                       const QString& workDir)
{
    if (m_isRunning) {
        qWarning() << "JavaLauncher: already running";
        return false;
    }

    QString javaCmd = findJavaExecutable();
    if (javaCmd.isEmpty()) {
        qWarning() << "JavaLauncher: Java executable not found";
        return false;
    }

    // 构建命令
    QStringList args;
    if (config.isFatJar && !config.fatJarPath.isEmpty()) {
        // Fat JAR 模式：java -jar xxx.jar
        args << "-jar" << config.fatJarPath;
        // 添加程序参数
        args << config.programArgs;
    } else {
        // 普通模式：java -cp classpath MainClass
        // JVM 参数
        args << config.jvmArgs;
        // 类路径
        QString cp = classpath.join(QDir::listSeparator());
        args << "-cp" << cp;
        // 主类
        if (config.mainClass.isEmpty()) {
            qWarning() << "JavaLauncher: mainClass is empty";
            return false;
        }
        args << config.mainClass;
        // 程序参数
        args << config.programArgs;
    }

    // 创建进程
    m_process = new QProcess(this);
    m_process->setProgram(javaCmd);
    m_process->setArguments(args);

    // 设置工作目录
    QString workingDir = workDir.isEmpty() ? config.workingDir : workDir;
    if (!workingDir.isEmpty()) {
        m_process->setWorkingDirectory(workingDir);
    }

    // 设置环境变量
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    for (auto it = config.environmentVariables.begin(); it != config.environmentVariables.end(); ++it) {
        env.insert(it.key(), it.value().toString());
    }
    m_process->setProcessEnvironment(env);

    // 连接信号
    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &JavaLauncher::onReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &JavaLauncher::onReadyReadStandardError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &JavaLauncher::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &JavaLauncher::onProcessError);

    // 启动
    m_process->start();
    if (!m_process->waitForStarted(5000)) {
        qWarning() << "JavaLauncher: failed to start process:" << m_process->errorString();
        delete m_process;
        m_process = nullptr;
        return false;
    }

    m_isRunning = true;
    emit programStarted();
    qDebug() << "JavaLauncher: program started";
    return true;
}

void JavaLauncher::stop()
{
    if (!m_isRunning || !m_process) {
        return;
    }

    qDebug() << "JavaLauncher: stopping program";
    m_process->terminate();
    if (!m_process->waitForFinished(3000)) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }
    m_isRunning = false;
}

bool JavaLauncher::isRunning() const
{
    return m_isRunning;
}

void JavaLauncher::onReadyReadStandardOutput()
{
    if (!m_process) return;
    QString output = QString::fromLocal8Bit(m_process->readAllStandardOutput());
    if (!output.isEmpty()) {
        emit outputReceived(output, false);
    }
}

void JavaLauncher::onReadyReadStandardError()
{
    if (!m_process) return;
    QString output = QString::fromLocal8Bit(m_process->readAllStandardError());
    if (!output.isEmpty()) {
        emit outputReceived(output, true);
    }
}

void JavaLauncher::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    m_isRunning = false;
    // 读取剩余输出
    if (m_process) {
        QString out = QString::fromLocal8Bit(m_process->readAllStandardOutput());
        if (!out.isEmpty()) {
            emit outputReceived(out, false);
        }
        QString err = QString::fromLocal8Bit(m_process->readAllStandardError());
        if (!err.isEmpty()) {
            emit outputReceived(err, true);
        }
    }
    emit programFinished(exitCode);
    qDebug() << "JavaLauncher: program finished with exit code" << exitCode;
}

void JavaLauncher::onProcessError(QProcess::ProcessError error)
{
    m_isRunning = false;
    QString errorMsg;
    switch (error) {
    case QProcess::FailedToStart:
        errorMsg = "启动失败";
        break;
    case QProcess::Crashed:
        errorMsg = "程序崩溃";
        break;
    case QProcess::Timedout:
        errorMsg = "超时";
        break;
    default:
        errorMsg = "未知错误";
        break;
    }
    qWarning() << "JavaLauncher: process error:" << errorMsg;
    emit outputReceived("程序错误: " + errorMsg, true);
}

QString JavaLauncher::findJavaExecutable() const
{
    // 优先从环境变量 JAVA_HOME 获取
    QString javaHome = QProcessEnvironment::systemEnvironment().value("JAVA_HOME");
    if (!javaHome.isEmpty()) {
        QString javaPath = PathUtils::joinPaths({javaHome, "bin", "java"});
#ifdef Q_OS_WIN
        javaPath += ".exe";
#endif
        if (QFile::exists(javaPath)) {
            return javaPath;
        }
    }

    // 从 PATH 查找
    QString javaCmd = "java";
#ifdef Q_OS_WIN
    javaCmd = "java.exe";
#endif
    ProcessManager pm;
    ProcessManager::ProcessResult result = pm.executeSync(
        javaCmd,
        {"-version"},
        QString(),
        2000
    );
    if (result.success) {
        return javaCmd;
    }

    return QString();
}

} // namespace SprintIDE
