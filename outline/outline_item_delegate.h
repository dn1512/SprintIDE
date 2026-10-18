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