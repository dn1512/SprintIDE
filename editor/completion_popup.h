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
