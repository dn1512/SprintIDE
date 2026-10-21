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
#ifndef COMPLETION_POPUP_H
#define COMPLETION_POPUP_H

#include <QListView>
#include "completion_model.h"

namespace SprintIDE {

/**
 * @brief 无侵入式补全弹窗
 *
 * 关键点：不使用 Qt::Popup（它会 grab 键盘/鼠标导致模态感）。
 * 使用 Qt::Tool | Qt::FramelessWindowHint，焦点始终留在编辑器。
 */
class CompletionPopup : public QListView
{
    Q_OBJECT
public:
    explicit CompletionPopup(QWidget* parent = nullptr);

    void showWithModel(CompletionModel* model, const QPoint& globalPos, int width);
    void hideIfVisible() { if (isVisible()) hide(); }

    int currentIndex() const;
    void selectNext();
    void selectPrev();
    void setCurrentIndexByRow(int row);

signals:
    void itemCommitted(int row);

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;

private:
    CompletionModel* m_model = nullptr;
};

} // namespace SprintIDE

#endif
