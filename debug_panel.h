// debug_panel.h
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

signals:
    void continueRequested();
    void stepOverRequested();
    void stepIntoRequested();
    void stepOutRequested();
    void stopRequested();

private slots:
    void onContinueClicked();
    void onStepOverClicked();
    void onStepIntoClicked();
    void onStepOutClicked();
    void onStopClicked();
    void onClearConsoleClicked();

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
};

} // namespace SprintIDE

#endif // DEBUG_PANEL_H
