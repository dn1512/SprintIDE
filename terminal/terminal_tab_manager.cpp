// SPDX-License-Identifier: GPL-3.0-or-later
#include "terminal_tab_manager.h"

#include "terminal/terminal_panel.h"

#include <QDir>
#include <QHBoxLayout>
#include <QIcon>
#include <QToolButton>
#include <QtCore/qtimer.h>
#include <QTabBar>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QLineEdit>

namespace SprintIDE {

// ============================================================================
// 构造 / 析构
// ============================================================================
TerminalTabManager::TerminalTabManager(QWidget* parent)
    : QTabWidget(parent)
{
    setTabsClosable(true);
    setMovable(true);
    setDocumentMode(true);
    setTabBarAutoHide(false);

    // 右上角"新建"按钮
    // 右上角容器：停止按钮 + 新建按钮
    QWidget* cornerWidget = new QWidget(this);
    auto* cornerLayout = new QHBoxLayout(cornerWidget);
    cornerLayout->setContentsMargins(0, 0, 0, 0);
    cornerLayout->setSpacing(2);

    m_stopButton = new QToolButton(cornerWidget);
    m_stopButton->setText(QStringLiteral("■"));
    m_stopButton->setToolTip(tr("中断当前命令 (Ctrl+C)"));
    m_stopButton->setAutoRaise(true);
    m_stopButton->setFixedSize(22, 22);
    m_stopButton->setEnabled(false);
    connect(m_stopButton, &QToolButton::clicked,
            this, &TerminalTabManager::onStopButtonClicked);
    cornerLayout->addWidget(m_stopButton);

    m_newTabButton = new QToolButton(cornerWidget);
    m_newTabButton->setText(QStringLiteral("+"));
    m_newTabButton->setToolTip(tr("新建终端"));
    m_newTabButton->setAutoRaise(true);
    m_newTabButton->setFixedSize(22, 22);
    connect(m_newTabButton, &QToolButton::clicked,
            this, &TerminalTabManager::onNewTabRequested);
    cornerLayout->addWidget(m_newTabButton);

    setCornerWidget(cornerWidget, Qt::TopRightCorner);

    connect(this, &QTabWidget::tabCloseRequested,
            this, &TerminalTabManager::onTabCloseRequested);

    // 只连一次
    connect(this, &QTabWidget::currentChanged, this, [this](int index) {
        if (auto* p = qobject_cast<TerminalPanel*>(widget(index))) {
            p->focusPanel();
        }
        updateStopButtonState();   // 切换 Tab 时更新停止按钮状态
    });

    // Tab 右键菜单
    tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tabBar(), &QTabBar::customContextMenuRequested,
            this, &TerminalTabManager::onTabBarContextMenu);

    // 构造时先建一个终端，避免空态
    createTerminal();
}

TerminalTabManager::~TerminalTabManager() = default;

// ============================================================================
// 新建终端
// ============================================================================
TerminalPanel* TerminalTabManager::createTerminal(const QString& workDir)
{
    auto* panel = new TerminalPanel(this);

    if (!workDir.isEmpty()) {
        panel->setWorkingDirectory(workDir);
    }

    // 初始标题
    const QString initialTitle = tr("终端 %1").arg(m_nextTerminalNumber++);
    const int idx = addTab(panel, initialTitle);
    setCurrentIndex(idx);

    // 转发 openFileAtLine
    connect(panel, &TerminalPanel::openFileAtLine,
            this, &TerminalTabManager::openFileAtLine);

    // shell 通过 OSC 设置 title → 更新 tab 文本
    connect(panel, &TerminalPanel::shellTitleChanged,
            this, [this, panel](const QString& title) {
                onShellTitleChanged(panel, title);
            });

    // 监听 shell 状态变化，更新停止按钮
    connect(panel, &TerminalPanel::shellRunningChanged,
            this, [this](bool) { updateStopButtonState(); });

    panel->focusPanel();
    updateTabCountUI();
    updateStopButtonState();   // 新 Tab 创建后更新
    return panel;
}

// ============================================================================
// 当前终端
// ============================================================================
TerminalPanel* TerminalTabManager::currentTerminal() const
{
    return qobject_cast<TerminalPanel*>(currentWidget());
}

// ============================================================================
// 关闭
// ============================================================================
void TerminalTabManager::onTabCloseRequested(int index)
{
    QWidget* w = widget(index);
    if (!w) return;

    // 清理重命名标记
    m_userRenamedTabs.remove(w);

    removeTab(index);
    w->deleteLater();

    updateTabCountUI();
    updateStopButtonState();   // 关闭后更新停止按钮状态

    // 关闭最后一个 tab 后，用户可能还想继续用终端 → 自动新建一个
    if (count() == 0) {
        QTimer::singleShot(0, this, [this]() {
            if (count() == 0) createTerminal();
        });
    }
}

void TerminalTabManager::closeAll()
{
    // 从后往前删，避免 index 漂移
    while (count() > 0) {
        QWidget* w = widget(count() - 1);
        removeTab(count() - 1);
        if (w) w->deleteLater();
    }
}

// ============================================================================
// 新建按钮
// ============================================================================
void TerminalTabManager::onNewTabRequested()
{
    // 新终端用当前终端的工作目录（如果当前有）
    QString dir;
    if (auto* cur = currentTerminal()) {
        dir = cur->workingDirectory();
    }
    if (dir.isEmpty()) {
        dir = QDir::currentPath();
    }
    createTerminal(dir);
}

// ============================================================================
// shell title → tab 标题
// ============================================================================
void TerminalTabManager::onShellTitleChanged(TerminalPanel* panel, const QString& title)
{
    if (!panel || title.isEmpty()) return;
    const int idx = indexOf(panel);
    if (idx < 0) return;

    // 用户手动重命名过的 Tab 不再被 shell title 覆盖
    //   但仍更新 tooltip（鼠标悬停时显示真实 shell title）
    if (m_userRenamedTabs.contains(panel)) {
        setTabToolTip(idx, title);
        return;
    }

    QString display = title;
    if (display.length() > 32) {
        display = display.left(30) + QStringLiteral("…");
    }
    setTabText(idx, display);
    setTabToolTip(idx, title);
}

// ============================================================================
// 停止按钮：点击 → 中断当前 Tab 的 shell 命令
// ============================================================================
void TerminalTabManager::onStopButtonClicked()
{
    auto* term = currentTerminal();
    if (!term) return;
    if (!term->isCommandRunning()) return;
    term->stopCurrentCommand();
}

// ============================================================================
// 更新停止按钮 enable 状态
//   规则：当前 Tab 的 shell 在运行 → enable；否则 disable
// ============================================================================
void TerminalTabManager::updateStopButtonState()
{
    if (!m_stopButton) return;

    auto* term = currentTerminal();
    const bool canStop = term && term->isCommandRunning();
    m_stopButton->setEnabled(canStop);
}

// ============================================================================
//   Tab 右键菜单
// ============================================================================
void TerminalTabManager::onTabBarContextMenu(const QPoint& pos)
{
    const int clickedIndex = tabBar()->tabAt(pos);
    if (clickedIndex < 0) return;   // 点在空白处

    QWidget* clickedWidget = widget(clickedIndex);
    if (!clickedWidget) return;

    const int total = count();

    QMenu menu(this);

    QAction* actRename       = menu.addAction(tr("重命名..."));
    QAction* actNewTab       = menu.addAction(tr("新建终端"));
    menu.addSeparator();
    QAction* actClose        = menu.addAction(tr("关闭"));
    QAction* actCloseLeft    = menu.addAction(tr("关闭左侧 Tab"));
    QAction* actCloseRight   = menu.addAction(tr("关闭右侧 Tab"));
    QAction* actCloseOthers  = menu.addAction(tr("关闭其他 Tab"));
    menu.addSeparator();
    QAction* actCloseAll     = menu.addAction(tr("关闭所有 Tab"));

    // 边界禁用
    actCloseLeft  ->setEnabled(clickedIndex > 0);
    actCloseRight ->setEnabled(clickedIndex < total - 1);
    actCloseOthers->setEnabled(total > 1);

    QAction* chosen = menu.exec(tabBar()->mapToGlobal(pos));
    if (!chosen) return;

    // menu.exec() 期间 tab 列表可能变化，重新定位
    const int cur = indexOf(clickedWidget);
    if (cur < 0 && chosen != actNewTab) return;

    // ---------- 重命名 ----------
    if (chosen == actRename) {
        bool ok = false;
        const QString newName = QInputDialog::getText(
            this, tr("重命名终端"),
            tr("新名称："),
            QLineEdit::Normal,
            tabText(cur), &ok);
        if (!ok) return;

        const QString trimmed = newName.trimmed();
        if (trimmed.isEmpty()) return;   // 空输入不处理（避免无标题）

        setTabText(cur, trimmed);
        m_userRenamedTabs.insert(clickedWidget);   // 标记：之后不再被 shell title 覆盖
        return;
    }

    // ---------- 新建 ----------
    if (chosen == actNewTab) {
        onNewTabRequested();
        return;
    }

    // ---------- 关闭当前 ----------
    if (chosen == actClose) {
        onTabCloseRequested(cur);
        return;
    }

    // ---------- 关闭左侧 ----------
    //   从后往前删：cur-1, cur-2, ..., 0，避免索引漂移
    if (chosen == actCloseLeft) {
        for (int i = cur - 1; i >= 0; --i) {
            onTabCloseRequested(i);
        }
        return;
    }

    // ---------- 关闭右侧 ----------
    //   从后往前删：count-1, ..., cur+1
    if (chosen == actCloseRight) {
        for (int i = count() - 1; i > cur; --i) {
            onTabCloseRequested(i);
        }
        return;
    }

    // ---------- 关闭其他 ----------
    if (chosen == actCloseOthers) {
        // 先删右侧
        for (int i = count() - 1; i > cur; --i) {
            onTabCloseRequested(i);
        }
        // 再删左侧（从后往前）
        for (int i = cur - 1; i >= 0; --i) {
            onTabCloseRequested(i);
        }
        // 选中剩下的这个
        const int finalIdx = indexOf(clickedWidget);
        if (finalIdx >= 0) setCurrentIndex(finalIdx);
        return;
    }

    // ---------- 关闭所有 ----------
    if (chosen == actCloseAll) {
        for (int i = count() - 1; i >= 0; --i) {
            onTabCloseRequested(i);
        }
        return;
    }
}

// ============================================================================
// 更新 UI 状态（当前没有额外依赖，留占位）
// ============================================================================
void TerminalTabManager::updateTabCountUI()
{
    // 未来可加：状态栏显示 tab 数、禁用"关闭"等
}

} // namespace SprintIDE