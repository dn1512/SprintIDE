// SPDX-License-Identifier: GPL-3.0-or-later
#include "terminal_output_overlay.h"

#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QPixmap>
#include <QToolButton>
#include <QVBoxLayout>

#include "theme_manager.h"

namespace SprintIDE {

static QIcon makeDotIcon(const QColor& color)
{
    QPixmap pix(12, 12);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(color);
    p.setPen(Qt::NoPen);
    p.drawEllipse(2, 2, 8, 8);
    return QIcon(pix);
}

static QColor kindColor(ParsedOutputItem::Kind k)
{
    auto& tm = ThemeManager::instance();
    using E = ThemeManager::UIElement;
    switch (k) {
    case ParsedOutputItem::Kind::Modified: return tm.color(E::WarningColor);
    case ParsedOutputItem::Kind::Added:    return tm.color(E::SuccessColor);
    case ParsedOutputItem::Kind::Deleted:  return tm.color(E::ErrorColor);
    case ParsedOutputItem::Kind::Warning:  return tm.color(E::WarningColor);
    case ParsedOutputItem::Kind::Normal:
    default:                               return tm.color(E::SecondaryTextColor);
    }
}

TerminalOutputOverlay::TerminalOutputOverlay(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("TerminalOutputOverlay"));
    setAutoFillBackground(true);
    setFocusPolicy(Qt::StrongFocus);

    setupUI();
    applyTheme();

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &TerminalOutputOverlay::applyTheme);

    hide();

    if (parent) {
        parent->installEventFilter(this);
    }
}

TerminalOutputOverlay::~TerminalOutputOverlay() = default;

void TerminalOutputOverlay::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(6);

    // ---- 标题栏 ----
    auto* topRow = new QHBoxLayout();
    topRow->setContentsMargins(0, 0, 0, 0);
    topRow->setSpacing(6);

    m_titleLabel = new QLabel(tr("输出分析"), this);
    {
        QFont f = m_titleLabel->font();
        f.setBold(true);
        m_titleLabel->setFont(f);
    }
    topRow->addWidget(m_titleLabel);

    m_subtitleLabel = new QLabel(this);
    topRow->addWidget(m_subtitleLabel);

    topRow->addStretch();

    m_closeButton = new QToolButton(this);
    m_closeButton->setText(QStringLiteral("×"));
    m_closeButton->setFixedSize(20, 20);
    m_closeButton->setAutoRaise(true);
    m_closeButton->setToolTip(tr("关闭 (Esc)"));
    connect(m_closeButton, &QToolButton::clicked,
            this, &TerminalOutputOverlay::dismiss);
    topRow->addWidget(m_closeButton);

    layout->addLayout(topRow);

    // ---- ★ C3：搜索框 ----
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("过滤..."));
    m_searchEdit->setClearButtonEnabled(true);
    layout->addWidget(m_searchEdit);

    // ---- 列表 ----
    m_list = new QListWidget(this);
    m_list->setAlternatingRowColors(true);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_list->setMaximumHeight(300);
    m_list->setIconSize(QSize(12, 12));
    connect(m_list, &QListWidget::itemActivated,
            this, &TerminalOutputOverlay::onItemActivated);
    connect(m_list, &QListWidget::itemClicked,
            this, &TerminalOutputOverlay::onItemActivated);
    layout->addWidget(m_list, 1);

    // ★ C3：搜索变化 → 重渲染
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        renderItems(text);
    });

    resize(460, 320);
}

void TerminalOutputOverlay::showResult(const ParsedOutputResult& result)
{
    if (!result.ok || result.items.isEmpty()) {
        dismiss();
        return;
    }

    m_titleLabel->setText(result.title.isEmpty() ? tr("输出分析") : result.title);
    m_subtitleLabel->setText(result.subtitle);

    m_allItems = result.items;

    // 搜索框清空 + 重建列表
    if (m_searchEdit) m_searchEdit->clear();
    renderItems(QString());

    reposition();
    show();
    raise();
    setFocus();

    // 焦点优先给搜索框（>= 10 项时；少量项直接给 list）
    if (m_allItems.size() > 10 && m_searchEdit) {
        m_searchEdit->setFocus();
    } else if (m_list) {
        m_list->setFocus();
    }
}

// ★ C3：按 filter 重建列表
void TerminalOutputOverlay::renderItems(const QString& filter)
{
    if (!m_list) return;

    const QString f = filter.trimmed().toLower();
    m_list->clear();

    for (const auto& item : m_allItems) {
        if (!f.isEmpty()) {
            const QString hay = (item.primaryText + QLatin1Char(' ')
                                 + item.secondaryText).toLower();
            if (!hay.contains(f)) continue;
        }

        auto* w = new QListWidgetItem();
        w->setText(item.primaryText);
        w->setData(Qt::UserRole, item.clickAction);
        w->setIcon(makeDotIcon(kindColor(item.kind)));

        QString tip = item.primaryText;
        if (!item.secondaryText.isEmpty()) {
            tip += QLatin1String("\n") + item.secondaryText;
        }
        if (item.clickAction.startsWith(QLatin1String("open:"))) {
            tip += QLatin1String("\n(点击打开文件)");
        } else if (item.clickAction.startsWith(QLatin1String("run:"))) {
            tip += QLatin1String("\n(点击回填命令)");
        } else if (item.clickAction.startsWith(QLatin1String("copy:"))) {
            tip += QLatin1String("\n(点击复制)");
        }
        w->setToolTip(tip);

        m_list->addItem(w);
    }

    if (m_list->count() > 0) m_list->setCurrentRow(0);
}

void TerminalOutputOverlay::dismiss()
{
    hide();
    m_allItems.clear();
}

bool TerminalOutputOverlay::isEmpty() const
{
    return !isVisible() || (m_list && m_list->count() == 0);
}

void TerminalOutputOverlay::onItemActivated(QListWidgetItem* item)
{
    if (!item) return;
    const QString action = item->data(Qt::UserRole).toString();
    if (action.isEmpty()) return;
    emit actionTriggered(action);
}

void TerminalOutputOverlay::reposition()
{
    QWidget* p = parentWidget();
    if (!p) return;

    const int margin = 12;
    const int w = qMin(width(), qMax(200, p->width() - margin * 2));
    const int h = qMin(height(), qMax(120, p->height() - margin * 2));

    resize(w, h);
    move(p->width() - w - margin, margin);
}

bool TerminalOutputOverlay::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == parentWidget() && event->type() == QEvent::Resize) {
        if (isVisible()) reposition();
    }
    return QWidget::eventFilter(watched, event);
}

void TerminalOutputOverlay::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        dismiss();
        return;
    }
    QWidget::keyPressEvent(e);
}

void TerminalOutputOverlay::applyTheme()
{
    auto& tm = ThemeManager::instance();
    const QString bg     = tm.colorString(ThemeManager::UIElement::MainWindowBackground);
    const QString border = tm.colorString(ThemeManager::UIElement::BorderColor);
    const QString fg     = tm.colorString(ThemeManager::UIElement::TextColor);
    const QString sub    = tm.colorString(ThemeManager::UIElement::SecondaryTextColor);
    const QString sel    = tm.colorString(ThemeManager::UIElement::TreeSelectedBackground);

    setStyleSheet(QStringLiteral(
        "QWidget#TerminalOutputOverlay {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  border-radius: 6px;"
        "}"
        "QLabel { color: %3; }"
        "QLineEdit {"
        "  background: transparent;"
        "  color: %3;"
        "  border: 1px solid %2;"
        "  border-radius: 3px;"
        "  padding: 2px 6px;"
        "}"
        "QListWidget {"
        "  background: transparent;"
        "  color: %3;"
        "  border: none;"
        "  outline: none;"
        "}"
        "QListWidget::item { padding: 4px 6px; }"
        "QListWidget::item:selected { background: %4; color: white; }"
        "QToolButton {"
        "  background: transparent;"
        "  border: none;"
        "  color: %3;"
        "  font-size: 16px;"
        "  border-radius: 3px;"
        "}"
        "QToolButton:hover { background: %4; color: white; }"
    ).arg(bg, border, fg, sel));

    if (m_subtitleLabel) {
        m_subtitleLabel->setStyleSheet(QStringLiteral("color: %1;").arg(sub));
    }
}

} // namespace SprintIDE