#include "python_debugger.h"
#include <QTcpSocket>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QTimer>
#include <QThread>

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

    void sendRequest(const QString& command, const QJsonObject& args = QJsonObject());
    void processMessage(const QByteArray& data);
    void handleStopped(const QJsonObject& params);
    void handleContinued();
    void handleExited(const QJsonObject& params);
    void handleOutput(const QJsonObject& params);
    void handleStackTrace(const QJsonObject& response);
    void handleScopes(const QJsonObject& response);
    void handleVariables(const QJsonObject& response);

    QList<Breakpoint> breakpoints;
    QList<StackFrame> frames;
    QList<DebugVariable> variables;
    QString currentFile;
    int currentLine = -1;
    bool pendingStackTrace = false;
};

PythonDebugger::PythonDebugger(QObject* parent) : QObject(parent), d(new Private(this)) {
    d->socket = new QTcpSocket(this);
    connect(d->socket, &QTcpSocket::readyRead, this, [this]() {
        d->buffer.append(d->socket->readAll());
        int pos;
        while ((pos = d->buffer.indexOf("Content-Length:")) != -1) {
            int end = d->buffer.indexOf("\r\n\r\n", pos);
            if (end == -1) break;
            int len = 0;
            QByteArray header = d->buffer.mid(pos, end - pos);
            QList<QByteArray> lines = header.split('\r');
            for (const QByteArray& line : lines) {
                if (line.startsWith("Content-Length:")) {
                    len = line.mid(15).trimmed().toInt();
                    break;
                }
            }
            int bodyStart = end + 4;
            if (d->buffer.size() < bodyStart + len) break;
            QByteArray body = d->buffer.mid(bodyStart, len);
            d->buffer.remove(0, bodyStart + len);
            d->processMessage(body);
        }
    });
    connect(d->socket, &QTcpSocket::errorOccurred, this, [this]() {
        emit errorOccurred(d->socket->errorString());
    });
}

PythonDebugger::~PythonDebugger() {
    stopDebugging();
}

void PythonDebugger::Private::sendRequest(const QString& command, const QJsonObject& args) {
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) return;
    QJsonObject obj;
    obj["type"] = "request";
    obj["command"] = command;
    obj["seq"] = ++seq;
    if (!args.isEmpty()) obj["arguments"] = args;
    QByteArray data = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QByteArray packet = "Content-Length: " + QByteArray::number(data.size()) + "\r\n\r\n" + data;
    socket->write(packet);
    pendingRequests[seq] = obj;
}

void PythonDebugger::Private::processMessage(const QByteArray& data) {
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) return;
    if (!doc.isObject()) return;
    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();

    if (type == "event") {
        QString event = obj["event"].toString();
        QJsonObject body = obj["body"].toObject();
        if (event == "stopped") handleStopped(body);
        else if (event == "continued") handleContinued();
        else if (event == "exited") handleExited(body);
        else if (event == "output") handleOutput(body);
    } else if (type == "response") {
        int reqSeq = obj["request_seq"].toInt();
        if (pendingRequests.contains(reqSeq)) {
            QJsonObject req = pendingRequests.take(reqSeq);
            QString cmd = req["command"].toString();
            if (cmd == "stackTrace") {
                handleStackTrace(obj["body"].toObject());
            } else if (cmd == "scopes") {
                handleScopes(obj["body"].toObject());
            } else if (cmd == "variables") {
                handleVariables(obj["body"].toObject());
            }
        }
    }
}

void PythonDebugger::Private::handleStopped(const QJsonObject& params) {
    // 不立即发射 stopped，而是请求栈帧
    pendingStackTrace = true;
    QJsonObject args;
    args["threadId"] = 1;
    sendRequest("stackTrace", args);
}

void PythonDebugger::Private::handleContinued() {
    // 清除当前高亮
    emit q_ptr->continued();
}

void PythonDebugger::Private::handleExited(const QJsonObject& params) {
    running = false;
    emit q_ptr->exited(params["exitCode"].toInt());
}

void PythonDebugger::Private::handleOutput(const QJsonObject& params) {
    QString output = params["output"].toString();
    QString category = params["category"].toString();
    bool isError = (category == "stderr");
    emit q_ptr->outputReceived(output, isError);
}

void PythonDebugger::Private::handleStackTrace(const QJsonObject& response) {
    QJsonArray framesArray = response["stackFrames"].toArray();
    QList<StackFrame> framesList;
    for (const QJsonValue& val : framesArray) {
        QJsonObject frameObj = val.toObject();
        StackFrame f;
        f.level = frameObj["id"].toInt();
        f.functionName = frameObj["name"].toString();
        f.filePath = frameObj["source"].toObject()["path"].toString();
        f.lineNumber = frameObj["line"].toInt();
        framesList << f;
    }
    frames = framesList;
    emit q_ptr->stackUpdated(frames);

    // 如果有待处理的停止事件，现在可以发射 stopped 信号
    if (pendingStackTrace && !frames.isEmpty()) {
        pendingStackTrace = false;
        // 取第一个帧
        const StackFrame& first = frames.first();
        currentFile = first.filePath;
        currentLine = first.lineNumber;
        emit q_ptr->stopped(currentFile, currentLine, "breakpoint"); // reason 可自定义
    }

    // 请求变量：获取第一个帧的 scopes
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
    QString python = config.debuggerPath;
    if (python.isEmpty()) python = "python3";

    // 检查 debugpy 是否安装
    QProcess check;
    check.start(python, { "-c", "import debugpy" });
    check.waitForFinished(2000);
    if (check.exitCode() != 0) {
        // 自动安装 debugpy
        emit outputReceived("正在安装 debugpy...", false);
        QProcess install;
        install.start(python, { "-m", "pip", "install", "debugpy" });
        install.waitForFinished(30000);
        if (install.exitCode() != 0) {
            emit errorOccurred("无法安装 debugpy，请手动安装: pip install debugpy");
            return false;
        }
        emit outputReceived("debugpy 安装成功", false);
    }

    // 启动 debugpy 服务
    d->debugpyProcess = new QProcess(this);
    QStringList args;
    args << "-m" << "debugpy" << "--listen" << "127.0.0.1:5678"
         << "--wait-for-client" << config.executablePath;
    d->debugpyProcess->setProgram(python);
    d->debugpyProcess->setArguments(args);
    d->debugpyProcess->setWorkingDirectory(config.workingDirectory);
    d->debugpyProcess->start();

    if (!d->debugpyProcess->waitForStarted(5000)) {
        emit errorOccurred("启动 debugpy 失败: " + d->debugpyProcess->errorString());
        return false;
    }

    // 等待端口就绪（带重试）
    QTcpSocket socket;
    int retries = 20;
    bool connected = false;
    while (retries-- > 0) {
        socket.connectToHost("127.0.0.1", 5678);
        if (socket.waitForConnected(500)) {
            connected = true;
            break;
        }
        QThread::msleep(200);
    }

    if (!connected) {
        emit errorOccurred("无法连接到 debugpy (端口 5678)");
        return false;
    }
    socket.disconnectFromHost();

    // 重新连接持久化 Socket
    d->socket->connectToHost("127.0.0.1", 5678);
    if (!d->socket->waitForConnected(2000)) {
        emit errorOccurred("建立调试连接失败");
        return false;
    }

    // 发送初始化
    QJsonObject initArgs;
    initArgs["clientID"] = "SprintIDE";
    initArgs["clientName"] = "SprintIDE";
    initArgs["adapterID"] = "debugpy";
    initArgs["pathFormat"] = "path";
    initArgs["linesStartAt1"] = true;
    initArgs["columnsStartAt1"] = true;
    initArgs["supportsVariableType"] = true;
    d->sendRequest("initialize", initArgs);

    // 发送 launch
    QJsonObject launchArgs;
    launchArgs["program"] = config.executablePath;
    launchArgs["args"] = QJsonArray::fromStringList(config.arguments);
    launchArgs["cwd"] = config.workingDirectory;
    launchArgs["stopOnEntry"] = config.stopOnEntry;
    d->sendRequest("launch", launchArgs);

    d->running = true;
    return true;
}

void PythonDebugger::stopDebugging() {
    if (d->debugpyProcess) {
        d->debugpyProcess->terminate();
        d->debugpyProcess->waitForFinished(2000);
        delete d->debugpyProcess;
        d->debugpyProcess = nullptr;
    }
    d->socket->disconnectFromHost();
    d->running = false;
}

void PythonDebugger::setBreakpoints(const QList<Breakpoint>& breakpoints) {
    d->breakpoints = breakpoints;
    QMap<QString, QList<int>> bpMap;
    for (const Breakpoint& bp : breakpoints) {
        if (bp.enabled) bpMap[bp.filePath].append(bp.lineNumber);
    }
    for (auto it = bpMap.begin(); it != bpMap.end(); ++it) {
        QJsonObject args;
        args["source"] = QJsonObject{{"path", it.key()}};
        QJsonArray lines;
        for (int line : it.value()) lines.append(line);
        args["breakpoints"] = lines;
        d->sendRequest("setBreakpoints", args);
    }
}

void PythonDebugger::continueExecution() { d->sendRequest("continue", {{"threadId", 1}}); }
void PythonDebugger::stepOver() { d->sendRequest("next", {{"threadId", 1}}); }
void PythonDebugger::stepInto() { d->sendRequest("stepIn", {{"threadId", 1}}); }
void PythonDebugger::stepOut() { d->sendRequest("stepOut", {{"threadId", 1}}); }
void PythonDebugger::pause() { d->sendRequest("pause", {{"threadId", 1}}); }

} // namespace SprintIDE
