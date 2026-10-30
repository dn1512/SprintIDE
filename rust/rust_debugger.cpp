// src/rust/rust_debugger.cpp
#include "rust_debugger.h"
#include "dap/dap_client.h"

#include <QFileInfo>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>
#include <QDebug>
#include <QDir>

namespace SprintIDE {

    // ============================================================
    // 辅助：DAP source.path 可能是空 / 相对，
    //       用 name + workingDir 补全（与 CppDebugger 完全一致）
    // ============================================================
    static QString normalizeDapSourcePath(const QString& rawPath,
        const QString& rawName,
        const QString& workingDir)
    {
        if (!rawPath.isEmpty()) {
            QFileInfo fi(rawPath);
            if (fi.isAbsolute() && fi.exists()) return rawPath;
        }
        if (!rawName.isEmpty() && !workingDir.isEmpty()) {
            const QString cand = QDir(workingDir).absoluteFilePath(rawName);
            if (QFileInfo::exists(cand)) return cand;
        }
        if (!rawPath.isEmpty() && !workingDir.isEmpty()) {
            const QString cand = QDir(workingDir).absoluteFilePath(rawPath);
            if (QFileInfo::exists(cand)) return cand;
        }
        return rawPath;
    }

    // ============================================================
    // Dap::Variable DebugVariable（与 CppDebugger 的 toDebugVariables 一致）
    // ============================================================
    static QList<SprintIDE::DebugVariable> toDebugVariables(
        const QList<SprintIDE::Dap::Variable>& vars)
    {
        QList<SprintIDE::DebugVariable> out;
        out.reserve(vars.size());
        for (const auto& v : vars) {
            SprintIDE::DebugVariable dv;
            dv.name = v.name;
            dv.type = v.type;
            dv.value = v.value;
            dv.isComplex = v.isContainer();
            dv.variablesReference = v.variablesReference;
            dv.evaluateName = v.name;
            out.append(dv);
        }
        return out;
    }

    // ============================================================
    // 构造 / 析构
    // ============================================================
    RustDebugger::RustDebugger(QObject* parent)
    : QObject(parent)
    {
    }

    RustDebugger::~RustDebugger()
    {
        stopDebugging();
    }

    // ============================================================
    // startDebugging
    // ============================================================
    bool RustDebugger::startDebugging(const RustDebugConfig& config)
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
            m_dap->disconnect();
            m_dap->deleteLater();
            m_dap = nullptr;
        }

        // 重置状态
        m_launched = false;
        m_initReceived = false;
        m_localsVariablesRef = 0;
        m_currentFrameId = 0;
        m_currentThreadId = 1;
        m_topLevelRefs.clear();

        m_dap = new DapClient(this);

        // ---- 连接信号（与 CppDebugger 完全同构）----
        connect(m_dap, &DapClient::adapterStarted,
            this, &RustDebugger::onAdapterStarted);
        connect(m_dap, &DapClient::initialized,
            this, &RustDebugger::onAdapterInitialized);
        connect(m_dap, &DapClient::stopped,
            this, &RustDebugger::onDapStopped);
        connect(m_dap, &DapClient::continued,
            this, &RustDebugger::onDapContinued);
        connect(m_dap, &DapClient::exited,
            this, &RustDebugger::onDapExited);
        connect(m_dap, &DapClient::terminated,
            this, &RustDebugger::onDapTerminated);
        connect(m_dap, &DapClient::output,
            this, &RustDebugger::onDapOutput);
        connect(m_dap, &DapClient::stackTraceReceived,
            this, &RustDebugger::onStackTrace);
        connect(m_dap, &DapClient::scopesReceived,
            this, &RustDebugger::onScopes);
        connect(m_dap, &DapClient::variablesReceived,
            this, &RustDebugger::onVariables);
        connect(m_dap, &DapClient::evaluateResult,
            this, &RustDebugger::onEvaluateResult);
        connect(m_dap, &DapClient::errorOccurred,
            this, &RustDebugger::onAdapterError);
        connect(m_dap, &DapClient::adapterExited,
            this, &RustDebugger::onAdapterExited);
        connect(m_dap, &DapClient::breakpointsValidated,
            this, [](const QString& file,
                const QList<SprintIDE::Dap::Breakpoint>& bps) {
                qDebug() << "[RustDebugger] 断点响应:" << file
                << "共" << bps.size() << "个";
                for (const auto& bp : bps) {
                    qDebug() << "  行" << bp.line
                    << "verified=" << bp.verified
                    << "message=" << bp.message;
                }
            });

        // ---- 启动 adapter ----
        const bool ok = m_dap->startAdapter(config.adapterPath,
            {},   // lldb-dap / codelldb 都走 stdio，无需额外参数
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
    // onAdapterStarted
    //   adapter 进程起来了，但还没发 initialize。
    //   这里发 initialize（adapterID 用 "lldb" —— codelldb 兼容 lldb adapterID）
    // ============================================================
    void RustDebugger::onAdapterStarted()
    {
        qDebug() << "[RustDebugger] adapter started, sending initialize";
        if (m_dap) {
            m_dap->initialize("lldb", "SprintIDE");
        }
    }

    // ============================================================
    // onAdapterInitialized
    //
    // 与 CppDebugger::onAdapterInitialized **完全同构**：
    //   第一次触发 = initialize 响应 发 launch
    //   第二次触发 = initialized 事件 setBreakpoints + configurationDone
    // ============================================================
    void RustDebugger::onAdapterInitialized()
    {
        if (!m_launched) {
            qDebug() << "[RustDebugger] initialize done, sending launch";
            m_launched = true;
            sendLaunch();
            return;
        }

        if (!m_initReceived) {
            qDebug() << "[RustDebugger] initialized event, applying breakpoints";
            m_initReceived = true;
            applyBreakpoints();
            m_dap->configurationDone();
        }
    }

    // ============================================================
    // sendLaunch —— 复用 CppDebugger 的 launch 拼法，字段名逐个对齐
    // ============================================================
    void RustDebugger::sendLaunch()
    {
        QJsonObject args;

        // env
        if (!m_config.environment.isEmpty()) {
            QJsonObject envObj;
            const auto keys = m_config.environment.keys();
            for (const QString& key : keys) {
                envObj[key] = m_config.environment.value(key);
            }
            args["env"] = envObj;
        }

        args["program"] = m_config.program;
        args["cwd"] = m_config.workingDirectory.isEmpty()
        ? QFileInfo(m_config.program).absolutePath()
        : m_config.workingDirectory;

        QJsonArray argArr;
        for (const QString& a : m_config.programArgs) argArr.append(a);
        args["args"] = argArr;

        // Rust 和 C++ 一样用 lldb adapter，字段与 CppDebugger 的 !isGdb 分支一致
        args["stopOnEntry"] = m_config.stopOnEntry;
        args["terminal"] = "console";
        args["console"] = "internalConsole";

        m_dap->launch(args);

        qDebug() << "[RustDebugger] launch sent:" << m_config.program
        << "adapter=" << m_config.adapterPath
        << "stopOnEntry=" << m_config.stopOnEntry;
    }

    // ============================================================
    // 断点同步（与 CppDebugger::applyBreakpoints 完全一致）
    // ============================================================
    void RustDebugger::setBreakpoints(const QList<SprintIDE::Breakpoint>& breakpoints)
    {
        m_breakpoints = breakpoints;
        if (m_initReceived) {
            applyBreakpoints();
        }
    }

    void RustDebugger::applyBreakpoints()
    {
        QMap<QString, QList<DapClient::BreakpointSpec>> byFile;

        for (const SprintIDE::Breakpoint& bp : m_breakpoints) {
            if (!bp.enabled) continue;
            const QString abs = QFileInfo(bp.filePath).absoluteFilePath();

            DapClient::BreakpointSpec spec;
            spec.line = bp.lineNumber;
            spec.condition = bp.condition;
            byFile[abs].append(spec);
        }

        for (auto it = byFile.begin(); it != byFile.end(); ++it) {
            qDebug() << "[RustDebugger] 准备设断点，文件 ="
            << it.key()
            << " 存在 =" << QFile::exists(it.key())
            << " count =" << it.value().size();
            m_dap->setBreakpoints(it.key(), it.value());
        }
    }

    // ============================================================
    // 执行控制（threadId 用 m_currentThreadId，与 CppDebugger 一致）
    // ============================================================
    void RustDebugger::continueExecution()
    {
        if (m_dap) m_dap->continueExecution(m_currentThreadId);
    }
    void RustDebugger::stepOver()
    {
        if (m_dap) m_dap->stepOver(m_currentThreadId);
    }
    void RustDebugger::stepInto()
    {
        if (m_dap) m_dap->stepIn(m_currentThreadId);
    }
    void RustDebugger::stepOut()
    {
        if (m_dap) m_dap->stepOut(m_currentThreadId);
    }
    void RustDebugger::pause()
    {
        if (m_dap) m_dap->pause(m_currentThreadId);
    }

    // ============================================================
    // 变量 / 求值
    //
    // DebugPanel::variablesExpandRequested(int) 只传 ref，无 parentRef。
    // parentRef 由 DapClient 内部追踪，variablesReceived 会带回来。
    // ============================================================
    void RustDebugger::requestChildVariables(int variablesReference)
    {
        if (m_dap && variablesReference > 0) {
            m_dap->variables(variablesReference);
        }
    }

    void RustDebugger::evaluate(const QString& expression)
    {
        if (!m_dap || expression.trimmed().isEmpty()) return;
        // 有当前帧就带上，否则让 adapter 在全局作用域求值
        m_dap->evaluate(expression, m_currentFrameId, "repl");
    }

    // ============================================================
    // DAP 事件
    // ============================================================
    void RustDebugger::onDapStopped(const Dap::StoppedInfo& info)
    {
        qDebug() << "[RustDebugger] stopped, reason=" << info.reason
        << "threadId=" << info.threadId;

        m_topLevelRefs.clear();
        m_currentThreadId = info.threadId > 0 ? info.threadId : 1;

        if (m_dap) m_dap->stackTrace(m_currentThreadId);
    }

    void RustDebugger::onStackTrace(const QList<Dap::StackFrame>& frames)
    {
        // 转成 DebugManager 用的 StackFrame（供 DebugPanel 显示调用栈）
        QList<SprintIDE::StackFrame> out;
        out.reserve(frames.size());
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

        m_currentFile = normalizeDapSourcePath(top.source.path,
            top.source.name,
            m_config.workingDirectory);
        m_currentLine = top.line;

        qDebug() << "[RustDebugger] 顶层帧: name=" << top.name
        << "rawPath=" << top.source.path
        << "name=" << top.source.name
        << "-> resolved=" << m_currentFile;

        // ---- dyld 伪停止过滤（与 CppDebugger 完全一致）----
        const QString funcLower = top.name.toLower();
        const QString fileLower = m_currentFile.toLower();

        const bool isStartupStop =
        funcLower == "_dyld_start" ||
        funcLower == "start" ||
        funcLower == "__start" ||
        funcLower == "_start" ||
        funcLower == "__libc_start_main" ||
        funcLower.contains("dyld") ||
        funcLower.contains("libdyld") ||
        funcLower.contains("libsystem") ||
        fileLower.startsWith("/usr/lib/dyld") ||
        fileLower.startsWith("/usr/lib/system") ||
        fileLower.contains("dyld") ||
        (!m_currentFile.isEmpty() && !QFile::exists(m_currentFile)) ||
        m_currentFile.isEmpty();

        if (isStartupStop) {
            qDebug() << "[RustDebugger] 检测到 startup 停止（" << top.name
            << "），自动 continue，等待真正的断点";
            if (m_dap) m_dap->continueExecution(m_currentThreadId);
            return;
        }

        // 正常停止 通知 MainWindow 高亮
        emit stopped(m_currentFile, m_currentLine, QStringLiteral("breakpoint"));

        // 拉顶层作用域（变量）
        if (m_dap) m_dap->scopes(top.id);
    }

    void RustDebugger::onScopes(int frameId, const QList<Dap::Scope>& scopes)
    {
        Q_UNUSED(frameId);

        // 优先 Locals；否则第一个非 expensive
        int localsRef = 0;
        for (const Dap::Scope& s : scopes) {
            if (s.name.compare("Locals", Qt::CaseInsensitive) == 0) {
                localsRef = s.variablesReference;
                break;
            }
        }
        if (localsRef == 0) {
            for (const Dap::Scope& s : scopes) {
                if (!s.expensive) { localsRef = s.variablesReference; break; }
            }
        }

        if (localsRef > 0) {
            m_localsVariablesRef = localsRef;
            m_topLevelRefs.insert(localsRef);
            if (m_dap) m_dap->variables(localsRef);
        } else {
            emit variablesUpdated({});
        }
    }

    void RustDebugger::onVariables(int parentRef, const QList<Dap::Variable>& vars)
    {
        QList<SprintIDE::DebugVariable> out = toDebugVariables(vars);

        if (m_topLevelRefs.contains(parentRef)) {
            m_topLevelRefs.remove(parentRef);
            emit variablesUpdated(out);                     // 顶层：替换整个列表
        } else {
            emit childVariablesUpdated(parentRef, out);     // 子级：只更新对应节点
        }
    }

    void RustDebugger::onEvaluateResult(const QString& expr,
        const QString& result,
        const QString& type)
    {
        emit evaluateResultReady(expr, result, type);
    }

    void RustDebugger::onDapContinued(int threadId)
    {
        Q_UNUSED(threadId);
        emit continued();
    }

    void RustDebugger::onDapExited(int exitCode)
    {
        m_isDebugging = false;
        emit exited(exitCode);
    }

    void RustDebugger::onDapTerminated()
    {
        m_isDebugging = false;
        emit terminated();
    }

    void RustDebugger::onDapOutput(const QString& category, const QString& text)
    {
        const bool isErr = (category == "stderr");
        emit outputReceived(text, isErr);
    }

    void RustDebugger::onAdapterError(const QString& error)
    {
        emit errorOccurred(error);
    }

    void RustDebugger::onAdapterExited(int exitCode)
    {
        Q_UNUSED(exitCode);
        if (m_isDebugging) {
            m_isDebugging = false;
            emit terminated();
        }
    }

    // ============================================================
    // stopDebugging（与 CppDebugger::stopDebugging 完全同构）
    // ============================================================
    void RustDebugger::stopDebugging()
    {
        if (!m_dap) {
            m_isDebugging = false;
            return;
        }

        m_dap->disconnect(this);
        m_dap->disconnect();
        m_dap->disconnect(true);
        QThread::msleep(100);

        m_dap->deleteLater();
        m_dap = nullptr;

        m_isDebugging = false;
        m_initReceived = false;
        m_launched = false;
        m_localsVariablesRef = 0;
        m_currentFrameId = 0;
        m_currentThreadId = 1;
        m_topLevelRefs.clear();
    }

} // namespace SprintIDE