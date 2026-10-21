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
#ifndef LOG_LIST_VIEW_H
#define LOG_LIST_VIEW_H

#include <QListView>

namespace SprintIDE {

class LogListModel;

class LogListView : public QListView
{
    Q_OBJECT
public:
    explicit LogListView(QWidget* parent = nullptr);
    void setLogModel(LogListModel* m);
    /// 设置 AI 是否可用（OutputPanel 在 AI 状态变化时调用）
    void setAIAvailable(bool available);

signals:
    /// 双击某条日志且它有 file+line 时发出，MainWindow 接过去 openFile + gotoLine
    void jumpRequested(const QString& file, int line, int column);
    /// 用户右键选了"AI 解释此错误"，附带错误文本
    void aiExplainErrorRequested(const QString& errorText);

protected:
    void keyPressEvent(QKeyEvent* e) override;
    void contextMenuEvent(QContextMenuEvent* e) override;

private slots:
    void onDoubleClicked(const QModelIndex& idx);

private:
    void copySelectionToClipboard();
    void copyAllToClipboard();
    LogListModel* m_logModel = nullptr;
    bool m_aiAvailable = false;
};

} // namespace SprintIDE

#endif // LOG_LIST_VIEW_H
