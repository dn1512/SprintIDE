#pragma once

#include <QTreeView>
#include <QFileSystemModel>
#include <QFileSystemWatcher>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QSortFilterProxyModel>
#include <QFileSystemModel>

#include "project_config.h"


namespace SprintIDE {

// 代理模型类（用于过滤排除目录）
class ExcludeDirProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit ExcludeDirProxyModel(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
    }

    void setExcludeDirs(const QStringList& dirs)
    {
        m_excludeDirs = dirs;
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override
    {
        QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);
        if (!idx.isValid()) return true;

        auto* fsModel = qobject_cast<QFileSystemModel*>(sourceModel());
        if (!fsModel) return true;

        // 获取文件信息
        QString filePath = fsModel->filePath(idx);
        QFileInfo info(filePath);

        if (info.isDir()) {
            QString dirName = info.fileName();
            // 排除特定目录
            if (m_excludeDirs.contains(dirName) || dirName.endsWith(".dSYM"))
                return false;
            return true;  // 其他目录全部显示
        } else {
            // 文件：只显示白名单扩展名
            QString ext = info.suffix().toLower();
            static const QStringList allowedExts = {
                "cpp", "c", "cc", "cxx", "h", "hpp", "hh", "hxx",
                "txt", "md", "json", "xml", "py", "js", "html", "css",
                "go", "rs", "java", "cs", "php", "cmake", "make", "lightcppproj"
            };
            return allowedExts.contains(ext);
        }
    }

private:
    QStringList m_excludeDirs;
};

/**
 * @brief 项目面板类
 *
 * 显示项目文件树，提供文件操作功能（新建、打开、删除等）。
 * 支持拖放操作，与编辑器集成，实时显示项目结构变化。
 *
 * 主要功能：
 * 1. 显示项目目录树结构
 * 2. 文件过滤（显示特定类型文件）
 * 3. 右键菜单操作
 * 4. 文件状态指示（修改状态、Git状态等）
 * 5. 与编辑器同步（双击打开文件）
 */
class ProjectPanel : public QTreeView
{
    Q_OBJECT

public:
    // ==================== 构造函数和析构函数 ====================

    explicit ProjectPanel(QWidget *parent = nullptr);
    ~ProjectPanel();

    // ==================== 公共方法 ====================

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

    /**
     * @brief 显示当前选中文件的本地历史
     */
    void showFileHistory();

signals:
    void fileOpenRequested(const QString& filePath);
    void fileCreated(const QString& filePath);
    void fileDeleted(const QString& filePath);
    void fileRenamed(const QString& oldPath, const QString& newPath);
    void projectClosed();
    void selectionChanged(const QString& filePath);

    /**
     * @brief 请求查看文件的本地历史（由 MainWindow 处理）
     */
    void requestFileHistory(const QString& filePath);
    /**
     * @brief 请求删除文件的本地历史（由 MainWindow 处理）
     */
    void deleteRequested(const QStringList& filePaths);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void onDirectoryChanged(const QString &path);
    void onFileRenamed(const QString &path, const QString &oldName, const QString &newName);
    void onWatchedDirectoryChanged(const QString &path);
    void onWatchedFileChanged(const QString &path);

private:
    void setupUI();
    void setupConnections();
    void createContextMenu();
    void updateContextMenu();

    QString getFilePath(const QModelIndex &index) const;
    bool isDirectory(const QModelIndex &index) const;
    bool isFile(const QModelIndex &index) const;
    QString selectedParentDir() const;
    void startRename(const QModelIndex &index);
    bool validateFileName(const QString& fileName) const;
    QString getNewFileName(const QString& parentDir, const QString& baseName, const QString& extension) const;

    // ==================== 成员变量 ====================

    QFileSystemWatcher* m_fileWatcher;  ///< 文件系统监视器
    QFileSystemModel* m_fileModel;      ///< 文件系统模型
    QMenu* m_contextMenu;               ///< 上下文菜单
    ProjectConfig* m_projectConfig;     ///< 项目配置指针
    QString m_projectPath;              ///< 当前项目路径
    bool m_showHiddenFiles;             ///< 是否显示隐藏文件

    QStringList m_fileFilters;          ///< 文件过滤器
    QSortFilterProxyModel* m_proxyModel;
    QStringList m_excludeDirs;          ///< 排除的目录

    // 上下文菜单动作
    QAction* m_newFileAction;           ///< 新建文件
    QAction* m_newDirAction;            ///< 新建目录
    QAction* m_openFileAction;          ///< 打开文件
    QAction* m_renameAction;            ///< 重命名
    QAction* m_deleteAction;            ///< 删除
    QAction* m_copyPathAction;          ///< 复制路径
    QAction* m_showInExplorerAction;    ///< 在文件管理器中显示
    QAction* m_refreshAction;           ///< 刷新
    QAction* m_expandAction;            ///< 展开
    QAction* m_historyAction;           ///< 查看历史（新增）
};

} // namespace SprintIDE
