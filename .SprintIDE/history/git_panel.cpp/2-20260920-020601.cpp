#include "git_panel.h"
#include "git_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QComboBox>
#include <QToolButton>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QStackedWidget>
#include <QMenu>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QDateTime>
#include <QDebug>
#include <QShortcut>

namespace SprintIDE {
namespace Git {

// ============================================================
// 构造 / 析构
// ============================================================
GitPanel::GitPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();

    auto& gm = GitManager::instance();
    connect(&gm, &GitManager::repositoryChanged,
            this, &GitPanel::onRepositoryChanged);
    connect(&gm, &GitManager::statusUpdated,
            this, &GitPanel::onStatusUpdated);
    connect(&gm, &GitManager::busyChanged,
            this, &GitPanel::onBusyChanged);
    connect(&gm, &GitManager::commandFailed,
            this, &GitPanel::onCommandFailed);

    // 初始状态
    onRepositoryChanged(gm.repositoryInfo());
    onStatusUpdated(gm.currentStatus());
}

GitPanel::~GitPanel() = default;

// ============================================================
// 项目绑定
// ============================================================
void GitPanel::setProjectRoot(const QString& path)
{
    m_projectRoot = path;
    GitManager::instance().setProjectRoot(path);
}

QString GitPanel::projectRoot() const
{
    return m_projectRoot;
}

// ============================================================
// UI 搭建
// ============================================================
void GitPanel::setupUI()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    root->addWidget(createTopBar());

    // 空仓库提示
    m_emptyRepoHint = new QLabel(this);
    m_emptyRepoHint->setAlignment(Qt::AlignCenter);
    m_emptyRepoHint->setWordWrap(true);
    m_emptyRepoHint->setStyleSheet(
        "QLabel { color: #7A8599; padding: 24px; font-size: 11pt; }");
    m_emptyRepoHint->setText(
        QStringLiteral("此目录尚未初始化为 Git 仓库。\n\n"
                       "点击上方「初始化仓库」按钮开始使用。"));
    m_emptyRepoHint->setVisible(false);
    root->addWidget(m_emptyRepoHint);

    // 主体：用 stack 切换「正常内容 / 空仓库提示」
    m_contentArea = new QWidget(this);
    auto* contentLayout = new QVBoxLayout(m_contentArea);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    m_splitter = new QSplitter(Qt::Vertical, m_contentArea);
    m_splitter->setChildrenCollapsible(false);
    m_splitter->setHandleWidth(3);

    m_splitter->addWidget(createStatusSection());
    m_splitter->addWidget(createCommitSection());
    m_splitter->addWidget(createHistorySection());

    m_splitter->setStretchFactor(0, 5);
    m_splitter->setStretchFactor(1, 0);
    m_splitter->setStretchFactor(2, 4);

    contentLayout->addWidget(m_splitter);
    root->addWidget(m_contentArea, 1);

    // 忙碌进度条
    m_busyBar = new QProgressBar(this);
    m_busyBar->setRange(0, 0);          // 不确定进度
    m_busyBar->setMaximumHeight(4);
    m_busyBar->setTextVisible(false);
    m_busyBar->setVisible(false);
    root->addWidget(m_busyBar);
}

QWidget* GitPanel::createTopBar()
{
    auto* bar = new QWidget(this);
    bar->setObjectName("GitTopBar");
    auto* lay = new QHBoxLayout(bar);
    lay->setContentsMargins(6, 6, 6, 4);
    lay->setSpacing(4);

    m_branchCombo = new QComboBox(bar);
    m_branchCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_branchCombo->setMinimumWidth(120);
    connect(m_branchCombo, QOverload<int>::of(&QComboBox::activated),
            this, &GitPanel::onBranchActivated);

    m_refreshBtn = new QToolButton(bar);
    m_refreshBtn->setText(QStringLiteral("⟳"));
    m_refreshBtn->setToolTip(QStringLiteral("刷新状态 (F5)"));
    connect(m_refreshBtn, &QToolButton::clicked,
            this, &GitPanel::onRefreshClicked);

    m_initBtn = new QToolButton(bar);
    m_initBtn->setText(QStringLiteral("初始化仓库"));
    m_initBtn->setToolTip(QStringLiteral("在当前项目根目录执行 git init"));
    m_initBtn->setVisible(false);
    connect(m_initBtn, &QToolButton::clicked,
            this, &GitPanel::onInitClicked);

    lay->addWidget(m_branchCombo, 1);
    lay->addWidget(m_refreshBtn);
    lay->addWidget(m_initBtn);
    return bar;
}

QWidget* GitPanel::createStatusSection()
{
    auto* w = new QWidget(this);
    auto* lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    // ---- 已暂存 ----
    auto* stagedBar = new QWidget(w);
    auto* sl = new QHBoxLayout(stagedBar);
    sl->setContentsMargins(6, 4, 6, 2);
    m_stagedHeader = new QLabel(QStringLiteral("已暂存的更改 (0)"), stagedBar);
    m_stagedHeader->setStyleSheet("font-weight: bold;");
    m_unstageAllBtn = new QToolButton(stagedBar);
    m_unstageAllBtn->setText(QStringLiteral("−"));
    m_unstageAllBtn->setToolTip(QStringLiteral("全部取消暂存"));
    connect(m_unstageAllBtn, &QToolButton::clicked,
            this, &GitPanel::onUnstageAllClicked);
    sl->addWidget(m_stagedHeader, 1);
    sl->addWidget(m_unstageAllBtn);
    lay->addWidget(stagedBar);

    m_stagedList = new QListWidget(w);
    m_stagedList->setObjectName("GitStagedList");
    m_stagedList->setAlternatingRowColors(false);
    m_stagedList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_stagedList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_stagedList, &QListWidget::itemDoubleClicked,
            this, &GitPanel::onStagedItemDoubleClicked);
    connect(m_stagedList, &QListWidget::customContextMenuRequested,
            this, &GitPanel::onStagedContextMenu);
    lay->addWidget(m_stagedList, 1);

    // ---- 未暂存 ----
    auto* unstagedBar = new QWidget(w);
    auto* ul = new QHBoxLayout(unstagedBar);
    ul->setContentsMargins(6, 4, 6, 2);
    m_unstagedHeader = new QLabel(QStringLiteral("更改 (0)"), unstagedBar);
    m_unstagedHeader->setStyleSheet("font-weight: bold;");
    m_stageAllBtn = new QToolButton(unstagedBar);
    m_stageAllBtn->setText(QStringLiteral("+"));
    m_stageAllBtn->setToolTip(QStringLiteral("全部暂存"));
    connect(m_stageAllBtn, &QToolButton::clicked,
            this, &GitPanel::onStageAllClicked);
    ul->addWidget(m_unstagedHeader, 1);
    ul->addWidget(m_stageAllBtn);
    lay->addWidget(unstagedBar);

    m_unstagedList = new QListWidget(w);
    m_unstagedList->setObjectName("GitUnstagedList");
    m_unstagedList->setAlternatingRowColors(false);
    m_unstagedList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_unstagedList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_unstagedList, &QListWidget::itemDoubleClicked,
            this, &GitPanel::onUnstagedItemDoubleClicked);
    connect(m_unstagedList, &QListWidget::customContextMenuRequested,
            this, &GitPanel::onUnstagedContextMenu);
    lay->addWidget(m_unstagedList, 1);

    return w;
}

QWidget* GitPanel::createCommitSection()
{
    auto* w = new QWidget(this);
    auto* lay = new QVBoxLayout(w);
    lay->setContentsMargins(6, 4, 6, 6);
    lay->setSpacing(4);

    m_messageEdit = new QPlainTextEdit(w);
    m_messageEdit->setPlaceholderText(QStringLiteral("提交信息 (Ctrl+Enter 提交)"));
    m_messageEdit->setMaximumHeight(80);
    m_messageEdit->setMinimumHeight(56);
    connect(m_messageEdit, &QPlainTextEdit::textChanged,
            this, &GitPanel::updateCommitButton);
    lay->addWidget(m_messageEdit);

    auto* btnRow = new QHBoxLayout;
    btnRow->setSpacing(6);

    m_amendCheck = new QCheckBox(QStringLiteral("修正上次提交"), w);
    btnRow->addWidget(m_amendCheck);
    btnRow->addStretch();

    m_commitBtn = new QPushButton(QStringLiteral("提交"), w);
    m_commitBtn->setEnabled(false);
    connect(m_commitBtn, &QPushButton::clicked,
            this, &GitPanel::onCommitClicked);
    btnRow->addWidget(m_commitBtn);

    lay->addLayout(btnRow);

    // Ctrl+Enter 提交
    auto* submitShortcut = new QShortcut(QKeySequence("Ctrl+Return"), m_messageEdit);
    connect(submitShortcut, &QShortcut::activated, this, [this]() {
        if (m_commitBtn->isEnabled()) onCommitClicked();
    });
    auto* submitShortcut2 = new QShortcut(QKeySequence("Ctrl+Enter"), m_messageEdit);
    connect(submitShortcut2, &QShortcut::activated, this, [this]() {
        if (m_commitBtn->isEnabled()) onCommitClicked();
    });

    return w;
}

QWidget* GitPanel::createHistorySection()
{
    auto* w = new QWidget(this);
    auto* lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    auto* headerBar = new QWidget(w);
    auto* hl = new QHBoxLayout(headerBar);
    hl->setContentsMargins(6, 4, 6, 2);
    auto* header = new QLabel(QStringLiteral("提交历史"), headerBar);
    header->setStyleSheet("font-weight: bold;");
    hl->addWidget(header, 1);
    lay->addWidget(headerBar);

    m_historyList = new QListWidget(w);
    m_historyList->setObjectName("GitHistoryList");
    m_historyList->setAlternatingRowColors(false);
    m_historyList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_historyList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_historyList, &QListWidget::itemDoubleClicked,
            this, &GitPanel::onHistoryItemDoubleClicked);
    lay->addWidget(m_historyList, 1);

    return w;
}

// ============================================================
// 状态填充
// ============================================================
QString GitPanel::statusBadge(Git::FileStatus s)
{
    switch (s) {
    case FileStatus::Added:       return QStringLiteral("A");
    case FileStatus::Modified:    return QStringLiteral("M");
    case FileStatus::Deleted:     return QStringLiteral("D");
    case FileStatus::Renamed:     return QStringLiteral("R");
    case FileStatus::Copied:      return QStringLiteral("C");
    case FileStatus::Untracked:   return QStringLiteral("?");
    case FileStatus::Ignored:     return QStringLiteral("!");
    case FileStatus::Conflicted:  return QStringLiteral("U");
    case FileStatus::TypeChanged: return QStringLiteral("T");
    default:                      return QStringLiteral(" ");
    }
}

QColor GitPanel::statusColor(Git::FileStatus s)
{
    switch (s) {
    case FileStatus::Added:       return QColor("#46C687");
    case FileStatus::Modified:    return QColor("#EBB65B");
    case FileStatus::Deleted:     return QColor("#E55B6B");
    case FileStatus::Renamed:     return QColor("#4A9EFF");
    case FileStatus::Copied:      return QColor("#4A9EFF");
    case FileStatus::Untracked:   return QColor("#7A8599");
    case FileStatus::Conflicted:  return QColor("#E55B6B");
    default:                      return QColor("#D4DCE8");
    }
}

void GitPanel::populateStatusLists(const QList<GitFileStatus>& status)
{
    m_stagedList->clear();
    m_unstagedList->clear();

    int stagedCount = 0;
    int unstagedCount = 0;

    for (const GitFileStatus& s : status) {
        // ---- 已暂存 ----
        if (s.isStaged()) {
            auto* item = new QListWidgetItem(m_stagedList);
            item->setText(QStringLiteral("[%1] %2")
                              .arg(statusBadge(s.indexStatus), s.path));
            item->setForeground(statusColor(s.indexStatus));
            item->setToolTip(s.absolutePath);
            item->setData(Qt::UserRole,     s.path);
            item->setData(Qt::UserRole + 1, s.absolutePath);
            ++stagedCount;
        }

        // ---- 未暂存 / 未跟踪 ----
        if (s.hasUnstagedChanges() || s.isUntracked() || s.hasConflict()) {
            FileStatus show = s.hasConflict() ? FileStatus::Conflicted
                                              : s.workTreeStatus;
            auto* item = new QListWidgetItem(m_unstagedList);
            item->setText(QStringLiteral("[%1] %2")
                              .arg(statusBadge(show), s.path));
            item->setForeground(statusColor(show));
            item->setToolTip(s.absolutePath);
            item->setData(Qt::UserRole,     s.path);
            item->setData(Qt::UserRole + 1, s.absolutePath);
            ++unstagedCount;
        }
    }

    m_stagedHeader->setText(
        QStringLiteral("已暂存的更改 (%1)").arg(stagedCount));
    m_unstagedHeader->setText(
        QStringLiteral("更改 (%1)").arg(unstagedCount));

    m_unstageAllBtn->setEnabled(stagedCount > 0);
    m_stageAllBtn->setEnabled(unstagedCount > 0);

    updateCommitButton();
}

void GitPanel::populateBranchCombo()
{
    m_branchCombo->blockSignals(true);
    m_branchCombo->clear();

    auto& gm = GitManager::instance();
    const auto info = gm.repositoryInfo();

    if (!info.isValid) {
        m_branchCombo->addItem(QStringLiteral("(非 Git 仓库)"));
        m_branchCombo->setEnabled(false);
        m_branchCombo->blockSignals(false);
        return;
    }

    if (info.isEmpty) {
        m_branchCombo->addItem(QStringLiteral("(尚无提交)"));
        m_branchCombo->setEnabled(false);
        m_branchCombo->blockSignals(false);
        return;
    }

    m_branchCombo->setEnabled(true);

    const auto branches = gm.branches();
    int currentIdx = -1;
    for (const GitBranch& b : branches) {
        if (b.isRemote) continue;   // 分支下拉只放本地分支
        QString label = b.name;
        if (b.ahead > 0 || b.behind > 0) {
            label += QStringLiteral("  ");
            if (b.ahead  > 0) label += QStringLiteral("↑%1").arg(b.ahead);
            if (b.behind > 0) label += QStringLiteral("↓%1").arg(b.behind);
        }
        m_branchCombo->addItem(label, b.name);
        if (b.isCurrent) currentIdx = m_branchCombo->count() - 1;
    }

    if (currentIdx < 0 && info.isDetachedHead) {
        // detached HEAD：单独加一条
        m_branchCombo->insertItem(0,
                                  QStringLiteral("(detached: %1)").arg(info.currentBranch),
                                  QString());
        m_branchCombo->setCurrentIndex(0);
    } else if (currentIdx >= 0) {
        m_branchCombo->setCurrentIndex(currentIdx);
    }

    m_branchCombo->blockSignals(false);
}

void GitPanel::populateHistory()
{
    m_historyList->clear();

    auto& gm = GitManager::instance();
    if (!gm.isActive()) return;

    const auto commits = gm.log(80);
    for (const GitCommit& c : commits) {
        const QString when = c.timestamp.toString("MM-dd hh:mm");
        QString text = QStringLiteral("%1  %2  %3")
                           .arg(c.shortHash, when, c.message);
        auto* item = new QListWidgetItem(m_historyList);
        item->setText(text);
        item->setToolTip(QStringLiteral("%1\n%2 <%3>\n%4")
                             .arg(c.hash, c.author, c.email,
                                  c.timestamp.toString(Qt::ISODate)));
        item->setData(Qt::UserRole, c.hash);
    }
}

// ============================================================
// 状态切换
// ============================================================
void GitPanel::setBusy(bool busy)
{
    m_busyBar->setVisible(busy);
    m_refreshBtn->setEnabled(!busy);
    m_commitBtn->setEnabled(!busy && !m_messageEdit->toPlainText().trimmed().isEmpty()
                            && m_stagedList->count() > 0);
}

void GitPanel::setEmptyRepoState(bool emptyRepo)
{
    m_contentArea->setVisible(!emptyRepo);
    m_emptyRepoHint->setVisible(emptyRepo);
    m_initBtn->setVisible(false);
}

void GitPanel::updateCommitButton()
{
    const bool hasMessage = !m_messageEdit->toPlainText().trimmed().isEmpty();
    const bool hasStaged  = m_stagedList->count() > 0;
    const bool busy = m_busyBar->isVisible();

    m_commitBtn->setEnabled(hasMessage && hasStaged && !busy);

    if (hasStaged) {
        m_commitBtn->setText(
            QStringLiteral("提交 (%1)").arg(m_stagedList->count()));
    } else {
        m_commitBtn->setText(QStringLiteral("提交"));
    }
}

// ============================================================
// 槽：GitManager 信号
// ============================================================
void GitPanel::onRepositoryChanged(const GitRepositoryInfo& info)
{
    // 游离 HEAD 时给个视觉提醒
    if (info.isValid && info.isDetachedHead && m_branchCombo) {
        m_branchCombo->setStyleSheet(
            "QComboBox { color: #EBB65B; font-weight: bold; }");
        m_branchCombo->setToolTip(
            QStringLiteral("游离 HEAD（%1）\n"
                           "当前不在任何分支上。建议创建一个分支以保存提交。")
                .arg(info.currentBranch));
    } else if (m_branchCombo) {
        m_branchCombo->setStyleSheet(QString());
        m_branchCombo->setToolTip(QString());
    }
    // 仓库有效性
    if (info.isValid) {
        m_contentArea->setVisible(true);
        m_emptyRepoHint->setVisible(false);
        m_initBtn->setVisible(false);
    } else {
        m_contentArea->setVisible(false);
        m_emptyRepoHint->setVisible(true);
        // 有项目根但非仓库 → 显示「初始化仓库」按钮
        m_initBtn->setVisible(!m_projectRoot.isEmpty());
        m_emptyRepoHint->setText(
            m_projectRoot.isEmpty()
                ? QStringLiteral("未打开项目。")
                : QStringLiteral("此目录尚未初始化为 Git 仓库。\n\n"
                                 "点击上方「初始化仓库」按钮开始使用。"));
    }

    // 空仓库（无任何 commit）
    if (info.isValid && info.isEmpty) {
        m_stagedList->clear();
        m_unstagedList->clear();
        m_historyList->clear();
        m_historyList->addItem(QStringLiteral("(尚无提交)"));
    }

    populateBranchCombo();

    if (info.isValid && !info.isEmpty) {
        populateHistory();
    }
}

void GitPanel::onStatusUpdated(const QList<GitFileStatus>& status)
{
    populateStatusLists(status);
}

void GitPanel::onBusyChanged(bool busy)
{
    setBusy(busy);
}

void GitPanel::onCommandFailed(const QString& op, const QString& err)
{
    // 不弹模态框，防止刷屏；仅打印到调试
    qWarning() << "[GitPanel] command failed:" << op << err;
}

// ============================================================
// 槽：顶部栏
// ============================================================
void GitPanel::onRefreshClicked()
{
    GitManager::instance().refreshStatus();
    populateBranchCombo();
    populateHistory();
}

void GitPanel::onInitClicked()
{
    if (m_projectRoot.isEmpty()) return;

    const auto reply = QMessageBox::question(
        this, QStringLiteral("初始化 Git 仓库"),
        QStringLiteral("将在当前项目根目录执行 git init：\n\n%1\n\n继续？")
            .arg(m_projectRoot),
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    QString err;
    if (!GitManager::instance().initRepository(m_projectRoot, &err)) {
        QMessageBox::warning(this, QStringLiteral("初始化失败"),
                             QStringLiteral("无法初始化 Git 仓库：\n\n%1").arg(err));
        return;
    }
    // initRepository 内部已触发 setProjectRoot 刷新
}

void GitPanel::onBranchActivated(int index)
{
    if (index < 0) return;
    const QString name = m_branchCombo->itemData(index).toString();
    if (name.isEmpty()) return;

    auto& gm = GitManager::instance();
    const auto info = gm.repositoryInfo();

    // 已经是当前分支（且不是 detached HEAD）→ 什么都不做
    if (!info.isDetachedHead && name == info.currentBranch) return;

    // ---- 1. 统计脏文件数量 ----
    int dirtyCount = 0;
    int untrackedCount = 0;
    for (const auto& s : gm.currentStatus()) {
        if (s.isUntracked()) {
            ++untrackedCount;
            ++dirtyCount;
        } else if (s.isStaged() || s.hasUnstagedChanges() || s.hasConflict()) {
            ++dirtyCount;
        }
    }

    // ---- 2. 拼装风险提示 ----
    QStringList warnings;
    bool hasRisk = false;

    if (info.isDetachedHead) {
        hasRisk = true;
        warnings << QStringLiteral(
            "⚠ 当前处于 <b>游离 HEAD</b> 状态（%1）。<br>"
            "切走后，此处的提交将不属于任何分支，<b>可能难以找回</b>。")
            .arg(info.currentBranch.toHtmlEscaped());
    }
    if (dirtyCount > 0) {
        hasRisk = true;
        warnings << QStringLiteral(
            "⚠ 当前有 <b>%1</b> 个未提交的更改（含 %2 个未跟踪文件）。<br>"
            "切换分支时可能产生冲突或被合并到其他基线。")
            .arg(dirtyCount).arg(untrackedCount);
    }

    // ---- 3. 无风险 → 简单确认 ----
    if (!hasRisk) {
        const auto reply = QMessageBox::question(
            this, QStringLiteral("切换分支"),
            QStringLiteral("切换到分支：%1 ？").arg(name),
            QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            populateBranchCombo();
            return;
        }
        gm.checkoutBranch(name);
        return;
    }

    // ---- 4. 有风险 → 三按钮对话框 ----
    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("切换分支"));
    box.setIcon(QMessageBox::Warning);
    box.setTextFormat(Qt::RichText);
    box.setText(warnings.join(QStringLiteral("<br><br>"))
                + QStringLiteral("<br><br>确认切换到分支：<b>%1</b> ？")
                      .arg(name.toHtmlEscaped()));

    auto* cancelBtn = box.addButton(QStringLiteral("取消"), QMessageBox::RejectRole);
    auto* stashBtn  = box.addButton(QStringLiteral("先暂存(stash)再切换"),
                                    QMessageBox::ActionRole);
    auto* forceBtn  = box.addButton(QStringLiteral("仍然直接切换"),
                                    QMessageBox::DestructiveRole);
    box.setDefaultButton(cancelBtn);
    box.exec();

    auto* clicked = box.clickedButton();

    if (clicked == cancelBtn) {
        populateBranchCombo();
        return;
    }

    if (clicked == stashBtn) {
        // 尚未实现 GitManager::stash，先引导用户手动操作
        QMessageBox::information(
            this, QStringLiteral("手动暂存"),
            QStringLiteral(
                "自动 stash 尚未实现，请先在终端执行：\n\n"
                "    git stash push -u -m \"切换分支前暂存\"\n\n"
                "然后回来再次点击切换分支。\n\n"
                "切换成功后可用 <code>git stash pop</code> 恢复。"));
        populateBranchCombo();
        return;
    }

    // forceBtn：用户明确知情，直接切
    gm.checkoutBranch(name);
}

// ============================================================
// 槽：状态区
// ============================================================
void GitPanel::onStageAllClicked()
{
    GitManager::instance().stageAll();
}

void GitPanel::onUnstageAllClicked()
{
    GitManager::instance().unstageAll();
}

void GitPanel::onCommitClicked()
{
    const QString msg = m_messageEdit->toPlainText().trimmed();
    if (msg.isEmpty()) return;
    if (m_stagedList->count() == 0) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先暂存至少一个文件。"));
        return;
    }

    // 提交前缓存，成功后再清空
    GitManager::instance().commit(msg, m_amendCheck->isChecked());

    // 简单做法：直接清空输入框（失败也不影响，用户可重输）
    m_messageEdit->clear();
    m_amendCheck->setChecked(false);
}

void GitPanel::onStagedItemDoubleClicked(QListWidgetItem* item)
{
    if (!item) return;
    const QString abs = item->data(Qt::UserRole + 1).toString();
    if (!abs.isEmpty()) emit fileDoubleClicked(abs);
}

void GitPanel::onUnstagedItemDoubleClicked(QListWidgetItem* item)
{
    if (!item) return;
    const QString abs = item->data(Qt::UserRole + 1).toString();
    if (!abs.isEmpty()) emit fileDoubleClicked(abs);
}

void GitPanel::onStagedContextMenu(const QPoint& pos)
{
    auto* item = m_stagedList->itemAt(pos);
    if (!item) return;

    QMenu menu(this);
    menu.addAction(QStringLiteral("取消暂存"), this, [this, item]() {
        const QString rel = item->data(Qt::UserRole).toString();
        GitManager::instance().unstageFile(rel);
    });
    menu.addAction(QStringLiteral("打开文件"), this, [this, item]() {
        const QString abs = item->data(Qt::UserRole + 1).toString();
        if (!abs.isEmpty()) emit fileDoubleClicked(abs);
    });
    menu.exec(m_stagedList->viewport()->mapToGlobal(pos));
}

void GitPanel::onUnstagedContextMenu(const QPoint& pos)
{
    auto* item = m_unstagedList->itemAt(pos);
    if (!item) return;

    QMenu menu(this);
    menu.addAction(QStringLiteral("暂存此文件"), this, [this, item]() {
        const QString rel = item->data(Qt::UserRole).toString();
        GitManager::instance().stageFile(rel);
    });
    menu.addAction(QStringLiteral("打开文件"), this, [this, item]() {
        const QString abs = item->data(Qt::UserRole + 1).toString();
        if (!abs.isEmpty()) emit fileDoubleClicked(abs);
    });
    menu.exec(m_unstagedList->viewport()->mapToGlobal(pos));
}

void GitPanel::onHistoryItemDoubleClicked(QListWidgetItem* item)
{
    if (!item) return;
    const QString hash = item->data(Qt::UserRole).toString();
    if (hash.isEmpty()) return;
    // 预留：未来弹出 diff 窗口
    Q_UNUSED(hash);
}

} // namespace Git
} // namespace SprintIDE
