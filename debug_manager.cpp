// debug_manager.cpp
#include "debug_manager.h"
#include "text_editor.h"
#include "process_manager.h"
#include "path_utils.h"
#include "config_manager.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QTimer>
#include <QThread>

namespace SprintIDE {

// ==================== Private 类 ====================

class DebugManager::Private
{
public:
    Private(DebugManager* q) : q_ptr(q) {}

    QProcess* debuggerProcess = nullptr;
    QString debuggerType;
    QString currentExecutable;
    QStringList currentArguments;
    QString workingDirectory;
    bool isDebugging = false;
    DebugState currentState = DebugState::Stopped;

    QMap<QString, Breakpoint> breakpoints;
    QMap<QString, int> breakpointIds;

    QList<StackFrame> callStack;
    QList<DebugVariable> localVariables;
    QString currentFile;
    int currentLine = 0;
    int stackDepth = 0;

    QList<TextEditor*> registeredEditors;

    QString outputBuffer;
    DebugConfig config;

    QList<QString> commandQueue;   // 待发送命令队列
    bool m_inDyld = false;  //dyld标志
private:
    DebugManager* q_ptr;
};


// ==================== 构造函数和析构函数 ====================

DebugManager::DebugManager(QObject* parent)
    : QObject(parent)
    , d(new Private(this))
{
    d->m_inDyld = false;
    d->commandQueue.clear();
    qDebug() << "DebugManager 创建";
}

DebugManager::~DebugManager()
{
    shutdown();
}

DebugManager& DebugManager::instance()
{
    static DebugManager instance;
    return instance;
}

// ==================== 初始化和关闭 ====================

bool DebugManager::initialize()
{
    qDebug() << "初始化调试管理器";

    // 如果已经初始化过，直接返回
    if (d->debuggerProcess) {
        qDebug() << "调试管理器已初始化";
        return true;
    }

    d->debuggerProcess = new QProcess(this);

    connect(d->debuggerProcess, &QProcess::readyReadStandardOutput,
            this, &DebugManager::onDebuggerReadyRead);
    connect(d->debuggerProcess, &QProcess::readyReadStandardError,
            this, &DebugManager::onDebuggerReadyReadError);
    connect(d->debuggerProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &DebugManager::onDebuggerFinished);
    connect(d->debuggerProcess, &QProcess::errorOccurred,
            this, &DebugManager::onDebuggerErrorOccurred);

    QString debuggerPath = findDebugger();
    if (debuggerPath.isEmpty()) {
        qWarning() << "未找到调试器，请安装GDB或LLDB";
    } else {
        d->debuggerType = detectDebuggerType(debuggerPath);
        qDebug() << "检测到调试器:" << debuggerPath << "类型:" << d->debuggerType;
    }

    qDebug() << "调试管理器初始化完成";
    return true;
}

void DebugManager::shutdown()
{
    // 幂等：允许被多次调用
    if (!d->debuggerProcess && !d->isDebugging) {
        return;
    }

    qDebug() << "[DebugManager] shutdown 开始";

    // ---- 1. 停止调试会话 ----
    if (d->isDebugging) {
        stopDebugging();
    }

    // ---- 2. 显式销毁 QProcess ----
    if (d->debuggerProcess) {
        // 断开所有信号，防止 finished/errorOccurred 回调
        d->debuggerProcess->disconnect(this);

        if (d->debuggerProcess->state() != QProcess::NotRunning) {
            d->debuggerProcess->terminate();
            if (!d->debuggerProcess->waitForFinished(1000)) {
                d->debuggerProcess->kill();
                d->debuggerProcess->waitForFinished(500);
            }
        }
        delete d->debuggerProcess;   // ← 不能用 deleteLater（事件循环可能已停）
        d->debuggerProcess = nullptr;
    }

    // ---- 3. 清理状态 ----
    d->isDebugging = false;
    d->currentState = DebugState::Stopped;
    d->breakpoints.clear();
    d->breakpointIds.clear();
    d->registeredEditors.clear();
    d->commandQueue.clear();
    d->outputBuffer.clear();

    qDebug() << "[DebugManager] shutdown 完成";
}

// ==================== 调试会话控制 ====================

bool DebugManager::startDebugging(const DebugConfig& config)
{
    // ========== 1. 确保调试器进程已初始化 ==========
    if (!d->debuggerProcess) {
        qWarning() << "调试器进程未初始化，正在自动初始化...";
        if (!initialize()) {
            QString error = "调试管理器初始化失败";
            qWarning() << error;
            emit errorOccurred(error);
            return false;
        }
    }

    // ========== 2. 清理残留进程 ==========
    if (d->debuggerProcess->state() != QProcess::NotRunning) {
        d->debuggerProcess->terminate();
        if (!d->debuggerProcess->waitForFinished(3000)) {
            d->debuggerProcess->kill();
            d->debuggerProcess->waitForFinished(1000);
        }
    }

    // ========== 3. 重置所有状态 ==========
    d->isDebugging = false;
    d->currentState = DebugState::Stopped;
    d->m_inDyld = false;
    d->commandQueue.clear();
    d->breakpointIds.clear();
    // 断点保留（不清除用户设置的断点）

    // ========== 4. 验证可执行文件 ==========
    QFileInfo exeInfo(config.executablePath);
    if (!exeInfo.exists()) {
        QString error = QString("可执行文件不存在: %1").arg(config.executablePath);
        qWarning() << error;
        emit errorOccurred(error);
        return false;
    }

    d->config = config;

    // ========== 5. 确定调试器路径 ==========
    QString debuggerPath = config.debuggerPath;
    if (debuggerPath.isEmpty()) {
        debuggerPath = findDebugger();
    }

    if (debuggerPath.isEmpty()) {
        QString error = "未找到调试器，请安装GDB或LLDB";
        qWarning() << error;
        emit errorOccurred(error);
        return false;
    }

    d->debuggerType = detectDebuggerType(debuggerPath);
    if (d->debuggerType.isEmpty()) {
        QString error = "无法识别的调试器";
        qWarning() << error;
        emit errorOccurred(error);
        return false;
    }

    // ========== 6. 设置调试器进程 ==========
    if (!config.workingDirectory.isEmpty()) {
        d->debuggerProcess->setWorkingDirectory(config.workingDirectory);
    }

    QStringList args;
    if (d->debuggerType == "gdb") {
        args << "--interpreter=mi2";
        args << config.executablePath;
        if (!config.arguments.isEmpty()) {
            args << "--args";
            args << config.arguments;
        }
    } else if (d->debuggerType == "lldb") {
        args << "-f" << config.executablePath;
    }

    d->debuggerProcess->setProgram(debuggerPath);
    d->debuggerProcess->setArguments(args);

    qDebug() << "启动调试器:" << debuggerPath << args.join(" ");

    // ========== 7. 启动调试器进程 ==========
    d->debuggerProcess->start();
    if (!d->debuggerProcess->waitForStarted(5000)) {
        QString error = QString("无法启动调试器: %1").arg(d->debuggerProcess->errorString());
        qWarning() << error;
        emit errorOccurred(error);
        return false;
    }
    qDebug() << "调试器进程已启动，PID:" << d->debuggerProcess->processId();

    d->isDebugging = true;
    d->currentState = DebugState::Starting;
    d->currentExecutable = config.executablePath;
    d->currentArguments = config.arguments;
    d->workingDirectory = config.workingDirectory;

    // ========== 8. 发送初始化命令 ==========
    if (d->debuggerType == "gdb") {
        sendCommand("-file-exec-and-symbols \"" + config.executablePath + "\"");
    } else if (d->debuggerType == "lldb") {
        sendCommand("target create \"" + config.executablePath + "\"");

        // 设置源文件映射
        QString projectRoot = QFileInfo(config.executablePath).absolutePath();
        while (!QFileInfo(projectRoot + "/.lightcppproj").exists()) {
            QString parent = QFileInfo(projectRoot).absolutePath();
            if (parent == projectRoot) break;
            projectRoot = parent;
        }
        sendCommand("settings set target.source-map . " + projectRoot);

        // LLDB 配置
        sendCommand("process handle -s false -n false -p true SIGTRAP");
        sendCommand("process handle -s false -n false -p true SIGSTOP");
        sendCommand("settings set target.process.thread.step-avoid-regexp ^(std::|dyld::|dyld4::)");
    }

    // ========== 9. 同步断点 ==========
    syncBreakpoints();

    // ========== 10. 设置入口断点（如果有需要） ==========
    if (config.stopOnEntry) {
        if (d->debuggerType == "gdb") {
            sendCommand("-break-insert main");
        } else if (d->debuggerType == "lldb") {
            sendCommand("breakpoint set -n main");
        }
    }

    // ========== 11. 启动程序 ==========
    if (d->debuggerType == "gdb") {
        sendCommand("-exec-run");
    } else if (d->debuggerType == "lldb") {
        // 使用 process launch -s 在入口处停止（如果未设置断点）
        sendCommand("process launch -s");
    }

    d->currentState = DebugState::Running;
    emit stateChanged(d->currentState);
    emit debuggingStarted();

    qDebug() << "调试会话启动成功";
    return true;
}

void DebugManager::stopDebugging()
{
    if (!d->isDebugging) {
        // 即使不在调试中，也要强制清理残留进程
        if (d->debuggerProcess) {
            d->debuggerProcess->disconnect(this);
            if (d->debuggerProcess->state() != QProcess::NotRunning) {
                d->debuggerProcess->terminate();
                d->debuggerProcess->waitForFinished(1000);
                if (d->debuggerProcess->state() != QProcess::NotRunning) {
                    d->debuggerProcess->kill();
                    d->debuggerProcess->waitForFinished(500);
                }
            }
            delete d->debuggerProcess;
            d->debuggerProcess = nullptr;
        }
        return;
    }

    qDebug() << "停止调试会话";

    // 断开信号连接
    if (d->debuggerProcess) {
        d->debuggerProcess->disconnect(this);
        if (d->debuggerProcess->state() != QProcess::NotRunning) {
            d->debuggerProcess->terminate();
            if (!d->debuggerProcess->waitForFinished(3000)) {
                d->debuggerProcess->kill();
                d->debuggerProcess->waitForFinished(1000);
            }
        }
        delete d->debuggerProcess;
        d->debuggerProcess = nullptr;
    }

    // 重置所有状态
    d->isDebugging = false;
    d->currentState = DebugState::Stopped;
    d->currentFile.clear();
    d->currentLine = 0;
    d->callStack.clear();
    d->localVariables.clear();
    d->commandQueue.clear();
    d->m_inDyld = false;          // 关键：重置 dyld 标志
    d->breakpoints.clear();       // 清空断点（但保留用户设置的断点？）
    d->breakpointIds.clear();

    emit stateChanged(d->currentState);
    emit debuggingStopped();
    emit callStackUpdated(QList<StackFrame>());
    emit variablesUpdated(QList<DebugVariable>());

    updateAllEditors();

    qDebug() << "调试会话已停止";
}

bool DebugManager::isDebugging() const
{
    return d->isDebugging;
}

DebugState DebugManager::currentState() const
{
    return d->currentState;
}

DebugConfig DebugManager::currentConfig() const
{
    return d->config;
}

// ==================== 执行控制 ====================

void DebugManager::continueExecution()
{
    if (!d->isDebugging || d->currentState == DebugState::Stopped) {
        return;
    }

    qDebug() << "继续执行";

    if (d->debuggerType == "gdb") {
        sendCommand("-exec-continue");
    } else if (d->debuggerType == "lldb") {
        sendCommand("continue");
    }

    d->currentState = DebugState::Running;
    emit stateChanged(d->currentState);
    emit programContinued();
}

void DebugManager::stepOver()
{
    if (!d->isDebugging || d->currentState == DebugState::Stopped) {
        return;
    }

    qDebug() << "步过";

    if (d->debuggerType == "gdb") {
        sendCommand("-exec-next");
    } else if (d->debuggerType == "lldb") {
        sendCommand("thread step-over");
    }

    d->currentState = DebugState::Stepping;
    emit stateChanged(d->currentState);
}

void DebugManager::stepInto()
{
    if (!d->isDebugging || d->currentState == DebugState::Stopped) {
        return;
    }

    qDebug() << "步入";

    if (d->debuggerType == "gdb") {
        sendCommand("-exec-step");
    } else if (d->debuggerType == "lldb") {
        sendCommand("step");
    }

    d->currentState = DebugState::Stepping;
    emit stateChanged(d->currentState);
}

void DebugManager::stepOut()
{
    if (!d->isDebugging || d->currentState == DebugState::Stopped) {
        return;
    }

    qDebug() << "步出";

    if (d->debuggerType == "gdb") {
        sendCommand("-exec-finish");
    } else if (d->debuggerType == "lldb") {
        sendCommand("finish");
    }

    d->currentState = DebugState::Stepping;
    emit stateChanged(d->currentState);
}

void DebugManager::runToCursor(const QString& filePath, int lineNumber)
{
    if (!d->isDebugging || d->currentState == DebugState::Stopped) {
        return;
    }

    qDebug() << "执行到光标:" << filePath << ":" << lineNumber;

    if (d->debuggerType == "gdb") {
        sendCommand(QString("-break-insert -t \"%1:%2\"")
                        .arg(filePath).arg(lineNumber));
        sendCommand("-exec-continue");
    } else if (d->debuggerType == "lldb") {
        sendCommand(QString("breakpoint set -f %1 -l %2 -o")
                        .arg(filePath).arg(lineNumber));
        sendCommand("continue");
    }
}

// ==================== 断点管理 ====================

bool DebugManager::addBreakpoint(const QString& filePath, int lineNumber, const QString& condition)
{
    if (filePath.isEmpty() || lineNumber < 1) {
        return false;
    }

    QString key = breakpointKey(filePath, lineNumber);

    if (d->breakpoints.contains(key)) {
        qDebug() << "断点已存在:" << filePath << ":" << lineNumber;
        return false;
    }

    Breakpoint bp;
    bp.filePath = filePath;
    bp.lineNumber = lineNumber;
    bp.condition = condition;
    bp.enabled = true;
    bp.hitCount = 0;

    if (d->isDebugging) {
        QString cmd;
        if (d->debuggerType == "gdb") {
            cmd = QString("-break-insert");
            if (!condition.isEmpty()) {
                cmd += " -c \"" + condition + "\"";
            }
            cmd += " \"" + filePath + ":" + QString::number(lineNumber) + "\"";
            sendCommand(cmd);
        } else if (d->debuggerType == "lldb") {
            cmd = QString("breakpoint set -f %1 -l %2").arg(filePath).arg(lineNumber);
            if (!condition.isEmpty()) {
                cmd += " -c \"" + condition + "\"";
            }
            sendCommand(cmd);
        }
    }

    d->breakpoints[key] = bp;
    updateAllEditors();

    qDebug() << "断点已添加:" << filePath << ":" << lineNumber;
    return true;
}

bool DebugManager::removeBreakpoint(const QString& filePath, int lineNumber)
{
    QString key = breakpointKey(filePath, lineNumber);

    if (!d->breakpoints.contains(key)) {
        return false;
    }

    if (d->isDebugging && d->breakpointIds.contains(key)) {
        int id = d->breakpointIds[key];
        if (d->debuggerType == "gdb") {
            sendCommand(QString("-break-delete %1").arg(id));
        } else if (d->debuggerType == "lldb") {
            sendCommand(QString("breakpoint delete %1").arg(id));
        }
        d->breakpointIds.remove(key);
    }

    d->breakpoints.remove(key);
    updateAllEditors();

    qDebug() << "断点已移除:" << filePath << ":" << lineNumber;
    return true;
}

void DebugManager::toggleBreakpoint(const QString& filePath, int lineNumber)
{
    QString key = breakpointKey(filePath, lineNumber);

    if (d->breakpoints.contains(key)) {
        removeBreakpoint(filePath, lineNumber);
    } else {
        addBreakpoint(filePath, lineNumber);
    }
}

bool DebugManager::setBreakpointEnabled(const QString& filePath, int lineNumber, bool enabled)
{
    QString key = breakpointKey(filePath, lineNumber);

    if (!d->breakpoints.contains(key)) {
        return false;
    }

    d->breakpoints[key].enabled = enabled;

    if (d->isDebugging && d->breakpointIds.contains(key)) {
        int id = d->breakpointIds[key];
        if (d->debuggerType == "gdb") {
            sendCommand(QString("-break-enable %1 %2")
                            .arg(enabled ? "enable" : "disable")
                            .arg(id));
        } else if (d->debuggerType == "lldb") {
            sendCommand(QString("breakpoint %1 %2")
                            .arg(enabled ? "enable" : "disable")
                            .arg(id));
        }
    }

    updateAllEditors();
    return true;
}

bool DebugManager::setBreakpointCondition(const QString& filePath, int lineNumber, const QString& condition)
{
    QString key = breakpointKey(filePath, lineNumber);

    if (!d->breakpoints.contains(key)) {
        return false;
    }

    d->breakpoints[key].condition = condition;

    if (d->isDebugging && d->breakpointIds.contains(key)) {
        int id = d->breakpointIds[key];
        if (d->debuggerType == "gdb") {
            if (condition.isEmpty()) {
                sendCommand(QString("-break-condition %1").arg(id));
            } else {
                sendCommand(QString("-break-condition %1 \"%2\"")
                                .arg(id).arg(condition));
            }
        } else if (d->debuggerType == "lldb") {
            if (condition.isEmpty()) {
                sendCommand(QString("breakpoint modify -c '' %1").arg(id));
            } else {
                sendCommand(QString("breakpoint modify -c \"%1\" %2")
                                .arg(condition).arg(id));
            }
        }
    }

    qDebug() << "断点条件已更新:" << filePath << ":" << lineNumber << " -> " << condition;
    return true;
}

QList<Breakpoint> DebugManager::getBreakpoints() const
{
    return d->breakpoints.values();
}

void DebugManager::clearAllBreakpoints()
{
    qDebug() << "清除所有断点";

    if (d->isDebugging) {
        if (d->debuggerType == "gdb") {
            sendCommand("-break-delete");
        } else if (d->debuggerType == "lldb") {
            sendCommand("breakpoint delete");
        }
    }

    d->breakpoints.clear();
    d->breakpointIds.clear();
    updateAllEditors();
}

bool DebugManager::hasBreakpoint(const QString& filePath, int lineNumber) const
{
    QString key = breakpointKey(filePath, lineNumber);
    return d->breakpoints.contains(key);
}

// ==================== 状态查询 ====================

QList<StackFrame> DebugManager::getCallStack() const
{
    return d->callStack;
}

QList<DebugVariable> DebugManager::getLocalVariables(int frameIndex) const
{
    if (frameIndex < 0 || frameIndex >= d->callStack.size()) {
        return QList<DebugVariable>();
    }

    if (frameIndex == 0) {
        return d->localVariables;
    }

    return d->localVariables;
}

QPair<QString, int> DebugManager::getCurrentPosition() const
{
    return qMakePair(d->currentFile, d->currentLine);
}

QString DebugManager::evaluateExpression(const QString& expression)
{
    if (!d->isDebugging || d->currentState == DebugState::Stopped) {
        return QString();
    }

    if (d->debuggerType == "gdb") {
        sendCommand(QString("-data-evaluate-expression \"%1\"").arg(expression));
    } else if (d->debuggerType == "lldb") {
        sendCommand(QString("expression %1").arg(expression));
    }

    return QString("");  // 暂未实现
}

// ==================== 编辑器集成 ====================

void DebugManager::registerEditor(TextEditor* editor)
{
    if (editor && !d->registeredEditors.contains(editor)) {
        d->registeredEditors.append(editor);
        qDebug() << "编辑器已注册到调试管理器";
    }
}

void DebugManager::unregisterEditor(TextEditor* editor)
{
    if (editor) {
        d->registeredEditors.removeAll(editor);
        qDebug() << "编辑器已从调试管理器取消注册";
    }
}

void DebugManager::updateAllEditors()
{
    // 通知所有注册的编辑器更新断点标记
    // 将在后续实现
}

bool DebugManager::startJavaDebugging(const DebugConfig& config)
{
    if (config.executablePath.isEmpty()) {
        emit errorOccurred("Java debugging: executable path is empty");
        return false;
    }

    // 对于 Java，executablePath 实际上是 mainClass 或 jar 路径
    // 我们需要构建 java -agentlib:jdwp 命令
    QString javaCmd = "java";
#ifdef Q_OS_WIN
    javaCmd = "java.exe";
#endif

    // 从环境获取 JAVA_HOME
    QString javaHome = QProcessEnvironment::systemEnvironment().value("JAVA_HOME");
    if (!javaHome.isEmpty()) {
        QString javaPath = PathUtils::joinPaths({javaHome, "bin", "java"});
#ifdef Q_OS_WIN
        javaPath += ".exe";
#endif
        if (QFile::exists(javaPath)) {
            javaCmd = javaPath;
        }
    }

    QStringList args;
    // JDWP 参数
    int debugPort = 5005;
    args << "-agentlib:jdwp=transport=dt_socket,server=y,suspend=y,address=" + QString::number(debugPort);

    // 添加程序参数
    if (!config.arguments.isEmpty()) {
        args << config.arguments;
    } else {
        // 默认使用 executablePath 作为主类或 jar
        if (config.executablePath.endsWith(".jar")) {
            args << "-jar" << config.executablePath;
        } else {
            args << config.executablePath;
        }
    }

    qDebug() << "Java debugging command:" << javaCmd << args.join(" ");

    // 启动进程
    if (d->debuggerProcess) {
        d->debuggerProcess->deleteLater();
    }
    d->debuggerProcess = new QProcess(this);

    if (!config.workingDirectory.isEmpty()) {
        d->debuggerProcess->setWorkingDirectory(config.workingDirectory);
    }

    connect(d->debuggerProcess, &QProcess::readyReadStandardOutput,
            this, &DebugManager::onDebuggerReadyRead);
    connect(d->debuggerProcess, &QProcess::readyReadStandardError,
            this, &DebugManager::onDebuggerReadyReadError);
    connect(d->debuggerProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &DebugManager::onDebuggerFinished);
    connect(d->debuggerProcess, &QProcess::errorOccurred,
            this, &DebugManager::onDebuggerErrorOccurred);

    d->debuggerProcess->setProgram(javaCmd);
    d->debuggerProcess->setArguments(args);
    d->debuggerProcess->start();

    if (!d->debuggerProcess->waitForStarted(5000)) {
        QString error = QString("Failed to start Java debugger: %1")
        .arg(d->debuggerProcess->errorString());
        emit errorOccurred(error);
        return false;
    }

    d->isDebugging = true;
    d->currentState = DebugState::Running;
    d->currentExecutable = config.executablePath;
    d->currentArguments = config.arguments;
    d->workingDirectory = config.workingDirectory;
    d->debuggerType = "java";

    emit stateChanged(d->currentState);
    emit debuggingStarted();

    // 输出调试提示
    QString msg = QString("Java 调试已启动，端口: %1\n"
                          "请使用 IDE 或 JDB 连接: jdb -attach %2")
                      .arg(debugPort)
                      .arg(debugPort);
    emit programOutput(msg, false);

    return true;
}

// ==================== 私有方法 ====================

QString DebugManager::findDebugger() const
{
    QStringList possibleDebuggers;

#ifdef Q_OS_MAC
    possibleDebuggers << "lldb"
                      << "/usr/bin/lldb"
                      << "/usr/local/bin/lldb"
                      << "/opt/homebrew/bin/lldb";
#elif defined(Q_OS_WIN)
    possibleDebuggers << "gdb.exe"
                      << "lldb.exe"
                      << "C:/mingw64/bin/gdb.exe"
                      << "C:/mingw/bin/gdb.exe"
                      << "C:/Program Files/LLVM/bin/lldb.exe";
#else
    possibleDebuggers << "gdb"
                      << "lldb"
                      << "/usr/bin/gdb"
                      << "/usr/bin/lldb"
                      << "/usr/local/bin/gdb"
                      << "/usr/local/bin/lldb";
#endif

    for (const QString& debugger : possibleDebuggers) {
        QFileInfo info(debugger);
        if (info.exists() && info.isExecutable()) {
            return debugger;
        }

#ifdef Q_OS_WIN
        if (!debugger.endsWith(".exe") && QFileInfo::exists(debugger + ".exe")) {
            return debugger + ".exe";
        }
#endif
    }

    return QString();
}

QString DebugManager::detectDebuggerType(const QString& debuggerPath) const
{
    QString lowerPath = debuggerPath.toLower();
    if (lowerPath.contains("gdb")) {
        return "gdb";
    } else if (lowerPath.contains("lldb")) {
        return "lldb";
    }
    return QString();
}

void DebugManager::sendCommand(const QString& command)
{
    if (!d->debuggerProcess || d->debuggerProcess->state() != QProcess::Running) {
        qWarning() << "调试器未运行，无法发送命令:" << command;
        return;
    }

    // 如果程序已退出，跳过需要进程状态的命令
    if (d->currentState == DebugState::Finished || d->currentState == DebugState::Stopped) {
        if (command.contains("frame variable") ||
            command.contains("thread backtrace") ||
            command.contains("stack-list")) {
            qDebug() << "程序已退出，跳过命令:" << command;
            return;
        }
    }

    // 简单限流：如果命令是调试信息请求，延迟 100ms 发送，避免堆积
    if (command.contains("thread backtrace") || command.contains("frame variable")) {
        // 可以添加简单延迟，但 QProcess 是异步的，可能不需要
    }

    qDebug() << "发送命令:" << command;

    QString cmd = command;
    if (d->debuggerType == "gdb") {
        if (!cmd.endsWith('\n')) cmd += '\n';
    } else if (d->debuggerType == "lldb") {
        if (!cmd.endsWith('\n')) cmd += '\n';
    }

    d->debuggerProcess->write(cmd.toUtf8());
}

void DebugManager::syncBreakpoints()
{
    if (!d->isDebugging) {
        qDebug() << "syncBreakpoints: not debugging, skipping";
        return;
    }

    qDebug() << "syncBreakpoints: syncing" << d->breakpoints.size() << "breakpoints";

    // 先清除调试器中所有断点
    if (d->debuggerType == "gdb") {
        sendCommand("-break-delete");
    } else if (d->debuggerType == "lldb") {
        sendCommand("breakpoint delete");
    }
    d->breakpointIds.clear();

    // 重新添加所有断点
    for (const Breakpoint& bp : d->breakpoints.values()) {
        if (!bp.enabled) continue;
        qDebug() << "Adding breakpoint:" << bp.filePath << ":" << bp.lineNumber;
        QString cmd;
        if (d->debuggerType == "gdb") {
            cmd = QString("-break-insert");
            if (!bp.condition.isEmpty()) {
                cmd += " -c \"" + bp.condition + "\"";
            }
            cmd += " \"" + bp.filePath + ":" + QString::number(bp.lineNumber) + "\"";
            sendCommand(cmd);
        } else if (d->debuggerType == "lldb") {
            // 使用文件名
            QString fileName = QFileInfo(bp.filePath).fileName();
            cmd = QString("breakpoint set -f \"%1\" -l %2").arg(fileName).arg(bp.lineNumber);
            if (!bp.condition.isEmpty()) {
                cmd += " -c \"" + bp.condition + "\"";
            }
            sendCommand(cmd);
        }
    }
}

QString DebugManager::breakpointKey(const QString& filePath, int lineNumber) const
{
    return QString("%1:%2").arg(filePath).arg(lineNumber);
}

// ==================== GDB输出解析方法 ====================

void DebugManager::parseGdbOutput(const QString& output)
{
    QStringList lines = output.split('\n');
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;

        if (trimmed.startsWith("*stopped")) {
            parseGdbStoppedEvent(trimmed);
        } else if (trimmed.startsWith("*running")) {
            d->currentState = DebugState::Running;
            emit stateChanged(d->currentState);
        } else if (trimmed.startsWith("=breakpoint-created")) {
            parseGdbBreakpointCreated(trimmed);
        } else if (trimmed.startsWith("=breakpoint-modified")) {
            // 忽略
        } else if (trimmed.startsWith("=breakpoint-deleted")) {
            // 忽略
        } else if (trimmed.startsWith("^done")) {
            parseGdbDoneResponse(trimmed);
        } else if (trimmed.startsWith("^error")) {
            parseGdbErrorResponse(trimmed);
        } else if (trimmed.startsWith("&")) {
            QString log = trimmed.mid(1);
            if (!log.isEmpty()) {
                emit programOutput(log, false);
            }
        } else if (trimmed.startsWith("~")) {
            QString outputText = trimmed.mid(1);
            if (!outputText.isEmpty()) {
                emit programOutput(outputText, false);
            }
        }
    }
}

void DebugManager::parseGdbStoppedEvent(const QString& event)
{
    // 错误写法（原始字符串字面量）
    QRegularExpression fileRegex("file=\"([^\"]*)\"");
    QRegularExpression lineRegex("line=\"([^\"]*)\"");
    QRegularExpression reasonRegex("reason=\"([^\"]*)\"");
    QRegularExpression funcRegex("func=\"([^\"]*)\"");

    QRegularExpressionMatch fileMatch = fileRegex.match(event);
    QRegularExpressionMatch lineMatch = lineRegex.match(event);
    QRegularExpressionMatch reasonMatch = reasonRegex.match(event);
    QRegularExpressionMatch funcMatch = funcRegex.match(event);

    QString file = fileMatch.hasMatch() ? fileMatch.captured(1) : "";
    int line = lineMatch.hasMatch() ? lineMatch.captured(1).toInt() : 0;
    QString reason = reasonMatch.hasMatch() ? reasonMatch.captured(1) : "";
    QString func = funcMatch.hasMatch() ? funcMatch.captured(1) : "";

    d->currentFile = file;
    d->currentLine = line;
    d->currentState = DebugState::Paused;

    emit stateChanged(d->currentState);

    if (reason == "breakpoint-hit") {
        emit breakpointHit(file, line);
        emit programStopped(file, line, "断点命中");
    } else if (reason == "function-finished") {
        emit programStopped(file, line, "函数执行完成");
    } else if (reason == "exited-normally" || reason == "exited") {
        d->currentState = DebugState::Finished;
        emit stateChanged(d->currentState);
        emit programExited(0);
        stopDebugging();
    } else if (reason == "signal-received") {
        emit programStopped(file, line, "收到信号");
    } else {
        emit programStopped(file, line, "程序暂停");
    }

    if (d->debuggerType == "gdb") {
        sendCommand("-stack-list-frames");
        sendCommand("-stack-list-variables --all-values");
    }
}

void DebugManager::parseGdbBreakpointCreated(const QString& event)
{
    QRegularExpression idRegex("id=\"([^\"]*)\"");
    QRegularExpression fileRegex("file=\"([^\"]*)\"");
    QRegularExpression lineRegex("line=\"([^\"]*)\"");

    QRegularExpressionMatch idMatch = idRegex.match(event);
    QRegularExpressionMatch fileMatch = fileRegex.match(event);
    QRegularExpressionMatch lineMatch = lineRegex.match(event);

    if (idMatch.hasMatch() && fileMatch.hasMatch() && lineMatch.hasMatch()) {
        int id = idMatch.captured(1).toInt();
        QString file = fileMatch.captured(1);
        int line = lineMatch.captured(1).toInt();

        QString key = breakpointKey(file, line);
        d->breakpointIds[key] = id;
    }
}

void DebugManager::parseGdbDoneResponse(const QString& response)
{
    if (response.contains("stack-list-frames")) {
        // 解析调用栈（暂略）
    } else if (response.contains("stack-list-variables")) {
        // 解析变量（暂略）
        emit variablesUpdated(d->localVariables);
    }
}

void DebugManager::parseGdbErrorResponse(const QString& response)
{
    QRegularExpression msgRegex("msg=\"([^\"]*)\"");
    QRegularExpressionMatch msgMatch = msgRegex.match(response);

    QString errorMsg = msgMatch.hasMatch() ? msgMatch.captured(1) : "未知错误";
    qWarning() << "GDB错误:" << errorMsg;
    emit errorOccurred(errorMsg);
}

void DebugManager::parseLldbOutput(const QString& output)
{
    QStringList lines = output.split('\n');
    bool processStopped = false;

    for (int i = 0; i < lines.size(); ++i) {
        QString trimmed = lines[i].trimmed();
        if (trimmed.isEmpty()) continue;

        qDebug() << "[LLDB]:" << trimmed;

        // ========== 程序退出检测（优先级最高） ==========
        static QRegularExpression exitRegex(
            "Process\\s+\\d+\\s+exited\\s+with\\s+status\\s*=\\s*(\\d+)",
            QRegularExpression::CaseInsensitiveOption
            );
        QRegularExpressionMatch exitMatch = exitRegex.match(trimmed);
        if (exitMatch.hasMatch()) {
            int exitCode = exitMatch.captured(2).toInt();
            qDebug() << "程序退出，退出码:" << exitCode;
            d->currentState = DebugState::Finished;
            emit stateChanged(d->currentState);
            emit programExited(exitCode);
            d->commandQueue.clear();
            stopDebugging();
            return;
        }

        // ========== 检测 "Process xxx stopped" ==========
        if (trimmed.contains("Process") && trimmed.contains("stopped")) {
            processStopped = true;
            // 检查后面的行是否包含 dyld
            for (int j = i + 1; j < qMin(i + 5, lines.size()); ++j) {
                QString nextLine = lines[j].trimmed();
                if (nextLine.contains("dyld") || nextLine.contains("libdyld") || nextLine.contains("start +")) {
                    qDebug() << "Process stopped in dyld，自动结束调试";
                    d->currentState = DebugState::Finished;
                    emit stateChanged(d->currentState);
                    emit programExited(0);
                    d->commandQueue.clear();
                    stopDebugging();
                    return;
                }
            }
            continue;
        }

        // ========== 检测停止原因行 ==========
        if (trimmed.contains("stop reason =")) {
            // 检查是否在 dyld 中
            bool inDyld = trimmed.contains("dyld") ||
                          trimmed.contains("dyld`") ||
                          trimmed.contains("dyld4::") ||
                          trimmed.contains("libdyld") ||
                          trimmed.contains("_exit") ||
                          trimmed.contains("start +");

            if (inDyld) {
                qDebug() << "stop reason 在 dyld 中，自动结束调试";
                d->currentState = DebugState::Finished;
                emit stateChanged(d->currentState);
                emit programExited(0);
                d->commandQueue.clear();
                stopDebugging();
                return;
            }

            d->currentState = DebugState::Paused;
            emit stateChanged(d->currentState);
            continue;
        }

        // ========== 从 frame 行提取文件信息 ==========
        if (trimmed.startsWith("frame #")) {
            // 检查是否在 dyld 中
            bool inDyld = trimmed.contains("dyld") ||
                          trimmed.contains("dyld`") ||
                          trimmed.contains("dyld4::") ||
                          trimmed.contains("libdyld");

            if (inDyld) {
                qDebug() << "frame 在 dyld 中，自动结束调试";
                d->currentState = DebugState::Finished;
                emit stateChanged(d->currentState);
                emit programExited(0);
                d->commandQueue.clear();
                stopDebugging();
                return;
            }

            // 如果包含 " at "，提取文件信息（正常源码帧）
            if (trimmed.contains(" at ")) {
                QRegularExpression frameRegex("at\\s+([^:]+):(\\d+)(?::\\d+)?");
                QRegularExpressionMatch frameMatch = frameRegex.match(trimmed);
                if (frameMatch.hasMatch()) {
                    QString file = frameMatch.captured(1);
                    int line = frameMatch.captured(2).toInt();
                    d->currentFile = file;
                    d->currentLine = line;

                    if (d->currentState == DebugState::Paused && !file.isEmpty() && line > 0) {
                        emit breakpointHit(file, line);
                        emit programStopped(file, line, "断点命中");
                    }
                }
            }
            continue;
        }

        // ========== 其他输出 ==========
        if (!trimmed.isEmpty()) {
            emit programOutput(trimmed, false);
        }
    }
}

// ==================== 调试器进程信号槽 ====================

void DebugManager::onDebuggerReadyRead()
{
    if (!d->debuggerProcess) return;

    QByteArray data = d->debuggerProcess->readAllStandardOutput();
    QString output = QString::fromLocal8Bit(data);

    d->outputBuffer += output;

    while (d->outputBuffer.contains('\n')) {
        int pos = d->outputBuffer.indexOf('\n');
        QString line = d->outputBuffer.left(pos);
        d->outputBuffer = d->outputBuffer.mid(pos + 1);

        if (!line.isEmpty()) {
            if (d->debuggerType == "gdb") {
                parseGdbOutput(line);
            } else if (d->debuggerType == "lldb") {
                parseLldbOutput(line);
            }
        }
    }
}

void DebugManager::onDebuggerReadyReadError()
{
    if (!d->debuggerProcess) return;

    QByteArray data = d->debuggerProcess->readAllStandardError();
    QString error = QString::fromLocal8Bit(data);

    if (!error.isEmpty()) {
        qWarning() << "调试器错误输出:" << error;
        emit programOutput(error, true);
    }
}

void DebugManager::onDebuggerFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "调试器进程退出，退出码:" << exitCode;

    if (d->isDebugging) {
        d->isDebugging = false;
        d->currentState = DebugState::Error;
        emit stateChanged(d->currentState);
        emit errorOccurred(QString("调试器异常退出，退出码: %1").arg(exitCode));
    }
}

void DebugManager::onDebuggerErrorOccurred(QProcess::ProcessError error)
{
    QString errorMsg;
    switch (error) {
    case QProcess::FailedToStart:
        errorMsg = "调试器启动失败";
        break;
    case QProcess::Crashed:
        errorMsg = "调试器崩溃";
        break;
    case QProcess::Timedout:
        errorMsg = "调试器超时";
        break;
    case QProcess::WriteError:
        errorMsg = "写入调试器失败";
        break;
    case QProcess::ReadError:
        errorMsg = "读取调试器失败";
        break;
    default:
        errorMsg = "调试器未知错误";
        break;
    }

    qWarning() << "调试器错误:" << errorMsg;
    emit errorOccurred(errorMsg);

    if (d->isDebugging) {
        d->isDebugging = false;
        d->currentState = DebugState::Error;
        emit stateChanged(d->currentState);
    }
}

} // namespace SprintIDE
