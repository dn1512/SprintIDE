// src/cpp/cpp_debugger.cpp
#include "cpp_debugger.h"
#include "dap/dap_client.h"

#include <QFileInfo>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>
#include <QDebug>
#include <QDir>
#include <QFileInfo>

namespace SprintIDE {

// ============================================================
// 辅助：DAP 返回的 source.path 可能是空的或相对的，
//       尝试用 name + workingDir 拼出真实路径。
// ============================================================
static QString normalizeDapSourcePath(const QString& rawPath,
                                      const QString& rawName,
                                      const QString& workingDir)
{
    // 1. 绝对路径且存在 → 直接用
    if (!rawPath.isEmpty()) {
        QFileInfo fi(rawPath);
        if (fi.isAbsolute() && fi.exists()) return rawPath;
    }

    // 2. workingDir + name
    if (!rawName.isEmpty() && !workingDir.isEmpty()) {
        QString cand = QDir(workingDir).absoluteFilePath(rawName);
        if (QFileInfo::exists(cand)) return cand;
    }

    // 3. workingDir + rawPath
    if (!rawPath.isEmpty() && !workingDir.isEmpty()) {
        QString cand = QDir(workingDir).absoluteFilePath(rawPath);
        if (QFileInfo::exists(cand)) return cand;
    }

    // 4. 都不行，返回原值（可能为空）
    return rawPath;
}

// ============================================================
// 构造 / 析构
// ============================================================
CppDebugger::CppDebugger(QObject* parent)
    : QObject(parent)
{
}

CppDebugger::~CppDebugger()
{
    stopDebugging();
}

// ============================================================
// startDebugging
// ============================================================
bool CppDebugger::startDebugging(const CppDebugConfig& config)
{
    m_config = config;

    if (config.adapterPath.isEmpty()) {
        emit errorOccurred("DAP adapter path is empty");
        return false;
    }
    if (config.program.isEmpty() || !QFileInfo::exists(config.program)) {
        emit errorOccurred("Program not found: " + config.program);
        return false;
    }

    // 清理旧实例
    if (m_dap) {
        m_dap->disconnect(this);
        m_dap->deleteLater();
        m_dap = nullptr;
    }

    // 重置状态
    m_launched = false;
    m_initReceived = false;
    m_localsVariablesRef = 0;
    m_currentFrameId = 0;
    m_currentThreadId = 1;

    m_dap = new DapClient(this);

    // ---- 连接信号 ----
    connect(m_dap, &DapClient::adapterStarted,
            this, [this]() {
                qDebug() << "[CppDebugger] adapter started, sending initialize";
                m_dap->initialize("lldb", "SprintIDE");
            });
    connect(m_dap, &DapClient::initialized,
            this, &CppDebugger::onAdapterInitialized);
    connect(m_dap, &DapClient::stopped,
            this, &CppDebugger::onDapStopped);
    connect(m_dap, &DapClient::continued,
            this, &CppDebugger::onDapContinued);
    connect(m_dap, &DapClient::exited,
            this, &CppDebugger::onDapExited);
    connect(m_dap, &DapClient::terminated,
            this, &CppDebugger::onDapTerminated);
    connect(m_dap, &DapClient::output,
            this, &CppDebugger::onDapOutput);
    connect(m_dap, &DapClient::stackTraceReceived,
            this, &CppDebugger::onStackTrace);
    connect(m_dap, &DapClient::scopesReceived,
            this, &CppDebugger::onScopes);
    connect(m_dap, &DapClient::variablesReceived,
            this, &CppDebugger::onVariables);
    connect(m_dap, &DapClient::errorOccurred,
            this, &CppDebugger::onAdapterError);
    connect(m_dap, &DapClient::adapterExited,
            this, &CppDebugger::onAdapterExited);
    connect(m_dap, &DapClient::breakpointsValidated,
            this, [](const QString& file,
               const QList<SprintIDE::Dap::Breakpoint>& bps) {
                qDebug() << "[CppDebugger] 断点响应:" << file
                         << "共" << bps.size() << "个";
                for (const auto& bp : bps) {
                    qDebug() << "  行" << bp.line
                             << "verified=" << bp.verified
                             << "message=" << bp.message;
                }
            });

    // 启动 adapter 进程
    bool ok = m_dap->startAdapter(config.adapterPath,
                                  {},
                                  config.workingDirectory,
                                  config.environment);
    if (!ok) {
        emit errorOccurred("Failed to start DAP adapter: " + config.adapterPath);
        return false;
    }

    m_isDebugging = true;
    return true;
}

// ============================================================
// onAdapterInitialized
//
// 这个槽会被触发两次：
//   1. initialize 响应到达  → 发送 launch
//   2. initialized 事件到达 → setBreakpoints + configurationDone
// ============================================================
void CppDebugger::onAdapterInitialized()
{
    if (!m_launched) {
        qDebug() << "[CppDebugger] initialize done, sending launch";
        m_launched = true;
        sendLaunch();
        return;
    }

    if (!m_initReceived) {
        qDebug() << "[CppDebugger] initialized event, applying breakpoints";
        m_initReceived = true;
        applyBreakpoints();
        m_dap->configurationDone();
    }
}

// ============================================================
// sendLaunch
// ============================================================
void CppDebugger::sendLaunch()
{
    QJsonObject args;
    args["program"] = m_config.program;
    args["cwd"] = m_config.workingDirectory.isEmpty()
                      ? QFileInfo(m_config.program).absolutePath()
                      : m_config.workingDirectory;

    QJsonArray argArr;
    for (const QString& a : m_config.programArgs) argArr.append(a);
    args["args"] = argArr;

    args["stopOnEntry"] = m_config.stopOnEntry;
    args["terminal"] = "console";
    args["console"] = "internalConsole";

    // LLDB 初始化命令：不让 LLDB 在 exec 后立即停
    //（macOS 上会表现为停在 _dyld_start，误导用户）
    // 暂时去掉 initCommands，避免和 codelldb 的解析逻辑冲突。
    // 我们用 onStackTrace 里的 startup-stop 过滤来处理 dyld 停止。
    // （如果未来需要，可以加 args["initCommands"]，但要用字符串数组格式）

    m_dap->launch(args);
    qDebug() << "[CppDebugger] launch sent:" << m_config.program
             << "stopOnEntry=" << m_config.stopOnEntry;
}

// ============================================================
// 断点同步
// ============================================================
void CppDebugger::setBreakpoints(const QList<SprintIDE::Breakpoint>& breakpoints)
{
    m_breakpoints = breakpoints;
    if (m_initReceived) {
        applyBreakpoints();
    }
}

void CppDebugger::applyBreakpoints()
{
    // 按文件分组
    QMap<QString, QList<int>> byFile;
    for (const SprintIDE::Breakpoint& bp : m_breakpoints) {
        if (!bp.enabled) continue;
        QString abs = QFileInfo(bp.filePath).absoluteFilePath();
        byFile[abs].append(bp.lineNumber);
    }

    // 每个文件发一次 setBreakpoints
    for (auto it = byFile.begin(); it != byFile.end(); ++it) {
        m_dap->setBreakpoints(it.key(), it.value());
        qDebug() << "[CppDebugger] setBreakpoints" << it.key() << it.value();
    }
}

// ============================================================
// 执行控制
// ============================================================
void CppDebugger::continueExecution()
{
    if (m_dap) m_dap->continueExecution(m_currentThreadId);
}

void CppDebugger::stepOver()
{
    if (m_dap) m_dap->stepOver(m_currentThreadId);
}

void CppDebugger::stepInto()
{
    if (m_dap) m_dap->stepIn(m_currentThreadId);
}

void CppDebugger::stepOut()
{
    if (m_dap) m_dap->stepOut(m_currentThreadId);
}

void CppDebugger::pause()
{
    if (m_dap) m_dap->pause(m_currentThreadId);
}

void CppDebugger::evaluateExpression(const QString& expr)
{
    if (m_dap) m_dap->evaluate(expr, m_currentFrameId, "repl");
}

// ============================================================
// DAP 事件响应
// ============================================================
void CppDebugger::onDapStopped(const Dap::StoppedInfo& info)
{
    qDebug() << "[CppDebugger] stopped, reason=" << info.reason
             << "threadId=" << info.threadId;
    m_currentThreadId = info.threadId > 0 ? info.threadId : 1;
    requestTopFrameData();
}

void CppDebugger::requestTopFrameData()
{
    if (!m_dap) return;
    m_dap->stackTrace(m_currentThreadId);
}

void CppDebugger::onStackTrace(const QList<Dap::StackFrame>& frames)
{
    // 1. 转成 DebugManager 用的 StackFrame（供 DebugPanel 显示调用栈）
    QList<SprintIDE::StackFrame> out;
    for (int i = 0; i < frames.size(); ++i) {
        const Dap::StackFrame& f = frames[i];
        SprintIDE::StackFrame sf;
        sf.level = i;
        sf.functionName = f.name;
        sf.filePath = normalizeDapSourcePath(f.source.path,
                                             f.source.name,
                                             m_config.workingDirectory);
        sf.lineNumber = f.line;
        out.append(sf);
    }
    emit stackUpdated(out);

    if (frames.isEmpty()) return;

    const Dap::StackFrame& top = frames.first();
    m_currentFrameId = top.id;

    // 2. 补全路径
    m_currentFile = normalizeDapSourcePath(top.source.path,
                                           top.source.name,
                                           m_config.workingDirectory);
    m_currentLine = top.line;

    qDebug() << "[CppDebugger] 顶层帧: name=" << top.name
             << "rawPath=" << top.source.path
             << "name=" << top.source.name
             << "-> resolved=" << m_currentFile;

    // 3. ★ 判断是否是"启动阶段的伪停止"（dyld / 系统加载器）
    //    macOS LLDB 启动时会在 dyld/startup 阶段收到一次 exception，
    //    这不是用户关心的停止，应自动 continue 直到真正的断点。
    bool isStartupStop = m_currentFile.isEmpty() &&
                         (top.name == "_dyld_start" ||
                          top.name == "start" ||
                          top.name == "__start" ||
                          top.name.startsWith("dyld", Qt::CaseInsensitive) ||
                          top.name.contains("dyld", Qt::CaseInsensitive) ||
                          top.name.contains("libdyld", Qt::CaseInsensitive) ||
                          top.name.contains("libsystem", Qt::CaseInsensitive));

    if (isStartupStop) {
        qDebug() << "[CppDebugger] 检测到 startup 停止（" << top.name
                 << "），自动 continue，等待真正的断点";
        // 不通知上层，直接继续
        m_dap->continueExecution(m_currentThreadId);
        return;
    }

    // 4. 正常停止 → 通知上层
    emit stopped(m_currentFile, m_currentLine, QStringLiteral("breakpoint"));

    // 5. 拉顶层作用域（变量）
    m_dap->scopes(top.id);
}

void CppDebugger::onScopes(int frameId, const QList<Dap::Scope>& scopes)
{
    Q_UNUSED(frameId);

    // 优先取 "Locals"
    int localsRef = 0;
    for (const Dap::Scope& s : scopes) {
        if (s.name.compare("Locals", Qt::CaseInsensitive) == 0) {
            localsRef = s.variablesReference;
            break;
        }
    }
    // 没有 Locals 就取第一个非 expensive 的
    if (localsRef == 0) {
        for (const Dap::Scope& s : scopes) {
            if (!s.expensive) { localsRef = s.variablesReference; break; }
        }
    }

    if (localsRef > 0) {
        m_localsVariablesRef = localsRef;
        m_dap->variables(localsRef);
    } else {
        emit variablesUpdated({});
    }
}

void CppDebugger::onVariables(int parentRef, const QList<Dap::Variable>& vars)
{
    Q_UNUSED(parentRef);
    QList<SprintIDE::DebugVariable> out;
    for (const Dap::Variable& v : vars) {
        SprintIDE::DebugVariable dv;
        dv.name = v.name;
        dv.type = v.type;
        dv.value = v.value;
        dv.isComplex = v.isContainer();
        out.append(dv);
    }
    emit variablesUpdated(out);
}

void CppDebugger::onDapContinued(int threadId)
{
    Q_UNUSED(threadId);
    emit continued();
}

void CppDebugger::onDapExited(int exitCode)
{
    m_isDebugging = false;
    emit exited(exitCode);
}

void CppDebugger::onDapTerminated()
{
    m_isDebugging = false;
    emit terminated();
}

void CppDebugger::onDapOutput(const QString& category, const QString& text)
{
    // category: "stdout" / "stderr" / "console" / "telemetry"
    bool isErr = (category == "stderr");
    emit outputReceived(text, isErr);
}

void CppDebugger::onAdapterError(const QString& error)
{
    emit errorOccurred(error);
}

void CppDebugger::onAdapterExited(int exitCode)
{
    Q_UNUSED(exitCode);
    if (m_isDebugging) {
        m_isDebugging = false;
        emit terminated();
    }
}

// ============================================================
// stopDebugging
// ============================================================
void CppDebugger::stopDebugging()
{
    if (!m_dap) {
        m_isDebugging = false;
        return;
    }

    m_dap->disconnect(this);
    m_dap->disconnect();

    // 请求优雅退出
    m_dap->disconnect(true);

    // 给 adapter 一点时间响应
    QThread::msleep(100);

    m_dap->deleteLater();
    m_dap = nullptr;

    m_isDebugging = false;
    m_initReceived = false;
    m_launched = false;
    m_localsVariablesRef = 0;
}

} // namespace SprintIDE
