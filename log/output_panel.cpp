#include "output_panel.h"
#include "log_list_model.h"
#include "log_list_view.h"
#include "log_item_delegate.h"
#include "theme_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QToolButton>
#include <QLabel>
#include <QScrollBar>
#include <QRegularExpression>

namespace SprintIDE {

OutputPanel::OutputPanel(QWidget* parent)
    : QWidget(parent)
{
    buildUi();

    connect(m_model, &LogListModel::countsChanged,
            this, &OutputPanel::onCountsChanged);
    connect(m_view->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &OutputPanel::onScrollBarValueChanged);
    connect(m_view, &LogListView::jumpRequested,
            this, &OutputPanel::jumpRequested);
}

OutputPanel::~OutputPanel() = default;

void OutputPanel::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ---------- 工具条 ----------
    auto* toolbar = new QWidget(this);
    toolbar->setObjectName("OutputToolbar");
    auto* h = new QHBoxLayout(toolbar);
    h->setContentsMargins(6, 4, 6, 4);
    h->setSpacing(6);
    // 把复选框的默认高度锁定，避免换行 / 挤压
    toolbar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    m_search = new QLineEdit(toolbar);
    m_search->setPlaceholderText(tr("在输出中过滤..."));
    m_search->setClearButtonEnabled(true);
    m_search->setMaximumWidth(260);
    connect(m_search, &QLineEdit::textChanged,
            this, &OutputPanel::onSearchChanged);
    h->addWidget(m_search);

    // ------------------------------------------------------------
    // 三档过滤按钮：纯图标 QToolButton + checkable
    //   - 图标沿用 git 状态图标（语义色已对齐 Polaris 调色板）
    //   - 视觉风格由 applyTheme() 统一控制，随主题切换
    //   - Tooltip 提供中文提示，不占界面空间
    // ------------------------------------------------------------
    auto makeFilterButton = [this, toolbar](const QString& iconPath,
                                            const QString& tip) -> QToolButton* {
        auto* b = new QToolButton(toolbar);
        b->setIcon(QIcon(iconPath));
        b->setIconSize(QSize(14, 14));    // ← 从 16 缩到 14
        b->setToolTip(tip);
        b->setCheckable(true);
        b->setChecked(true);
        b->setFocusPolicy(Qt::NoFocus);
        b->setAutoRaise(true);
        b->setCursor(Qt::PointingHandCursor);
        b->setFixedSize(22, 20);          // ← 从 24×22 缩到 22×20
        return b;
    };

    m_chkErr  = makeFilterButton(QStringLiteral(":/icons/icons/status_conflict.svg"),
                                tr("显示 / 隐藏错误"));
    m_chkWarn = makeFilterButton(QStringLiteral(":/icons/icons/status_modified.svg"),
                                 tr("显示 / 隐藏警告"));
    m_chkInfo = makeFilterButton(QStringLiteral(":/icons/icons/status_untracked.svg"),
                                 tr("显示 / 隐藏信息"));

    connect(m_chkErr,  &QToolButton::toggled, this, &OutputPanel::onFilterChanged);
    connect(m_chkWarn, &QToolButton::toggled, this, &OutputPanel::onFilterChanged);
    connect(m_chkInfo, &QToolButton::toggled, this, &OutputPanel::onFilterChanged);

    h->addWidget(m_chkErr);
    h->addSpacing(4);
    h->addWidget(m_chkWarn);
    h->addSpacing(4);
    h->addWidget(m_chkInfo);

    h->addStretch();

    m_btnAutoScroll = new QToolButton(toolbar);
    m_btnAutoScroll->setIcon(QIcon(QStringLiteral(":/icons/icons/log_follow.svg")));
    m_btnAutoScroll->setIconSize(QSize(14, 14));
    m_btnAutoScroll->setCheckable(true);
    m_btnAutoScroll->setChecked(true);
    m_btnAutoScroll->setToolTip(tr("跟随最新输出（自动滚动到底部）"));
    m_btnAutoScroll->setFocusPolicy(Qt::NoFocus);
    m_btnAutoScroll->setAutoRaise(true);
    m_btnAutoScroll->setCursor(Qt::PointingHandCursor);
    m_btnAutoScroll->setFixedSize(22, 20);
    connect(m_btnAutoScroll, &QToolButton::toggled,
            this, &OutputPanel::setAutoScroll);
    h->addWidget(m_btnAutoScroll);

    m_btnClear = new QToolButton(toolbar);
    m_btnClear->setIcon(QIcon(QStringLiteral(":/icons/icons/clear.svg")));
    m_btnClear->setIconSize(QSize(14, 14));
    m_btnClear->setToolTip(tr("清空输出"));
    m_btnClear->setFocusPolicy(Qt::NoFocus);
    m_btnClear->setAutoRaise(true);
    m_btnClear->setCursor(Qt::PointingHandCursor);
    m_btnClear->setFixedSize(22, 20);
    connect(m_btnClear, &QToolButton::clicked, this, &OutputPanel::clear);
    h->addWidget(m_btnClear);
    // 让复选框在窄面板下也不被压扁
    h->setAlignment(m_chkErr, Qt::AlignVCenter);
    toolbar->setMinimumHeight(toolbar->sizeHint().height());
    root->addWidget(toolbar);
    h->setAlignment(m_chkWarn, Qt::AlignVCenter);
    toolbar->setMinimumHeight(toolbar->sizeHint().height());
    root->addWidget(toolbar);
    h->setAlignment(m_chkInfo, Qt::AlignVCenter);
    toolbar->setMinimumHeight(toolbar->sizeHint().height());
    root->addWidget(toolbar);

    // ---------- 视图 ----------
    m_model = new LogListModel(this);
    m_view  = new LogListView(this);
    m_view->setLogModel(m_model);
    m_view->setItemDelegate(new LogItemDelegate(m_view));
    root->addWidget(m_view, 1);

    // ---------- 状态栏 ----------
    m_summary = new QLabel(this);
    m_summary->setObjectName("OutputSummary");
    m_summary->setContentsMargins(8, 2, 8, 2);
    root->addWidget(m_summary);

    // 首次上色（后续主题切换由 MainWindow::applyCurrentTheme 再调）
    applyTheme();

    onCountsChanged(0, 0, 0, 0);
}

// ============================================================
//  写入
// ============================================================
void OutputPanel::appendEntry(const LogEntry& e)
{
    m_model->appendEntry(e);
    applyAutoScrollIfNeeded();
}

void OutputPanel::appendEntries(const QList<LogEntry>& entries)
{
    m_model->appendEntries(entries);
    applyAutoScrollIfNeeded();
}

void OutputPanel::appendPlainText(const QString& text)
{
    const QStringList lines = text.split('\n');
    QList<LogEntry> batch;
    batch.reserve(lines.size());
    for (const QString& l : lines) {
        if (l.isEmpty()) continue;
        batch.append(LogEntry::make(LogKind::System, LogLevel::Info,
                                    QStringLiteral("IDE"), l));
    }
    appendEntries(batch);
}

void OutputPanel::appendHtml(const QString& html)
{
    // 简易 HTML 剥离：去掉标签、还原常见实体，然后走纯文本
    QString plain = html;
    static const QRegularExpression tagRe("<[^>]+>");
    plain.remove(tagRe);
    plain.replace("&lt;",   "<");
    plain.replace("&gt;",   ">");
    plain.replace("&amp;",  "&");
    plain.replace("&quot;", "\"");
    plain.replace("&#39;",  "'");
    plain.replace("&nbsp;", " ");
    appendPlainText(plain);
}

// ============================================================
//  操作
// ============================================================
void OutputPanel::clear()
{
    m_model->clear();
    emit cleared();
}

void OutputPanel::setAutoScroll(bool on)
{
    m_autoScroll = on;
    if (on) {
        m_stickToBottom = true;
        m_view->scrollToBottom();
    }
}

// ============================================================
//  主题刷新：颜色全部走 ThemeManager，切主题时由 MainWindow 调用
// ============================================================
void OutputPanel::applyTheme()
{
    ThemeManager& tm = ThemeManager::instance();

    // ---------- 工具条背景 ----------
    if (auto* tb = findChild<QWidget*>("OutputToolbar")) {
        tb->setStyleSheet(QString(
                              "QWidget#OutputToolbar {"
                              "  background-color: %1;"
                              "  border-bottom: 1px solid %2;"
                              "}"
                              "QWidget#OutputToolbar QLineEdit {"
                              "  background-color: %3;"
                              "  color: %4;"
                              "  border: 1px solid %2;"
                              "  border-radius: 4px;"
                              "  padding: 2px 6px;"
                              "}"
                              "QWidget#OutputToolbar QLineEdit:focus {"
                              "  border: 1px solid %5;"
                              "}"
                              ).arg(
                                  tm.colorString(ThemeManager::UIElement::MainWindowBackground),
                                  tm.colorString(ThemeManager::UIElement::BorderColor),
                                  tm.colorString(ThemeManager::UIElement::EditorBackground),
                                  tm.colorString(ThemeManager::UIElement::TextColor),
                                  tm.colorString(ThemeManager::UIElement::PrimaryColor)
                                  ));
    }

    // ---------- 三个图标按钮：只用背景 / 边框表达状态，不动图标颜色 ----------
    //   图标本身是彩色 SVG，交给 SVG 控制颜色；
    //   按钮的 hover / checked / 边框 由主题令牌控制，切主题时跟随。
    struct FilterStyle {
        QToolButton* btn;
        QColor accent;                 // 语义色（错误红 / 警告橙 / 信息灰）
    };
    const QList<FilterStyle> filters = {
                                        { m_chkErr,  tm.color(ThemeManager::UIElement::ErrorColor)   },
                                        { m_chkWarn, tm.color(ThemeManager::UIElement::WarningColor) },
                                        { m_chkInfo, tm.color(ThemeManager::UIElement::SecondaryTextColor) },
                                        };

    const QString hoverBg   = tm.rgba(ThemeManager::UIElement::HoverBackground, 160);
    const QString borderCol = tm.colorString(ThemeManager::UIElement::BorderColor);
    const QString disabledTxt = tm.colorString(ThemeManager::UIElement::DisabledTextColor);

    for (const auto& f : filters) {
        if (!f.btn) continue;

        const QString accentHex = f.accent.name();
        const QString accentBg  = QString("rgba(%1,%2,%3,50)")
                                     .arg(f.accent.red())
                                     .arg(f.accent.green())
                                     .arg(f.accent.blue());

        f.btn->setStyleSheet(QString(
                                 "QToolButton {"
                                 "  background: transparent;"
                                 "  border: 1px solid transparent;"     // 默认无边框，更干净
                                 "  border-radius: 3px;"
                                 "  padding: 1px;"
                                 "}"
                                 "QToolButton:hover {"
                                 "  background-color: %1;"              // 悬停：主题悬停色
                                 "  border-color: %2;"                  // 边框提示"可点"
                                 "}"
                                 "QToolButton:checked {"
                                 "  background-color: %3;"              // 选中：语义色 20% 透明
                                 "  border-color: %4;"                  // 边框：语义色实色
                                 "}"
                                 "QToolButton:!checked {"
                                 "  background: transparent;"
                                 "  border: 1px solid transparent;"
                                 "}"
                                 "QToolButton:disabled {"
                                 "  color: %5;"
                                 "}"
                                 ).arg(
                                     hoverBg,        // %1 悬停背景
                                     borderCol,      // %2 悬停边框
                                     accentBg,       // %3 选中背景（语义色 alpha=50）
                                     accentHex,      // %4 选中边框（语义色实色）
                                     disabledTxt     // %5 禁用（保留，虽然不常用）
                                     ));
    }

    // ---------- 跟随 / 清空：中性按钮样式（无 checkable 语义色） ----------
    const QString toolBtnCommonQss = QString(
                                         "QToolButton {"
                                         "  background: transparent;"
                                         "  border: 1px solid transparent;"
                                         "  border-radius: 3px;"
                                         "  padding: 1px;"
                                         "}"
                                         "QToolButton:hover {"
                                         "  background-color: %1;"
                                         "  border-color: %2;"
                                         "}"
                                         "QToolButton:pressed {"
                                         "  background-color: %3;"
                                         "}"
                                         "QToolButton:checked {"
                                         "  background-color: %3;"
                                         "  border-color: %2;"
                                         "}"
                                         "QToolButton:disabled {"
                                         "  color: %4;"
                                         "}"
                                         ).arg(
                                             tm.rgba(ThemeManager::UIElement::HoverBackground, 160),
                                             tm.colorString(ThemeManager::UIElement::BorderColor),
                                             tm.rgba(ThemeManager::UIElement::ActiveTabBackground, 220),
                                             tm.colorString(ThemeManager::UIElement::DisabledTextColor)
                                             );

    if (m_btnAutoScroll) m_btnAutoScroll->setStyleSheet(toolBtnCommonQss);
    if (m_btnClear)      m_btnClear->setStyleSheet(toolBtnCommonQss);

    // ---------- 底部摘要 ----------
    if (m_summary) {
        m_summary->setStyleSheet(QString(
                                     "QLabel#OutputSummary {"
                                     "  color: %1;"
                                     "  background-color: %2;"
                                     "  border-top: 1px solid %3;"
                                     "  padding: 2px 8px;"
                                     "}"
                                     ).arg(
                                         tm.colorString(ThemeManager::UIElement::SecondaryTextColor),
                                         tm.colorString(ThemeManager::UIElement::MainWindowBackground),
                                         tm.colorString(ThemeManager::UIElement::BorderColor)
                                         ));
    }
}

// ============================================================
//  槽
// ============================================================
void OutputPanel::onFilterChanged()
{
    m_model->setShowError(m_chkErr->isChecked());
    m_model->setShowWarning(m_chkWarn->isChecked());
    m_model->setShowInfo(m_chkInfo->isChecked());
}

void OutputPanel::onSearchChanged(const QString& text)
{
    m_model->setTextFilter(text.trimmed());
}

void OutputPanel::onCountsChanged(int errs, int warns, int infos, int visible)
{
    QString s = tr("错误 %1 · 警告 %2 · 信息 %3").arg(errs).arg(warns).arg(infos);
    if (visible != errs + warns + infos) {
        s += tr("  ·  过滤后 %1 条").arg(visible);
    } else {
        s += tr("  ·  共 %1 条").arg(visible);
    }
    m_summary->setText(s);
}

void OutputPanel::onScrollBarValueChanged(int value)
{
    auto* sb = m_view->verticalScrollBar();
    m_stickToBottom = (value >= sb->maximum() - 2);
}

void OutputPanel::applyAutoScrollIfNeeded()
{
    if (!m_autoScroll) return;
    if (!m_stickToBottom) return;
    m_view->scrollToBottom();
}

} // namespace SprintIDE
