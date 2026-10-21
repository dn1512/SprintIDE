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
#ifndef BREAKPOINT_LIST_PANEL_H
#define BREAKPOINT_LIST_PANEL_H

#include <QWidget>
#include <QListWidget>

class QListWidget;
class QLabel;

namespace SprintIDE {

class DebugManager;

/**
 * @brief 断点列表页签内容（放在底部 Tab 区域，由窄条按钮唤出）。
 *
 * 数据源：DebugManager::getBreakpoints()
 * 用户操作以信号发出，由 MainWindow 统一改 DebugManager 后回调 refresh()。
 */
class BreakpointListPanel : public QWidget
{
    Q_OBJECT
public:
    explicit BreakpointListPanel(DebugManager* manager, QWidget* parent = nullptr);

    /// 从 DebugManager 重新拉取并渲染
    void refresh();

signals:
    void breakpointActivated(const QString& filePath, int line);
    void breakpointRemoveRequested(const QString& filePath, int line);
    void breakpointToggleEnabledRequested(const QString& filePath, int line, bool enabled);
    void breakpointEditConditionRequested(const QString& filePath, int line);

private slots:
    void onItemDoubleClicked(QListWidgetItem* item);
    void onContextMenuRequested(const QPoint& pos);

private:
    DebugManager* m_debugManager = nullptr;
    QListWidget*  m_list         = nullptr;
    QLabel*       m_summaryLabel = nullptr;
};

} // namespace SprintIDE

#endif // BREAKPOINT_LIST_PANEL_H
