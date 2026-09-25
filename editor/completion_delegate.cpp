#include "completion_delegate.h"
#include "completion_model.h"
#include <QPainter>

namespace SprintIDE {

CompletionDelegate::CompletionDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

QSize CompletionDelegate::sizeHint(const QStyleOptionViewItem& option,
                                    const QModelIndex& index) const
{
    QSize base = QStyledItemDelegate::sizeHint(option, index);
    base.setHeight(qMax(base.height(), 22));
    return base;
}

void CompletionDelegate::paint(QPainter* painter,
                               const QStyleOptionViewItem& option,
                               const QModelIndex& index) const
{
    painter->save();

    // ---- 背景 ----
    QColor bg;
    if (option.state & QStyle::State_Selected) {
        bg = QColor("#094771");   // 选中色
    } else if (option.state & QStyle::State_MouseOver) {
        bg = QColor("#2A2D2E");
    } else {
        bg = option.palette.color(QPalette::Base);
    }
    painter->fillRect(option.rect, bg);

    // ---- 文字（同上）----
    const bool selected = option.state & QStyle::State_Selected;
    const QColor labelColor  = selected ? QColor("#FFFFFF") : option.palette.color(QPalette::Text);
    const QColor detailColor = selected ? QColor("#C8D6E5") : QColor("#808080");

    const QString label  = index.data(Qt::DisplayRole).toString();
    const QString detail = index.data(CompletionModel::DetailRole).toString();

    QRect r = option.rect.adjusted(8, 0, -8, 0);

    QFont f = painter->font();
    painter->setFont(f);
    painter->setPen(labelColor);

    const QFontMetrics fmLabel(f);
    const int labelWidth = qMin(r.width() - 60, fmLabel.horizontalAdvance(label) + 4);
    painter->drawText(QRect(r.left(), r.top(), labelWidth, r.height()),
                      Qt::AlignLeft | Qt::AlignVCenter, label);

    if (!detail.isEmpty()) {
        QFont df = f;
        df.setPointSize(qMax(8, f.pointSize() - 1));
        painter->setFont(df);
        painter->setPen(detailColor);
        painter->drawText(QRect(r.left() + labelWidth, r.top(),
                                r.width() - labelWidth, r.height()),
                          Qt::AlignRight | Qt::AlignVCenter, detail);
    }

    painter->restore();
}

} // namespace SprintIDE
