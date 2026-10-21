/*
* SprintIDE - SprintIDE is a cross-platform IDE built from scratch with Qt 6 and C++20. Its core capability is to provide AI Agents with an executable, verifiable code execution environment.
* Copyright (C) 2026  Liu Tengjun (刘腾骏)
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef SPRINTIDE_GIT_PANEL_H
#define SPRINTIDE_GIT_PANEL_H

#include <QWidget>
#include <QString>
#include <QList>

#include "git_types.h"

QT_BEGIN_NAMESPACE
class QComboBox;
class QToolButton;
class QListWidget;
class QListWidgetItem;
class QPlainTextEdit;
class QCheckBox;
class QPushButton;
class QLabel;
class QProgressBar;
class QSplitter;
class QStackedWidget;
QT_END_NAMESPACE

namespace SprintIDE {
namespace Git {

class GitPanel : public QWidget
{
    Q_OBJECT
public:
    explicit GitPanel(QWidget* parent = nullptr);
    ~GitPanel() override;

    void setProjectRoot(const QString& path);
    QString projectRoot() const;

    // 完整刷新：分支 + 历史 + stash + 状态
    // 供外部（MainWindow 打开窗口时）和内部（刷新按钮）调用
    void refreshAll();

signals:
    // 双击文件 → 交给 MainWindow 打开编辑器
    void fileDoubleClicked(const QString& absolutePath);
    // 提交前检查失败 → 引导用户去设置页
    void openSettingsRequested();

private slots:
    void onRepositoryChanged(const GitRepositoryInfo& info);
    void onNewBranchClicked();       // 保留（由菜单调用）
    void onStatusUpdated(const QList<GitFileStatus>& status);

    // 合并 / Rebase / 操作横幅
    void onMergeIntoCurrent();       // 选择分支 → merge
    void onRebaseOnto();             // 选择分支 → rebase
    void onAbortOperation();         // 中止当前操作
    void onContinueOperation();      // 继续 rebase
    void onBusyChanged(bool busy);
    void onCommandFailed(const QString& op, const QString& err);
    void onStashListUpdated(const QList<GitStash>& stashes);

    void onRefreshClicked();
    void onInitClicked();
    void onBranchActivated(int index);

    void onStageAllClicked();
    void onUnstageAllClicked();
    void onCommitClicked();

    void onStagedItemDoubleClicked(QListWidgetItem* item);
    void onUnstagedItemDoubleClicked(QListWidgetItem* item);
    void onStagedContextMenu(const QPoint& pos);
    void onUnstagedContextMenu(const QPoint& pos);
    void onHistoryItemDoubleClicked(QListWidgetItem* item);

    // Stash 相关
    void onStashClicked();                          // 创建 stash
    void onStashItemDoubleClicked(QListWidgetItem* item);
    void onStashContextMenu(const QPoint& pos);

private:
    void setupUI();
    QWidget* createTopBar();
    QWidget* createStatusSection();
    QWidget* createCommitSection();
    QWidget* createStashSection();
    QWidget* createHistorySection();

    void populateBranchCombo();
    void populateStatusLists(const QList<GitFileStatus>& status);
    void populateHistory();

    void setBusy(bool busy);
    void setEmptyRepoState(bool emptyRepo);
    void updateCommitButton();

    static QString statusBadge(Git::FileStatus s);
    static QColor  statusColor(Git::FileStatus s);   // 内部从 ThemeManager 取色
    static QIcon   statusIcon(Git::FileStatus s);

    // ---- 顶部栏 ----
    QComboBox*    m_branchCombo    = nullptr;
    QToolButton*  m_branchMenuBtn  = nullptr;   // 分支菜单（新建 / 合并 / rebase）
    QToolButton*  m_refreshBtn     = nullptr;
    QToolButton*  m_initBtn        = nullptr;
    QLabel*       m_repoLabel      = nullptr;

    // ---- 操作状态横幅（merge / rebase 进行中） ----
    QWidget*      m_operationBanner       = nullptr;
    QLabel*       m_operationLabel        = nullptr;
    QPushButton*  m_operationContinueBtn  = nullptr;
    QPushButton*  m_operationAbortBtn     = nullptr;

    // ---- 状态区 ----
    QLabel*       m_stagedHeader   = nullptr;
    QListWidget*  m_stagedList     = nullptr;
    QLabel*       m_unstagedHeader = nullptr;
    QListWidget*  m_unstagedList   = nullptr;
    QToolButton*  m_stageAllBtn    = nullptr;
    QToolButton*  m_unstageAllBtn  = nullptr;

    // ---- 提交区 ----
    QPlainTextEdit* m_messageEdit = nullptr;
    QCheckBox*      m_amendCheck  = nullptr;
    QPushButton*    m_commitBtn   = nullptr;

    // ---- Stash 区 ----
    QLabel*         m_stashHeader = nullptr;
    QListWidget*    m_stashList   = nullptr;
    QToolButton*    m_stashBtn    = nullptr;

    // ---- 历史区 ----
    QListWidget*    m_historyList = nullptr;

    // ---- 整体 ----
    QSplitter*      m_splitter     = nullptr;
    QProgressBar*   m_busyBar      = nullptr;
    QWidget*        m_contentArea  = nullptr;
    QLabel*         m_emptyRepoHint= nullptr;

    QString m_projectRoot;
};

} // namespace Git
} // namespace SprintIDE

#endif // SPRINTIDE_GIT_PANEL_H
