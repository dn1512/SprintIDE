#ifndef PANEL_FOCUS_MANAGER_H
#define PANEL_FOCUS_MANAGER_H

#include <QObject>
#include <QKeySequence>
#include <QPointer>

namespace SprintIDE {

class MainWindow;

/**
 * @brief 面板焦点与显隐管理器
 *
 * 提供键盘驱动的面板导航：
 *   - F6 / Shift+F6     → 循环切换焦点
 *   - Ctrl+1..4         → 直跳面板（macOS = Cmd+1..4）
 *   - Ctrl+Shift+A      → 显示/隐藏 AI 面板
 *
 * 设计：纯新增，不改动任何现有面板行为。
 */
class PanelFocusManager : public QObject
{
    Q_OBJECT

public:
    enum class Panel {
        Editor = 0,
        ProjectPanel,
        BottomPanel,
        AIChatPanel,
    };

    explicit PanelFocusManager(MainWindow* window);
    ~PanelFocusManager() override;

    /// 循环切换：direction > 0 正向；< 0 反向
    void cycleFocus(int direction);

    /// 直接聚焦某个面板
    void focusPanel(Panel panel);

    /// 显示/隐藏面板（不转移焦点）
    void togglePanelVisibility(Panel panel);

    /// 安装快捷键（在 MainWindow 所有面板 setup 完成后调用一次）
    void installShortcuts();

private:
    /// 从 current 开始，沿 direction 找下一个"可见"面板
    Panel nextVisiblePanel(Panel current, int direction) const;

    /// 判断某面板当前是否可见
    bool isPanelVisible(Panel panel) const;

    /// 当前焦点在哪个面板
    Panel currentFocusedPanel() const;

    void focusEditor();
    void focusProjectPanel();
    void focusBottomPanel();
    void focusAIChatPanel();

    MainWindow* m_window = nullptr;
};

} // namespace SprintIDE
#endif // PANEL_FOCUS_MANAGER_H