#ifndef LOG_ITEM_DELEGATE_H
#define LOG_ITEM_DELEGATE_H

#include <QStyledItemDelegate>

namespace SprintIDE {
class LogItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit LogItemDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;
};
}
#endif
