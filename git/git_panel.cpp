#include "git_panel.h"
#include "git_manager.h"
#include "git_diff_dialog.h"
#include "theme_manager.h"

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
#include <QIcon>
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
    {
        auto* bl = new QHBoxLayout(m_operationBanner);
        bl->setContentsMargins(8, 6, 8, 6);
        bl->setSpacing(6);

        m_operationLabel = new QLabel(m_operationBanner);

        m_operationContinueBtn = new QPushButton(
            tr("继续"), m_operationBanner);
        m_operationContinueBtn->setToolTip(
            tr("冲突全部解决后继续当前操作"));
        connect(m_operationContinueBtn, &QPushButton::clicked,
                this, &GitPanel::onContinueOperation);

        m_operationAbortBtn = new QPushButton(
            tr("中止"), m_operationBanner);
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
        tr("此目录尚未初始化为 Git 仓库。\n\n点击上方「初始化仓库」按钮开始使用。"));
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
    m_busyBar->setObjectName("GitBusyBar");
    m_busyBar->setRange(0, 0);          // 不确定进度
    m_busyBar->setMaximumHeight(4);
    m_busyBar->setTextVisible(false);
    m_busyBar->setVisible(false);
    root->addWidget(m_busyBar);
}

QWidget* GitPanel::createTopBar()
{
    auto* bar = new QWidget(this);
    auto* lay = new QHBoxLayout(bar);
    lay->setContentsMargins(8, 6, 8, 6);
    lay->setSpacing(4);

    // ---- 分支下拉 ----
    m_branchCombo = new QComboBox(bar);
    m_branchCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_branchCombo->setMinimumWidth(140);
    m_branchCombo->setIconSize(QSize(14, 14));
    m_branchCombo->setEditable(false);
    connect(m_branchCombo, QOverload<int>::of(&QComboBox::activated),
            this, &GitPanel::onBranchActivated);

    // ---- 分支操作菜单按钮（⋮）----
    m_branchMenuBtn = new QToolButton(bar);
    m_branchMenuBtn->setText(QStringLiteral("⋮"));
    m_branchMenuBtn->setToolTip(tr("分支操作"));
    m_branchMenuBtn->setPopupMode(QToolButton::InstantPopup);
    m_branchMenuBtn->setCursor(Qt::PointingHandCursor);
    m_branchMenuBtn->setFixedSize(28, 28);

    {
        auto* menu = new QMenu(m_branchMenuBtn);

        QAction* newBranchAct = menu->addAction(
            QIcon(QStringLiteral(":/icons/icons/git_branch.svg")),
            tr("新建分支..."));
        connect(newBranchAct, &QAction::triggered,
                this, &GitPanel::onNewBranchClicked);

        menu->addSeparator();

        QAction* mergeAct = menu->addAction(tr("合并分支到当前分支..."));
        connect(mergeAct, &QAction::triggered,
                this, &GitPanel::onMergeIntoCurrent);

        QAction* rebaseAct = menu->addAction(tr("将当前分支 Rebase 到..."));
        connect(rebaseAct, &QAction::triggered,
                this, &GitPanel::onRebaseOnto);

        m_branchMenuBtn->setMenu(menu);
    }

    // ---- 刷新 ----
    m_refreshBtn = new QToolButton(bar);
    m_refreshBtn->setIcon(QIcon(QStringLiteral(":/icons/icons/git_refresh.svg")));
    m_refreshBtn->setIconSize(QSize(16, 16));
    m_refreshBtn->setToolTip(tr("刷新状态 (F5)"));
    m_refreshBtn->setCursor(Qt::PointingHandCursor);
    m_refreshBtn->setFixedSize(28, 28);
    connect(m_refreshBtn, &QToolButton::clicked,
            this, &GitPanel::onRefreshClicked);

    // ---- 初始化仓库 ----
    m_initBtn = new QToolButton(bar);
    m_initBtn->setIcon(QIcon(QStringLiteral(":/icons/icons/git_init.svg")));
    m_initBtn->setIconSize(QSize(16, 16));
    m_initBtn->setText(tr(" 初始化仓库"));
    m_initBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_initBtn->setToolTip(tr("在当前项目根目录执行 git init"));
    m_initBtn->setCursor(Qt::PointingHandCursor);
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

    // ============================================================
    // 已暂存
    // ============================================================
    auto* stagedBar = new QWidget(w);
    auto* sl = new QHBoxLayout(stagedBar);
    sl->setContentsMargins(8, 6, 6, 2);

    m_stagedHeader = new QLabel(tr("已暂存的更改 (0)"), stagedBar);
    QFont headerFont = m_stagedHeader->font();
    headerFont.setBold(true);
    headerFont.setPointSizeF(headerFont.pointSizeF() - 1.0);
    m_stagedHeader->setFont(headerFont);

    m_unstageAllBtn = new QToolButton(stagedBar);
    m_unstageAllBtn->setText(QStringLiteral("−"));
    m_unstageAllBtn->setToolTip(tr("全部取消暂存"));
    m_unstageAllBtn->setCursor(Qt::PointingHandCursor);
    m_unstageAllBtn->setFixedSize(22, 22);
    connect(m_unstageAllBtn, &QToolButton::clicked,
            this, &GitPanel::onUnstageAllClicked);

    sl->addWidget(m_stagedHeader, 1);
    sl->addWidget(m_unstageAllBtn);
    lay->addWidget(stagedBar);

    m_stagedList = new QListWidget(w);
    m_stagedList->setAlternatingRowColors(false);
    m_stagedList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_stagedList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_stagedList->setIconSize(QSize(14, 14));
    m_stagedList->setSpacing(1);
    connect(m_stagedList, &QListWidget::itemDoubleClicked,
            this, &GitPanel::onStagedItemDoubleClicked);
    connect(m_stagedList, &QListWidget::customContextMenuRequested,
            this, &GitPanel::onStagedContextMenu);
    lay->addWidget(m_stagedList, 1);

    // ============================================================
    // 未暂存 / 更改
    // ============================================================
    auto* unstagedBar = new QWidget(w);
    auto* ul = new QHBoxLayout(unstagedBar);
    ul->setContentsMargins(8, 6, 6, 2);

    m_unstagedHeader = new QLabel(tr("更改 (0)"), unstagedBar);
    m_unstagedHeader->setFont(headerFont);

    m_stageAllBtn = new QToolButton(unstagedBar);
    m_stageAllBtn->setText(QStringLiteral("+"));
    m_stageAllBtn->setToolTip(tr("全部暂存"));
    m_stageAllBtn->setCursor(Qt::PointingHandCursor);
    m_stageAllBtn->setFixedSize(22, 22);
    connect(m_stageAllBtn, &QToolButton::clicked,
            this, &GitPanel::onStageAllClicked);

    ul->addWidget(m_unstagedHeader, 1);
    ul->addWidget(m_stageAllBtn);
    lay->addWidget(unstagedBar);

    m_unstagedList = new QListWidget(w);
    m_unstagedList->setAlternatingRowColors(false);
    m_unstagedList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_unstagedList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_unstagedList->setIconSize(QSize(14, 14));
    m_unstagedList->setSpacing(1);
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
    lay->setContentsMargins(8, 6, 8, 8);
    lay->setSpacing(6);

    m_messageEdit = new QPlainTextEdit(w);
    m_messageEdit->setPlaceholderText(tr("提交信息 (Ctrl+Enter 提交)"));
    m_messageEdit->setMaximumHeight(88);
    m_messageEdit->setMinimumHeight(60);
    connect(m_messageEdit, &QPlainTextEdit::textChanged,
            this, &GitPanel::updateCommitButton);
    lay->addWidget(m_messageEdit);

    auto* btnRow = new QHBoxLayout;
    btnRow->setSpacing(8);

    m_amendCheck = new QCheckBox(tr("修正上次提交"), w);
    m_amendCheck->setCursor(Qt::PointingHandCursor);
    btnRow->addWidget(m_amendCheck);
    btnRow->addStretch();

    m_commitBtn = new QPushButton(w);
    // 复用全局 QSS 中的 "主按钮" 样式（QPushButton#sendButton）
    //   —— 与 AI 面板发送按钮共用同一主色，不重复定义
    m_commitBtn->setObjectName("sendButton");
    m_commitBtn->setText(tr("提交"));
    m_commitBtn->setIcon(QIcon(QStringLiteral(":/icons/icons/git_commit.svg")));
    m_commitBtn->setIconSize(QSize(14, 14));
    m_commitBtn->setCursor(Qt::PointingHandCursor);
    m_commitBtn->setEnabled(false);
    connect(m_commitBtn, &QPushButton::clicked,
            this, &GitPanel::onCommitClicked);
    btnRow->addWidget(m_commitBtn);

    lay->addLayout(btnRow);

    // Ctrl+Enter / Ctrl+Return 提交
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
    hl->setContentsMargins(8, 6, 6, 2);

    m_stashHeader = new QLabel(tr("Stash (0)"), bar);
    QFont headerFont = m_stashHeader->font();
    headerFont.setBold(true);
    headerFont.setPointSizeF(headerFont.pointSizeF() - 1.0);
    m_stashHeader->setFont(headerFont);

    m_stashBtn = new QToolButton(bar);
    m_stashBtn->setText(QStringLiteral("+"));
    m_stashBtn->setToolTip(tr("创建 stash（暂存当前改动）"));
    m_stashBtn->setCursor(Qt::PointingHandCursor);
    m_stashBtn->setFixedSize(22, 22);
    connect(m_stashBtn, &QToolButton::clicked,
            this, &GitPanel::onStashClicked);

    hl->addWidget(m_stashHeader, 1);
    hl->addWidget(m_stashBtn);
    lay->addWidget(bar);

    // ---- 列表 ----
    m_stashList = new QListWidget(w);
    m_stashList->setAlternatingRowColors(false);
    m_stashList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_stashList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_stashList->setMaximumHeight(150);
    m_stashList->setSpacing(1);
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
    hl->setContentsMargins(8, 6, 6, 2);

    auto* header = new QLabel(tr("提交历史"), headerBar);
    QFont headerFont = header->font();
    headerFont.setBold(true);
    headerFont.setPointSizeF(headerFont.pointSizeF() - 1.0);
    header->setFont(headerFont);

    hl->addWidget(header, 1);
    lay->addWidget(headerBar);

    m_historyList = new QListWidget(w);
    m_historyList->setAlternatingRowColors(false);
    m_historyList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_historyList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_historyList->setSpacing(1);
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
    // 复用 ThemeManager 的语义色，跟随主题切换
    using UI = ThemeManager::UIElement;
    const auto& tm = ThemeManager::instance();

    switch (s) {
    case FileStatus::Added:       return tm.color(UI::SuccessColor);
    case FileStatus::Modified:    return tm.color(UI::WarningColor);
    case FileStatus::Deleted:     return tm.color(UI::ErrorColor);
    case FileStatus::Renamed:     return tm.color(UI::PrimaryColor);
    case FileStatus::Copied:      return tm.color(UI::PrimaryColor);
    case FileStatus::Untracked:   return tm.color(UI::SecondaryTextColor);
    case FileStatus::Conflicted:  return tm.color(UI::ErrorColor);
    case FileStatus::TypeChanged: return tm.color(UI::PrimaryColor);
    default:                      return tm.color(UI::TextColor);
    }
}

QIcon GitPanel::statusIcon(Git::FileStatus s)
{
    switch (s) {
    case FileStatus::Added:
        return QIcon(QStringLiteral(":/icons/icons/status_added.svg"));
    case FileStatus::Modified:
        return QIcon(QStringLiteral(":/icons/icons/status_modified.svg"));
    case FileStatus::Deleted:
        return QIcon(QStringLiteral(":/icons/icons/status_deleted.svg"));
    case FileStatus::Conflicted:
        return QIcon(QStringLiteral(":/icons/icons/status_conflict.svg"));
    case FileStatus::Untracked:
        return QIcon(QStringLiteral(":/icons/icons/status_untracked.svg"));
    case FileStatus::Renamed:
    case FileStatus::TypeChanged:
        return QIcon(QStringLiteral(":/icons/icons/status_modified.svg"));
    case FileStatus::Copied:
        return QIcon(QStringLiteral(":/icons/icons/status_added.svg"));
    default:
        return QIcon();
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
            item->setText(s.path);                                       // 不再带 [M] 前缀
            item->setIcon(statusIcon(s.indexStatus));                    // 状态图标
            item->setForeground(statusColor(s.indexStatus));             // 文件名着色
            item->setToolTip(tr("%1\n[已暂存] %2")
                                 .arg(s.absolutePath, statusBadge(s.indexStatus)));
            item->setData(Qt::UserRole,     s.path);
            item->setData(Qt::UserRole + 1, s.absolutePath);
            ++stagedCount;
        }

        // ---- 未暂存 / 未跟踪 / 冲突 ----
        if (s.hasUnstagedChanges() || s.isUntracked() || s.hasConflict()) {
            FileStatus show = s.hasConflict() ? FileStatus::Conflicted
                                              : s.workTreeStatus;

            auto* item = new QListWidgetItem(m_unstagedList);
            item->setText(s.path);
            item->setIcon(statusIcon(show));
            item->setForeground(statusColor(show));

            if (s.hasConflict()) {
                const bool rebaseMode =
                    GitManager::instance().isRebaseInProgress();
                const QString opName = rebaseMode ? QStringLiteral("Rebase")
                                                  : QStringLiteral("Merge");
                item->setToolTip(
                    tr("%1\n\n存在冲突\n"
                       "右键「打开编辑器手动解决」或选择接受某一侧\n"
                       "（当前为 %2 冲突，右键菜单会相应调整语义）")
                        .arg(s.absolutePath, opName));
            } else {
                item->setToolTip(s.absolutePath);
            }

            item->setData(Qt::UserRole,     s.path);
            item->setData(Qt::UserRole + 1, s.absolutePath);
            item->setData(Qt::UserRole + 2, s.hasConflict());
            ++unstagedCount;
        }
    }

    m_stagedHeader->setText(
        tr("已暂存的更改 (%1)").arg(stagedCount));
    m_unstagedHeader->setText(
        tr("更改 (%1)").arg(unstagedCount));

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
        m_branchCombo->addItem(tr("(非 Git 仓库)"));
        m_branchCombo->setEnabled(false);
        m_branchCombo->blockSignals(false);
        return;
    }

    if (info.isEmpty) {
        m_branchCombo->addItem(tr("(尚无提交)"));
        m_branchCombo->setEnabled(false);
        m_branchCombo->blockSignals(false);
        return;
    }

    m_branchCombo->setEnabled(true);

    const QIcon branchIcon(QStringLiteral(":/icons/icons/branch.svg"));

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
        m_branchCombo->addItem(branchIcon, label, b.name);
        if (b.isCurrent) currentIdx = m_branchCombo->count() - 1;
    }

    if (currentIdx < 0 && info.isDetachedHead) {
        // detached HEAD：单独加一条
        m_branchCombo->insertItem(0,
                                  QIcon(QStringLiteral(":/icons/icons/git_branch.svg")),
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
            tr("提交 (%1)").arg(m_stagedList->count()));
    } else {
        m_commitBtn->setText(tr("提交"));
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
            tr("游离 HEAD（%1）\n"
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

            // Rebase 场景额外提示 ours/theirs 语义反转，避免用户按 merge 直觉操作
            QString banner = tr("  正在进行：%1").arg(opName);
            if (info.operation == GitOperation::Rebase) {
                banner += tr(
                    "（提示：本模式下「接受我的提交 / 目标分支」"
                    "已自动按 rebase 语义处理）");
            }
            m_operationLabel->setText(banner);

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
        // 有项目根但非仓库 显示「初始化仓库」按钮
        m_initBtn->setVisible(!m_projectRoot.isEmpty());
        m_emptyRepoHint->setText(
            m_projectRoot.isEmpty()
                ? tr("未打开项目。")
                : tr("此目录尚未初始化为 Git 仓库。\n\n 点击上方「初始化仓库」按钮开始使用。"));
    }

    // 空仓库（无任何 commit）
    if (info.isValid && info.isEmpty) {
        m_stagedList->clear();
        m_unstagedList->clear();
        m_historyList->clear();
        m_historyList->addItem(tr("(尚无提交)"));
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
        this, tr("初始化 Git 仓库"),
        tr("将在当前项目根目录执行 git init：\n\n%1\n\n继续？")
            .arg(m_projectRoot),
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    QString err;
    if (!GitManager::instance().initRepository(m_projectRoot, &err)) {
        QMessageBox::warning(this, tr("初始化失败"),
                             tr("无法初始化 Git 仓库：\n\n%1").arg(err));
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

    // 已经是当前分支（且不是 detached HEAD）什么都不做
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
        warnings << tr("当前处于 <b>游离 HEAD</b> 状态（%1）。<br>"
                       "切走后，此处的提交将不属于任何分支，<b>可能难以找回</b>。")
                        .arg(info.currentBranch.toHtmlEscaped());
    }
    if (dirtyCount > 0) {
        hasRisk = true;
        warnings << tr("当前有 <b>%1</b> 个未提交的更改（含 %2 个未跟踪文件）。<br>"
                       "切换分支时可能产生冲突或被合并到其他基线。")
                        .arg(dirtyCount).arg(untrackedCount);
    }

    // ---- 3. 无风险 简单确认 ----
    if (!hasRisk) {
        const auto reply = QMessageBox::question(
            this, tr("切换分支"),
            tr("切换到分支：%1 ？").arg(name),
            QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            populateBranchCombo();
            return;
        }
        gm.checkoutBranch(name);
        return;
    }

    // ---- 4. 有风险 三按钮对话框 ----
    QMessageBox box(this);
    box.setWindowTitle(tr("切换分支"));
    box.setIcon(QMessageBox::Warning);
    box.setTextFormat(Qt::RichText);
    box.setText(
        warnings.join(QStringLiteral("<br><br>"))
        + QStringLiteral("<br><br>")
        + tr("确认切换到分支：<b>%1</b> ？").arg(name.toHtmlEscaped())
        );

    auto* cancelBtn = box.addButton(tr("取消"), QMessageBox::RejectRole);
    auto* stashBtn  = box.addButton(tr("先暂存(stash)再切换"),
                                   QMessageBox::ActionRole);
    auto* forceBtn  = box.addButton(tr("仍然直接切换"),
                                   QMessageBox::DestructiveRole);
    box.setDefaultButton(cancelBtn);
    box.exec();

    auto* clicked = box.clickedButton();

    if (clicked == cancelBtn) {
        populateBranchCombo();
        return;
    }

    if (clicked == stashBtn) {
        // 自动 stash 切换；GitManager 内部会在 stash 完成后 checkout
        gm.stashPush(
            tr("切换分支 %1 %2 前自动暂存")
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
                       QString());   // data 为空 使用默认（当前 HEAD）

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
        QMessageBox::warning(this, tr("新建分支"),
                             tr("分支 <b>%1</b> 已存在。").arg(name.toHtmlEscaped()));
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
                             tr("分支名包含非法字符。") + "<br><br>" +
                                 tr("不允许：") + "<code>空格 ~ ^ : ? * [ \\ .. </code>，<br>" +
                                 tr("不允许以") + " <code>- . /</code> " + tr("开头，") + "<br>" +
                                 tr("不允许以") + " <code>.lock</code> " + tr("或") + " <code>/</code> " + tr("结尾。"));
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
                             tr("工作区有 <b>%1</b> 个未提交的更改。<br><br>"
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
        tr("将把当前分支 <b>%1</b> 的所有提交重放到 <b>%2</b> 之上。<br><br>"
           "Rebase 会重写当前分支的提交历史。<br>"
           "如果该分支已推送到远程，之后需要 <code>git push --force</code>。<br>"
           "冲突时 ours/theirs 语义与 merge 相反。<br><br>"
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
        this, tr("中止操作"),
        tr("确定要中止当前的") + " <b>%1</b> " + tr("吗") + "？<br><br>"
                       + tr("工作区里已解决的改动会保留，未提交的改动不会丢失。").arg(opName));
    if (reply != QMessageBox::Yes) return;

    if (info.operation == GitOperation::Merge) {
        gm.abortMerge();
    } else if (info.operation == GitOperation::Rebase) {
        gm.abortRebase();
    } else {
        QMessageBox::information(this, tr("中止操作"),
                                 tr("暂不支持中止 %1。").arg(opName));
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
        QMessageBox::information(this, tr("继续 Rebase"),
                                 tr("仍有未解决的冲突文件，请先在「更改」列表里全部解决。"));
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
        box.setWindowTitle(tr("Git 身份未配置"));
        box.setIcon(QMessageBox::Warning);
        box.setTextFormat(Qt::RichText);
        box.setText(
            tr("提交前需要先配置 Git 身份。<br><br>"
               "在终端运行以下命令：<br>"
               "<code>%1</code><br>"
               "<code>%2</code><br><br>"
               "或打开设置对话框，在 <b>Git</b> 页填写并保存。")
                .arg(
                    QStringLiteral("git config --global user.name \"%1\"")
                        .arg(tr("你的名字")),
                    QStringLiteral("git config --global user.email \"%1\"")
                        .arg(tr("你的邮箱"))
                    )
            );

        auto* openBtn = box.addButton(tr("打开设置"),
                                      QMessageBox::AcceptRole);
        box.addButton(tr("取消"), QMessageBox::RejectRole);
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
        QMessageBox::information(this, tr("提示"),
                                 tr("请先暂存至少一个文件。"));
        return;
    }

    // 通过检查 提交
    gm.commit(msg, m_amendCheck->isChecked());

    m_messageEdit->clear();
    m_amendCheck->setChecked(false);
}

void GitPanel::onStagedItemDoubleClicked(QListWidgetItem* item)
{
    if (!item) return;
    const QString rel = item->data(Qt::UserRole).toString();
    const QString abs = item->data(Qt::UserRole + 1).toString();

    // 双击已暂存文件 显示暂存区 vs HEAD 的 diff
    if (rel.isEmpty()) return;

    auto& gm = GitManager::instance();
    const QString diffText = gm.rawDiff(rel, /*staged=*/true);

    auto* dlg = new GitDiffDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    connect(dlg, &GitDiffDialog::openFileRequested,
            this, [this](const QString& p) { emit fileDoubleClicked(p); });
    dlg->showFileDiff(
        tr("已暂存: %1").arg(rel),
        diffText,
        abs);
}

void GitPanel::onUnstagedItemDoubleClicked(QListWidgetItem* item)
{
    if (!item) return;
    const QString rel = item->data(Qt::UserRole).toString();
    const QString abs = item->data(Qt::UserRole + 1).toString();

    // 双击未暂存文件 显示工作区 vs 暂存区的 diff
    if (rel.isEmpty()) return;

    auto& gm = GitManager::instance();
    const QString diffText = gm.rawDiff(rel, /*staged=*/false);

    auto* dlg = new GitDiffDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    connect(dlg, &GitDiffDialog::openFileRequested,
            this, [this](const QString& p) { emit fileDoubleClicked(p); });
    dlg->showFileDiff(
        tr("工作区: %1").arg(rel),
        diffText,
        abs);
}

void GitPanel::onStagedContextMenu(const QPoint& pos)
{
    auto* item = m_stagedList->itemAt(pos);
    if (!item) return;

    QMenu menu(this);
    menu.addAction(tr("查看差异"), this, [this, item]() {
        onStagedItemDoubleClicked(item);
    });
    menu.addSeparator();
    menu.addAction(tr("取消暂存"), this, [this, item]() {
        const QString rel = item->data(Qt::UserRole).toString();
        GitManager::instance().unstageFile(rel);
    });
    menu.addAction(tr("打开文件"), this, [this, item]() {
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
    menu.addAction(tr("查看差异"), this, [this, item]() {
        onUnstagedItemDoubleClicked(item);
    });
    menu.addSeparator();

    // ---------- 冲突解决（仅当文件状态为冲突时显示） ----------
    if (isConflict) {
        menu.addAction(tr("打开编辑器手动解决"), this, [this, abs]() {
            if (!abs.isEmpty()) emit fileDoubleClicked(abs);
        });
        menu.addSeparator();

        // ============================================================
        // ours / theirs 在 merge 与 rebase 下语义相反：
        //   merge  : ours   = 当前分支（HEAD）, theirs = 被合入的分支
        //   rebase : "ours" = 目标分支（base）  , "theirs" = 你正在重放的提交
        //
        // 用户直觉里的"我的版本"= 我自己写的代码：
        //   merge 时 --ours
        //   rebase 时 --theirs
        //
        // 因此这里不暴露 ours/theirs 术语，直接用行为描述标签，
        // 并根据当前 operation 选择正确的底层命令。
        // ============================================================
        const bool isRebase = GitManager::instance().isRebaseInProgress();

        if (isRebase) {
            // ---- Rebase 场景 ----
            QAction* mineAct = menu.addAction(
                tr("接受我的提交（正在重放的版本）"));
            mineAct->setToolTip("Rebase" + tr("冲突时，保留你正在重放的提交的内容\n"
                "底层执行：") + "git checkout --theirs");
            QObject::connect(mineAct, &QAction::triggered,
                             this, [this, rel]() {
                                 GitManager::instance().acceptTheirs(rel);
                             });

            QAction* baseAct = menu.addAction(
                tr("接受目标分支（base 上的版本）"));
            baseAct->setToolTip(tr(
                "Rebase 冲突时，保留目标分支（base）的内容\n"
                "底层执行：") + "git checkout --ours");
            QObject::connect(baseAct, &QAction::triggered,
                             this, [this, rel]() {
                                 GitManager::instance().acceptOurs(rel);
                             });
        } else {
            // ---- Merge 场景（或无进行中操作，按 merge 处理） ----
            QAction* mineAct = menu.addAction(
                tr("接受当前分支（本地的版本）"));
            mineAct->setToolTip(tr(
                "保留当前分支（HEAD）的内容\n"
                "底层执行：") + "git checkout --ours");
            QObject::connect(mineAct, &QAction::triggered,
                             this, [this, rel]() {
                                 GitManager::instance().acceptOurs(rel);
                             });

            QAction* otherAct = menu.addAction(
                tr("接受被合入的分支（外来的版本）"));
            otherAct->setToolTip(tr(
                "保留被合入分支的内容\n"
                "底层执行：") + "git checkout --theirs");
            QObject::connect(otherAct, &QAction::triggered,
                             this, [this, rel]() {
                                 GitManager::instance().acceptTheirs(rel);
                             });
        }

        menu.addAction(tr("标记为已解决") + "（git add）", this, [this, rel]() {
            GitManager::instance().markResolved(rel);
        });
        menu.addSeparator();
    }

    // ---------- 常规操作 ----------
    menu.addAction(tr("暂存此文件"), this, [this, rel]() {
        GitManager::instance().stageFile(rel);
    });
    menu.addAction(tr("打开文件"), this, [this, abs]() {
        if (!abs.isEmpty()) emit fileDoubleClicked(abs);
    });

    menu.addSeparator();

    // ---------- 丢弃更改 ----------
    menu.addAction(tr("丢弃更改…"), this, [this, rel, abs, isConflict]() {
        Q_UNUSED(abs);
        // 冲突文件不允许直接丢弃（会造成状态混乱）
        if (isConflict) {
            QMessageBox::information(
                this, tr("丢弃更改"),
                tr("冲突文件不能直接丢弃，请先用「接受本地 / 接受远程 / 手动解决」处理。"));
            return;
        }

        auto& gm = GitManager::instance();
        const bool isUntracked = [&]() {
            for (const auto& s : gm.currentStatus()) {
                if (s.path == rel) return s.isUntracked() && !s.isStaged();
            }
            return false;
        }();

        const QString title = isUntracked ? tr("删除文件") : tr("丢弃更改");
        const QString question = isUntracked
                                     ? tr("确定要删除以下未跟踪文件吗？") + "<br><br><code>"
                                           + rel.toHtmlEscaped() + "</code><br><br>" + tr("此操作不可恢复。")
                                     : tr("确定要丢弃对该文件的所有未提交更改吗？") + "<br><br><code>"
                                           + rel.toHtmlEscaped() + "</code><br><br>" + tr("此操作不可恢复。");

        QMessageBox box(this);
        box.setWindowTitle(title);
        box.setIcon(QMessageBox::Warning);
        box.setTextFormat(Qt::RichText);
        box.setText(question);

        const QString okText = isUntracked
                                   ? tr("删除")
                                   : tr("丢弃更改");
        auto* okBtn     = box.addButton(okText, QMessageBox::AcceptRole);
        auto* cancelBtn = box.addButton(tr("取消"), QMessageBox::RejectRole);
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
        QMessageBox::information(this, tr("提示"),
                                 tr("无法获取该 commit 的 diff。"));
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
    m_stashHeader->setText(tr("Stash (%1)").arg(stashes.size()));

    for (const GitStash& s : stashes) {
        const QString when = s.timestamp.toString("MM-dd hh:mm");
        QString label = QStringLiteral("%1  %2").arg(when, s.message);
        if (label.length() > 80) label = label.left(77) + QStringLiteral("...");

        auto* item = new QListWidgetItem(m_stashList);
        item->setText(label);
        item->setData(Qt::UserRole, s.index);
        item->setToolTip(tr("%1\n分支: %2\n消息: %3\n时间: %4")
                             .arg(s.ref,
                                  s.branch.isEmpty() ? tr("(未知)") : s.branch,
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
        tr("创建 Stash"),
        tr("Stash 消息（可留空，默认 WIP）:"),
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
    box.setWindowTitle(tr("Stash 操作"));
    box.setText(tr("对 %1 执行什么操作？").arg(item->text()));
    auto* popBtn   = box.addButton(tr("应用并删除 (pop)"),
                                 QMessageBox::AcceptRole);
    auto* applyBtn = box.addButton(tr("仅应用 (apply)"),
                                   QMessageBox::ActionRole);
    box.addButton(tr("取消"), QMessageBox::RejectRole);
    box.exec();

    auto* c = box.clickedButton();
    if (c == popBtn)         GitManager::instance().stashPop(idx);
    else if (c == applyBtn)  GitManager::instance().stashApply(idx);
}

void GitPanel::onStashContextMenu(const QPoint& pos)
{
    auto* item = m_stashList->itemAt(pos);

    // 空白处右键 清空所有 stash
    if (!item) {
        QMenu menu(this);
        QAction* clearAll = menu.addAction(tr("清空所有 Stash"));
        clearAll->setEnabled(m_stashList->count() > 0);
        connect(clearAll, &QAction::triggered, this, [this]() {
            if (QMessageBox::question(
                    this, tr("清空 Stash"),
                    tr("确定要清空所有 stash 吗？此操作不可恢复。"))
                == QMessageBox::Yes) {
                GitManager::instance().stashClear();
            }
        });
        menu.exec(m_stashList->viewport()->mapToGlobal(pos));
        return;
    }

    const int idx = item->data(Qt::UserRole).toInt();

    QMenu menu(this);
    connect(menu.addAction(tr("应用并删除 (pop)")),
            &QAction::triggered, this, [idx]() {
                GitManager::instance().stashPop(idx);
            });
    connect(menu.addAction(tr("仅应用 (apply)")),
            &QAction::triggered, this, [idx]() {
                GitManager::instance().stashApply(idx);
            });
    menu.addSeparator();
    connect(menu.addAction(tr("删除此 Stash")),
            &QAction::triggered, this, [this, idx]() {
                if (QMessageBox::question(
                        this, QStringLiteral("删除 Stash"),
                        tr("确定要删除此 stash 吗？此操作不可恢复。"))
                    == QMessageBox::Yes) {
                    GitManager::instance().stashDrop(idx);
                }
            });
    menu.exec(m_stashList->viewport()->mapToGlobal(pos));
}

} // namespace Git
} // namespace SprintIDE
