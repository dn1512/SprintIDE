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
#ifndef PROBLEMS_PANEL_H
#define PROBLEMS_PANEL_H

#include <QWidget>
#include "problems/diagnostics_store.h"

class QTreeWidget;
class QTreeWidgetItem;
class QLineEdit;
class QToolButton;
class QLabel;

namespace SprintIDE {

    /**
    * @brief Problems 面板：以文件为分组显示诊断
    *
    * 特性：
    *  - 树形：文件 → 诊断列表
    *  - 三档过滤：错误 / 警告 / 信息
    *  - 文本搜索框
    *  - 双击 → jumpToFileRequested(filePath, line, column)
    *  - 右键 → quickFixRequested(filePath, line, column)
    *  - 颜色全部走 ThemeManager
    */
    class ProblemsPanel : public QWidget
    {
        Q_OBJECT
        public:
        explicit ProblemsPanel(DiagnosticsStore* store, QWidget* parent = nullptr);
        ~ProblemsPanel() override = default;

        /// 供 MainWindow 在主题切换时调用
        void applyTheme();

        signals:
        /// 双击诊断 / 点击跳转
        void jumpToFileRequested(const QString& filePath, int line, int column);

        /// 右键 → 请求 Quick Fix
        void quickFixRequested(const QString& filePath, int line, int column);

        private slots:
        void rebuild();
        void onFilterErrorToggled(bool on);
        void onFilterWarningToggled(bool on);
        void onFilterInfoToggled(bool on);
        void onSearchTextChanged(const QString& text);
        void onItemDoubleClicked(QTreeWidgetItem* item, int column);
        void onContextMenu(const QPoint& pos);

        private:
        bool passesFilter(const Diagnostic& d) const;
        bool passesSearch(const Diagnostic& d) const;
        QTreeWidgetItem* createFileItem(const QString& filePath, int count);
        QTreeWidgetItem* createDiagItem(const Diagnostic& d);
        void updateSummary();

        DiagnosticsStore* m_store = nullptr;

        QToolButton* m_errorBtn   = nullptr;
        QToolButton* m_warningBtn = nullptr;
        QToolButton* m_infoBtn    = nullptr;
        QLineEdit*   m_searchEdit = nullptr;
        QTreeWidget* m_tree       = nullptr;
        QLabel*      m_summary    = nullptr;

        bool m_showError   = true;
        bool m_showWarning = true;
        bool m_showInfo    = false;   // 默认隐藏 info/hint
    };

} // namespace SprintIDE

#endif