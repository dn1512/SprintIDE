// debug_panel.cpp
#include "debug_panel.h"
#include <QAction>
#include <QPushButton>
#include <QSplitter>
#include <QHeaderView>
#include <QFileInfo>
#include <QWidget>
#include <QtWidgets/qscrollbar.h>

namespace SprintIDE {

DebugPanel::DebugPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    setVisible(false);  // 默认隐藏
    setObjectName("DebugPanel");
}

DebugPanel::~DebugPanel() {}

void DebugPanel::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(2);

    // ========== 顶部控制工具栏 ==========
    m_controlToolBar = new QToolBar("调试控制", this);
    m_controlToolBar->setMovable(false);
    m_controlToolBar->setIconSize(QSize(16, 16));
    m_controlToolBar->setStyleSheet(
        "QToolBar { background-color: #3C3C3C; border: none; padding: 4px; }"
        );

    // 继续按钮
    m_continueAction = new QAction(QIcon(":/icons/icons/run.svg"), "继续", this);
    m_continueAction->setShortcut(QKeySequence("F5"));
    m_continueAction->setEnabled(false);
    connect(m_continueAction, &QAction::triggered, this, &DebugPanel::onContinueClicked);
    m_controlToolBar->addAction(m_continueAction);

    m_controlToolBar->addSeparator();

    // 步过按钮
    m_stepOverAction = new QAction(QIcon(":/icons/icons/stop_over.svg"), "步过", this);
    m_stepOverAction->setShortcut(QKeySequence("F10"));
    m_stepOverAction->setEnabled(false);
    connect(m_stepOverAction, &QAction::triggered, this, &DebugPanel::onStepOverClicked);
    m_controlToolBar->addAction(m_stepOverAction);

    // 步入按钮
    m_stepIntoAction = new QAction(QIcon(":/icons/icons/step_into.svg"), "步入", this);
    m_stepIntoAction->setShortcut(QKeySequence("F11"));
    m_stepIntoAction->setEnabled(false);
    connect(m_stepIntoAction, &QAction::triggered, this, &DebugPanel::onStepIntoClicked);
    m_controlToolBar->addAction(m_stepIntoAction);

    // 步出按钮
    m_stepOutAction = new QAction(QIcon(":/icons/icons/step_out.svg"), "步出", this);
    m_stepOutAction->setShortcut(QKeySequence("Shift+F11"));
    m_stepOutAction->setEnabled(false);
    connect(m_stepOutAction, &QAction::triggered, this, &DebugPanel::onStepOutClicked);
    m_controlToolBar->addAction(m_stepOutAction);

    m_controlToolBar->addSeparator();

    // 停止按钮
    m_stopAction = new QAction(QIcon(":/icons/icons/stop.svg"), "停止", this);
    m_stopAction->setShortcut(QKeySequence("Shift+F5"));
    m_stopAction->setEnabled(false);
    connect(m_stopAction, &QAction::triggered, this, &DebugPanel::onStopClicked);
    m_controlToolBar->addAction(m_stopAction);

    m_controlToolBar->addSeparator();

    // 清空控制台按钮
    QAction* clearAction = new QAction(QIcon(":/icons/icons/clear.svg"), "清空", this);
    connect(clearAction, &QAction::triggered, this, &DebugPanel::onClearConsoleClicked);
    m_controlToolBar->addAction(clearAction);

    mainLayout->addWidget(m_controlToolBar);

    // ========== 中间分割区域：调用栈 + 变量 ==========
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);

    // 调用栈
    QWidget* callStackContainer = new QWidget();
    QVBoxLayout* callStackLayout = new QVBoxLayout(callStackContainer);
    callStackLayout->setContentsMargins(2, 2, 2, 2);
    callStackLayout->setSpacing(2);

    QLabel* callStackLabel = new QLabel("调用栈");
    callStackLabel->setStyleSheet("font-weight: bold; color: #E0E0E0;");
    callStackLayout->addWidget(callStackLabel);

    m_callStackWidget = new QTreeWidget();
    m_callStackWidget->setHeaderLabels(QStringList() << "函数" << "文件" << "行号");
    m_callStackWidget->setIndentation(10);
    m_callStackWidget->setAlternatingRowColors(true);
    m_callStackWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_callStackWidget->setStyleSheet(
        "QTreeWidget { background-color: #1E1E1E; color: #E0E0E0; gridline-color: #3C3C3C; }"
        "QTreeWidget::item:selected { background-color: #3A3D41; }"
        );
    callStackLayout->addWidget(m_callStackWidget);

    // 变量监视
    QWidget* variableContainer = new QWidget();
    QVBoxLayout* variableLayout = new QVBoxLayout(variableContainer);
    variableLayout->setContentsMargins(2, 2, 2, 2);
    variableLayout->setSpacing(2);

    QLabel* variableLabel = new QLabel("局部变量");
    variableLabel->setStyleSheet("font-weight: bold; color: #E0E0E0;");
    variableLayout->addWidget(variableLabel);

    m_variableWidget = new QTreeWidget();
    m_variableWidget->setHeaderLabels(QStringList() << "名称" << "值" << "类型");
    m_variableWidget->setIndentation(10);
    m_variableWidget->setAlternatingRowColors(true);
    m_variableWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_variableWidget->setStyleSheet(
        "QTreeWidget { background-color: #1E1E1E; color: #E0E0E0; gridline-color: #3C3C3C; }"
        "QTreeWidget::item:selected { background-color: #3A3D41; }"
        );
    variableLayout->addWidget(m_variableWidget);

    splitter->addWidget(callStackContainer);
    splitter->addWidget(variableContainer);
    splitter->setSizes(QList<int>() << 300 << 300);

    mainLayout->addWidget(splitter, 1);

    // ========== 底部调试控制台 ==========
    QWidget* consoleContainer = new QWidget();
    QVBoxLayout* consoleLayout = new QVBoxLayout(consoleContainer);
    consoleLayout->setContentsMargins(2, 2, 2, 2);
    consoleLayout->setSpacing(2);

    QLabel* consoleLabel = new QLabel("调试控制台");
    consoleLabel->setStyleSheet("font-weight: bold; color: #E0E0E0;");
    consoleLayout->addWidget(consoleLabel);

    m_consoleWidget = new QTextEdit();
    m_consoleWidget->setReadOnly(true);
    m_consoleWidget->setFont(QFont("Menlo", 10));
    m_consoleWidget->setStyleSheet(
        "QTextEdit { background-color: #1E1E1E; color: #CCCCCC; border: 1px solid #3C3C3C; }"
        );
    m_consoleWidget->setMinimumHeight(80);
    consoleLayout->addWidget(m_consoleWidget);

    mainLayout->addWidget(consoleContainer);

    // 设置整体样式
    setStyleSheet(
        "QWidget { background-color: #2D2D2D; color: #E0E0E0; }"
        );
}

void DebugPanel::setDebuggingState(bool isDebugging)
{
    m_isDebugging = isDebugging;
    setVisible(isDebugging);

    // 更新按钮状态
    m_continueAction->setEnabled(isDebugging);
    m_stepOverAction->setEnabled(isDebugging);
    m_stepIntoAction->setEnabled(isDebugging);
    m_stepOutAction->setEnabled(isDebugging);
    m_stopAction->setEnabled(isDebugging);
}

void DebugPanel::updateCallStack(const QList<StackFrame>& stack)
{
    m_callStackWidget->clear();

    for (const StackFrame& frame : stack) {
        QStringList items;
        items << frame.functionName
              << QFileInfo(frame.filePath).fileName()
              << QString::number(frame.lineNumber);
        QTreeWidgetItem* item = new QTreeWidgetItem(m_callStackWidget, items);
        item->setToolTip(0, frame.filePath);  // 完整路径作为工具提示
    }
}

void DebugPanel::updateVariables(const QList<DebugVariable>& variables)
{
    m_variableWidget->clear();

    for (const DebugVariable& var : variables) {
        QStringList items;
        items << var.name << var.value << var.type;
        QTreeWidgetItem* item = new QTreeWidgetItem(m_variableWidget, items);
        // 如果是复杂类型，可以添加子项
        if (var.isComplex) {
            for (const DebugVariable& child : var.children) {
                QStringList childItems;
                childItems << "  " + child.name << child.value << child.type;
                QTreeWidgetItem* childItem = new QTreeWidgetItem(item, childItems);
            }
            item->setExpanded(true);
        }
    }
}

void DebugPanel::appendConsoleOutput(const QString& output, bool isError)
{
    QString color = isError ? "#FF6B6B" : "#CCCCCC";
    m_consoleWidget->append(
        QString("<span style='color:%1;'>%2</span>")
            .arg(color)
            .arg(output.toHtmlEscaped())
        );
    // 自动滚动到底部
    QScrollBar* scrollBar = m_consoleWidget->verticalScrollBar();
    if (scrollBar) {
        scrollBar->setValue(scrollBar->maximum());
    }
}

void DebugPanel::clearConsole()
{
    m_consoleWidget->clear();
}

void DebugPanel::connectToDebugManager(DebugManager* manager)
{
    if (!manager) return;

    connect(manager, &DebugManager::callStackUpdated,
            this, &DebugPanel::updateCallStack);
    connect(manager, &DebugManager::variablesUpdated,
            this, &DebugPanel::updateVariables);
    connect(manager, &DebugManager::programOutput,
            this, &DebugPanel::appendConsoleOutput);
    connect(manager, &DebugManager::debuggingStarted,
            this, [this]() { setDebuggingState(true); });
    connect(manager, &DebugManager::debuggingStopped,
            this, [this]() { setDebuggingState(false); });
}

// ========== 按钮槽函数 ==========

void DebugPanel::onContinueClicked()
{
    emit continueRequested();
}

void DebugPanel::onStepOverClicked()
{
    emit stepOverRequested();
}

void DebugPanel::onStepIntoClicked()
{
    emit stepIntoRequested();
}

void DebugPanel::onStepOutClicked()
{
    emit stepOutRequested();
}

void DebugPanel::onStopClicked()
{
    emit stopRequested();
}

void DebugPanel::onClearConsoleClicked()
{
    clearConsole();
}

} // namespace SprintIDE
