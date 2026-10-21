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
#ifndef OUTLINE_OVERLAY_H
#define OUTLINE_OVERLAY_H

#include <QDialog>
#include <QList>
#include "outline_types.h"

class QSplitter;
class QListWidget;
class QListWidgetItem;
class QLabel;

namespace SprintIDE {

    class OutlineItemDelegate;

    /**
    * @brief 大纲浮层：属性 / 方法双栏显示
    *
    * 显示时机：TextEditor 的 Ctrl/Cmd + ` 按住时调用 showAt()
    * 关闭时机：TextEditor 的释放事件调用 hideOverlay()
    *
    * 设计：
    *   - Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus
    *     —— 显示时不抢编辑器焦点，键盘事件仍由编辑器转发
    *   - 4:6 分栏，两列各自独立纵向滚动，无横向滚动
    *   - 文字超出自动折行，项间距 24px > 换行行距 ~15px
    *   - 颜色全部从 ThemeManager 取
    */
    class OutlineOverlay : public QDialog
    {
        Q_OBJECT
        public:
        explicit OutlineOverlay(QWidget* parent = nullptr);
        ~OutlineOverlay() override;

        /// 设置当前文件的符号树（每次显示前调用）
        void setSymbols(const QList<OutlineSymbol>& symbols);

        /// 显示在 globalPos（通常是光标位置）下方
        void showAt(const QPoint& globalPos);

        /// 关闭
        void hideOverlay();

        /// 鼠标是否在浮层上（用于松开快捷键时的延迟关闭判断）
        bool isMouseInside() const;

        // ---- 键盘导航（由 TextEditor 转发）----
        void selectNext();
        void selectPrev();
        void focusLeft();
        void focusRight();
        void activateCurrent();

        signals:
        /// 用户确认某一项（Enter 或 鼠标点击）→ 跳转到该行
        void symbolActivated(int line);

        private:
        void applyTheme();
        void rebuildLists();
        void appendSymbol(QListWidget* list,
            const OutlineSymbol& symbol,
            const QString& prefix);
        QListWidget* activeList() const { return m_activeList; }

        QList<OutlineSymbol> m_symbols;

        QLabel*      m_propTitle    = nullptr;
        QLabel*      m_methodTitle  = nullptr;
        QListWidget* m_propList     = nullptr;
        QListWidget* m_methodList   = nullptr;
        QSplitter*   m_splitter     = nullptr;
        QListWidget* m_activeList   = nullptr;

        OutlineItemDelegate* m_delegate = nullptr;
    };

} // namespace SprintIDE

#endif