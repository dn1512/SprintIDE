#include "outline_item_delegate.h"
#include "outline_types.h"
#include "theme_manager.h"

#include <QPainter>
#include <QTextLayout>
#include <QTextLine>
#include <QListView>
#include <QIcon>

namespace SprintIDE {

    namespace {

        QIcon iconForKind(OutlineSymbol::Kind kind)
        {
            switch (kind) {
                case OutlineSymbol::Kind::Class:
                case OutlineSymbol::Kind::Namespace:
                return QIcon(QStringLiteral(":/icons/icons/file_project.svg"));
                case OutlineSymbol::Kind::Function:
                case OutlineSymbol::Kind::Method:
                return QIcon(QStringLiteral(":/icons/icons/file_default.svg"));
                case OutlineSymbol::Kind::Variable:
                case OutlineSymbol::Kind::Property:
                return QIcon(QStringLiteral(":/icons/icons/file_text.svg"));
                case OutlineSymbol::Kind::Define:
                return QIcon(QStringLiteral(":/icons/icons/text.svg"));
            }
            return QIcon();
        }

    } // anonymous namespace

    OutlineItemDelegate::OutlineItemDelegate(QListView* view, QObject* parent)
    : QStyledItemDelegate(parent)
    , m_view(view)
    {
    }

    int OutlineItemDelegate::availableTextWidth() const
    {
        // 宽度 = viewport - 左右内边距(8+8) - 图标(14) - 图标与文字间距(6)
        int vpWidth = m_view ? m_view->viewport()->width() : 400;
        return qMax(60, vpWidth - 8 - 8 - 14 - 6);
    }

    QSize OutlineItemDelegate::sizeHint(const QStyleOptionViewItem& option,
        const QModelIndex& index) const
    {
        const QString name = index.data(Qt::DisplayRole).toString();
        const QString type = index.data(Qt::UserRole + 1).toString();

        QFont nameFont = option.font;
        QFont typeFont = option.font;
        typeFont.setPointSizeF(typeFont.pointSizeF() * 0.9);

        const QString typePart = type.isEmpty()
        ? QString()
        : QStringLiteral("[%1] ").arg(type);
        const QString fullText = typePart + name;

        QTextLayout layout(fullText, nameFont);

        // 给类型段一个较小的字体，便于换行计算更准
        if (!typePart.isEmpty()) {
            QList<QTextLayout::FormatRange> formats;
            QTextLayout::FormatRange r;
            r.start = 0;
            r.length = typePart.length();
            r.format.setFont(typeFont);
            formats.append(r);
            layout.setFormats(formats);
        }

        layout.beginLayout();
        qreal textHeight = 0;
        const int availWidth = availableTextWidth();
        while (true) {
            QTextLine line = layout.createLine();
            if (!line.isValid()) break;
            line.setLineWidth(availWidth);
            line.setPosition(QPointF(0, textHeight));
            textHeight += line.height();
        }
        layout.endLayout();

        constexpr int kPadTop    = 12;
        constexpr int kPadBottom = 12;
        constexpr int kIconSpace = 14 + 6;   // 图标宽 + 间距

        const int h = qMax(option.fontMetrics.height(),
            static_cast<int>(textHeight))
        + kPadTop + kPadBottom;

        return QSize(availWidth + kIconSpace + 16, h);
    }

    void OutlineItemDelegate::paint(QPainter* painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) const
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, false);

        ThemeManager& tm = ThemeManager::instance();

        // ---- 背景 ----
        QColor bg = tm.color(ThemeManager::UIElement::OutlineOverlayBackground);
        if (option.state & QStyle::State_Selected) {
            bg = tm.color(ThemeManager::UIElement::TreeSelectedBackground);
        } else if (option.state & QStyle::State_MouseOver) {
            bg = tm.color(ThemeManager::UIElement::TreeHoverBackground);
        }
        painter->fillRect(option.rect, bg);

        // ---- 图标 ----
        constexpr int kIconSize = 14;
        constexpr int kPadLeft  = 8;
        const int iconX = option.rect.left() + kPadLeft;
        const int iconY = option.rect.top() + 12;   // 与文字首行对齐

        const int kindInt = index.data(Qt::UserRole + 2).toInt();
        const OutlineSymbol::Kind kind = static_cast<OutlineSymbol::Kind>(kindInt);
        const QIcon icon = iconForKind(kind);
        if (!icon.isNull()) {
            icon.paint(painter, QRect(iconX, iconY, kIconSize, kIconSize));
        }

        // ---- 文本 ----
        const QString name = index.data(Qt::DisplayRole).toString();
        const QString type = index.data(Qt::UserRole + 1).toString();

        QFont nameFont = option.font;
        QFont typeFont = option.font;
        typeFont.setPointSizeF(typeFont.pointSizeF() * 0.9);

        const QString typePart = type.isEmpty()
        ? QString()
        : QStringLiteral("[%1] ").arg(type);
        const QString fullText = typePart + name;

        QTextLayout layout(fullText, nameFont);

        QList<QTextLayout::FormatRange> formats;
        if (!typePart.isEmpty()) {
            QTextLayout::FormatRange r;
            r.start = 0;
            r.length = typePart.length();
            r.format.setFont(typeFont);
            r.format.setForeground(
                tm.color(ThemeManager::UIElement::SecondaryTextColor));
            formats.append(r);
        }
        QTextLayout::FormatRange nameRange;
        nameRange.start = typePart.length();
        nameRange.length = name.length();
        nameRange.format.setForeground(
            tm.color(ThemeManager::UIElement::TextColor));
        formats.append(nameRange);
        layout.setFormats(formats);

        layout.beginLayout();
        qreal y = 0;
        const int availWidth = availableTextWidth();
        while (true) {
            QTextLine line = layout.createLine();
            if (!line.isValid()) break;
            line.setLineWidth(availWidth);
            line.setPosition(QPointF(0, y));
            y += line.height();
        }
        layout.endLayout();

        const int textX = iconX + kIconSize + 6;
        const int textY = option.rect.top() + 12;

        painter->setPen(tm.color(ThemeManager::UIElement::TextColor));
        layout.draw(painter, QPointF(textX, textY));

        painter->restore();
    }

} // namespace SprintIDE