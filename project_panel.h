#pragma once

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QFileSystemWatcher>
#include <QMenu>
#include <QAction>
#include <QFileInfo>
#include <QDir>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QAbstractItemModel>
#include <QSplitter>

#include "project_config.h"
#include "git/git_overlay_proxy_model.h"

namespace SprintIDE {

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

    // Git 状态更新（由 MainWindow 转发，或直接连 GitManager 信号）
    void onGitStatusUpdated(const QList<Git::GitFileStatus>& status);

signals:
    void fileOpenRequested(const QString& filePath);
    void fileCreated(const QString& filePath);
    void fileDeleted(const QString& filePath);
    void fileRenamed(const QString& oldPath, const QString& newPath);
    void projectClosed();
    void selectionChanged(const QString& filePath);
    void requestFileHistory(const QString& filePath);
    void deleteRequested(const QStringList& filePaths);
    void runTestRequested(const QString& filePath, const QString& testFullName);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

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
    void createFileTree();
    void createTestTree();
    void createContextMenu();
    void updateContextMenu();

    bool isDirectory(const QModelIndex &index) const;
    bool isFile(const QModelIndex &index) const;
    QString selectedParentDir() const;
    void startRename(const QModelIndex &index);
    bool validateFileName(const QString& fileName) const;
    QString getNewFileName(const QString& parentDir, const QString& baseName, const QString& extension) const;

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
};

} // namespace SprintIDE
