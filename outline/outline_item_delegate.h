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
#ifndef OUTLINE_ITEM_DELEGATE_H
#define OUTLINE_ITEM_DELEGATE_H

#include <QStyledItemDelegate>

class QListView;

namespace SprintIDE {

    /**
    * @brief 大纲项 delegate
    *
    * 布局：
    *   [图标] [类型] 名字
    *
    * 换行与间距：
    *   - 宽度不足时 QTextLayout 自动折行
    *   - 项间距 24px（上下各 12px），换行行距 ~15px，比例 1:1.6，
    *     用户能清晰分辨"折行"和"下一项"
    */
    class OutlineItemDelegate : public QStyledItemDelegate
    {
        Q_OBJECT
        public:
        explicit OutlineItemDelegate(QListView* view, QObject* parent = nullptr);

        void paint(QPainter* painter, const QStyleOptionViewItem& option,
            const QModelIndex& index) const override;
        QSize sizeHint(const QStyleOptionViewItem& option,
            const QModelIndex& index) const override;

        private:
        int availableTextWidth() const;
        QListView* m_view = nullptr;
    };

} // namespace SprintIDE

#endif