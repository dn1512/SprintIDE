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