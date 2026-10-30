// SPDX-License-Identifier: GPL-3.0-or-later
/*
* SprintIDE - SprintIDE is a cross-platform IDE built from scratch with Qt 6 and C++20. Its core capability is to provide AI Agents with an executable, verifiable code execution environment.
* Copyright (C) 2026  Liu Tengjun (刘腾骏)
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef DAP_CLIENT_H
#define DAP_CLIENT_H

#include <QObject>
#include <QProcess>
#include <QMap>
#include <QJsonObject>
#include <QProcessEnvironment>
#include "dap_types.h"

namespace SprintIDE {

/**
 * @brief 通用 DAP 客户端
 *
 * 与调试适配器（lldb-dap / codelldb / OpenDebugAD7 / debugpy）通过 stdio
 * 进行 JSON-RPC 通信，实现 DAP (Debug Adapter Protocol) 协议。
 *
 * 使用流程：
 *   1. startAdapter()        启动 adapter 进程
 *   2. initialize()          发送 initialize 请求
 *   3. （收到 initialized 事件后）setBreakpoints()
 *   4. launch()              发送 launch/attach 请求
 *   5. configurationDone()   通知 adapter 配置完成
 *   6. 监听 stopped / output / exited / terminated 信号
 *
 * 协议格式：与 LSP 一致——Content-Length: <n>\r\n\r\n<json>
 *
 * 设计参考：python_debugger.cpp（debugpy 走 TCP），本类统一用 stdio。
 * 未来 Python 也可迁移到本类，只需 adapter 用 stdio 模式启动。
 */
class DapClient : public QObject
{
    Q_OBJECT
public:
    enum class State {
        Disconnected,   ///< 未启动
        Starting,       ///< 启动 adapter 进程
        Initializing,   ///< 已发 initialize，等待响应
        Launching,      ///< 已发 launch，等待 initialized 事件
        Configuring,    ///< 已发 setBreakpoints，等待 configurationDone 完成
        Running,        ///< 程序运行中 / 暂停中
        Stopping,
    };
    Q_ENUM(State)

    explicit DapClient(QObject* parent = nullptr);
    ~DapClient() override;


    struct BreakpointSpec {
        int line = 0;
        QString condition;      // 空 = 无条件
        QString hitCondition;   // 可选
    };

    // ---------- 启动 ----------
    /// 启动 adapter 进程
    bool startAdapter(const QString& program,
                      const QStringList& args = {},
                      const QString& workingDir = QString(),
                      const QProcessEnvironment& env = QProcessEnvironment());

    // ---------- 低层请求（返回 seq）----------
    int sendRequest(const QString& command, const QJsonObject& args = QJsonObject());

    // ---------- 便捷封装 ----------
    void initialize(const QString& adapterID,
                    const QString& clientID = QStringLiteral("SprintIDE"));
    void launch(const QJsonObject& launchArgs);
    void attach(const QJsonObject& attachArgs);
    void setBreakpoints(const QString& sourcePath,
                        const QList<BreakpointSpec>& specs);
    void setBreakpoints(const QString& sourcePath,
                        const QList<int>& lines);   ///< 行号 1-based
    void configurationDone();
    void continueExecution(int threadId);
    void stepOver(int threadId);
    void stepIn(int threadId);
    void stepOut(int threadId);
    void pause(int threadId);
    void stackTrace(int threadId);
    void scopes(int frameId);
    void variables(int variablesReference);
    void evaluate(const QString& expr, int frameId = 0,
                  const QString& context = QStringLiteral("repl"));
    void disconnect(bool terminateDebuggee = true);

    // ---------- 状态 ----------
    State state() const { return m_state; }
    bool isConnected() const { return m_process && m_process->state() == QProcess::Running; }

signals:
    // 高层信号（供 CppDebugger 等上层封装订阅）
    void adapterStarted();
    void adapterExited(int exitCode);
    void initialized();
    void stopped(const SprintIDE::Dap::StoppedInfo& info);
    void continued(int threadId);
    void exited(int exitCode);
    void terminated();
    void output(const QString& category, const QString& text);
    void breakpointsValidated(const QString& sourcePath,
                              const QList<SprintIDE::Dap::Breakpoint>& breakpoints);
    void stackTraceReceived(const QList<SprintIDE::Dap::StackFrame>& frames);
    void scopesReceived(int frameId, const QList<SprintIDE::Dap::Scope>& scopes);
    void variablesReceived(int parentRef, const QList<SprintIDE::Dap::Variable>& variables);
    void evaluateResult(const QString& expression, const QString& result, const QString& type);
    void processStartedEvent(const QString& name, int systemProcessId);
    void threadEvent(const QString& reason, int threadId);
    void errorOccurred(const QString& error);

private slots:
    void onStdout();
    void onStderr();
    void onFinished(int exitCode, QProcess::ExitStatus status);
    void onErrorOccurred(QProcess::ProcessError err);

private:
    // 协议
    void sendRaw(const QJsonObject& msg);
    void appendToBuffer(const QByteArray& data);
    bool tryExtractOneMessage(QByteArray& outBody);
    void handleMessage(const QJsonObject& msg);
    void handleResponse(const QJsonObject& msg);
    void handleEvent(const QJsonObject& msg);
    void handleReverseRequest(const QJsonObject& msg);

    // 解析辅助
    static QList<Dap::StackFrame> parseStackFrames(const QJsonArray& arr);
    static QList<Dap::Variable>   parseVariables(const QJsonArray& arr);
    static QList<Dap::Scope>      parseScopes(const QJsonArray& arr);
    static QList<Dap::Breakpoint> parseBreakpoints(const QString& sourcePath,
                                                   const QJsonArray& arr);

    QProcess* m_process = nullptr;
    QByteArray m_buffer;
    int m_nextSeq = 1;
    QMap<int, QString> m_pendingCommands;   ///< seq command 名，用于响应匹配
    QMap<int, int> m_pendingVariablesRef;   //seq variablesReference
    State m_state = State::Disconnected;
    QString m_adapterID;
    QString m_clientID;
};

} // namespace SprintIDE

#endif // DAP_CLIENT_H
