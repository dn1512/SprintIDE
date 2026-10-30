#include "panel_focus_manager.h"
#include "mainwindow.h"

#include "project_panel.h"
#include "terminal/terminal_panel.h"
#include "log/output_panel.h"
#include "ai/chat_panel.h"
#include "debug_panel.h"
#include "problems/problems_panel.h"
#include "breakpoint_list_panel.h"


#include <QShortcut>
#include <QApplication>
#include <QMainWindow>
#include <QDockWidget>
#include <QTabWidget>
#include <QTreeView>
#include <QDebug>
#include <functional>

namespace SprintIDE {

// ============================================================================
//  构造 / 析构
// ============================================================================
PanelFocusManager::PanelFocusManager(MainWindow* window)
    : QObject(window)
    , m_window(window)
{
}

PanelFocusManager::~PanelFocusManager() = default;

// ============================================================================
//  安装快捷键
//
//  context 用 Qt::ApplicationShortcut，保证任何焦点下都能触发
//  快捷键命名：
//    "Ctrl+1" 在 macOS 上自动映射为 "Cmd+1"
//    "Ctrl+Shift+A" 在 macOS 上自动映射为 "Cmd+Shift+A"
// ============================================================================
void PanelFocusManager::installShortcuts()
{
    if (!m_window) return;

    auto install = [this](const QKeySequence& seq,
                           std::function<void()> handler) {
        auto* sc = new QShortcut(seq, m_window);
        sc->setContext(Qt::ApplicationShortcut);
        connect(sc, &QShortcut::activated, this, std::move(handler));
    };

    // ---------- F6 / Shift+F6：循环切换面板 ----------
    install(QKeySequence(Qt::Key_F6),
            [this]() { cycleFocus(+1); });

    install(QKeySequence(Qt::SHIFT | Qt::Key_F6),
            [this]() { cycleFocus(-1); });

    // ---------- Ctrl+1..4：直跳面板 ----------
    install(QKeySequence("Ctrl+1"),
            [this]() { focusPanel(Panel::Editor); });
    install(QKeySequence("Ctrl+2"),
            [this]() { focusPanel(Panel::ProjectPanel); });
    install(QKeySequence("Ctrl+3"),
            [this]() { focusPanel(Panel::BottomPanel); });
    install(QKeySequence("Ctrl+4"),
            [this]() { focusPanel(Panel::AIChatPanel); });
    // ---------- 项目面板显隐 ----------
    //  原快捷键 Ctrl+Shift+E 被 macOS 输入法切换占用，
    //  菜单里显示的还是 Ctrl+Shift+E。
    install(QKeySequence("Ctrl+Alt+E"),
            [this]() { togglePanelVisibility(Panel::ProjectPanel); });

    // ---------- Ctrl+Shift+A：显示/隐藏 AI 面板 ----------
    //    注：Ctrl+Shift+E（项目面板）和 Ctrl+J（底部面板）已有 QAction 绑定，
    //        这里不重复安装，避免和用户已熟悉的快捷键冲突
    install(QKeySequence("Ctrl+Shift+A"),
            [this]() { togglePanelVisibility(Panel::AIChatPanel); });

        // ============================================================
    //  底部面板内标签切换（输出 / 终端 / 调试）
    //
    //  用【类型】而不是【索引】匹配——顺序变化不会出错
    // ============================================================
    // ============================================================
    //  v8.5：第二个参数 forceShowHidden
    //    - false（默认）：跳过隐藏 tab（调试面板未调试时隐藏，按快捷键不强制弹出）
    //    - true：找到匹配 tab 就显示（断点 tab 默认隐藏，用户按 Ctrl+Alt+5 想主动看）
    // ============================================================
    auto switchBottomTabByType = [this](const QString& type,
        bool forceShowHidden = false) {
        auto* tabs = m_window->findChild<QTabWidget*>("BottomTabWidget");
        if (!tabs) return;

        // 若底部面板隐藏先显示
        auto* dock = m_window->findChild<QDockWidget*>("BottomPanel");
        if (dock && !dock->isVisible()) {
            dock->setVisible(true);
        }

        for (int i = 0; i < tabs->count(); ++i) {
            QWidget* w = tabs->widget(i);
            if (!w) continue;

            bool match = false;
            if (type == QLatin1String("output")
                && qobject_cast<OutputPanel*>(w)) {
                match = true;
            } else if (type == QLatin1String("terminal")
                && qobject_cast<TerminalPanel*>(w)) {
                match = true;
            } else if (type == QLatin1String("debug")
                && qobject_cast<DebugPanel*>(w)) {
                match = true;
            } else if (type == QLatin1String("problems")
                && qobject_cast<ProblemsPanel*>(w)) {
                match = true;
            } else if (type == QLatin1String("breakpoints")
                && qobject_cast<BreakpointListPanel*>(w)) {
                match = true;
            }

            if (match) {
                if (!tabs->isTabVisible(i)) {
                    if (!forceShowHidden) continue;
                    tabs->setTabVisible(i, true);
                }
                tabs->setCurrentIndex(i);
                focusBottomPanel();
                return;
            }
        }
        // 找不到就静默忽略
    };


    install(QKeySequence("Ctrl+Alt+1"),
        [switchBottomTabByType]() { switchBottomTabByType("output"); });
    install(QKeySequence("Ctrl+Alt+2"),
        [switchBottomTabByType]() { switchBottomTabByType("problems"); });
    install(QKeySequence("Ctrl+Alt+3"),
        [switchBottomTabByType]() { switchBottomTabByType("terminal"); });
    install(QKeySequence("Ctrl+Alt+4"),
        [switchBottomTabByType]() { switchBottomTabByType("debug"); });
    install(QKeySequence("Ctrl+Alt+5"),
        [switchBottomTabByType]() { switchBottomTabByType("breakpoints", true); });

    // ============================================================
    //  编辑器标签左右切换
    //
    //  作用域：WindowShortcut —— 焦点在主窗口内任意位置都响应
    //  触发条件：当前焦点在编辑器（由 currentFocusedPanel 判定）
    //            否则直接忽略，避免在项目面板按 -> 结果标签变了
    // ============================================================
    auto switchEditorTab = [this](int direction) {
        // 只在焦点位于编辑器时才处理
        if (currentFocusedPanel() != Panel::Editor) {
            return;
        }
        if (direction > 0) {
            m_window->activateNextEditorTab();
        } else {
            m_window->activatePreviousEditorTab();
        }
    };

    install(QKeySequence("Ctrl+Alt+Right"),
        [switchEditorTab]() { switchEditorTab(+1); });

    install(QKeySequence("Ctrl+Alt+Left"),
        [switchEditorTab]() { switchEditorTab(-1); });

    qDebug() << "[PanelFocusManager] 快捷键已安装";
}

// ============================================================================
//  判断当前焦点在哪个面板
// ============================================================================
PanelFocusManager::Panel PanelFocusManager::currentFocusedPanel() const
{
    QWidget* focus = QApplication::focusWidget();
    if (!focus) return Panel::Editor;

    // 项目面板
    if (auto* dock = m_window->findChild<QDockWidget*>("ProjectDock")) {
        if (dock->isAncestorOf(focus)) return Panel::ProjectPanel;
    }

    // 底部面板
    if (auto* dock = m_window->findChild<QDockWidget*>("BottomPanel")) {
        if (dock->isAncestorOf(focus)) return Panel::BottomPanel;
    }

    // AI 面板
    if (auto* chat = m_window->findChild<ChatPanel*>()) {
        if (chat->isAncestorOf(focus)) return Panel::AIChatPanel;
    }

    // 其余情况视为编辑器
    return Panel::Editor;
}

// ============================================================================
//  循环切换
// ============================================================================
void PanelFocusManager::cycleFocus(int direction)
{
    if (direction == 0) return;
    const Panel current = currentFocusedPanel();
    const Panel next    = nextVisiblePanel(current, direction);
    focusPanel(next);
}

PanelFocusManager::Panel
PanelFocusManager::nextVisiblePanel(Panel current, int direction) const
{
    // 固定顺序：Editor -> ProjectPanel -> BottomPanel -> AIChatPanel
    static const Panel kOrder[] = {
        Panel::Editor,
        Panel::ProjectPanel,
        Panel::BottomPanel,
        Panel::AIChatPanel,
    };
    constexpr int kCount = 4;

    int idx = 0;
    for (int i = 0; i < kCount; ++i) {
        if (kOrder[i] == current) { idx = i; break; }
    }

    for (int step = 1; step <= kCount; ++step) {
        const int ni = ((idx + direction * step) % kCount + kCount) % kCount;
        const Panel p = kOrder[ni];
        if (p == Panel::Editor) return p;   // 编辑器永远可用
        if (isPanelVisible(p)) return p;
    }
    return Panel::Editor;   // 兜底
}

// ============================================================================
//  可见性判断
// ============================================================================
bool PanelFocusManager::isPanelVisible(Panel panel) const
{
    switch (panel) {
    case Panel::Editor:
        return true;

    case Panel::ProjectPanel:
        if (auto* d = m_window->findChild<QDockWidget*>("ProjectDock"))
            return d->isVisible();
        return false;

    case Panel::BottomPanel:
        if (auto* d = m_window->findChild<QDockWidget*>("BottomPanel"))
            return d->isVisible();
        return false;

    case Panel::AIChatPanel:
        if (auto* c = m_window->findChild<ChatPanel*>())
            return c->isVisible();
        return false;
    }
    return false;
}

// ============================================================================
//  聚焦
// ============================================================================
void PanelFocusManager::focusPanel(Panel panel)
{
    switch (panel) {
    case Panel::Editor:        focusEditor();       break;
    case Panel::ProjectPanel:  focusProjectPanel(); break;
    case Panel::BottomPanel:   focusBottomPanel();  break;
    case Panel::AIChatPanel:   focusAIChatPanel();  break;
    }
}

void PanelFocusManager::focusEditor()
{
    if (auto* ed = m_window->currentEditor()) {
        ed->setFocus();
    }
}

void PanelFocusManager::focusProjectPanel()
{
    auto* dock = m_window->findChild<QDockWidget*>("ProjectDock");
    if (!dock) return;
    if (!dock->isVisible()) dock->setVisible(true);

    if (auto* panel = m_window->findChild<ProjectPanel*>()) {
        panel->focusPanel();
    }
}

void PanelFocusManager::focusBottomPanel()
{
    auto* dock = m_window->findChild<QDockWidget*>("BottomPanel");
    if (!dock) return;
    if (!dock->isVisible()) dock->setVisible(true);

    auto* tabs = m_window->findChild<QTabWidget*>("BottomTabWidget");
    if (!tabs) return;

    QWidget* current = tabs->currentWidget();
    if (!current) return;

    // 每个面板自己决定焦点给谁
    if (auto* p = qobject_cast<OutputPanel*>(current)) {
        p->focusPanel();
    } else if (auto* p = qobject_cast<TerminalPanel*>(current)) {
        p->focusPanel();
    } else if (auto* p = qobject_cast<DebugPanel*>(current)) {
        p->setFocus();
    } else {
        current->setFocus();
    }
}

void PanelFocusManager::focusAIChatPanel()
{
    if (auto* chat = m_window->findChild<ChatPanel*>()) {
        if (!chat->isVisible()) chat->setVisible(true);
        chat->focusPanel();
    }
}

// ============================================================================
//  显示 / 隐藏
// ============================================================================
void PanelFocusManager::togglePanelVisibility(Panel panel)
{
    switch (panel) {
    case Panel::ProjectPanel:
        if (auto* d = m_window->findChild<QDockWidget*>("ProjectDock"))
            d->setVisible(!d->isVisible());
        break;

    case Panel::BottomPanel:
        if (auto* d = m_window->findChild<QDockWidget*>("BottomPanel"))
            d->setVisible(!d->isVisible());
        break;

    case Panel::AIChatPanel:
        if (auto* c = m_window->findChild<ChatPanel*>())
            c->setVisible(!c->isVisible());
        break;

    case Panel::Editor:
        // 编辑器不可隐藏
        break;
    }
}

} // namespace SprintIDE