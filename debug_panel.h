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
#ifndef DEBUG_PANEL_H
#define DEBUG_PANEL_H

#include <QDockWidget>
#include <QToolBar>
#include <QTreeWidget>
#include <QTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

#include "debug_manager.h"

namespace SprintIDE {

/**
 * @class DebugPanel
 * @brief 调试面板 - 包含调试控制按钮、调用栈、变量监视和调试控制台
 */
class DebugPanel : public QWidget
{
    Q_OBJECT

public:
    explicit DebugPanel(QWidget* parent = nullptr);
    ~DebugPanel();

    // 更新调试状态
    void setDebuggingState(bool isDebugging);
    void updateCallStack(const QList<StackFrame>& stack);
    void updateVariables(const QList<DebugVariable>& variables);
    void appendConsoleOutput(const QString& output, bool isError = false);
    void clearConsole();

    // 连接调试管理器信号
    void connectToDebugManager(DebugManager* manager);

public slots:
    void onChildVariablesUpdated(int parentRef,
                                 const QList<DebugVariable>& children);
    void onEvaluateResult(const QString& expression,
                          const QString& result,
                          const QString& type);
signals:
    void continueRequested();
    void stepOverRequested();
    void stepIntoRequested();
    void stepOutRequested();
    void stopRequested();

    void variablesExpandRequested(int variablesReference);
    void evaluateRequested(const QString& expression);

private slots:
    void onContinueClicked();
    void onStepOverClicked();
    void onStepIntoClicked();
    void onStepOutClicked();
    void onStopClicked();
    void onClearConsoleClicked();

    void onVariableItemExpanded(QTreeWidgetItem* item);
    void onEvaluateInputEntered();

private:
    void setupUI();

    // 调试控制工具栏
    QToolBar* m_controlToolBar = nullptr;
    QAction* m_continueAction = nullptr;
    QAction* m_stepOverAction = nullptr;
    QAction* m_stepIntoAction = nullptr;
    QAction* m_stepOutAction = nullptr;
    QAction* m_stopAction = nullptr;

    // 调用栈
    QTreeWidget* m_callStackWidget = nullptr;

    // 变量监视
    QTreeWidget* m_variableWidget = nullptr;

    // 调试控制台
    QTextEdit* m_consoleWidget = nullptr;

    bool m_isDebugging = false;

    QMap<int, QTreeWidgetItem*> m_refToItem;
    QMap<QTreeWidgetItem*, int> m_itemToRef;
    QSet<QTreeWidgetItem*> m_placeholderItems;

    QLineEdit* m_evalInput = nullptr;                          // 表达式输入框
    QTextEdit* m_evalOutput = nullptr;                         // 表达式结果输出
};

} // namespace SprintIDE

#endif // DEBUG_PANEL_H
