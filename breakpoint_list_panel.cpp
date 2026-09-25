#include "breakpoint_list_panel.h"
#include "debug_manager.h"
#include "theme_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QAction>
#include <QFileInfo>
#include <QPainter>
#include <QPixmap>
#include <QList>
#include <QLabel>

using namespace SprintIDE;

namespace {

// 与编辑器行号区一致的红点风格
QIcon makeDotIcon(const QColor& color, bool filled)
{
    constexpr int kSize = 12;
    QPixmap pix(kSize, kSize);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(color, 1.6));
    p.setBrush(filled ? QBrush(color) : Qt::NoBrush);
    p.drawEllipse(QRectF(2.5, 2.5, kSize - 5.0, kSize - 5.0));
    return QIcon(pix);
}

} // anonymous namespace

namespace SprintIDE {

BreakpointListPanel::BreakpointListPanel(DebugManager* manager, QWidget* parent)
    : QWidget(parent)
    , m_debugManager(manager)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // ---- 顶部摘要条 ----
    m_summaryLabel = new QLabel(this);
    m_summaryLabel->setObjectName("BreakpointsSummary");
    m_summaryLabel->setContentsMargins(10, 6, 10, 6);
    layout->addWidget(m_summaryLabel);

    // ---- 断点列表 ----
    m_list = new QListWidget(this);
    m_list->setObjectName("BreakpointsList");
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_list->setUniformItemSizes(true);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(m_list, 1);

    connect(m_list, &QListWidget::itemDoubleClicked,
            this, &BreakpointListPanel::onItemDoubleClicked);
    connect(m_list, &QListWidget::customContextMenuRequested,
            this, &BreakpointListPanel::onContextMenuRequested);

    refresh();
}

void BreakpointListPanel::refresh()
{
    m_list->clear();
    if (!m_debugManager) return;

    ThemeManager& tm = ThemeManager::instance();
    const QColor dotOn   = tm.color(ThemeManager::UIElement::BreakpointColor);
    const QColor dotOff  = tm.color(ThemeManager::UIElement::DisabledTextColor);
    const QColor condClr = tm.color(ThemeManager::UIElement::WarningColor);
    const QColor textOn  = tm.color(ThemeManager::UIElement::TextColor);
    const QColor textOff = tm.color(ThemeManager::UIElement::DisabledTextColor);

    const QList<Breakpoint> bps = m_debugManager->getBreakpoints();

    // 摘要条
    int enabledCount = 0;
    for (const auto& bp : bps) if (bp.enabled) ++enabledCount;
    m_summaryLabel->setText(QStringLiteral("共 %1 个断点，%2 个已启用")
                                .arg(bps.size()).arg(enabledCount));

    for (const Breakpoint& bp : bps) {
        const QString fileName = QFileInfo(bp.filePath).fileName();
        const bool hasCond = !bp.condition.trimmed().isEmpty();

        QString text = QStringLiteral("%1 : %2")
                           .arg(fileName).arg(bp.lineNumber);
        if (hasCond) text.prepend(QStringLiteral("★ "));

        auto* item = new QListWidgetItem(text, m_list);
        item->setIcon(makeDotIcon(bp.enabled ? dotOn : dotOff, bp.enabled));
        item->setForeground(hasCond ? condClr
                                    : (bp.enabled ? textOn : textOff));
        item->setToolTip(QStringLiteral("%1\n行: %2%3")
                             .arg(bp.filePath)
                             .arg(bp.lineNumber)
                             .arg(hasCond
                                      ? QStringLiteral("\n条件: %1").arg(bp.condition)
                                      : QString()));

        // 数据载荷
        item->setData(Qt::UserRole + 0, bp.filePath);
        item->setData(Qt::UserRole + 1, bp.lineNumber);
        item->setData(Qt::UserRole + 2, bp.enabled);
        item->setData(Qt::UserRole + 3, bp.condition);
    }
}

void BreakpointListPanel::onItemDoubleClicked(QListWidgetItem* item)
{
    if (!item) return;
    const QString file = item->data(Qt::UserRole + 0).toString();
    const int line     = item->data(Qt::UserRole + 1).toInt();
    if (file.isEmpty() || line <= 0) return;
    emit breakpointActivated(file, line);
}

void BreakpointListPanel::onContextMenuRequested(const QPoint& pos)
{
    QListWidgetItem* item = m_list->itemAt(pos);
    if (!item) return;

    const QString file = item->data(Qt::UserRole + 0).toString();
    const int     line = item->data(Qt::UserRole + 1).toInt();
    const bool    en   = item->data(Qt::UserRole + 2).toBool();
    const QString cond = item->data(Qt::UserRole + 3).toString();

    QMenu menu(this);

    QAction* toggleAct = menu.addAction(en ? tr("禁用断点") : tr("启用断点"));
    connect(toggleAct, &QAction::triggered, this, [this, file, line, en]() {
        emit breakpointToggleEnabledRequested(file, line, !en);
    });

    QAction* editCondAct = menu.addAction(tr("编辑条件..."));
    connect(editCondAct, &QAction::triggered, this, [this, file, line]() {
        emit breakpointEditConditionRequested(file, line);
    });

    if (!cond.trimmed().isEmpty()) {
        QAction* clearCondAct = menu.addAction(tr("清除条件"));
        connect(clearCondAct, &QAction::triggered, this, [this, file, line]() {
            emit breakpointEditConditionRequested(file, line);
        });
    }

    menu.addSeparator();

    QAction* removeAct = menu.addAction(tr("删除断点"));
    connect(removeAct, &QAction::triggered, this, [this, file, line]() {
        emit breakpointRemoveRequested(file, line);
    });

    menu.exec(m_list->viewport()->mapToGlobal(pos));
}

} // namespace SprintIDE
