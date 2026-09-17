// debug_panel.cpp
#include "debug_panel.h"
#include <QAction>
#include <QPushButton>
#include <QSplitter>
#include <QHeaderView>
#include <QFileInfo>
#include <QWidget>
#include <QLineEdit>
#include <QtWidgets/qscrollbar.h>

namespace SprintIDE {

DebugPanel::DebugPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    setVisible(false);  // 默认隐藏
    setObjectName("DebugPanel");
    setMinimumHeight(0);                      // 明确允许缩到 0
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);   // 忽略内容最小约束
}

DebugPanel::~DebugPanel() {}

void DebugPanel::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(2);

    // ========== 顶部控制工具栏 ==========
    m_controlToolBar = new QToolBar("调试控制", this);
    m_controlToolBar->setObjectName("DebugControlBar");
    m_controlToolBar->setMovable(false);
    m_controlToolBar->setIconSize(QSize(16, 16));

    // 继续
    m_continueAction = new QAction(QIcon(":/icons/icons/run.svg"), "继续", this);
    m_continueAction->setShortcut(QKeySequence("F5"));
    m_continueAction->setEnabled(false);
    connect(m_continueAction, &QAction::triggered, this, &DebugPanel::onContinueClicked);
    m_controlToolBar->addAction(m_continueAction);

    // 步过
    m_stepOverAction = new QAction(QIcon(":/icons/icons/stop_over.svg"), "步过", this);
    m_stepOverAction->setShortcut(QKeySequence("F10"));
    m_stepOverAction->setEnabled(false);
    connect(m_stepOverAction, &QAction::triggered, this, &DebugPanel::onStepOverClicked);
    m_controlToolBar->addAction(m_stepOverAction);

    // 步入
    m_stepIntoAction = new QAction(QIcon(":/icons/icons/step_into.svg"), "步入", this);
    m_stepIntoAction->setShortcut(QKeySequence("F11"));
    m_stepIntoAction->setEnabled(false);
    connect(m_stepIntoAction, &QAction::triggered, this, &DebugPanel::onStepIntoClicked);
    m_controlToolBar->addAction(m_stepIntoAction);

    // 步出
    m_stepOutAction = new QAction(QIcon(":/icons/icons/step_out.svg"), "步出", this);
    m_stepOutAction->setShortcut(QKeySequence("Shift+F11"));
    m_stepOutAction->setEnabled(false);
    connect(m_stepOutAction, &QAction::triggered, this, &DebugPanel::onStepOutClicked);
    m_controlToolBar->addAction(m_stepOutAction);

    m_controlToolBar->addSeparator();

    // 停止
    m_stopAction = new QAction(QIcon(":/icons/icons/stop.svg"), "停止", this);
    m_stopAction->setShortcut(QKeySequence("Shift+F5"));
    m_stopAction->setEnabled(false);
    connect(m_stopAction, &QAction::triggered, this, &DebugPanel::onStopClicked);
    m_controlToolBar->addAction(m_stopAction);

    m_controlToolBar->addSeparator();

    // 清空
    QAction* clearAction = new QAction(QIcon(":/icons/icons/clear.svg"), "清空", this);
    connect(clearAction, &QAction::triggered, this, &DebugPanel::onClearConsoleClicked);
    m_controlToolBar->addAction(clearAction);

    mainLayout->addWidget(m_controlToolBar);

    // ============================================================
    // 中间：调用栈 + 变量（横向 splitter）
    // ============================================================
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);

    // ---------- 调用栈 ----------
    QWidget* callStackContainer = new QWidget();
    callStackContainer->setObjectName("DebugPanelSection");
    QVBoxLayout* callStackLayout = new QVBoxLayout(callStackContainer);
    callStackLayout->setContentsMargins(2, 2, 2, 2);
    callStackLayout->setSpacing(2);

    QLabel* callStackLabel = new QLabel("调用栈");
    callStackLabel->setObjectName("DebugSectionLabel");
    callStackLayout->addWidget(callStackLabel);

    m_callStackWidget = new QTreeWidget();
    m_callStackWidget->setObjectName("CallStackTree");
    m_callStackWidget->setHeaderLabels(QStringList() << "函数" << "文件" << "行号");
    m_callStackWidget->setIndentation(10);
    m_callStackWidget->setAlternatingRowColors(false);
    m_callStackWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    callStackLayout->addWidget(m_callStackWidget);

    // ---------- 变量 ----------
    QWidget* variableContainer = new QWidget();
    variableContainer->setObjectName("DebugPanelSection");
    QVBoxLayout* variableLayout = new QVBoxLayout(variableContainer);
    variableLayout->setContentsMargins(2, 2, 2, 2);
    variableLayout->setSpacing(2);

    QLabel* variableLabel = new QLabel("局部变量");
    variableLabel->setObjectName("DebugSectionLabel");
    variableLayout->addWidget(variableLabel);

    m_variableWidget = new QTreeWidget();
    m_variableWidget->setObjectName("VariableTree");
    m_variableWidget->setHeaderLabels(QStringList() << "名称" << "值" << "类型");
    m_variableWidget->setIndentation(10);
    m_variableWidget->setAlternatingRowColors(false);
    m_variableWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    variableLayout->addWidget(m_variableWidget);

    // ---------- 表达式求值区 ----------
    QLabel* evalLabel = new QLabel("表达式求值");
    evalLabel->setObjectName("DebugSectionLabel");
    variableLayout->addWidget(evalLabel);

    m_evalOutput = new QTextEdit();
    m_evalOutput->setObjectName("DebugEvalOutput");
    m_evalOutput->setReadOnly(true);
    m_evalOutput->setMaximumHeight(80);
    m_evalOutput->setFrameShape(QFrame::NoFrame);
    variableLayout->addWidget(m_evalOutput);

    m_evalInput = new QLineEdit();
    m_evalInput->setObjectName("DebugEvalInput");
    m_evalInput->setPlaceholderText("输入表达式，回车求值…");
    m_evalInput->setFrame(false);
    connect(m_evalInput, &QLineEdit::returnPressed,
            this, &DebugPanel::onEvaluateInputEntered);
    variableLayout->addWidget(m_evalInput);

    connect(m_variableWidget, &QTreeWidget::itemExpanded,
            this, &DebugPanel::onVariableItemExpanded);

    splitter->addWidget(callStackContainer);
    splitter->addWidget(variableContainer);
    splitter->setSizes(QList<int>() << 300 << 300);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter, 1);   // stretch = 1，占满剩余空间

    // ============================================================
    // 底部调试控制台
    // ============================================================
    QWidget* consoleContainer = new QWidget();
    consoleContainer->setObjectName("DebugPanelSection");
    QVBoxLayout* consoleLayout = new QVBoxLayout(consoleContainer);
    consoleLayout->setContentsMargins(2, 2, 2, 2);
    consoleLayout->setSpacing(2);

    QLabel* consoleLabel = new QLabel("调试控制台");
    consoleLabel->setObjectName("DebugSectionLabel");
    consoleLayout->addWidget(consoleLabel);

    m_consoleWidget = new QTextEdit();
    m_consoleWidget->setObjectName("DebugConsole");
    m_consoleWidget->setReadOnly(true);
    m_consoleWidget->setMinimumHeight(40);
    consoleLayout->addWidget(m_consoleWidget);

    mainLayout->addWidget(consoleContainer);
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
    // 清空所有 ref 映射（新的变量列表 = 新的会话/栈帧）
    m_variableWidget->clear();
    m_refToItem.clear();
    m_itemToRef.clear();
    m_placeholderItems.clear();

    for (const DebugVariable& var : variables) {
        QStringList items;
        items << var.name << var.value << var.type;
        QTreeWidgetItem* item = new QTreeWidgetItem(m_variableWidget, items);

        if (var.variablesReference > 0) {
            m_refToItem[var.variablesReference] = item;
            m_itemToRef[item] = var.variablesReference;
            // 加占位子节点（让箭头出现，但实际未请求）
            QTreeWidgetItem* placeholder =
                new QTreeWidgetItem(item, QStringList() << "Loading..." << "" << "");
            m_placeholderItems.insert(placeholder);
        }
    }

    // 恢复列宽自适应
    m_variableWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
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

void DebugPanel::onVariableItemExpanded(QTreeWidgetItem* item)
{
    if (!item) return;
    if (!m_itemToRef.contains(item)) return;

    // 只有当前还是占位状态才请求，避免重复请求
    if (item->childCount() == 1 &&
        m_placeholderItems.contains(item->child(0))) {
        int ref = m_itemToRef[item];
        emit variablesExpandRequested(ref);
    }
}

void DebugPanel::onChildVariablesUpdated(int parentRef,
                                         const QList<DebugVariable>& children)
{
    QTreeWidgetItem* parent = m_refToItem.value(parentRef, nullptr);
    if (!parent) {
        qWarning() << "[DebugPanel] 收到未知 parentRef:" << parentRef;
        return;
    }

    // 删除占位子节点
    for (int i = parent->childCount() - 1; i >= 0; --i) {
        QTreeWidgetItem* child = parent->child(i);
        if (m_placeholderItems.contains(child)) {
            m_placeholderItems.remove(child);
            delete child;
        }
    }

    // 填充子节点
    for (const DebugVariable& var : children) {
        QStringList items;
        items << var.name << var.value << var.type;
        QTreeWidgetItem* item = new QTreeWidgetItem(parent, items);

        if (var.variablesReference > 0) {
            m_refToItem[var.variablesReference] = item;
            m_itemToRef[item] = var.variablesReference;
            QTreeWidgetItem* placeholder =
                new QTreeWidgetItem(item, QStringList() << "Loading..." << "" << "");
            m_placeholderItems.insert(placeholder);
        }
    }

    parent->setExpanded(true);
}

void DebugPanel::onEvaluateInputEntered()
{
    if (!m_evalInput) return;
    QString expr = m_evalInput->text().trimmed();
    if (expr.isEmpty()) return;
    emit evaluateRequested(expr);
    m_evalInput->clear();
}

void DebugPanel::onEvaluateResult(const QString& expression,
                                  const QString& result,
                                  const QString& type)
{
    if (!m_evalOutput) return;
    QString line = QString("<b>%1</b> = <span style='color:#88DDFF;'>%2</span>")
                       .arg(expression.toHtmlEscaped(),
                            result.toHtmlEscaped());
    if (!type.isEmpty()) {
        line += QString(" <i style='color:#888;'>(%1)</i>")
        .arg(type.toHtmlEscaped());
    }
    m_evalOutput->append(line);
}

} // namespace SprintIDE
