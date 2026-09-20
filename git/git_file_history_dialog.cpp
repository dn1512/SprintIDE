#include "git_file_history_dialog.h"
#include "git_manager.h"
#include "git_diff_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QFileInfo>
#include <QDateTime>
#include <QMessageBox>

namespace SprintIDE {
namespace Git {

GitFileHistoryDialog::GitFileHistoryDialog(const QString& absoluteFilePath,
                                           const QString& relativePath,
                                           QWidget* parent)
    : QDialog(parent)
    , m_absoluteFilePath(absoluteFilePath)
    , m_relativePath(relativePath)
{
    setupUI();
    loadHistory();
}

GitFileHistoryDialog::~GitFileHistoryDialog() = default;

void GitFileHistoryDialog::setupUI()
{
    const QString fileName = QFileInfo(m_absoluteFilePath).fileName();
    setWindowTitle(QStringLiteral("历史 - %1").arg(fileName));
    resize(760, 560);
    setModal(false);
    setWindowFlag(Qt::Window, true);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    // ---- 顶部信息栏 ----
    auto* topBar = new QHBoxLayout;

    m_titleLabel = new QLabel(this);
    m_titleLabel->setTextFormat(Qt::RichText);
    m_titleLabel->setText(QStringLiteral("文件: <b>%1</b>")
                              .arg(fileName.toHtmlEscaped()));
    m_titleLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    topBar->addWidget(m_titleLabel, 1);

    m_statsLabel = new QLabel(this);
    topBar->addWidget(m_statsLabel);

    auto* closeBtn = new QPushButton(QStringLiteral("关闭"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
    topBar->addWidget(closeBtn);

    root->addLayout(topBar);

    // ---- 提交列表 ----
    m_list = new QListWidget(this);
    m_list->setAlternatingRowColors(false);
    connect(m_list, &QListWidget::itemDoubleClicked,
            this, &GitFileHistoryDialog::onItemDoubleClicked);
    root->addWidget(m_list, 1);
}

void GitFileHistoryDialog::loadHistory()
{
    auto& gm = GitManager::instance();
    const auto commits = gm.fileLog(m_relativePath, 200);

    m_list->clear();
    m_statsLabel->setText(QStringLiteral("共 %1 条提交").arg(commits.size()));

    if (commits.isEmpty()) {
        auto* item = new QListWidgetItem(m_list);
        item->setText(QStringLiteral("(该文件没有任何提交记录)"));
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        return;
    }

    for (const GitCommit& c : commits) {
        const QString when = c.timestamp.toString("yyyy-MM-dd hh:mm");
        QString text = QStringLiteral("%1  %2  %3")
                           .arg(c.shortHash, when, c.message);
        if (text.length() > 120) {
            text = text.left(117) + QStringLiteral("...");
        }

        auto* item = new QListWidgetItem(m_list);
        item->setText(text);
        item->setData(Qt::UserRole, c.hash);
        item->setToolTip(QStringLiteral(
            "Commit:  %1\nAuthor:  %2 <%3>\nDate:    %4\n\n%5")
            .arg(c.hash, c.author, c.email,
                 c.timestamp.toString(Qt::ISODate), c.message));
    }
}

void GitFileHistoryDialog::onItemDoubleClicked(QListWidgetItem* item)
{
    if (!item) return;
    const QString hash = item->data(Qt::UserRole).toString();
    if (hash.isEmpty()) return;

    auto& gm = GitManager::instance();
    const QString diffText = gm.rawCommitDiff(hash);
    if (diffText.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"),
            QStringLiteral("无法获取该 commit 的 diff。"));
        return;
    }

    const QString shortHash = hash.left(8);
    QString firstLine = item->text();
    // 简单裁剪：去掉 "abc1234  yyyy-mm-dd hh:mm  " 前缀
    const int pos = firstLine.indexOf(QStringLiteral("  "), 10);
    if (pos > 0) firstLine = firstLine.mid(pos + 2);

    auto* dlg = new GitDiffDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    dlg->showCommitDiff(
        QStringLiteral("%1  %2").arg(shortHash, firstLine),
        diffText);
}

} // namespace Git
} // namespace SprintIDE