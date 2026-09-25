#ifndef COMPLETION_DELEGATE_H
#define COMPLETION_DELEGATE_H

#include <QStyledItemDelegate>

namespace SprintIDE {

class CompletionDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit CompletionDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;
};

} // namespace SprintIDE

#endif
