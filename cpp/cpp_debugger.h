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
#ifndef CPP_DEBUGGER_H
#define CPP_DEBUGGER_H

#include <QObject>
#include <QMap>
#include <QProcessEnvironment>
#include <QSet>

#include "debug_manager.h"   // 复用 DebugVariable / StackFrame / Breakpoint
#include "dap/dap_types.h"

namespace SprintIDE {

class DapClient;

/**
 * @brief C++ 调试配置
 */
struct CppDebugConfig {
    QString adapterPath;                ///< codelldb / lldb-dap 可执行文件
    QString program;                    ///< 被调试的可执行文件
    QStringList programArgs;            ///< 传给 main 的参数
    QString workingDirectory;
    QProcessEnvironment environment;
    bool stopOnEntry = false;
};

/**
 * @brief C++ 调试器（基于 DAP）
 *
 * 使用 DapClient 与 lldb-dap / codelldb 通信。
 *
 * 信号签名与 PythonDebugger 完全一致，MainWindow 的调试槽可直接复用。
 */
class CppDebugger : public QObject
{
    Q_OBJECT
public:
    explicit CppDebugger(QObject* parent = nullptr);
    ~CppDebugger() override;

    bool startDebugging(const CppDebugConfig& config);
    void stopDebugging();
    bool isDebugging() const { return m_isDebugging; }

    // 断点：1-based 行号
    void setBreakpoints(const QList<Breakpoint>& breakpoints);

    // 执行控制
    void continueExecution();
    void stepOver();
    void stepInto();
    void stepOut();
    void pause();
    void evaluateExpression(const QString& expr);

    // UI 请求展开某个变量引用
    void requestChildVariables(int variablesReference);
    // UI 请求求值（无 frameId 时用当前帧）
    void evaluate(const QString& expression);

signals:
    // 与 PythonDebugger 一致，便于 MainWindow 复用连接
    void stopped(const QString& file, int line, const QString& reason);
    void continued();
    void exited(int exitCode);
    void terminated();
    void outputReceived(const QString& output, bool isError);
    void errorOccurred(const QString& error);
    void variablesUpdated(const QList<DebugVariable>& variables);
    void stackUpdated(const QList<StackFrame>& frames);

    void childVariablesUpdated(int parentRef, const QList<DebugVariable>& children);
    void evaluateResultReady(const QString& expression,
                             const QString& result,
                             const QString& type);

private slots:
    void onAdapterInitialized();
    void onDapStopped(const SprintIDE::Dap::StoppedInfo& info);
    void onDapContinued(int threadId);
    void onDapExited(int exitCode);
    void onDapTerminated();
    void onDapOutput(const QString& category, const QString& text);
    void onStackTrace(const QList<SprintIDE::Dap::StackFrame>& frames);
    void onScopes(int frameId, const QList<SprintIDE::Dap::Scope>& scopes);
    void onVariables(int parentRef, const QList<SprintIDE::Dap::Variable>& vars);
    void onAdapterError(const QString& error);
    void onAdapterExited(int exitCode);

private:
    void applyBreakpoints();
    void sendLaunch();
    void requestTopFrameData();

    DapClient* m_dap = nullptr;
    CppDebugConfig m_config;
    QList<Breakpoint> m_breakpoints;
    bool m_isDebugging = false;
    bool m_launched = false;    ///< 是否已发过 launch
    bool m_initReceived = false;///< 是否收到过 initialized 事件

    // 运行状态
    int m_currentThreadId = 1;
    int m_currentFrameId = 0;
    QString m_currentFile;
    int m_currentLine = 0;

    // 顶层作用域 → 变量引用
    int m_localsVariablesRef = 0;

    QSet<int> m_topLevelRefs;
};

} // namespace SprintIDE

#endif // CPP_DEBUGGER_H
