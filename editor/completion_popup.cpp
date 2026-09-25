#include "completion_popup.h"
#include <QMouseEvent>
#include <QScrollBar>

namespace SprintIDE {

CompletionPopup::CompletionPopup(QWidget* parent)
    : QListView(parent)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_ShowWithoutActivating);   // ← 关键：弹出不抢焦点
    setFocusPolicy(Qt::NoFocus);                  // ← 关键：键盘永远留在编辑器
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setUniformItemSizes(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setResizeMode(QListView::Adjust);

    // 视觉风格（暗色）
    setStyleSheet(R"(
        QListView {
            background-color: #252526;
            color: #E0E0E0;
            border: 1px solid #454545;
            outline: none;
            padding: 2px;
        }
        QListView::item {
            padding: 3px 8px;
        }
        QListView::item:selected {
            background-color: #094771;
            color: #FFFFFF;
        }
    )");
}

void CompletionPopup::showWithModel(CompletionModel* model, const QPoint& globalPos, int width)
{
    m_model = model;
    setModel(model);

    // 高度：最多 10 行
    const int rowHeight = sizeHintForRow(0) > 0 ? sizeHintForRow(0) : 22;
    const int maxRows = 10;
    const int rows = qMin(model->rowCount(), maxRows);
    const int height = rows > 0 ? rows * rowHeight + 4 : 22;

    resize(width, height);
    move(globalPos);
    show();
    raise();

    if (model->rowCount() > 0) {
        setCurrentIndex(model->index(0, 0));
    }
}

int CompletionPopup::currentIndex() const
{
    return QListView::currentIndex().row();
}

void CompletionPopup::selectNext()
{
    if (!m_model || m_model->rowCount() == 0) return;
    int row = currentIndex();
    row = qMin(row + 1, m_model->rowCount() - 1);
    setCurrentIndex(m_model->index(row, 0));
    scrollTo(m_model->index(row, 0));
}

void CompletionPopup::selectPrev()
{
    if (!m_model || m_model->rowCount() == 0) return;
    int row = currentIndex();
    row = qMax(row - 1, 0);
    setCurrentIndex(m_model->index(row, 0));
    scrollTo(m_model->index(row, 0));
}

void CompletionPopup::setCurrentIndexByRow(int row)
{
    if (!m_model || row < 0 || row >= m_model->rowCount()) return;
    setCurrentIndex(m_model->index(row, 0));
    scrollTo(m_model->index(row, 0));
}

void CompletionPopup::mousePressEvent(QMouseEvent* e)
{
    QListView::mousePressEvent(e);
    const QModelIndex idx = indexAt(e->pos());
    if (idx.isValid()) {
        emit itemCommitted(idx.row());
    }
}

void CompletionPopup::mouseDoubleClickEvent(QMouseEvent* e)
{
    QMouseEvent single(QEvent::MouseButtonPress, e->pos(), e->globalPos(),
                       e->button(), e->buttons(), e->modifiers());
    mousePressEvent(&single);
    e->accept();
}

} // namespace SprintIDE
