#include "git_panel.h"
#include "git_manager.h"
#include "git_diff_dialog.h"

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
#include <QAction>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QInputDialog>
#include <QLineEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
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
    connect(&gm, &GitManager::stashListUpdated,
            this, &GitPanel::onStashListUpdated);

    // commit 成功后立即刷新分支/历史
    //   refreshStatus() 只发 statusUpdated，不会触发 populateHistory()
    //   必须单独监听 commitSucceeded 才能让历史列表实时更新
    connect(&gm, &GitManager::commitSucceeded,
            this, [this](const QString&) {
                populateBranchCombo();
                populateHistory();
            });

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

    // ---- 操作状态横幅（merge / rebase 进行中） ----
    m_operationBanner = new QWidget(this);
    m_operationBanner->setObjectName("GitOperationBanner");
    m_operationBanner->setStyleSheet(
        "QWidget#GitOperationBanner {"
        "  background-color: #3A2E1A;"
        "  border-bottom: 1px solid #EBB65B;"
        "}");
    {
        auto* bl = new QHBoxLayout(m_operationBanner);
        bl->setContentsMargins(8, 6, 8, 6);
        bl->setSpacing(6);

        m_operationLabel = new QLabel(m_operationBanner);
        m_operationLabel->setStyleSheet(
            "QLabel { color: #EBB65B; font-weight: bold; }");

        m_operationContinueBtn = new QPushButton(
            QStringLiteral("继续"), m_operationBanner);
        m_operationContinueBtn->setToolTip(
            QStringLiteral("冲突全部解决后继续当前操作"));
        connect(m_operationContinueBtn, &QPushButton::clicked,
                this, &GitPanel::onContinueOperation);

        m_operationAbortBtn = new QPushButton(
            QStringLiteral("中止"), m_operationBanner);
        connect(m_operationAbortBtn, &QPushButton::clicked,
                this, &GitPanel::onAbortOperation);

        bl->addWidget(m_operationLabel, 1);
        bl->addWidget(m_operationContinueBtn);
        bl->addWidget(m_operationAbortBtn);
    }
    m_operationBanner->setVisible(false);
    root->addWidget(m_operationBanner);

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
    m_splitter->addWidget(createStashSection());
    m_splitter->addWidget(createHistorySection());

    m_splitter->setStretchFactor(0, 5);
    m_splitter->setStretchFactor(1, 0);
    m_splitter->setStretchFactor(2, 0);
    m_splitter->setStretchFactor(3, 4);

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

    // ---- 分支操作菜单按钮 ----
    m_branchMenuBtn = new QToolButton(bar);
    m_branchMenuBtn->setText(QStringLiteral("⋮"));
    m_branchMenuBtn->setToolTip(QStringLiteral("分支操作"));
    m_branchMenuBtn->setPopupMode(QToolButton::InstantPopup);

    {
        auto* menu = new QMenu(m_branchMenuBtn);

        QAction* newBranchAct = menu->addAction(QStringLiteral("新建分支..."));
        connect(newBranchAct, &QAction::triggered,
                this, &GitPanel::onNewBranchClicked);

        menu->addSeparator();

        QAction* mergeAct = menu->addAction(QStringLiteral("合并分支到当前分支..."));
        connect(mergeAct, &QAction::triggered,
                this, &GitPanel::onMergeIntoCurrent);

        QAction* rebaseAct = menu->addAction(QStringLiteral("将当前分支 Rebase 到..."));
        connect(rebaseAct, &QAction::triggered,
                this, &GitPanel::onRebaseOnto);

        m_branchMenuBtn->setMenu(menu);
    }

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
    lay->addWidget(m_branchMenuBtn);
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

QWidget* GitPanel::createStashSection()
{
    auto* w = new QWidget(this);
    auto* lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    // ---- 标题栏 ----
    auto* bar = new QWidget(w);
    auto* hl = new QHBoxLayout(bar);
    hl->setContentsMargins(6, 4, 6, 2);

    m_stashHeader = new QLabel(QStringLiteral("Stash (0)"), bar);
    m_stashHeader->setStyleSheet("font-weight: bold;");

    m_stashBtn = new QToolButton(bar);
    m_stashBtn->setText(QStringLiteral("+"));
    m_stashBtn->setToolTip(QStringLiteral("创建 stash（暂存当前改动）"));
    connect(m_stashBtn, &QToolButton::clicked,
            this, &GitPanel::onStashClicked);

    hl->addWidget(m_stashHeader, 1);
    hl->addWidget(m_stashBtn);
    lay->addWidget(bar);

    // ---- 列表 ----
    m_stashList = new QListWidget(w);
    m_stashList->setObjectName("GitStashList");
    m_stashList->setAlternatingRowColors(false);
    m_stashList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_stashList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_stashList->setMaximumHeight(150);
    connect(m_stashList, &QListWidget::itemDoubleClicked,
            this, &GitPanel::onStashItemDoubleClicked);
    connect(m_stashList, &QListWidget::customContextMenuRequested,
            this, &GitPanel::onStashContextMenu);
    lay->addWidget(m_stashList);

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
            item->setToolTip(s.hasConflict()
                                 ? QStringLiteral("%1\n（存在冲突）").arg(s.absolutePath)
                                 : s.absolutePath);
            item->setData(Qt::UserRole,     s.path);
            item->setData(Qt::UserRole + 1, s.absolutePath);
            // UserRole + 2：是否冲突，供右键菜单判断
            item->setData(Qt::UserRole + 2, s.hasConflict());
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
    if (m_branchMenuBtn) {
        m_branchMenuBtn->setEnabled(!busy && !m_projectRoot.isEmpty());
    }
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
    // ---- 操作状态横幅 ----
    if (m_operationBanner) {
        if (info.isValid && info.hasOperationInProgress()) {
            const QString opName = gitOperationName(info.operation);
            m_operationLabel->setText(
                QStringLiteral("⚠ 正在进行：%1").arg(opName));

            // 只有 rebase 支持"继续"；merge 冲突解决后直接 commit
            m_operationContinueBtn->setVisible(
                info.operation == GitOperation::Rebase);

            m_operationBanner->setVisible(true);
        } else {
            m_operationBanner->setVisible(false);
        }
    }

    // 仓库有效性
    if (info.isValid) {
        m_contentArea->setVisible(true);
        m_emptyRepoHint->setVisible(false);
        m_initBtn->setVisible(false);
        if (m_branchMenuBtn) m_branchMenuBtn->setEnabled(true);
    } else {
        if (m_branchMenuBtn) m_branchMenuBtn->setEnabled(false);
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
void GitPanel::refreshAll()
{
    auto& gm = GitManager::instance();
    gm.refreshStatus();               // 异步：稍后触发 statusUpdated
    populateBranchCombo();            // 同步
    populateHistory();                // 同步
    onStashListUpdated(gm.stashList());// 同步
}

void GitPanel::onRefreshClicked()
{
    refreshAll();
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
        // 自动 stash → 切换；GitManager 内部会在 stash 完成后 checkout
        gm.stashPush(
            QStringLiteral("切换分支 %1 → %2 前自动暂存")
                .arg(info.currentBranch.isEmpty()
                         ? QStringLiteral("(detached)")
                         : info.currentBranch,
                     name),
            /*includeUntracked=*/true,
            /*thenCheckout=*/name);
        populateBranchCombo();
        return;
    }

    // forceBtn：用户明确知情，直接切
    gm.checkoutBranch(name);
}

// ============================================================
// 新建分支
// ============================================================
void GitPanel::onNewBranchClicked()
{
    auto& gm = GitManager::instance();
    if (!gm.isActive()) return;

    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("新建分支"));
    dlg.setMinimumWidth(420);

    auto* root = new QVBoxLayout(&dlg);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(10);

    auto* form = new QFormLayout;
    form->setSpacing(8);

    // ---- 分支名 ----
    auto* nameEdit = new QLineEdit(&dlg);
    nameEdit->setPlaceholderText(QStringLiteral("例如 feature/diff-window"));
    form->addRow(QStringLiteral("分支名:"), nameEdit);

    // ---- 基点 ----
    auto* baseCombo = new QComboBox(&dlg);
    const auto info = gm.repositoryInfo();

    QString headLabel;
    if (info.isDetachedHead) {
        headLabel = QStringLiteral("(detached: %1)").arg(info.currentBranch);
    } else if (info.isEmpty) {
        headLabel = QStringLiteral("(尚无提交)");
    } else {
        headLabel = info.currentBranch;
    }
    baseCombo->addItem(QStringLiteral("当前 HEAD（%1）").arg(headLabel),
                       QString());   // data 为空 → 使用默认（当前 HEAD）

    for (const auto& b : gm.branches()) {
        if (b.isRemote) continue;
        if (b.isCurrent) continue;
        baseCombo->addItem(b.name, b.name);
    }
    form->addRow(QStringLiteral("基点:"), baseCombo);

    root->addLayout(form);

    // ---- 立即切换 ----
    auto* checkoutCheck = new QCheckBox(QStringLiteral("创建后立即切换到新分支"), &dlg);
    checkoutCheck->setChecked(true);
    root->addWidget(checkoutCheck);

    // ---- 按钮 ----
    auto* btnBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    btnBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("创建"));
    root->addWidget(btnBox);

    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    // 默认焦点到分支名输入框
    nameEdit->setFocus();

    if (dlg.exec() != QDialog::Accepted) return;

    // ---- 校验 ----
    const QString name = nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("新建分支"),
                             QStringLiteral("分支名不能为空。"));
        return;
    }
    // Git 分支名规则（简化版）：
    //   不能有空格、不能以 - 或 . 开头、不能有连续 ..、不能以 .lock 结尾、
    //   不能包含 ~ ^ : ? * [ \ 和控制字符
    auto isInvalidName = [](const QString& n) {
        if (n.startsWith(QLatin1Char('-')) || n.startsWith(QLatin1Char('.'))
            || n.startsWith(QLatin1Char('/')))
            return true;
        if (n.contains(QLatin1Char(' '))
            || n.contains(QStringLiteral(".."))
            || n.contains(QLatin1Char('~'))
            || n.contains(QLatin1Char('^'))
            || n.contains(QLatin1Char(':'))
            || n.contains(QLatin1Char('?'))
            || n.contains(QLatin1Char('*'))
            || n.contains(QLatin1Char('['))
            || n.contains(QLatin1Char('\\')))
            return true;
        if (n.endsWith(QLatin1Char('/'))
            || n.endsWith(QStringLiteral(".lock"))
            || n.endsWith(QLatin1Char('.')))
            return true;
        return false;
    };

    if (isInvalidName(name)) {
        QMessageBox::warning(this, QStringLiteral("新建分支"),
                             QStringLiteral(
                                 "分支名包含非法字符。<br><br>"
                                 "不允许：<code>空格 ~ ^ : ? * [ \\ .. </code>，<br>"
                                 "不允许以 <code>- . /</code> 开头，<br>"
                                 "不允许以 <code>.lock</code> 或 <code>/</code> 结尾。"));
        return;
    }

    // 本地已存在同名分支
    for (const auto& b : gm.branches()) {
        if (!b.isRemote && b.name == name) {
            QMessageBox::warning(this, QStringLiteral("新建分支"),
                                 QStringLiteral("分支 <b>%1</b> 已存在。").arg(name.toHtmlEscaped()));
            return;
        }
    }

    // ---- 执行 ----
    const QString base = baseCombo->currentData().toString();
    if (checkoutCheck->isChecked()) {
        gm.createBranchAndCheckout(name, base);
    } else {
        gm.createBranch(name, base);
    }
    dlg.accept();
}

// ============================================================
// 合并 / Rebase
// ============================================================

// 弹出分支选择对话框，返回选中的本地分支名（或空表示取消）
// excludeCurrent=true 时不列出当前分支
static QString pickBranch(QWidget* parent,
                          GitManager& gm,
                          const QString& title,
                          const QString& prompt)
{
    const auto info = gm.repositoryInfo();
    const QString current = info.currentBranch;

    QDialog dlg(parent);
    dlg.setWindowTitle(title);
    dlg.setMinimumWidth(400);

    auto* root = new QVBoxLayout(&dlg);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(10);

    auto* promptLbl = new QLabel(prompt, &dlg);
    promptLbl->setWordWrap(true);
    root->addWidget(promptLbl);

    auto* combo = new QComboBox(&dlg);
    combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    for (const auto& b : gm.branches()) {
        if (b.isRemote) continue;
        if (b.name == current) continue;
        combo->addItem(b.name, b.name);
    }
    root->addWidget(combo);

    if (combo->count() == 0) {
        QMessageBox::information(parent, title,
                                 QStringLiteral("没有其他可选的本地分支。"));
        return QString();
    }

    auto* bb = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    bb->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    QObject::connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    root->addWidget(bb);

    if (dlg.exec() != QDialog::Accepted) return QString();
    return combo->currentData().toString();
}

void GitPanel::onMergeIntoCurrent()
{
    auto& gm = GitManager::instance();
    if (!gm.isActive()) return;

    if (gm.repositoryInfo().hasOperationInProgress()) {
        QMessageBox::information(this, QStringLiteral("合并"),
                                 QStringLiteral("已有进行中的操作：%1，请先处理。")
                                     .arg(gitOperationName(gm.repositoryInfo().operation)));
        return;
    }

    const QString branch = pickBranch(
        this, gm,
        QStringLiteral("合并分支"),
        QStringLiteral("选择要合并到当前分支 <b>%1</b> 的分支：")
            .arg(gm.repositoryInfo().currentBranch.toHtmlEscaped()));

    if (branch.isEmpty()) return;

    const auto reply = QMessageBox::question(
        this, QStringLiteral("确认合并"),
        QStringLiteral("将把 <b>%1</b> 合并到 <b>%2</b>。<br><br>继续？")
            .arg(branch.toHtmlEscaped(),
                 gm.repositoryInfo().currentBranch.toHtmlEscaped()));
    if (reply != QMessageBox::Yes) return;

    gm.merge(branch);
}

void GitPanel::onRebaseOnto()
{
    auto& gm = GitManager::instance();
    if (!gm.isActive()) return;

    if (gm.repositoryInfo().hasOperationInProgress()) {
        QMessageBox::information(this, QStringLiteral("Rebase"),
                                 QStringLiteral("已有进行中的操作：%1，请先处理。")
                                     .arg(gitOperationName(gm.repositoryInfo().operation)));
        return;
    }

    // 有未提交更改时 rebase 会被拒绝，先提示
    int dirtyCount = 0;
    for (const auto& s : gm.currentStatus()) {
        if (s.isStaged() || s.hasUnstagedChanges()) ++dirtyCount;
    }
    if (dirtyCount > 0) {
        QMessageBox::warning(this, QStringLiteral("Rebase"),
                             QStringLiteral("工作区有 <b>%1</b> 个未提交的更改。<br><br>"
                                            "请先提交或 stash，再执行 rebase。")
                                 .arg(dirtyCount));
        return;
    }

    const QString branch = pickBranch(
        this, gm,
        QStringLiteral("Rebase"),
        QStringLiteral("把当前分支 <b>%1</b> rebase 到：")
            .arg(gm.repositoryInfo().currentBranch.toHtmlEscaped()));

    if (branch.isEmpty()) return;

    // 强警告
    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("确认 Rebase"));
    box.setIcon(QMessageBox::Warning);
    box.setTextFormat(Qt::RichText);
    box.setText(
        QStringLiteral(
            "将把当前分支 <b>%1</b> 的所有提交重放到 <b>%2</b> 之上。<br><br>"
            "⚠ Rebase 会重写当前分支的提交历史。<br>"
            "⚠ 如果该分支已推送到远程，之后需要 <code>git push --force</code>。<br>"
            "⚠ 冲突时 ours/theirs 语义与 merge 相反。<br><br>"
            "确定继续？")
            .arg(gm.repositoryInfo().currentBranch.toHtmlEscaped(),
                 branch.toHtmlEscaped()));

    auto* okBtn     = box.addButton(QStringLiteral("开始 Rebase"),
                                QMessageBox::AcceptRole);
    auto* cancelBtn = box.addButton(QStringLiteral("取消"), QMessageBox::RejectRole);
    box.setDefaultButton(cancelBtn);
    box.exec();

    if (box.clickedButton() != okBtn) return;

    gm.rebase(branch);
}

void GitPanel::onAbortOperation()
{
    auto& gm = GitManager::instance();
    const auto info = gm.repositoryInfo();
    if (!info.hasOperationInProgress()) return;

    const QString opName = gitOperationName(info.operation);

    const auto reply = QMessageBox::question(
        this, QStringLiteral("中止操作"),
        QStringLiteral("确定要中止当前的 <b>%1</b> 吗？<br><br>"
                       "工作区里已解决的改动会保留，未提交的改动不会丢失。")
            .arg(opName));
    if (reply != QMessageBox::Yes) return;

    if (info.operation == GitOperation::Merge) {
        gm.abortMerge();
    } else if (info.operation == GitOperation::Rebase) {
        gm.abortRebase();
    } else {
        QMessageBox::information(this, QStringLiteral("中止操作"),
                                 QStringLiteral("暂不支持中止 %1。").arg(opName));
    }
}

void GitPanel::onContinueOperation()
{
    auto& gm = GitManager::instance();
    const auto info = gm.repositoryInfo();

    if (info.operation != GitOperation::Rebase) return;

    // 检查是否还有冲突文件
    bool hasConflict = false;
    for (const auto& s : gm.currentStatus()) {
        if (s.hasConflict()) { hasConflict = true; break; }
    }
    if (hasConflict) {
        QMessageBox::information(this, QStringLiteral("继续 Rebase"),
                                 QStringLiteral("仍有未解决的冲突文件，请先在「更改」列表里全部解决。"));
        return;
    }

    gm.continueRebase();
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
    auto& gm = GitManager::instance();

    // ============================================================
    // 提交前检查 1：Git 身份是否已配置
    // ------------------------------------------------------------
    // git commit 在没有 user.name / user.email 时会失败，
    // 但报错文案不友好（"Please tell me who you are"）。
    // 提前拦截，给出可操作的中文提示。
    // ============================================================
    if (gm.userName().isEmpty() || gm.userEmail().isEmpty()) {
        QMessageBox box(this);
        box.setWindowTitle(QStringLiteral("Git 身份未配置"));
        box.setIcon(QMessageBox::Warning);
        box.setTextFormat(Qt::RichText);
        box.setText(
            QStringLiteral(
                "提交前需要先配置 Git 身份。<br><br>"
                "在终端运行以下命令：<br>"
                "<code>git config --global user.name \"你的名字\"</code><br>"
                "<code>git config --global user.email \"你的邮箱\"</code><br><br>"
                "或打开设置对话框，在 <b>Git</b> 页填写并保存。"));

        auto* openBtn = box.addButton(QStringLiteral("打开设置"),
                                      QMessageBox::AcceptRole);
        box.addButton(QStringLiteral("取消"), QMessageBox::RejectRole);
        box.exec();

        if (box.clickedButton() == openBtn) {
            emit openSettingsRequested();
        }
        return;
    }

    // ============================================================
    // 提交前检查 2：消息 + 暂存区
    // ============================================================
    const QString msg = m_messageEdit->toPlainText().trimmed();
    if (msg.isEmpty()) return;
    if (m_stagedList->count() == 0) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先暂存至少一个文件。"));
        return;
    }

    // 通过检查 → 提交
    gm.commit(msg, m_amendCheck->isChecked());

    m_messageEdit->clear();
    m_amendCheck->setChecked(false);
}

void GitPanel::onStagedItemDoubleClicked(QListWidgetItem* item)
{
    if (!item) return;
    const QString rel = item->data(Qt::UserRole).toString();
    const QString abs = item->data(Qt::UserRole + 1).toString();

    // 双击已暂存文件 → 显示暂存区 vs HEAD 的 diff
    if (rel.isEmpty()) return;

    auto& gm = GitManager::instance();
    const QString diffText = gm.rawDiff(rel, /*staged=*/true);

    auto* dlg = new GitDiffDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    connect(dlg, &GitDiffDialog::openFileRequested,
            this, [this](const QString& p) { emit fileDoubleClicked(p); });
    dlg->showFileDiff(
        QStringLiteral("已暂存: %1").arg(rel),
        diffText,
        abs);
}

void GitPanel::onUnstagedItemDoubleClicked(QListWidgetItem* item)
{
    if (!item) return;
    const QString rel = item->data(Qt::UserRole).toString();
    const QString abs = item->data(Qt::UserRole + 1).toString();

    // 双击未暂存文件 → 显示工作区 vs 暂存区的 diff
    if (rel.isEmpty()) return;

    auto& gm = GitManager::instance();
    const QString diffText = gm.rawDiff(rel, /*staged=*/false);

    auto* dlg = new GitDiffDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    connect(dlg, &GitDiffDialog::openFileRequested,
            this, [this](const QString& p) { emit fileDoubleClicked(p); });
    dlg->showFileDiff(
        QStringLiteral("工作区: %1").arg(rel),
        diffText,
        abs);
}

void GitPanel::onStagedContextMenu(const QPoint& pos)
{
    auto* item = m_stagedList->itemAt(pos);
    if (!item) return;

    QMenu menu(this);
    menu.addAction(QStringLiteral("查看差异"), this, [this, item]() {
        onStagedItemDoubleClicked(item);
    });
    menu.addSeparator();
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

    const bool    isConflict = item->data(Qt::UserRole + 2).toBool();
    const QString rel        = item->data(Qt::UserRole).toString();
    const QString abs        = item->data(Qt::UserRole + 1).toString();

    QMenu menu(this);
    menu.addAction(QStringLiteral("查看差异"), this, [this, item]() {
        onUnstagedItemDoubleClicked(item);
    });
    menu.addSeparator();

    // ---------- 冲突解决（仅当文件状态为冲突时显示） ----------
    if (isConflict) {
        menu.addAction(QStringLiteral("打开编辑器手动解决"), this, [this, abs]() {
            if (!abs.isEmpty()) emit fileDoubleClicked(abs);
        });
        menu.addSeparator();

        menu.addAction(QStringLiteral("接受本地版本（ours）"), this, [this, rel]() {
            GitManager::instance().acceptOurs(rel);
        });
        menu.addAction(QStringLiteral("接受远程版本（theirs）"), this, [this, rel]() {
            GitManager::instance().acceptTheirs(rel);
        });
        menu.addAction(QStringLiteral("标记为已解决（git add）"), this, [this, rel]() {
            GitManager::instance().markResolved(rel);
        });
        menu.addSeparator();
    }

    // ---------- 常规操作 ----------
    menu.addAction(QStringLiteral("暂存此文件"), this, [this, rel]() {
        GitManager::instance().stageFile(rel);
    });
    menu.addAction(QStringLiteral("打开文件"), this, [this, abs]() {
        if (!abs.isEmpty()) emit fileDoubleClicked(abs);
    });

    menu.addSeparator();

    // ---------- 丢弃更改 ----------
    menu.addAction(QStringLiteral("丢弃更改…"), this, [this, rel, abs, isConflict]() {
        Q_UNUSED(abs);
        // 冲突文件不允许直接丢弃（会造成状态混乱）
        if (isConflict) {
            QMessageBox::information(
                this, QStringLiteral("丢弃更改"),
                QStringLiteral("冲突文件不能直接丢弃，请先用「接受本地 / 接受远程 / 手动解决」处理。"));
            return;
        }

        auto& gm = GitManager::instance();
        const bool isUntracked = [&]() {
            for (const auto& s : gm.currentStatus()) {
                if (s.path == rel) return s.isUntracked() && !s.isStaged();
            }
            return false;
        }();

        const QString title = isUntracked
                                  ? QStringLiteral("删除文件")
                                  : QStringLiteral("丢弃更改");
        const QString question = isUntracked
                                     ? QStringLiteral("确定要删除以下未跟踪文件吗？<br><br>"
                                                      "<code>%1</code><br><br>此操作不可恢复。")
                                           .arg(rel.toHtmlEscaped())
                                     : QStringLiteral("确定要丢弃对该文件的所有未提交更改吗？<br><br>"
                                                      "<code>%1</code><br><br>此操作不可恢复。")
                                           .arg(rel.toHtmlEscaped());

        QMessageBox box(this);
        box.setWindowTitle(title);
        box.setIcon(QMessageBox::Warning);
        box.setTextFormat(Qt::RichText);
        box.setText(question);

        const QString okText = isUntracked
                                   ? QStringLiteral("删除")
                                   : QStringLiteral("丢弃更改");
        auto* okBtn     = box.addButton(okText, QMessageBox::AcceptRole);
        auto* cancelBtn = box.addButton(QStringLiteral("取消"), QMessageBox::RejectRole);
        box.setDefaultButton(cancelBtn);
        box.exec();

        if (box.clickedButton() == okBtn) {
            gm.discardFileChanges(rel);
        }
    });

    menu.exec(m_unstagedList->viewport()->mapToGlobal(pos));
}

void GitPanel::onHistoryItemDoubleClicked(QListWidgetItem* item)
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

    // 标题：shortHash + message 首行
    const QString shortHash = hash.left(8);
    QString firstLine;
    const auto commits = gm.log(1, hash);
    if (!commits.isEmpty()) {
        firstLine = commits.first().message;
        // 只取首行
        const int nl = firstLine.indexOf('\n');
        if (nl >= 0) firstLine = firstLine.left(nl);
    }

    auto* dlg = new GitDiffDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    dlg->showCommitDiff(
        QStringLiteral("%1  %2").arg(shortHash, firstLine),
        diffText);
}

// ============================================================
// Stash
// ============================================================

void GitPanel::onStashListUpdated(const QList<GitStash>& stashes)
{
    if (!m_stashList) return;

    m_stashList->clear();
    m_stashHeader->setText(QStringLiteral("Stash (%1)").arg(stashes.size()));

    for (const GitStash& s : stashes) {
        const QString when = s.timestamp.toString("MM-dd hh:mm");
        QString label = QStringLiteral("%1  %2").arg(when, s.message);
        if (label.length() > 80) label = label.left(77) + QStringLiteral("...");

        auto* item = new QListWidgetItem(m_stashList);
        item->setText(label);
        item->setData(Qt::UserRole, s.index);
        item->setToolTip(QStringLiteral("%1\n分支: %2\n消息: %3\n时间: %4")
                             .arg(s.ref,
                                  s.branch.isEmpty() ? QStringLiteral("(未知)") : s.branch,
                                  s.message,
                                  s.timestamp.toString(Qt::ISODate)));
    }

    m_stashBtn->setEnabled(m_projectRoot.size() > 0);
}

void GitPanel::onStashClicked()
{
    auto& gm = GitManager::instance();
    if (!gm.isActive()) return;

    bool ok = false;
    const QString msg = QInputDialog::getText(
        this,
        QStringLiteral("创建 Stash"),
        QStringLiteral("Stash 消息（可留空，默认 WIP）:"),
        QLineEdit::Normal, QString(), &ok);
    if (!ok) return;

    gm.stashPush(msg.isEmpty() ? QStringLiteral("WIP") : msg,
                 /*includeUntracked=*/true);
}

void GitPanel::onStashItemDoubleClicked(QListWidgetItem* item)
{
    if (!item) return;
    const int idx = item->data(Qt::UserRole).toInt();

    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("Stash 操作"));
    box.setText(QStringLiteral("对 %1 执行什么操作？").arg(item->text()));
    auto* popBtn   = box.addButton(QStringLiteral("应用并删除 (pop)"),
                                 QMessageBox::AcceptRole);
    auto* applyBtn = box.addButton(QStringLiteral("仅应用 (apply)"),
                                   QMessageBox::ActionRole);
    box.addButton(QStringLiteral("取消"), QMessageBox::RejectRole);
    box.exec();

    auto* c = box.clickedButton();
    if (c == popBtn)         GitManager::instance().stashPop(idx);
    else if (c == applyBtn)  GitManager::instance().stashApply(idx);
}

void GitPanel::onStashContextMenu(const QPoint& pos)
{
    auto* item = m_stashList->itemAt(pos);

    // 空白处右键 → 清空所有 stash
    if (!item) {
        QMenu menu(this);
        QAction* clearAll = menu.addAction(QStringLiteral("清空所有 Stash"));
        clearAll->setEnabled(m_stashList->count() > 0);
        connect(clearAll, &QAction::triggered, this, [this]() {
            if (QMessageBox::question(
                    this, QStringLiteral("清空 Stash"),
                    QStringLiteral("确定要清空所有 stash 吗？此操作不可恢复。"))
                == QMessageBox::Yes) {
                GitManager::instance().stashClear();
            }
        });
        menu.exec(m_stashList->viewport()->mapToGlobal(pos));
        return;
    }

    const int idx = item->data(Qt::UserRole).toInt();

    QMenu menu(this);
    connect(menu.addAction(QStringLiteral("应用并删除 (pop)")),
            &QAction::triggered, this, [idx]() {
                GitManager::instance().stashPop(idx);
            });
    connect(menu.addAction(QStringLiteral("仅应用 (apply)")),
            &QAction::triggered, this, [idx]() {
                GitManager::instance().stashApply(idx);
            });
    menu.addSeparator();
    connect(menu.addAction(QStringLiteral("删除此 Stash")),
            &QAction::triggered, this, [this, idx]() {
                if (QMessageBox::question(
                        this, QStringLiteral("删除 Stash"),
                        QStringLiteral("确定要删除此 stash 吗？此操作不可恢复。"))
                    == QMessageBox::Yes) {
                    GitManager::instance().stashDrop(idx);
                }
            });
    menu.exec(m_stashList->viewport()->mapToGlobal(pos));
}

} // namespace Git
} // namespace SprintIDE
