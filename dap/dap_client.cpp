#include "dap_client.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QFileInfo>

namespace SprintIDE {

using namespace SprintIDE::Dap;

// ============================================================
// 构造 / 析构
// ============================================================
DapClient::DapClient(QObject* parent)
    : QObject(parent)
{
}

DapClient::~DapClient()
{
    if (m_process) {
        m_process->disconnect(this);
        if (m_process->state() != QProcess::NotRunning) {
            m_process->terminate();
            if (!m_process->waitForFinished(1000)) {
                m_process->kill();
                m_process->waitForFinished(500);
            }
        }
        delete m_process;
        m_process = nullptr;
    }
}

// ============================================================
// startAdapter
// ============================================================
bool DapClient::startAdapter(const QString& program,
                             const QStringList& args,
                             const QString& workingDir,
                             const QProcessEnvironment& env)
{
    if (m_process) {
        qWarning() << "[DapClient] already started";
        return false;
    }

    m_process = new QProcess(this);
    m_process->setProgram(program);
    m_process->setArguments(args);
    if (!workingDir.isEmpty()) m_process->setWorkingDirectory(workingDir);
    if (!env.isEmpty()) m_process->setProcessEnvironment(env);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &DapClient::onStdout);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &DapClient::onStderr);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &DapClient::onFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &DapClient::onErrorOccurred);

    m_state = State::Starting;
    m_process->start();

    if (!m_process->waitForStarted(5000)) {
        QString err = QString("Failed to start adapter: %1").arg(m_process->errorString());
        qWarning() << "[DapClient]" << err;
        emit errorOccurred(err);
        m_process->deleteLater();
        m_process = nullptr;
        m_state = State::Disconnected;
        return false;
    }

    qDebug() << "[DapClient] adapter started:" << program << args;
    m_state = State::Initializing;
    emit adapterStarted();
    return true;
}

// ============================================================
// sendRequest / sendRaw
// ============================================================
int DapClient::sendRequest(const QString& command, const QJsonObject& args)
{
    QJsonObject msg;
    msg["seq"] = m_nextSeq;
    msg["type"] = "request";
    msg["command"] = command;
    if (!args.isEmpty()) msg["arguments"] = args;

    m_pendingCommands[m_nextSeq] = command;
    int seq = m_nextSeq++;
    sendRaw(msg);
    return seq;
}

void DapClient::sendRaw(const QJsonObject& msg)
{
    if (!m_process || m_process->state() != QProcess::Running) {
        qWarning() << "[DapClient] sendRaw: process not running";
        return;
    }
    QByteArray body = QJsonDocument(msg).toJson(QJsonDocument::Compact);
    QByteArray header = "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n";
    m_process->write(header + body);
    m_process->waitForBytesWritten(500);
}

// ============================================================
// 便捷封装
// ============================================================
void DapClient::initialize(const QString& adapterID, const QString& clientID)
{
    m_adapterID = adapterID;
    m_clientID = clientID;

    QJsonObject args;
    args["clientID"] = clientID;
    args["clientName"] = clientID;
    args["adapterID"] = adapterID;
    args["pathFormat"] = "path";
    args["linesStartAt1"] = true;
    args["columnsStartAt1"] = true;
    args["supportsVariableType"] = true;
    args["supportsVariablePaging"] = false;
    args["supportsRunInTerminalRequest"] = false;
    args["locale"] = "en-us";

    m_state = State::Initializing;
    sendRequest("initialize", args);
}

void DapClient::launch(const QJsonObject& launchArgs)
{
    m_state = State::Launching;
    sendRequest("launch", launchArgs);
}

void DapClient::attach(const QJsonObject& attachArgs)
{
    m_state = State::Launching;
    sendRequest("attach", attachArgs);
}

void DapClient::setBreakpoints(const QString& sourcePath,
                               const QList<BreakpointSpec>& specs)
{
    QJsonObject src;
    src["path"] = sourcePath;
    src["name"] = QFileInfo(sourcePath).fileName();

    QJsonArray bps;
    for (const auto& spec : specs) {
        QJsonObject bp;
        bp["line"] = spec.line;
        if (!spec.condition.isEmpty())    bp["condition"] = spec.condition;
        if (!spec.hitCondition.isEmpty()) bp["hitCondition"] = spec.hitCondition;
        bps.append(bp);
    }

    QJsonObject args;
    args["source"] = src;
    args["breakpoints"] = bps;
    sendRequest("setBreakpoints", args);
}

void DapClient::setBreakpoints(const QString& sourcePath, const QList<int>& lines)
{
    QList<BreakpointSpec> specs;
    for (int l : lines) {
        BreakpointSpec s; s.line = l;
        specs.append(s);
    }
    setBreakpoints(sourcePath, specs);
}

void DapClient::configurationDone()
{
    m_state = State::Configuring;
    sendRequest("configurationDone", QJsonObject());
}

void DapClient::continueExecution(int threadId)
{
    QJsonObject args;
    if (threadId > 0) args["threadId"] = threadId;
    sendRequest("continue", args);
}

void DapClient::stepOver(int threadId)
{
    QJsonObject args;
    if (threadId > 0) args["threadId"] = threadId;
    sendRequest("next", args);
}

void DapClient::stepIn(int threadId)
{
    QJsonObject args;
    if (threadId > 0) args["threadId"] = threadId;
    sendRequest("stepIn", args);
}

void DapClient::stepOut(int threadId)
{
    QJsonObject args;
    if (threadId > 0) args["threadId"] = threadId;
    sendRequest("stepOut", args);
}

void DapClient::pause(int threadId)
{
    QJsonObject args;
    if (threadId > 0) args["threadId"] = threadId;
    sendRequest("pause", args);
}

void DapClient::stackTrace(int threadId)
{
    QJsonObject args;
    args["threadId"] = threadId;
    args["startFrame"] = 0;
    args["levels"] = 20;
    sendRequest("stackTrace", args);
}

void DapClient::scopes(int frameId)
{
    QJsonObject args;
    args["frameId"] = frameId;
    sendRequest("scopes", args);
}

void DapClient::variables(int variablesReference)
{
    QJsonObject args;
    args["variablesReference"] = variablesReference;
    int seq = sendRequest("variables", args);
    m_pendingVariablesRef[seq] = variablesReference;
}

void DapClient::evaluate(const QString& expr, int frameId, const QString& context)
{
    QJsonObject args;
    args["expression"] = expr;
    if (frameId > 0) args["frameId"] = frameId;
    args["context"] = context;
    sendRequest("evaluate", args);
}

void DapClient::disconnect(bool terminateDebuggee)
{
    m_state = State::Stopping;
    QJsonObject args;
    args["terminateDebuggee"] = terminateDebuggee;
    sendRequest("disconnect", args);
}

// ============================================================
// stdout 解析
// ============================================================
void DapClient::onStdout()
{
    appendToBuffer(m_process->readAllStandardOutput());

    QByteArray body;
    while (tryExtractOneMessage(body)) {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(body, &err);
        if (err.error != QJsonParseError::NoError) {
            qWarning() << "[DapClient] JSON parse error:" << err.errorString()
            << "body:" << body.left(200);
            continue;
        }
        if (doc.isObject()) {
            handleMessage(doc.object());
        }
    }
}

void DapClient::appendToBuffer(const QByteArray& data)
{
    m_buffer.append(data);
}

bool DapClient::tryExtractOneMessage(QByteArray& outBody)
{
    // 丢弃不是 Content-Length 开头的垃圾数据
    while (!m_buffer.isEmpty() && !m_buffer.startsWith("Content-Length:")) {
        int nl = m_buffer.indexOf('\n');
        if (nl == -1) {
            if (m_buffer.size() > 8192) m_buffer.clear();
            return false;
        }
        m_buffer.remove(0, nl + 1);
    }

    int headerEnd = m_buffer.indexOf("\r\n\r\n");
    if (headerEnd == -1) return false;

    QByteArray header = m_buffer.left(headerEnd);
    int contentLength = -1;
    for (const QByteArray& line : header.split('\n')) {
        QByteArray l = line.trimmed();
        if (l.startsWith("Content-Length:")) {
            contentLength = l.mid(QByteArray("Content-Length:").size()).trimmed().toInt();
            break;
        }
    }
    if (contentLength <= 0) {
        m_buffer.remove(0, headerEnd + 4);
        return false;
    }

    int bodyStart = headerEnd + 4;
    if (m_buffer.size() < bodyStart + contentLength) return false;

    outBody = m_buffer.mid(bodyStart, contentLength);
    m_buffer.remove(0, bodyStart + contentLength);
    return true;
}

// ============================================================
// 消息分发
// ============================================================
void DapClient::handleMessage(const QJsonObject& msg)
{
    const QString type = msg["type"].toString();
    if (type == "response")           handleResponse(msg);
    else if (type == "event")         handleEvent(msg);
    else if (type == "request")       handleReverseRequest(msg);
    else qWarning() << "[DapClient] unknown message type:" << type;
}

void DapClient::handleResponse(const QJsonObject& msg)
{
    int requestSeq = msg["request_seq"].toInt();
    QString command = msg["command"].toString();
    bool success = msg["success"].toBool();
    const QJsonObject body = msg["body"].toObject();

    // 从 pending 队列移除
    m_pendingCommands.remove(requestSeq);

    if (!success) {
        if (command == "variables") {
            m_pendingVariablesRef.remove(requestSeq);
        }

        QString errMsg = msg["message"].toString();
        qWarning() << "[DapClient] request failed:" << command << "->" << errMsg;
        emit errorOccurred(QString("%1 failed: %2").arg(command, errMsg));
        return;
    }

    // ---- 分发 ----
    if (command == "initialize") {
        // 等待 adapter 发 initialized 事件再 launch，不在这里做
        emit initialized();
    }
    else if (command == "stackTrace") {
        QList<StackFrame> frames = parseStackFrames(body["stackFrames"].toArray());
        emit stackTraceReceived(frames);
    }
    else if (command == "scopes") {
        int frameId = body["frameId"].toInt();  // 有些 adapter 不返回，就发 0
        QList<Scope> scopes = parseScopes(body["scopes"].toArray());
        emit scopesReceived(frameId, scopes);
    }
    else if (command == "variables") {
        int parentRef = m_pendingVariablesRef.take(requestSeq);
        QList<Variable> vars = parseVariables(body["variables"].toArray());
        emit variablesReceived(parentRef, vars);
    }
    else if (command == "evaluate") {
        QString result = body["result"].toString();
        QString type = body["type"].toString();
        QString expr = msg["arguments"].toObject()["expression"].toString();
        emit evaluateResult(expr, result, type);
    }
    else if (command == "setBreakpoints") {
        // 从 pendingCommands 拿不到 sourcePath，需要 adapter 返回的 source 或我们从发起时记住
        QString sourcePath = body["source"].toObject()["path"].toString();
        QList<Breakpoint> bps = parseBreakpoints(sourcePath, body["breakpoints"].toArray());
        emit breakpointsValidated(sourcePath, bps);
    }
    else if (command == "disconnect") {
        qDebug() << "[DapClient] disconnect acknowledged";
        m_state = State::Disconnected;
    }
    // 其他命令（continue/next/stepIn/stepOut/pause/launch/configurationDone）无需特殊处理
}

void DapClient::handleEvent(const QJsonObject& msg)
{
    const QString event = msg["event"].toString();
    const QJsonObject body = msg["body"].toObject();

    if (event == "initialized") {
        // adapter 已准备好接收 setBreakpoints / configurationDone
        emit initialized();
    }
    else if (event == "stopped") {
        StoppedInfo info;
        info.reason = body["reason"].toString();
        info.threadId = body["threadId"].toInt();
        info.description = body["description"].toString();
        info.text = body["text"].toString();
        info.allThreadsStopped = body["allThreadsStopped"].toBool(false);
        m_state = State::Running;   // paused 也算 running
        emit stopped(info);
    }
    else if (event == "continued") {
        int threadId = body["threadId"].toInt();
        emit continued(threadId);
    }
    else if (event == "exited") {
        int code = body["exitCode"].toInt();
        emit exited(code);
    }
    else if (event == "terminated") {
        m_state = State::Stopping;
        emit terminated();
    }
    else if (event == "output") {
        QString category = body["category"].toString();
        QString text = body["output"].toString();
        emit output(category, text);
    }
    else if (event == "process") {
        QString name = body["name"].toString();
        int pid = body["systemProcessId"].toInt();
        m_state = State::Running;
        emit processStartedEvent(name, pid);
    }
    else if (event == "thread") {
        QString reason = body["reason"].toString();
        int tid = body["threadId"].toInt();
        emit threadEvent(reason, tid);
    }
    else if (event == "module" || event == "loadedSource" ||
             event == "process" || event == "capabilities") {
        // 常见但暂时不用的 DAP 事件，静默忽略
    }
    else {
        qDebug() << "[DapClient] unhandled event:" << event;
    }
}

void DapClient::handleReverseRequest(const QJsonObject& msg)
{
    // adapter 发起的反向请求，常见的有 runInTerminal
    const QString command = msg["command"].toString();
    const int reqSeq = msg["seq"].toInt();

    QJsonObject resp;
    resp["seq"] = m_nextSeq++;
    resp["type"] = "response";
    resp["request_seq"] = reqSeq;
    resp["command"] = command;
    resp["success"] = false;    // 默认不支持
    resp["message"] = "not supported by SprintIDE";

    if (command == "runInTerminal") {
        // 暂不支持（未来可以 fork 一个终端）
        qDebug() << "[DapClient] runInTerminal requested, not supported";
    }

    sendRaw(resp);
}

// ============================================================
// stderr / 进程生命周期
// ============================================================
void DapClient::onStderr()
{
    QByteArray data = m_process->readAllStandardError();
    QString text = QString::fromLocal8Bit(data);
    if (text.isEmpty()) return;
    // adapter 自己的日志，转发到 output 信号
    for (const QString& line : text.split('\n', Qt::SkipEmptyParts)) {
        emit output("stderr", line);
    }
}

void DapClient::onFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(status);
    qDebug() << "[DapClient] adapter exited, code=" << exitCode;
    m_state = State::Disconnected;
    emit adapterExited(exitCode);
}

void DapClient::onErrorOccurred(QProcess::ProcessError err)
{
    QString msg;
    switch (err) {
    case QProcess::FailedToStart: msg = "Adapter failed to start"; break;
    case QProcess::Crashed:       msg = "Adapter crashed";         break;
    case QProcess::Timedout:      msg = "Adapter timeout";         break;
    case QProcess::WriteError:    msg = "Write to adapter failed"; break;
    case QProcess::ReadError:     msg = "Read from adapter failed";break;
    default:                      msg = "Adapter unknown error";   break;
    }
    if (m_process) msg += ": " + m_process->errorString();
    qWarning() << "[DapClient]" << msg;
    emit errorOccurred(msg);
}

// ============================================================
// 解析辅助
// ============================================================
QList<StackFrame> DapClient::parseStackFrames(const QJsonArray& arr)
{
    QList<StackFrame> out;
    for (const QJsonValue& v : arr) {
        QJsonObject o = v.toObject();
        StackFrame f;
        f.id = o["id"].toInt();
        f.name = o["name"].toString();
        QJsonObject src = o["source"].toObject();
        f.source.path = src["path"].toString();
        f.source.name = src["name"].toString();
        f.line = o["line"].toInt();
        f.column = o["column"].toInt();
        out.append(f);
    }
    return out;
}

QList<Variable> DapClient::parseVariables(const QJsonArray& arr)
{
    QList<Variable> out;
    for (const QJsonValue& v : arr) {
        QJsonObject o = v.toObject();
        Variable var;
        var.name = o["name"].toString();
        var.value = o["value"].toString();
        var.type = o["type"].toString();
        var.variablesReference = o["variablesReference"].toInt();
        out.append(var);
    }
    return out;
}

QList<Scope> DapClient::parseScopes(const QJsonArray& arr)
{
    QList<Scope> out;
    for (const QJsonValue& v : arr) {
        QJsonObject o = v.toObject();
        Scope s;
        s.name = o["name"].toString();
        s.variablesReference = o["variablesReference"].toInt();
        s.expensive = o["expensive"].toBool(false);
        out.append(s);
    }
    return out;
}

QList<Breakpoint> DapClient::parseBreakpoints(const QString& sourcePath,
                                              const QJsonArray& arr)
{
    QList<Breakpoint> out;
    for (const QJsonValue& v : arr) {
        QJsonObject o = v.toObject();
        Breakpoint b;
        b.id = o["id"].toInt();
        b.verified = o["verified"].toBool();
        b.line = o["line"].toInt();
        b.column = o["column"].toInt();
        b.message = o["message"].toString();
        b.sourcePath = sourcePath;
        out.append(b);
    }
    return out;
}

} // namespace SprintIDE
