#include "python_debugger.h"
#include <QTcpSocket>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QTimer>
#include <QThread>
#include <QSet>
#include <QFile>
#include <QFileInfo>

namespace SprintIDE {

class PythonDebugger::Private {
public:
    Private(PythonDebugger* q) : q_ptr(q) {}

    QProcess* debugpyProcess = nullptr;
    QTcpSocket* socket = nullptr;
    QByteArray buffer;
    bool running = false;
    QString program;
    int seq = 0;
    QMap<int, QJsonObject> pendingRequests;

    PythonDebugger* q_ptr;

    DebugConfig config;

    void sendRequest(const QString& command, const QJsonObject& args = QJsonObject());
    void processMessage(const QByteArray& data);
    void handleStopped(const QJsonObject& params);
    void handleContinued();
    void handleExited(const QJsonObject& params);
    void handleOutput(const QJsonObject& params);
    void handleStackTrace(const QJsonObject& response);
    void handleScopes(const QJsonObject& response);
    void handleVariables(const QJsonObject& response);
    void applyBreakpoints();
    void sendLaunchRequest();
    void sendAttachRequest();

    QList<Breakpoint> breakpoints;
    QList<StackFrame> frames;
    QList<DebugVariable> variables;
    QString currentFile;
    int currentLine = -1;
    bool pendingStackTrace = false;
    bool initialized = false;
    bool continuePending = false;   // 标记是否需要发送 continue
    bool finishedEmitted = false;
    QString currentStopReason;   // 保存最近一次停止的原因（breakpoint / step / ...）
    int currentThreadId = 1;     // 保存最近一次停止的线程 id
};

// ---------- 端口清理工具函数 ----------
static void killProcessOnPort(int port) {
#ifdef Q_OS_WIN
    QProcess proc;
    proc.start("cmd", {"/c", "netstat -ano | findstr :" + QString::number(port)});
    proc.waitForFinished(1000);
    QString output = proc.readAllStandardOutput();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    QSet<QString> pids;
    for (const QString& line : lines) {
        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.size() >= 5) {
            QString pid = parts.last().trimmed();
            if (!pid.isEmpty()) pids.insert(pid);
        }
    }
    for (const QString& pid : pids) {
        QProcess::execute("taskkill", {"/F", "/PID", pid});
    }
#else
    QProcess proc;
    proc.start("lsof", {"-ti", ":" + QString::number(port)});
    proc.waitForFinished(1000);
    QString output = proc.readAllStandardOutput().trimmed();
    QStringList pids = output.split('\n', Qt::SkipEmptyParts);
    for (const QString& pid : pids) {
        QProcess::execute("kill", {"-9", pid});
    }
#endif
}

// ---------- PythonDebugger 实现 ----------
PythonDebugger::PythonDebugger(QObject* parent) : QObject(parent), d(new Private(this)) {
    d->socket = new QTcpSocket(this);
    connect(d->socket, &QTcpSocket::readyRead, this, [this]() {
        // 1. 读取新数据并追加到缓冲区
        QByteArray data = d->socket->readAll();
        d->buffer.append(data);

        // 2. 循环处理缓冲区中的所有完整消息
        while (true) {
            // 查找消息头结束符
            int headerEnd = d->buffer.indexOf("\r\n\r\n");
            if (headerEnd == -1) {
                // 消息头不完整，等待更多数据
                break;
            }

            // 解析 Content-Length
            QByteArray header = d->buffer.left(headerEnd);
            int contentLength = -1;
            QString headerStr = QString::fromUtf8(header);
            for (const QString &line : headerStr.split("\r\n")) {
                if (line.startsWith("Content-Length:", Qt::CaseInsensitive)) {
                    contentLength = line.mid(15).trimmed().toInt();
                    break;
                }
            }

            if (contentLength <= 0) {
                // 头部格式错误，清空缓冲区以防死循环
                qWarning() << "[PythonDebugger] Invalid Content-Length header";
                d->buffer.clear();
                break;
            }

            // 计算完整消息的总长度
            int bodyStart = headerEnd + 4; // \r\n\r\n 的长度
            int totalLength = bodyStart + contentLength;

            if (d->buffer.size() < totalLength) {
                // 消息体不完整，等待更多数据
                break;
            }

            // 提取完整的消息体
            QByteArray body = d->buffer.mid(bodyStart, contentLength);

            // --- 关键修复：从缓冲区头部移除已处理的完整消息 ---
            d->buffer.remove(0, totalLength);
            // -----------------------------------------------

            // 处理消息
            d->processMessage(body);
        }
    });
    connect(d->socket, &QTcpSocket::errorOccurred, this, [this]() {
        qDebug() << "[PythonDebugger] Socket error:" << d->socket->errorString();
        emit outputReceived("Socket error: " + d->socket->errorString(), true);
    });
}

PythonDebugger::~PythonDebugger() {
    stopDebugging();
}

void PythonDebugger::Private::sendRequest(const QString& command, const QJsonObject& args) {
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
        qDebug() << "[PythonDebugger] socket not connected, cannot send" << command;
        emit q_ptr->outputReceived("调试: socket 未连接，无法发送命令 " + command, true);
        return;
    }
    QJsonObject obj;
    obj["type"] = "request";
    obj["command"] = command;
    obj["seq"] = ++seq;
    if (!args.isEmpty()) obj["arguments"] = args;
    QByteArray data = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QByteArray packet = "Content-Length: " + QByteArray::number(data.size()) + "\r\n\r\n" + data;
    socket->write(packet);
    pendingRequests[seq] = obj;
    qDebug() << "[PythonDebugger] Sent request:" << command << "seq=" << seq;
    emit q_ptr->outputReceived(QString("调试: 发送请求 %1 (seq=%2)").arg(command).arg(seq), false);
}

void PythonDebugger::Private::processMessage(const QByteArray &data) {
    qDebug() << "[PythonDebugger] processMessage called with data size:" << data.size();
    qDebug() << "[PythonDebugger] RAW MESSAGE:" << data;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        qWarning() << "[PythonDebugger] JSON 解析失败";
        return;
    }

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();

    // ==================== 处理事件 (Event) ====================
    if (type == "event") {
        QString event = obj["event"].toString();
        qDebug() << "[PythonDebugger] Event:" << event;

        if (event == "output") {
            QJsonObject body = obj["body"].toObject();
            QString category = body["category"].toString();
            QString output = body["output"].toString();
            emit q_ptr->outputReceived(output, category == "stderr");
        } else if (event == "initialized") {
            // 收到 initialized 事件后，发送 launch 请求
            qDebug() << "[PythonDebugger] 收到 initialized 事件，开始配置断点和调试会话...";
            // 1. 标记已初始化
            initialized = true;

            // 2. 立即发送 setBreakpoints
            // 注意：这里直接调用 applyBreakpoints，它内部会发送 setBreakpoints 请求
            applyBreakpoints();

            // 3. 立即发送 configurationDone
            // applyBreakpoints 不再自动发送 configurationDone，我们需要在这里显式发送
            QJsonObject configDoneRequest;
            configDoneRequest["seq"] = ++seq;
            configDoneRequest["type"] = "request";
            configDoneRequest["command"] = "configurationDone";
            configDoneRequest["arguments"] = QJsonObject();

            QByteArray configDoneBody = QJsonDocument(configDoneRequest).toJson(QJsonDocument::Compact);
            QByteArray configDoneHeader = QString("Content-Length: %1\r\n\r\n").arg(configDoneBody.size()).toUtf8();
            socket->write(configDoneHeader + configDoneBody);
            qDebug() << "[PythonDebugger] Sent request: \"configurationDone\" seq=" << configDoneRequest["seq"].toInt();
        } else if (event == "stopped") {
            QJsonObject body = obj["body"].toObject();
            handleStopped(body);
        }else if (event == "continued") {
            handleContinued();
        } else if (event == "terminated") {
            qDebug() << "[PythonDebugger] 程序已终止";
            running = false;                                      // 复位运行状态
            pendingStackTrace = false;                            // 复位挂起状态
            initialized = false;
            if (!finishedEmitted) {                               // 通知 MainWindow
                finishedEmitted = true;
                emit q_ptr->terminated();
            }
        } else if (event == "exited") {
            QJsonObject body = obj["body"].toObject();
            int exitCode = body["exitCode"].toInt();
            qDebug() << "[PythonDebugger] 程序退出，退出码:" << exitCode;
            emit q_ptr->exited(exitCode);
        }
    }
    // ==================== 处理响应 (Response) ====================
    else if (type == "response") {
        QString command = obj["command"].toString();
        bool success = obj["success"].toBool();
        int requestSeq = obj["request_seq"].toInt();
        qDebug() << "[PythonDebugger] Response for command:" << command << "success:" << success;
        // 失败统一处理
        if (!success) {
            QString message = obj["message"].toString();
            qWarning() << "[PythonDebugger] 请求失败:" << command << "错误:" << message;
            // stackTrace 失败要复位，否则后续 stopped 永远不触发
            if (command == "stackTrace") pendingStackTrace = false;
            emit q_ptr->outputReceived("请求失败: " + command + " - " + message, true);
            return;
        }

        // 成功分支
        if (command == "initialize") {
            qDebug() << "[PythonDebugger] initialize 成功，发送 attach";
            sendAttachRequest();
        }
        else if (command == "attach") {
            qDebug() << "[PythonDebugger] Attach 成功";
        }
        else if (command == "setBreakpoints") {
            qDebug() << "[PythonDebugger] 断点设置成功";
        }
        else if (command == "configurationDone") {
            qDebug() << "[PythonDebugger] configurationDone 完成，调试正式开始";
        }
        else if (command == "disconnect") {
            qDebug() << "[PythonDebugger] 断开连接成功";
        }
        // ============ 新增：这几个命令的响应才是 UI 的关键 ============
        else if (command == "stackTrace") {
            handleStackTrace(obj["body"].toObject());
        }
        else if (command == "scopes") {
            handleScopes(obj["body"].toObject());
        }
        else if (command == "variables") {
            handleVariables(obj["body"].toObject());
        }
        else if (command == "next" || command == "stepIn"
                 || command == "stepOut" || command == "continue" || command == "pause") {
            // 这些命令本身不需要额外处理，真正的 UI 更新由随后的 stopped / continued 事件驱动
            qDebug() << "[PythonDebugger] 命令" << command << "被接受";
        }
    }
}

void PythonDebugger::Private::applyBreakpoints() {
    // 如果还没收到 initialized 事件，不要发送，等待事件触发
    if (!initialized) {
        qDebug() << "[PythonDebugger] 尚未初始化，暂缓发送断点";
        return;
    }
    if (breakpoints.isEmpty()) {
        qDebug() << "[PythonDebugger] 警告: 尝试应用断点，但断点列表为空！";
        // 即使没有断点，也应该发送 configurationDone，但这通常由 initialized 事件触发
        return;
    }
    if (!breakpoints.isEmpty()) {
        // 按文件分组断点
        QMap<QString, QList<int>> bpMap;
        for (const Breakpoint& bp : breakpoints) {
            if (bp.enabled) {
                QFileInfo fileInfo(bp.filePath);
                // 确保是绝对路径
                QString absoluteFilePath = fileInfo.absoluteFilePath();
                // 解析符号链接 (Symlinks)
                QString canonicalPath = fileInfo.canonicalFilePath();
                if (!canonicalPath.isEmpty()) {
                    absoluteFilePath = canonicalPath;
                }
                // 确保使用正斜杠
                absoluteFilePath = absoluteFilePath.replace("\\", "/");
                bpMap[absoluteFilePath].append(bp.lineNumber);
            }
        }
        // 对每个文件发送 setBreakpoints 请求
        for (auto it = bpMap.begin(); it != bpMap.end(); ++it) {
            QJsonObject args;
            // 构造 source 对象
            QJsonObject source;
            source["path"] = it.key();
            args["source"] = source;
            // 构造 breakpoints 数组
            QJsonArray bpArray;
            for (int line : it.value()) {
                QJsonObject bpObj;
                bpObj["line"] = line;
                bpArray.append(bpObj);
            }
            args["breakpoints"] = bpArray;
            sendRequest("setBreakpoints", args);
        }
    }
}

void PythonDebugger::Private::handleStopped(const QJsonObject& params) {
    qDebug() << "[PythonDebugger] handleStopped called, params:" << QJsonDocument(params).toJson();
    int threadId = params["threadId"].toInt();
    QString reason = params["reason"].toString();


    currentThreadId = threadId;
    currentStopReason = reason;

    emit q_ptr->outputReceived("调试: 收到 stopped 事件", false);
    pendingStackTrace = true;
    QJsonObject args;
    args["threadId"] = 1;
    sendRequest("stackTrace", args);
}

void PythonDebugger::Private::handleContinued() {
    qDebug() << "[PythonDebugger] handleContinued";
    emit q_ptr->outputReceived("调试: 收到 continued 事件", false);
    emit q_ptr->continued();
}

void PythonDebugger::Private::handleExited(const QJsonObject& params) {
    running = false;
    int code = params["exitCode"].toInt();
    qDebug() << "[PythonDebugger] Program exited with code:" << code;
    emit q_ptr->outputReceived(QString("调试: 程序退出，退出码 %1").arg(code), false);
    emit q_ptr->exited(code);
}

void PythonDebugger::Private::handleOutput(const QJsonObject& params) {
    QString output = params["output"].toString();
    QString category = params["category"].toString();
    bool isError = (category == "stderr");
    qDebug() << "[PythonDebugger] Output:" << output;
    emit q_ptr->outputReceived(output, isError);
}

void PythonDebugger::Private::handleStackTrace(const QJsonObject& response) {
    QJsonArray framesArray = response["stackFrames"].toArray();
    qDebug() << "[PythonDebugger] Stack frames count:" << framesArray.size();
    emit q_ptr->outputReceived(QString("调试: 栈帧数量 %1").arg(framesArray.size()), false);

    QList<StackFrame> framesList;
    for (const QJsonValue& val : framesArray) {
        QJsonObject frameObj = val.toObject();
        StackFrame f;
        f.level = frameObj["id"].toInt();
        f.functionName = frameObj["name"].toString();
        f.filePath = frameObj["source"].toObject()["path"].toString();
        f.lineNumber = frameObj["line"].toInt();
        framesList << f;
        qDebug() << "[PythonDebugger] Frame:" << f.functionName << f.filePath << f.lineNumber;
        emit q_ptr->outputReceived(QString("调试: 帧 %1 %2:%3").arg(f.functionName).arg(f.filePath).arg(f.lineNumber), false);
    }
    frames = framesList;
    emit q_ptr->stackUpdated(frames);

    if (pendingStackTrace && !frames.isEmpty()) {
        pendingStackTrace = false;
        const StackFrame& first = frames.first();
        currentFile = first.filePath;
        currentLine = first.lineNumber;
        qDebug() << "[PythonDebugger] Stopped at" << currentFile << ":" << currentLine;
        QString reason = currentStopReason.isEmpty() ? "breakpoint" : currentStopReason;
        emit q_ptr->stopped(currentFile, currentLine, reason);
    }

    if (!frames.isEmpty()) {
        QJsonObject args;
        args["frameId"] = frames.first().level;
        sendRequest("scopes", args);
    }
}

void PythonDebugger::Private::handleScopes(const QJsonObject& response) {
    QJsonArray scopes = response["scopes"].toArray();
    if (!scopes.isEmpty()) {
        int varsRef = scopes.first().toObject()["variablesReference"].toInt();
        QJsonObject args;
        args["variablesReference"] = varsRef;
        sendRequest("variables", args);
    }
}

void PythonDebugger::Private::handleVariables(const QJsonObject& response) {
    QJsonArray vars = response["variables"].toArray();
    QList<DebugVariable> varList;
    for (const QJsonValue& val : vars) {
        QJsonObject obj = val.toObject();
        DebugVariable v;
        v.name = obj["name"].toString();
        v.type = obj["type"].toString();
        v.value = obj["value"].toString();
        v.isComplex = obj["variablesReference"].toInt() > 0;
        varList << v;
    }
    variables = varList;
    emit q_ptr->variablesUpdated(variables);
}

bool PythonDebugger::startDebugging(const DebugConfig& config) {
    qDebug() << "[PythonDebugger] startDebugging entered";
    d->finishedEmitted = false;
    // 清理端口
    killProcessOnPort(5678);
    QThread::msleep(200);

    // 重置状态
    d->initialized = false;
    d->breakpoints.clear();
    d->continuePending = false;

    QString python = config.debuggerPath;
    if (python.isEmpty()) python = "python3";
    d->config = config;

    // 检查 debugpy
    QProcess check;
    check.start(python, { "-c", "import debugpy" });
    check.waitForFinished(2000);
    if (check.exitCode() != 0) {
        emit outputReceived("正在安装 debugpy...", false);
        QProcess install;
        install.start(python, { "-m", "pip", "install", "debugpy" });
        install.waitForFinished(30000);
        if (install.exitCode() != 0) {
            emit outputReceived("无法安装 debugpy，请手动安装: pip install debugpy", true);
            return false;
        }
        emit outputReceived("debugpy 安装成功", false);
    }

    // 启动 debugpy
    d->debugpyProcess = new QProcess(this);
    QStringList args;
    args << "-m" << "debugpy" << "--listen" << "127.0.0.1:5678" << "--wait-for-client" << config.executablePath;
    d->debugpyProcess->setProgram(python);
    d->debugpyProcess->setArguments(args);
    d->debugpyProcess->setWorkingDirectory(config.workingDirectory);
    d->debugpyProcess->setProcessChannelMode(QProcess::MergedChannels);

    connect(d->debugpyProcess, &QProcess::readyReadStandardError, this, [this]() {
        QByteArray data = d->debugpyProcess->readAllStandardError();
        QString msg = QString::fromLocal8Bit(data);
        if (!msg.isEmpty()) {
            qDebug() << "[PythonDebugger] debugpy stderr:" << msg;
            emit outputReceived("[debugpy stderr] " + msg, true);
        }
    });
    connect(d->debugpyProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        QByteArray data = d->debugpyProcess->readAllStandardOutput();
        QString msg = QString::fromLocal8Bit(data);
        if (!msg.isEmpty()) {
            qDebug() << "[PythonDebugger] debugpy stdout:" << msg;
            emit outputReceived("[debugpy stdout] " + msg, false);
        }
    });

    d->debugpyProcess->start();
    qDebug() << "[PythonDebugger] debugpy process started with command:" << python << args;

    if (!d->debugpyProcess->waitForStarted(5000)) {
        qDebug() << "[PythonDebugger] 进程启动失败！";
        d->debugpyProcess->disconnect();
        d->debugpyProcess->deleteLater();
        d->debugpyProcess = nullptr;
        return false;
    }

    // 确保 socket 未连接
    if (d->socket->state() != QAbstractSocket::UnconnectedState) {
        d->socket->disconnectFromHost();
        d->socket->waitForDisconnected(500);
    }

    // 连接 socket
    bool connected = false;
    int retries = 30;
    while (retries-- > 0) {
        if (d->debugpyProcess->state() == QProcess::NotRunning) {
            d->debugpyProcess->disconnect();
            d->debugpyProcess->deleteLater();
            d->debugpyProcess = nullptr;
            return false;
        }

        d->socket->connectToHost("127.0.0.1", 5678);
        if (d->socket->waitForConnected(500)) {
            connected = true;
            break;
        }
        if (d->socket->state() != QAbstractSocket::UnconnectedState) {
            d->socket->disconnectFromHost();
            d->socket->waitForDisconnected(500);
        }
        QThread::msleep(200);
    }

    if (!connected) {
        d->debugpyProcess->disconnect();
        d->debugpyProcess->deleteLater();
        d->debugpyProcess = nullptr;
        d->socket->disconnect();
        return false;
    }

    qDebug() << "[PythonDebugger] Socket connected to debugpy";

    // 发送 initialize
    QJsonObject initArgs;
    initArgs["clientID"] = "SprintIDE";
    initArgs["clientName"] = "SprintIDE";
    initArgs["adapterID"] = "debugpy";
    initArgs["pathFormat"] = "path";
    initArgs["linesStartAt1"] = true;
    initArgs["columnsStartAt1"] = true;
    initArgs["supportsVariableType"] = true;
    initArgs["supportsVariablePaging"] = false;
    initArgs["supportsRunInTerminalRequest"] = false;
    initArgs["locale"] = "en-us";
    d->sendRequest("initialize", initArgs);

    d->running = true;
    emit outputReceived("调试器已连接，等待程序启动...", false);
    qDebug() << "进程状态:" << d->debugpyProcess->state();
    return true;
}

void PythonDebugger::stopDebugging() {
    if (d->debugpyProcess == nullptr && (d->socket == nullptr || d->socket->state() == QAbstractSocket::UnconnectedState)) {
        qDebug() << "[PythonDebugger] stopDebugging already cleaned up.";
        return;
    }
    qDebug() << "[PythonDebugger] stopDebugging called";
    if (d->debugpyProcess) {
        d->debugpyProcess->disconnect();
        d->debugpyProcess->terminate();
        if (!d->debugpyProcess->waitForFinished(3000)) {
            d->debugpyProcess->kill();
            d->debugpyProcess->waitForFinished(1000);
        }
        delete d->debugpyProcess;
        d->debugpyProcess = nullptr;
    }
    if (d->socket) {
        d->socket->disconnect();
        if (d->socket->state() == QAbstractSocket::ConnectedState) {
            d->socket->disconnectFromHost();
            d->socket->waitForDisconnected(500);
        }
    }
    d->running = false;
    d->initialized = false;
    d->continuePending = false;
}

void PythonDebugger::setBreakpoints(const QList<Breakpoint>& breakpoints) {
    qDebug() << "[PythonDebugger] setBreakpoints called with" << breakpoints.size() << "breakpoints";
    for (const auto& bp : breakpoints) {
        qDebug() << "  - BP:" << bp.filePath << ":" << bp.lineNumber;
    }
    d->breakpoints = breakpoints;
    if (d->initialized) {
        d->applyBreakpoints();
    } else {
        qDebug() << "[PythonDebugger] Breakpoints stored, will apply after initialized.";
    }
}

void PythonDebugger::continueExecution() {
    d->sendRequest("continue", QJsonObject());
}
void PythonDebugger::stepOver() {
    d->sendRequest("next", {{"threadId", 1}});
}
void PythonDebugger::stepInto() {
    d->sendRequest("stepIn", {{"threadId", 1}});
}
void PythonDebugger::stepOut() {
    d->sendRequest("stepOut", {{"threadId", 1}});
}
void PythonDebugger::pause() {
    d->sendRequest("pause", {{"threadId", 1}});
}

void SprintIDE::PythonDebugger::Private::sendLaunchRequest()
{
    QJsonObject args;
    args["noDebug"] = false;
    args["program"] = config.executablePath;
    args["cwd"] = config.workingDirectory;
    args["justMyCode"] = false;

    sendRequest("launch", args);
}

void PythonDebugger::Private::sendAttachRequest() {
    QJsonObject request;
    request["seq"] = ++seq;
    request["type"] = "request";
    request["command"] = "attach";  // 改为 "attach"

    QJsonObject args;
    QJsonObject connect;
    connect["host"] = "127.0.0.1";
    connect["port"] = 5678;
    args["connect"] = connect;

    request["arguments"] = args;

    QByteArray body = QJsonDocument(request).toJson(QJsonDocument::Compact);
    QByteArray header = QString("Content-Length: %1\r\n\r\n").arg(body.size()).toUtf8();
    socket->write(header + body);
    qDebug() << "[PythonDebugger] Sent request: \"attach\" seq=" << request["seq"].toInt();
}

} // namespace SprintIDE
