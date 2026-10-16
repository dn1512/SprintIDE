#pragma once

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QFileSystemWatcher>
#include <QMenu>
#include <QAction>
#include <QToolButton>
#include <QFileInfo>
#include <QDir>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QAbstractItemModel>
#include <QSplitter>
#include <QDropEvent>
#include <QDragMoveEvent>

#include "project_config.h"
#include "git/git_overlay_proxy_model.h"

namespace SprintIDE {

// ============================================================
// 项目面板右键 Git 子菜单的动作
// ============================================================
enum class GitContextAction {
    // ---- 文件级（需要 filePath） ----
    Add,                   // 添加（暂存）
    ShowDiff,              // 显示差异
    ShowHistory,           // 显示历史记录
    ShowCurrentRevision,   // 显示当前修订
    Rollback,              // 回滚（丢弃更改）
    Annotate,              // 使用 Git 追溯注解（占位）
    CompareWithRevision,   // 与修订比较（占位）
    CompareWithBranch,     // 与分支比较（占位）

    // ---- 仓库级 ----
    Commit,                // 提交
    Push,                  // 推送
    Pull,                  // 拉取
    Fetch,                 // 提取
    Merge,                 // 合并（占位）
    Rebase,                // 变基（占位）
    Branches,              // 分支管理（占位）
    NewBranch,             // 新建分支
    NewTag,                // 新建标记（占位）
    ResetHead,             // 重置 HEAD（占位）
    StashChanges,          // 存储变更
    UnstashChanges,        // 取消存储变更（占位）
    ManageRemotes,         // 管理远程（占位）
};

// 代理模型（原样保留）
class ExcludeDirProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit ExcludeDirProxyModel(QObject* parent = nullptr);
    void setExcludeDirs(const QStringList& dirs);
    void setAllowedExtensions(const QStringList& exts);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;
private:
    QStringList m_excludeDirs;
    QStringList m_allowedExtensions;
};

/**
 * @brief 项目面板（文件树 + 测试树）
 */
class ProjectPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectPanel(QWidget *parent = nullptr);
    ~ProjectPanel();

    // ==================== 原有公共方法（完全兼容） ====================
    bool openProject(const QString& projectPath, const ProjectConfig* config = nullptr);
    void closeProject();
    QString currentProjectPath() const;
    const ProjectConfig* projectConfig() const;
    void setFileFilters(const QStringList& filters);
    QString selectedFilePath() const;
    QStringList selectedFilePaths() const;
    void refresh();
    void locateFile(const QString& filePath);
    bool addNewFile(const QString& parentDir, const QString& fileName, bool isDirectory = false);
    bool renameFile(const QString& oldPath, const QString& newName);
    bool deleteFile(const QString& filePath, bool moveToTrash = true);
    void setShowHiddenFiles(bool show);
    void setExcludeDirs(const QStringList& excludeDirs);
    QIcon getFileIcon(const QString& filePath) const;

    QString getFilePath(const QModelIndex &index) const;
    QModelIndex findIndexByPath(const QString &path) const;

    // ==================== 新增转发方法（适配 MainWindow 原有调用） ====================
    QTreeView* getFileTreeView() const { return m_fileTreeView; }
    void setAlternatingRowColors(bool enable) { m_fileTreeView->setAlternatingRowColors(enable); }
    QModelIndex rootIndex() const { return m_fileTreeView->rootIndex(); }
    QAbstractItemModel* model() const { return m_fileTreeView->model(); }
    bool isExpanded(const QModelIndex& index) const { return m_fileTreeView->isExpanded(index); }
    void expand(const QModelIndex& index) { m_fileTreeView->expand(index); }
    /// 让项目面板获得键盘焦点（内部自动处理"无选中项"的情况）
    void focusPanel();

public slots:
    void newFile();
    void newDirectory();
    void openSelectedFile();
    void renameSelectedFile();
    void deleteSelectedFile();
    void copyFilePath();
    void showInFileManager();
    void refreshSelected();
    void expandToSelected();
    void showFileHistory();

    // 外部触发刷新（按钮 + 信号都走这里）
    void onRefreshRequested();

    // Git 状态更新（由 MainWindow 转发，或直接连 GitManager 信号）
    void onGitStatusUpdated(const QList<Git::GitFileStatus>& status);

signals:
    void fileOpenRequested(const QString& filePath);
    void fileCreated(const QString& filePath);

    // 用户点击"定位"按钮。MainWindow 收到后调用 locateFile()
    void locateCurrentFileRequested();

    // 右键 Git 子菜单动作
    //   action : 用户选择的动作
    //   filePaths : 当前选中的文件（绝对路径列表）
    //               仓库级动作用空列表
    void gitActionRequested(SprintIDE::GitContextAction action,
                            const QStringList& filePaths);
    void fileDeleted(const QString& filePath);
    void fileRenamed(const QString& oldPath, const QString& newPath);
    void projectClosed();
    void selectionChanged(const QString& filePath);
    void requestFileHistory(const QString& filePath);
    void deleteRequested(const QStringList& filePaths);
    void runTestRequested(const QString& filePath, const QString& testFullName);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onFileTreeSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void onFileTreeDoubleClicked(const QModelIndex &index);
    void onDirectoryChanged(const QString &path);
    void onFileRenamed(const QString &path, const QString &oldName, const QString &newName);
    void onWatchedDirectoryChanged(const QString &path);
    void onWatchedFileChanged(const QString &path);
    void onTestTreeDoubleClicked(const QModelIndex &index);

private:
    void setupUI();
    void setupToolbar(QVBoxLayout* parentLayout);
    void createFileTree();
    void createTestTree();
    void createContextMenu();
    void updateContextMenu();

    bool isDirectory(const QModelIndex &index) const;
    bool isFile(const QModelIndex &index) const;
    QString selectedParentDir() const;
    void startRename(const QModelIndex &index);
    bool validateFileName(const QString& fileName) const;

    bool handleDrop(QDropEvent* event);
    bool movePath(const QString& src, const QString& dst);

    QString getNewFileName(const QString& parentDir, const QString& baseName, const QString& extension) const;

    // 构建 Git 子菜单（只在 createContextMenu() 里调用一次）
    void buildGitSubMenu();
    // 根据当前选中项刷新 Git 子菜单的启用/禁用
    void updateGitSubMenuState();

    void refreshTestTree();
    void addPythonTestNodes(const QString& projectPath);
    QStandardItem* findOrCreateTestRoot();

    // 双段索引映射
    QModelIndex toProxy(const QModelIndex& sourceIndex) const;
    QModelIndex toSource(const QModelIndex& proxyIndex) const;

    QSplitter* m_splitter;

    QTreeView* m_fileTreeView;
    QFileSystemWatcher* m_fileWatcher;
    QFileSystemModel* m_fileModel;
    ExcludeDirProxyModel* m_proxyModel;
    Git::GitOverlayProxyModel* m_gitOverlay = nullptr;   // 最外层：叠加 git 状态圆点
    QString m_projectPath;
    ProjectConfig* m_projectConfig;
    bool m_showHiddenFiles = false;
    QStringList m_excludeDirs;

    QTreeView* m_testTreeView;
    QStandardItemModel* m_testModel;

    // 顶部工具栏按钮
    QToolButton* m_locateButton  = nullptr;
    QToolButton* m_refreshButton = nullptr;

    QMenu* m_contextMenu;
    QAction* m_newFileAction;
    QAction* m_newDirAction;
    QAction* m_openFileAction;
    QAction* m_renameAction;
    QAction* m_deleteAction;
    QAction* m_copyPathAction;
    QAction* m_showInExplorerAction;
    QAction* m_refreshAction;
    QAction* m_expandAction;
    QAction* m_historyAction;

    // ---- Git 子菜单 ----
    QMenu*   m_gitSubMenu          = nullptr;
    QAction* m_gitCommitAct        = nullptr;
    QAction* m_gitAddAct           = nullptr;
    QAction* m_gitAnnotateAct      = nullptr;
    QAction* m_gitShowDiffAct      = nullptr;
    QAction* m_gitCompareRevAct    = nullptr;
    QAction* m_gitCompareBranchAct = nullptr;
    QAction* m_gitShowHistoryAct   = nullptr;
    QAction* m_gitShowRevisionAct  = nullptr;
    QAction* m_gitRollbackAct      = nullptr;
    QAction* m_gitPushAct          = nullptr;
    QAction* m_gitPullAct          = nullptr;
    QAction* m_gitFetchAct         = nullptr;
    QAction* m_gitMergeAct         = nullptr;
    QAction* m_gitRebaseAct        = nullptr;
    QAction* m_gitBranchesAct      = nullptr;
    QAction* m_gitNewBranchAct     = nullptr;
    QAction* m_gitNewTagAct        = nullptr;
    QAction* m_gitResetHeadAct     = nullptr;
    QAction* m_gitStashChangesAct  = nullptr;
    QAction* m_gitUnstashAct       = nullptr;
    QAction* m_gitManageRemotesAct = nullptr;

    // 记录最近一次右键选中的文件路径（供菜单项回调使用）
    QStringList m_lastSelectedPaths;
};

} // namespace SprintIDE
