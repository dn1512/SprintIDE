#include "log_item_delegate.h"
#include "log_list_model.h"
#include "theme_manager.h"
#include "core/log_buffer.h"
#include <QPainter>
#include <QApplication>

namespace SprintIDE {

LogItemDelegate::LogItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {}

QSize LogItemDelegate::sizeHint(const QStyleOptionViewItem& /*option*/,
                                const QModelIndex& /*index*/) const
{
    // 固定行高：给 QListView 开 uniformItemSizes，10 万行也不卡
    return QSize(0, 22);
}

void LogItemDelegate::paint(QPainter* painter,
                            const QStyleOptionViewItem& option,
                            const QModelIndex& index) const
{
    painter->save();

    ThemeManager& tm = ThemeManager::instance();

    // ---------- 1. 背景 ----------
    QColor bg = tm.color(ThemeManager::UIElement::EditorBackground);
    if (option.state & QStyle::State_Selected) {
        bg = tm.color(ThemeManager::UIElement::SelectionBackground);
    } else if (option.state & QStyle::State_MouseOver) {
        bg = tm.color(ThemeManager::UIElement::HoverBackground);
    }
    painter->fillRect(option.rect, bg);

    // ---------- 2. 左侧级别色条 ----------
    const int level = index.data(LogListModel::LevelRole).toInt();
    QColor levelColor = tm.color(ThemeManager::UIElement::TextColor);
    QString levelTag;
    switch (static_cast<LogLevel>(level)) {
    case LogLevel::Error:
        levelColor = tm.color(ThemeManager::UIElement::ErrorColor);
        levelTag   = QStringLiteral("E");
        break;
    case LogLevel::Warning:
        levelColor = tm.color(ThemeManager::UIElement::WarningColor);
        levelTag   = QStringLiteral("W");
        break;
    default:
        levelColor = tm.color(ThemeManager::UIElement::SecondaryTextColor);
        levelTag   = QStringLiteral("I");
        break;
    }
    QRect tagRect(option.rect.left(), option.rect.top(), 20, option.rect.height());
    painter->fillRect(tagRect, levelColor);
    painter->setPen(Qt::white);
    QFont bold = option.font;
    bold.setBold(true);
    painter->setFont(bold);
    painter->drawText(tagRect, Qt::AlignCenter, levelTag);

    // ---------- 3. 时间 + 来源 + 消息 ----------
    QFont normal = option.font;
    painter->setFont(normal);

    int x = tagRect.right() + 6;
    const int cy = option.rect.center().y() + normal.pointSize() / 2;

    // 时间
    const qint64 ms = index.data(LogListModel::TimeRole).toLongLong();
    if (ms > 0) {
        const QString t = QDateTime::fromMSecsSinceEpoch(ms).toString("hh:mm:ss");
        painter->setPen(tm.color(ThemeManager::UIElement::SecondaryTextColor));
        painter->drawText(x, cy, t);
        x += painter->fontMetrics().horizontalAdvance(t) + 8;
    }

    // 来源
    const QString src = index.data(LogListModel::SourceRole).toString();
    if (!src.isEmpty()) {
        const QString s = QString("[%1]").arg(src);
        painter->setPen(tm.color(ThemeManager::UIElement::SecondaryTextColor));
        painter->drawText(x, cy, s);
        x += painter->fontMetrics().horizontalAdvance(s) + 8;
    }

    // 消息（按级别上色）
    const QString msg = index.data(LogListModel::MessageRole).toString();
    painter->setPen(levelColor);
    QRect msgRect(x, option.rect.top(),
                  option.rect.right() - x - 4,
                  option.rect.height());
    const QString elided = painter->fontMetrics().elidedText(
        msg, Qt::ElideRight, msgRect.width());
    painter->drawText(msgRect, Qt::AlignVCenter | Qt::AlignLeft, elided);

    // ---------- 4. 选中/悬停边框 ----------
    if (option.state & QStyle::State_HasFocus) {
        painter->setPen(tm.color(ThemeManager::UIElement::PrimaryColor));
        painter->drawRect(option.rect.adjusted(0, 0, -1, -1));
    }

    painter->restore();
}

} // namespace SprintIDE
