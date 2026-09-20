#include "git_diff_dialog.h"
#include "theme_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QToolButton>
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QScrollBar>
#include <QDebug>
#include <QTimer>

namespace SprintIDE {
namespace Git {

// 单次渲染的行数上限，防止大文件把 UI 拖死
static constexpr int kMaxDiffLines = 50000;

GitDiffDialog::GitDiffDialog(QWidget* parent)
    : QDialog(parent)
{
    setupUI();
}

GitDiffDialog::~GitDiffDialog() = default;

void GitDiffDialog::setupUI()
{
    setWindowTitle(QStringLiteral("Git Diff"));
    resize(1000, 720);
    setModal(false);   // 非模态，可同时开着编辑器对照
    setWindowFlag(Qt::Window, true);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    // ============================================================
    // 顶部栏：[标题]  [统计]  [打开文件]  [复制] [关闭]
    // ============================================================
    auto* topBar = new QHBoxLayout;

    m_titleLabel = new QLabel(this);
    m_titleLabel->setStyleSheet(
        "QLabel { font-weight: bold; font-size: 11pt; color: #D4DCE8; }");
    m_titleLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    topBar->addWidget(m_titleLabel);
    topBar->addStretch();

    m_statsLabel = new QLabel(this);
    m_statsLabel->setStyleSheet("QLabel { color: #7A8599; }");
    topBar->addWidget(m_statsLabel);

    topBar->addSpacing(12);

    m_openFileBtn = new QPushButton(QStringLiteral("打开文件"), this);
    m_openFileBtn->setEnabled(false);
    connect(m_openFileBtn, &QPushButton::clicked,
            this, &GitDiffDialog::onOpenFileClicked);
    topBar->addWidget(m_openFileBtn);

    m_copyBtn = new QToolButton(this);
    m_copyBtn->setText(QStringLiteral("复制"));
    m_copyBtn->setToolTip(QStringLiteral("复制全部 diff"));
    connect(m_copyBtn, &QToolButton::clicked,
            this, &GitDiffDialog::onCopyClicked);
    topBar->addWidget(m_copyBtn);

    m_closeBtn = new QToolButton(this);
    m_closeBtn->setText(QStringLiteral("✕"));
    m_closeBtn->setToolTip(QStringLiteral("关闭"));
    connect(m_closeBtn, &QToolButton::clicked, this, &QDialog::close);
    topBar->addWidget(m_closeBtn);

    root->addLayout(topBar);

    // ============================================================
    // Diff 视图
    // ============================================================
    m_diffView = new QTextEdit(this);
    m_diffView->setReadOnly(true);
    m_diffView->setLineWrapMode(QTextEdit::NoWrap);
    // 样式由全局 QTextEdit 规则提供；等宽字体通过 setFont 局部指定（字体不算 UI 风格）
    QFont monoFont = m_diffView->font();
    monoFont.setStyleHint(QFont::Monospace);
    monoFont.setFamily(QStringLiteral("Menlo"));
    m_diffView->setFont(monoFont);
    root->addWidget(m_diffView, 1);
}

void GitDiffDialog::showFileDiff(const QString& title,
                                 const QString& diffText,
                                 const QString& openFileAbsolutePath)
{
    setWindowTitle(QStringLiteral("Diff - %1").arg(title));
    m_titleLabel->setText(title);
    setOpenFileButtonEnabled(openFileAbsolutePath);
    renderDiff(diffText);
    show();
    raise();
    activateWindow();
}

void GitDiffDialog::showCommitDiff(const QString& title,
                                   const QString& diffText)
{
    setWindowTitle(QStringLiteral("Commit - %1").arg(title));
    m_titleLabel->setText(title);
    setOpenFileButtonEnabled(QString());   // commit 级不启用"打开文件"
    renderDiff(diffText);
    show();
    raise();
    activateWindow();
}

void GitDiffDialog::setOpenFileButtonEnabled(const QString& absPath)
{
    m_openFileAbsPath = absPath;
    m_openFileBtn->setEnabled(!absPath.isEmpty());
}

void GitDiffDialog::onOpenFileClicked()
{
    if (!m_openFileAbsPath.isEmpty()) {
        emit openFileRequested(m_openFileAbsPath);
    }
}

void GitDiffDialog::onCopyClicked()
{
    QApplication::clipboard()->setText(m_diffView->toPlainText());
    m_copyBtn->setText(QStringLiteral("已复制"));
    QTimer::singleShot(1200, this, [this]() {
        m_copyBtn->setText(QStringLiteral("复制"));
    });
}

void GitDiffDialog::renderDiff(const QString& diffText)
{
    // ---- 空 diff ----
    if (diffText.trimmed().isEmpty()) {
        m_statsLabel->setText(QStringLiteral("无差异"));
        using UI = ThemeManager::UIElement;
        const QString dim = ThemeManager::instance()
                                .color(UI::SecondaryTextColor).name();
        m_diffView->setHtml(
            QStringLiteral("<div style='color:%1; padding:20px; "
                           "font-family:Menlo,Consolas,monospace;'>"
                           "此文件没有任何差异。</div>").arg(dim));
        return;
    }

    const QStringList lines = diffText.split('\n');

    // ---- 统计 + 截断 ----
    int addCount = 0, delCount = 0;
    for (const QString& l : lines) {
        if (l.startsWith('+') && !l.startsWith("+++")) ++addCount;
        else if (l.startsWith('-') && !l.startsWith("---")) ++delCount;
    }

    int totalLines = lines.size();
    bool truncated = false;
    int limit = qMin(totalLines, kMaxDiffLines);
    if (totalLines > kMaxDiffLines) truncated = true;

    QString stats = QStringLiteral("+%1  −%2").arg(addCount).arg(delCount);
    if (truncated) {
        stats += QStringLiteral("  （共 %1 行，仅显示前 %2 行）")
                     .arg(totalLines).arg(kMaxDiffLines);
    }
    m_statsLabel->setText(stats);

    // ---- 渲染 HTML ----
    QString html;
    html.reserve(limit * 80);

    using UI = ThemeManager::UIElement;
    const auto& tm = ThemeManager::instance();
    const QString bodyBg = tm.color(UI::EditorBackground).name();
    const QString bodyFg = tm.color(UI::TextColor).name();

    html += QStringLiteral(
                "<html><body style='"
                "font-family: Menlo, Consolas, \"JetBrains Mono\", monospace;"
                "font-size: 11pt;"
                "background-color: %1;"
                "color: %2;"
                "margin: 0; padding: 6px;"
                "'>").arg(bodyBg, bodyFg);

    for (int i = 0; i < limit; ++i) {
        const QString& line = lines.at(i);
        const QString escaped = line.toHtmlEscaped();

        // 从 ThemeManager 拿颜色，随主题变化
        using UI = ThemeManager::UIElement;
        const auto& tm = ThemeManager::instance();
        const QString cPrimary    = tm.color(UI::PrimaryColor).name();
        const QString cSecondary  = tm.color(UI::SecondaryTextColor).name();
        const QString cText       = tm.color(UI::TextColor).name();
        const QString cWarning    = tm.color(UI::WarningColor).name();
        const QString cSuccess    = tm.color(UI::SuccessColor).name();
        const QString cError      = tm.color(UI::ErrorColor).name();
        const QString cBg         = tm.color(UI::EditorBackground).name();
        const QString cBgAlt      = tm.color(UI::MainWindowBackground).name();
        const QString cAddedBg    = tm.color(UI::SuccessColor).darker(350).name();
        const QString cRemovedBg  = tm.color(UI::ErrorColor).darker(350).name();

        QString style;
        if (line.startsWith("diff --git")
            || line.startsWith("index ")
            || line.startsWith("old mode")
            || line.startsWith("new mode")
            || line.startsWith("new file mode")
            || line.startsWith("deleted file mode")
            || line.startsWith("rename from")
            || line.startsWith("rename to")
            || line.startsWith("similarity index")) {
            style = QStringLiteral("color:%1; background-color:%2; font-weight:bold;")
            .arg(cPrimary, cBgAlt);
        } else if (line.startsWith("@@")) {
            style = QStringLiteral("color:%1; background-color:%2;")
            .arg(cPrimary, cBgAlt);
        } else if (line.startsWith("+++") || line.startsWith("---")) {
            style = QStringLiteral("color:%1; background-color:%2;")
            .arg(cSecondary, cBgAlt);
        } else if (line.startsWith("commit ")) {
            style = QStringLiteral("color:%1; background-color:%2; font-weight:bold;")
            .arg(cWarning, cBgAlt);
        } else if (line.startsWith("Author:") || line.startsWith("Date:")
                   || line.startsWith("Merge:") || line.startsWith("    ")) {
            style = QStringLiteral("color:%1;").arg(cSecondary);
        } else if (line.startsWith('+')) {
            style = QStringLiteral("color:%1; background-color:%2;")
            .arg(cSuccess, cAddedBg);
        } else if (line.startsWith('-')) {
            style = QStringLiteral("color:%1; background-color:%2;")
            .arg(cError, cRemovedBg);
        } else {
            style = QStringLiteral("color:%1;").arg(cText);
        }

        // white-space:pre 保留空格；空行用 &nbsp; 撑起高度
        html += QStringLiteral(
                    "<div style='white-space:pre; %1; padding:1px 4px;'>%2</div>")
                    .arg(style, escaped.isEmpty() ? QStringLiteral("&nbsp;") : escaped);
    }

    if (truncated) {
        html += QStringLiteral(
                    "<div style='color:#EBB65B; background:#2A3340; padding:8px; "
                    "margin-top:10px;'>"
                    "... 剩余 %1 行已省略（diff 过大）</div>")
                    .arg(totalLines - kMaxDiffLines);
    }

    html += "</body></html>";

    m_diffView->setHtml(html);
    m_diffView->verticalScrollBar()->setValue(0);
}

} // namespace Git
} // namespace SprintIDE
