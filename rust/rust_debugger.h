// src/rust/rust_debugger.h
// SPDX-License-Identifier: GPL-3.0-or-later
/*
* SprintIDE - SprintIDE is a cross-platform IDE built from scratch with Qt 6 and C++20.
* Copyright (C) 2026  Liu Tengjun (刘腾骏)
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#ifndef RUST_DEBUGGER_H
#define RUST_DEBUGGER_H

#include <QObject>
#include <QProcessEnvironment>
#include <QSet>

#include "debug_manager.h"     // 复用 DebugVariable / StackFrame / Breakpoint
#include "dap/dap_types.h"

namespace SprintIDE {

    class DapClient;

    /**
    * @brief Rust 调试配置（语义等价于 CppDebugConfig）
    *
    * 单独定义而不复用 CppDebugConfig 的原因：
    *   - 未来 Rust 可能加 rust-gdb / miri 等 adapter 专有字段
    *   - 让 RustDebugger 与 CppDebugger 解耦，避免互相影响
    */
    struct RustDebugConfig {
        QString adapterPath;               ///< lldb-dap / codelldb 可执行文件
        QString program;                   ///< target/debug/<name> 或 target/release/<name>
        QStringList programArgs;           ///< 传给 main 的参数（暂不接 UI）
        QString workingDirectory;
        QProcessEnvironment environment;
        bool stopOnEntry = false;
    };

    /**
    * @brief Rust 调试器（基于 DAP）
    *
    * 设计要点：
    *   - 内部组合 DapClient，与 CppDebugger **完全平级**、互不依赖
    *   - 信号签名与 PythonDebugger / CppDebugger 完全一致，MainWindow 的
    *     调试槽（onDebuggerContinued / updateDebugActions / DebugPanel 的
    *     variablesExpandRequested 等）可以直接复用
    *   - dyld 伪停止过滤逻辑与 CppDebugger 一致（macOS 上 Rust 二进制
    *     同样会经历 dyld 启动阶段）
    *
    * 使用流程（与 CppDebugger 一致）：
    *   startDebugging(cfg)  →  adapter 启动 → initialize 响应 → launch
    *                        →  initialized 事件 → setBreakpoints + configurationDone
    *                        →  stopped 事件 → 通知 MainWindow 高亮
    */
    class RustDebugger : public QObject
    {
        Q_OBJECT
        public:
        explicit RustDebugger(QObject* parent = nullptr);
        ~RustDebugger() override;

        // ---------- 生命周期 ----------
        bool startDebugging(const RustDebugConfig& config);
        void stopDebugging();
        bool isDebugging() const { return m_isDebugging; }

        // ---------- 断点 ----------
        /// 与 CppDebugger::setBreakpoints 语义一致：1-based 行号，按文件分组下发
        void setBreakpoints(const QList<Breakpoint>& breakpoints);

        // ---------- 执行控制 ----------
        void continueExecution();
        void stepOver();
        void stepInto();
        void stepOut();
        void pause();

        // ---------- 变量 / 求值 ----------
        /// UI 请求展开某个变量引用（DebugPanel::variablesExpandRequested 只传 ref）
        void requestChildVariables(int variablesReference);
        /// UI 请求求值（DebugPanel::evaluateRequested 传 expression）
        void evaluate(const QString& expression);

        signals:
        // 与 PythonDebugger / CppDebugger 完全一致，便于 MainWindow 复用
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
        void onAdapterStarted();
        void onAdapterInitialized();
        void onDapStopped(const SprintIDE::Dap::StoppedInfo& info);
        void onDapContinued(int threadId);
        void onDapExited(int exitCode);
        void onDapTerminated();
        void onDapOutput(const QString& category, const QString& text);
        void onStackTrace(const QList<SprintIDE::Dap::StackFrame>& frames);
        void onScopes(int frameId, const QList<SprintIDE::Dap::Scope>& scopes);
        void onVariables(int parentRef, const QList<SprintIDE::Dap::Variable>& vars);
        void onEvaluateResult(const QString& expr, const QString& result, const QString& type);
        void onAdapterError(const QString& error);
        void onAdapterExited(int exitCode);

        private:
        void applyBreakpoints();
        void sendLaunch();

        DapClient* m_dap = nullptr;
        RustDebugConfig m_config;

        QList<Breakpoint> m_breakpoints;
        bool m_isDebugging = false;

        // 初始化状态机（与 CppDebugger 同构）
        bool m_launched = false;        ///< 是否已发过 launch
        bool m_initReceived = false;    ///< 是否收到过 initialized 事件

        // 运行状态
        int m_currentThreadId = 1;
        int m_currentFrameId = 0;
        QString m_currentFile;
        int m_currentLine = 0;

        // 顶层作用域 → 变量引用
        int m_localsVariablesRef = 0;
        QSet<int> m_topLevelRefs;       ///< 用于区分顶层 vs 子级 variables 响应
    };

} // namespace SprintIDE

#endif // RUST_DEBUGGER_H