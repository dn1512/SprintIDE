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
#ifndef MAINWINDOW_P_H
#define MAINWINDOW_P_H

#include <QObject>
#include <QPointer>
#include <QTreeView>
#include <QDockWidget>
#include <QLabel>
#include <QTabWidget>
#include <QStandardItemModel>

// 新增包含 ProjectPanel 头文件
#include "project_panel.h"

class TabWidget;
class TextEditor;

namespace SprintIDE {

class MainWindow;

class MainWindowPrivate {
public:
    MainWindowPrivate(MainWindow *parent)
        : q_ptr(parent)
        , editorTabs(nullptr)
        , projectPanel(nullptr)
        , projectDock(nullptr)
        , statusLabel(nullptr)
        , cursorPositionLabel(nullptr)
        , editModeLabel(nullptr)
        , hasUnsavedChanges(false)
    {
    }

    ~MainWindowPrivate()
    {
        // 注意：Qt对象有父对象，会自动删除，这里不需要手动删除
    }

    ProjectPanel* projectPanel = nullptr;
    // UI组件
    TabWidget *editorTabs;
    QDockWidget *projectDock;


    // 状态栏组件
    QLabel *statusLabel;
    QLabel *cursorPositionLabel;
    QLabel *editModeLabel;

    // 当前状态
    QString currentProjectPath;  ///< 当前项目路径
    QString currentProjectName;  ///< 当前项目名称
    QString currentFilePath;
    bool hasUnsavedChanges;

private:

    MainWindow *q_ptr; // 指向主窗口的指针
};

} // namespace SprintIDE

#endif // MAINWINDOW_P_H
