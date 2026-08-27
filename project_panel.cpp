// project_panel.cpp
#include "project_panel.h"

#include <QFileSystemModel>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QKeyEvent>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDebug>
#include <QHeaderView>
#include <QSettings>


namespace SprintIDE {

// ==================== 静态辅助函数和常量 ====================

// 默认排除的目录
static const QStringList DEFAULT_EXCLUDE_DIRS = {
    ".git", ".svn", ".hg", ".idea", ".vs", "docs",
    "build", "bin", "obj", "Debug", "Release",
    "__pycache__", "node_modules", "CMakeFiles",
    ".dSYM"
};

// 支持的文件扩展名
static const QStringList SUPPORTED_EXTENSIONS = {
    // C/C++ 文件
    "cpp", "c", "cc", "cxx", "c++",
    "h", "hpp", "hh", "hxx", "h++",
    // 其他文本文件
    "txt", "md", "json", "xml", "html", "css", "js",
    // 配置文件
    "ini", "cfg", "conf", "pro", "pri",
    // 项目文件
    "lightcppproj", "cmake", "makefile"
};

// 图标映射
static const QMap<QString, QString> FILE_ICON_MAP = {
    // C/C++ 文件
    {"cpp", ":/icons/icons/file_.svg"},
    {"c", ":/icons/icons/file_c.svg"},
    {"h", ":/icons/icons/file_h.svg"},
    {"hpp", ":/icons/icons/file_hpp.svg"},
    // 文本文件
    {"txt", ":/icons/icons/file_text.svg"},
    {"md", ":/icons/icons/file_markdown.svg"},
    {"json", ":/icons/icons/file_json.svg"},
    {"xml", ":/icons/icons/file_xml.svg"},
    // 配置文件
    {"ini", ":/icons/icons/file_config.svg"},
    {"cfg", ":/icons/icons/file_config.svg"},
    {"conf", ":/icons/icons/file_config.svg"},
    // 项目文件
    {"lightcppproj", ":/icons/icons/file_project.svg"},
    {"cmake", ":/icons/icons/file_cmake.svg"},
    {"makefile", ":/icons/icons/file_make.svg"},
    // 默认图标
    {"default", ":/icons/icons/file_default.svg"},
    {"directory", ":/icons/icons/folder.svg"},
    {"directory_open", ":/icons/icons/folder_open.svg"}
};

// ==================== 构造函数和析构函数 ====================

ProjectPanel::ProjectPanel(QWidget *parent)
    : QTreeView(parent)
    , m_fileModel(new QFileSystemModel(this))
    , m_contextMenu(new QMenu(this))
    , m_projectConfig(nullptr)
    , m_showHiddenFiles(false)
    , m_fileWatcher(new QFileSystemWatcher(this))
{
    // 设置模型
    setModel(m_fileModel);

    // 初始化界面
    setupUI();

    // 初始化信号连接
    setupConnections();

    // 创建上下文菜单
    createContextMenu();

    // 设置默认排除目录
    m_excludeDirs = DEFAULT_EXCLUDE_DIRS;

    // 设置默认文件过滤器
    QStringList filters;
    for (const QString& ext : SUPPORTED_EXTENSIONS) {
        filters << "*." + ext;
    }
    // setFileFilters(filters);

    // 隐藏列（只显示文件名）
    setHeaderHidden(true);
    for (int i = 1; i < m_fileModel->columnCount(); ++i) {
        hideColumn(i);
    }

    // 设置拖放支持
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::DragDrop);

    // 设置选择模式
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    // 设置编辑触发方式
    setEditTriggers(QAbstractItemView::EditKeyPressed);

    // 设置文件系统监视器
    connect(m_fileWatcher, &QFileSystemWatcher::directoryChanged,
            this, &ProjectPanel::onWatchedDirectoryChanged);
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged,
            this, &ProjectPanel::onWatchedFileChanged);
}

ProjectPanel::~ProjectPanel()
{
    // 不需要手动删除，Qt对象树会自动管理
}

// ==================== 公共方法 ====================

bool ProjectPanel::openProject(const QString& projectPath, const ProjectConfig* config)
{
    if (projectPath.isEmpty() || !QDir(projectPath).exists()) {
        qWarning() << "项目路径不存在或为空:" << projectPath;
        return false;
    }

    closeProject();

    m_projectPath = QDir(projectPath).absolutePath();

    // 处理配置（保持不变）...

    // 设置根路径（触发异步加载）
    QModelIndex rootIndex = m_fileModel->setRootPath(m_projectPath);
    if (!rootIndex.isValid()) {
        qWarning() << "Invalid root index for:" << m_projectPath;
        return false;
    }

    // 使用 directoryLoaded 信号，在目录加载完成后设置视图根索引
    connect(m_fileModel, &QFileSystemModel::directoryLoaded,
            this, [this, rootIndex](const QString& loadedPath) {
                if (loadedPath == m_projectPath) {
                    QModelIndex proxyRoot = m_proxyModel->mapFromSource(rootIndex);
                    if (proxyRoot.isValid()) {
                        setRootIndex(proxyRoot);
                        expand(proxyRoot);
                        qDebug() << "Row count after load:" << model()->rowCount(proxyRoot);
                    } else {
                        qWarning() << "proxyRoot invalid after load";
                    }
                    // 断开连接，避免重复触发
                    disconnect(m_fileModel, &QFileSystemModel::directoryLoaded, this, nullptr);
                }
            });

    return true;
}

void ProjectPanel::closeProject()
{
    // 清除模型数据
    m_fileModel->setRootPath("");
    setRootIndex(QModelIndex());

    // 清理项目配置
    if (m_projectConfig) {
        delete m_projectConfig;
        m_projectConfig = nullptr;
    }

    // 清空项目路径
    m_projectPath.clear();

    // 发射项目关闭信号
    emit projectClosed();

    qDebug() << "项目已关闭";
}

QString ProjectPanel::currentProjectPath() const
{
    return m_projectPath;
}

const ProjectConfig* ProjectPanel::projectConfig() const
{
    return m_projectConfig;
}

void ProjectPanel::setFileFilters(const QStringList& filters)
{
    m_fileFilters = filters;

    // 构建包含和排除的过滤器
    QStringList nameFilters;

    // 添加包含的文件扩展名
    // for (const QString& filter : filters) {
    //     nameFilters << filter;
    // }

    // if (!nameFilters.isEmpty()) {
    //     m_fileModel->setNameFilters(nameFilters);
    //     m_fileModel->setNameFilterDisables(false);  // 隐藏不匹配的项
    // } else {
    //     m_fileModel->setNameFilters(QStringList());
    //     m_fileModel->setNameFilterDisables(false);
    // }

    // 设置名称过滤器
    if (!filters.isEmpty()) {
        m_fileModel->setNameFilters(filters);
        m_fileModel->setNameFilterDisables(false);
    }
}

QString ProjectPanel::selectedFilePath() const
{
    QModelIndexList selected = selectedIndexes();
    if (!selected.isEmpty()) {
        // 只取第一列
        QModelIndex first = selected.first();
        if (first.column() != 0) {
            // 找到同一行的第0列
            QModelIndex idx0 = model()->index(first.row(), 0, first.parent());
            if (idx0.isValid()) first = idx0;
        }
        return getFilePath(first);
    }
    return QString();
}

QStringList ProjectPanel::selectedFilePaths() const
{
    QStringList paths;
    QModelIndexList selected = selectedIndexes();
    for (const QModelIndex& index : selected) {
        if (index.column() == 0) {
            QString path = getFilePath(index);
            if (!path.isEmpty()) {
                paths.append(path);
            }
        }
    }
    return paths;
}

void ProjectPanel::refresh()
{
    if (!m_projectPath.isEmpty()) {
        // 重新设置根路径会触发刷新
        m_fileModel->setRootPath(m_projectPath);
        // 重新获取代理根索引
        QModelIndex sourceRoot = m_fileModel->index(m_projectPath);
        if (sourceRoot.isValid()) {
            QModelIndex proxyRoot = m_proxyModel->mapFromSource(sourceRoot);
            setRootIndex(proxyRoot);
        }
    }
}

void ProjectPanel::locateFile(const QString& filePath)
{
    if (filePath.isEmpty() || !QFileInfo::exists(filePath)) {
        return;
    }
    QModelIndex sourceIndex = m_fileModel->index(filePath);
    if (sourceIndex.isValid()) {
        QModelIndex proxyIndex = m_proxyModel->mapFromSource(sourceIndex);
        setCurrentIndex(proxyIndex);
        scrollTo(proxyIndex, QAbstractItemView::PositionAtCenter);
        // 展开父目录
        QModelIndex parent = proxyIndex.parent();
        while (parent.isValid()) {
            expand(parent);
            parent = parent.parent();
        }
    }
}

bool ProjectPanel::addNewFile(const QString& parentDir, const QString& fileName, bool isDirectory)
{
    qDebug() << "addNewFile: 父目录=" << parentDir << "文件名=" << fileName;
    if (parentDir.isEmpty() || fileName.isEmpty()) {
        return false;
    }

    // 验证文件名
    if (!validateFileName(fileName)) {
        QMessageBox::warning(this, "文件名错误",
                             "文件名包含非法字符或格式不正确。");
        return false;
    }

    QString fullPath = QDir(parentDir).absoluteFilePath(fileName);

    if (isDirectory) {
        // 创建目录
        QDir dir(parentDir);
        if (!dir.mkdir(fileName)) {
            QMessageBox::critical(this, "创建失败",
                                  "无法创建目录: " + fileName);
            return false;
        }

        qDebug() << "目录已创建:" << fullPath;
    } else {
        // 创建文件
        QFile file(fullPath);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, "创建失败",
                                  "无法创建文件: " + fileName + "\n错误: " + file.errorString());
            return false;
        }
        file.close();

        qDebug() << "文件已创建:" << fullPath;
    }

    // 刷新父目录
    m_fileModel->setRootPath(parentDir);

    // 定位到新创建的文件/目录
    QTimer::singleShot(200, [this, fullPath]() {
        // 先检查文件是否已经出现在模型中
        QModelIndex index = m_fileModel->index(fullPath);
        if (!index.isValid()) {
            // 如果模型还没有加载该文件，延迟重试
            QTimer::singleShot(200, [this, fullPath]() {
                QModelIndex idx = m_fileModel->index(fullPath);
                if (idx.isValid()) {
                    locateFile(fullPath);
                    startRename(idx);
                }
            });
            return;
        }
        locateFile(fullPath);
        startRename(index);
    });

    // 发射文件创建信号
    emit fileCreated(fullPath);

    return true;
}

bool ProjectPanel::renameFile(const QString& oldPath, const QString& newName)
{
    if (oldPath.isEmpty() || newName.isEmpty()) {
        return false;
    }

    // 验证新文件名
    if (!validateFileName(newName)) {
        QMessageBox::warning(this, "文件名错误",
                             "新文件名包含非法字符或格式不正确。");
        return false;
    }

    QFileInfo oldInfo(oldPath);
    QDir parentDir = oldInfo.dir();
    QString newPath = parentDir.absoluteFilePath(newName);

    // 检查新路径是否已存在
    if (QFileInfo::exists(newPath)) {
        QMessageBox::warning(this, "重命名失败",
                             "目标文件已存在: " + newName);
        return false;
    }

    // 执行重命名
    if (!QFile::rename(oldPath, newPath)) {
        QMessageBox::critical(this, "重命名失败",
                              "无法重命名文件: " + oldInfo.fileName());
        return false;
    }

    // 刷新父目录
    m_fileModel->setRootPath(parentDir.absolutePath());

    qDebug() << "文件已重命名:" << oldPath << "->" << newPath;

    // 发射文件重命名信号
    emit fileRenamed(oldPath, newPath);

    return true;
}

bool ProjectPanel::deleteFile(const QString& filePath, bool moveToTrash)
{
    if (filePath.isEmpty() || !QFileInfo::exists(filePath)) {
        return false;
    }

    QFileInfo info(filePath);
    QString displayName = info.fileName();

    // 确认对话框
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "确认删除",
        QString("确定要删除 \"%1\" 吗？").arg(displayName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
        );

    if (reply != QMessageBox::Yes) {
        return false;
    }

    bool success = false;
    if (info.isDir()) {
        QDir dir(filePath);
        success = dir.removeRecursively();
    } else {
        success = QFile::remove(filePath);
    }

    if (success) {
        QDir parentDir = info.dir();
        m_fileModel->setRootPath(parentDir.absolutePath());

        // 重置当前索引，避免指向已删除的项
        setCurrentIndex(QModelIndex());
        viewport()->update();

        qDebug() << "文件已删除:" << filePath;
        emit fileDeleted(filePath);
    } else {
        QMessageBox::critical(this, "删除失败", "无法删除: " + displayName);
    }
    return success;
}

void ProjectPanel::setShowHiddenFiles(bool show)
{
    m_showHiddenFiles = show;
    m_fileModel->setFilter(show ?
                               QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden :
                               QDir::AllEntries | QDir::NoDotAndDotDot);
}

void ProjectPanel::setExcludeDirs(const QStringList& excludeDirs)
{
    if (auto* proxy = qobject_cast<ExcludeDirProxyModel*>(m_proxyModel)) {
        proxy->setExcludeDirs(excludeDirs);
    }
}

QIcon ProjectPanel::getFileIcon(const QString& filePath) const
{
    QFileInfo info(filePath);

    if (info.isDir()) {
        // 使用目录图标
        return QIcon(FILE_ICON_MAP.value("directory"));
    }

    // 获取文件扩展名
    QString ext = info.suffix().toLower();

    // 查找对应的图标
    if (FILE_ICON_MAP.contains(ext)) {
        return QIcon(FILE_ICON_MAP.value(ext));
    }

    // 使用默认图标
    return QIcon(FILE_ICON_MAP.value("default"));
}

// ==================== 公共槽函数 ====================

void ProjectPanel::newFile()
{
    QString parentDir = selectedParentDir();
    if (parentDir.isEmpty()) {
        parentDir = m_projectPath;
    }

    // 获取新文件名
    QString baseName = getNewFileName(parentDir, "new_file", "cpp");

    bool ok;
    QString fileName = QInputDialog::getText(
        this,
        "新建文件",
        "请输入文件名:",
        QLineEdit::Normal,
        baseName,
        &ok
        );

    if (ok && !fileName.isEmpty()) {
        addNewFile(parentDir, fileName, false);
    }
}

void ProjectPanel::newDirectory()
{
    QString parentDir = selectedParentDir();
    if (parentDir.isEmpty()) {
        parentDir = m_projectPath;
    }

    // 获取新目录名
    QString baseName = getNewFileName(parentDir, "NewFolder", "");

    bool ok;
    QString dirName = QInputDialog::getText(
        this,
        "新建目录",
        "请输入目录名:",
        QLineEdit::Normal,
        baseName,
        &ok
        );

    if (ok && !dirName.isEmpty()) {
        addNewFile(parentDir, dirName, true);
    }
}

void ProjectPanel::openSelectedFile()
{
    QString filePath = selectedFilePath();
    if (!filePath.isEmpty() && isFile(currentIndex())) {
        emit fileOpenRequested(filePath);
    }
}

void ProjectPanel::renameSelectedFile()
{
    QModelIndex index = currentIndex();
    if (index.isValid()) {
        startRename(index);
    }
}

void ProjectPanel::deleteSelectedFile()
{
    QStringList files = selectedFilePaths();
    if (files.isEmpty()) {
        qWarning() << "deleteSelectedFile: 没有选中的文件";
        return;
    }

    clearSelection();  // 提前清除选择

    // for (const QString& filePath : files) {
    //     deleteFile(filePath, true);
    // } 删除逻辑改成发送删除信号，在Mainwindow中执行deleteFile(filePath, true);
    emit deleteRequested(files);
}

void ProjectPanel::copyFilePath()
{
    QString filePath = selectedFilePath();
    if (!filePath.isEmpty()) {
        QApplication::clipboard()->setText(filePath);
        qDebug() << "文件路径已复制到剪贴板:" << filePath;
    }
}

void ProjectPanel::showInFileManager()
{
    QString filePath = selectedFilePath();
    if (!filePath.isEmpty()) {
        QFileInfo info(filePath);
        QString pathToOpen = info.isDir() ? filePath : info.absolutePath();

        QUrl url = QUrl::fromLocalFile(pathToOpen);
        QDesktopServices::openUrl(url);
    }
}

void ProjectPanel::refreshSelected()
{
    QModelIndex index = currentIndex();
    if (index.isValid()) {
        QString path = getFilePath(index);
        m_fileModel->setRootPath(path);
    } else {
        refresh();
    }
}

void ProjectPanel::expandToSelected()
{
    QModelIndex index = currentIndex();
    while (index.isValid()) {
        expand(index);
        index = index.parent();
    }
}

void ProjectPanel::showFileHistory()
{
    QString filePath = selectedFilePath();
    if (filePath.isEmpty()) {
        return;
    }
    emit requestFileHistory(filePath);
}

// ==================== 保护方法 ====================

void ProjectPanel::contextMenuEvent(QContextMenuEvent *event)
{
    QModelIndex index = indexAt(event->pos());

    if (index.isValid()) {
        // 选中右键点击的项
        setCurrentIndex(index);

        // 更新上下文菜单状态
        updateContextMenu();

        // 显示菜单
        m_contextMenu->exec(event->globalPos());
    } else {
        // 在空白处点击，显示项目级菜单
        QMenu blankMenu(this);

        QAction* newFileAction = blankMenu.addAction("新建文件");
        QAction* newDirAction = blankMenu.addAction("新建目录");
        blankMenu.addSeparator();
        QAction* refreshAction = blankMenu.addAction("刷新");

        connect(newFileAction, &QAction::triggered, this, &ProjectPanel::newFile);
        connect(newDirAction, &QAction::triggered, this, &ProjectPanel::newDirectory);
        connect(refreshAction, &QAction::triggered, this, &ProjectPanel::refresh);

        blankMenu.exec(event->globalPos());
    }
}

void ProjectPanel::mouseDoubleClickEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());

    if (index.isValid() && isFile(index)) {
        emit fileOpenRequested(getFilePath(index));
    } else {
        QTreeView::mouseDoubleClickEvent(event);
    }
}

void ProjectPanel::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void ProjectPanel::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void ProjectPanel::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->accept();
}

void ProjectPanel::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();

        // 获取目标目录
        QModelIndex targetIndex = indexAt(event->pos());
        QString targetDir;

        if (targetIndex.isValid() && isDirectory(targetIndex)) {
            targetDir = getFilePath(targetIndex);
        } else {
            targetDir = m_projectPath;
        }

        // 处理每个拖放的文件
        for (const QUrl& url : urls) {
            QString sourcePath = url.toLocalFile();
            QString fileName = QFileInfo(sourcePath).fileName();
            QString destPath = QDir(targetDir).absoluteFilePath(fileName);

            // 检查是否在同一目录（重命名）
            if (sourcePath == destPath) {
                continue;
            }

            // 检查目标文件是否存在
            if (QFileInfo::exists(destPath)) {
                QMessageBox::StandardButton reply = QMessageBox::question(
                    this,
                    "文件已存在",
                    QString("文件 \"%1\" 已存在，是否替换？").arg(fileName),
                    QMessageBox::Yes | QMessageBox::No
                    );

                if (reply == QMessageBox::No) {
                    continue;
                }
            }

            // 执行复制或移动
            if (event->keyboardModifiers() & Qt::ControlModifier) {
                // Ctrl键：复制
                QFile::copy(sourcePath, destPath);
            } else {
                // 无修饰键：移动
                QFile::rename(sourcePath, destPath);
            }
        }

        event->acceptProposedAction();

        // 刷新目标目录
        m_fileModel->setRootPath(targetDir);
    }
}

void ProjectPanel::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Delete:
        deleteSelectedFile();
        break;

    case Qt::Key_F2:
        renameSelectedFile();
        break;

    case Qt::Key_F5:
        refreshSelected();
        break;

    case Qt::Key_Enter:
    case Qt::Key_Return:
        if (!event->modifiers()) {
            openSelectedFile();
        } else {
            QTreeView::keyPressEvent(event);
        }
        break;

    default:
        QTreeView::keyPressEvent(event);
        break;
    }
}

// ==================== 私有槽函数 ====================

void ProjectPanel::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected);
    if (!selected.isEmpty()) {
        QModelIndex idx = selected.indexes().first();
        QString path = getFilePath(idx);

        if (!path.isEmpty()) {
            emit selectionChanged(path);
        }
    } else {
        emit selectionChanged(QString());
    }
}

void ProjectPanel::onDirectoryChanged(const QString &path)
{
    Q_UNUSED(path);
    // 目录改变时自动刷新
    // 注意：这个信号可能会频繁触发，在实际使用中可能需要节流
    // refresh();
}

void ProjectPanel::onFileRenamed(const QString &path, const QString &oldName, const QString &newName)
{
    Q_UNUSED(path);
    Q_UNUSED(oldName);
    Q_UNUSED(newName);
    // 文件重命名后的处理
    // 这里可以添加额外的逻辑，如更新编辑器中的文件引用
}

void ProjectPanel::onWatchedDirectoryChanged(const QString &path)
{
    qDebug() << "监控的目录发生变化:" << path;

    // 只刷新发生变化的目录
    QModelIndex index = m_fileModel->index(path);
    if (index.isValid()) {
        // 重新加载该目录
        m_fileModel->fetchMore(index);

        // 更新视图
        viewport()->update();
    }
}

void ProjectPanel::onWatchedFileChanged(const QString &path)
{
    qDebug() << "监控的文件发生变化:" << path;

    // 文件变化时，更新其父目录
    QFileInfo info(path);
    if (info.exists()) {
        QString parentDir = info.absolutePath();
        onWatchedDirectoryChanged(parentDir);
    }
}

// ==================== 私有方法 ====================

void ProjectPanel::setupUI()
{
    // 创建文件系统模型
    m_fileModel = new QFileSystemModel(this);
    m_fileModel->setReadOnly(false);
    // 设置过滤器：显示所有条目（文件、目录，不含 . 和 ..）
    m_fileModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);

    // 设置名称过滤器（只显示源文件和常见配置文件，不匹配的隐藏）
    // m_fileModel->setNameFilters(QStringList()
    //                             << "*.cpp" << "*.c" << "*.cc" << "*.cxx"
    //                             << "*.h" << "*.hpp" << "*.hh" << "*.hxx"
    //                             << "*.txt" << "*.md" << "*.json" << "*.xml"
    //                             << "*.py" << "*.js" << "*.html" << "*.css"
    //                             << "*.go" << "*.rs" << "*.java" << "*.cs" << "*.php"
    //                             << "*.cmake" << "*.make" << "*.lightcppproj");
    // m_fileModel->setNameFilterDisables(false);  // 隐藏不匹配的文件

    // 创建代理模型并设置排除目录
    auto* proxy = new ExcludeDirProxyModel(this);
    proxy->setSourceModel(m_fileModel);
    // 使用默认排除目录列表（含 .dSYM、bin 等）
    proxy->setExcludeDirs(DEFAULT_EXCLUDE_DIRS);
    m_proxyModel = proxy;

    // 将代理模型设置为视图模型
    setModel(m_proxyModel);

    // 视图属性设置
    setAnimated(true);
    setIndentation(20);
    setSortingEnabled(true);
    setHeaderHidden(true);
    for (int i = 1; i < m_fileModel->columnCount(); ++i) {
        hideColumn(i);
    }

    // 拖放支持
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setEditTriggers(QAbstractItemView::EditKeyPressed);

    // 连接信号
    connect(selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &ProjectPanel::onSelectionChanged);
    connect(selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this](const QModelIndex &current, const QModelIndex &previous) {
                qDebug() << "currentChanged: " << getFilePath(current);
            });

    // 文件系统模型信号
    connect(m_fileModel, &QFileSystemModel::directoryLoaded,
            this, &ProjectPanel::onDirectoryChanged);
    connect(m_fileModel, &QFileSystemModel::fileRenamed,
            this, &ProjectPanel::onFileRenamed);

    // 文件监视器
    m_fileWatcher = new QFileSystemWatcher(this);
    connect(m_fileWatcher, &QFileSystemWatcher::directoryChanged,
            this, &ProjectPanel::onWatchedDirectoryChanged);
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged,
            this, &ProjectPanel::onWatchedFileChanged);
}

void ProjectPanel::setupConnections()
{
    // 连接已在 setupUI 中完成
}

void ProjectPanel::createContextMenu()
{
    // 创建菜单动作
    m_newFileAction = m_contextMenu->addAction("新建文件");
    m_newDirAction = m_contextMenu->addAction("新建目录");
    m_contextMenu->addSeparator();

    m_openFileAction = m_contextMenu->addAction("打开");
    m_renameAction = m_contextMenu->addAction("重命名");
    m_deleteAction = m_contextMenu->addAction("删除");
    m_contextMenu->addSeparator();

    m_copyPathAction = m_contextMenu->addAction("复制路径");
    m_showInExplorerAction = m_contextMenu->addAction("在文件管理器中显示");
    m_contextMenu->addSeparator();

    m_historyAction = m_contextMenu->addAction("查看历史");
    m_historyAction->setIcon(QIcon(":/icons/icons/history.svg"));  // 如果图标不存在，可忽略
    connect(m_historyAction, &QAction::triggered, this, &ProjectPanel::showFileHistory);
    m_contextMenu->addSeparator();

    m_refreshAction = m_contextMenu->addAction("刷新");
    m_expandAction = m_contextMenu->addAction("展开");

    // 连接动作信号
    connect(m_newFileAction, &QAction::triggered, this, &ProjectPanel::newFile);
    connect(m_newDirAction, &QAction::triggered, this, &ProjectPanel::newDirectory);
    connect(m_openFileAction, &QAction::triggered, this, &ProjectPanel::openSelectedFile);
    connect(m_renameAction, &QAction::triggered, this, &ProjectPanel::renameSelectedFile);
    connect(m_deleteAction, &QAction::triggered, this, &ProjectPanel::deleteSelectedFile);
    connect(m_copyPathAction, &QAction::triggered, this, &ProjectPanel::copyFilePath);
    connect(m_showInExplorerAction, &QAction::triggered, this, &ProjectPanel::showInFileManager);
    connect(m_refreshAction, &QAction::triggered, this, &ProjectPanel::refreshSelected);
    connect(m_expandAction, &QAction::triggered, this, &ProjectPanel::expandToSelected);
}

void ProjectPanel::updateContextMenu()
{
    QModelIndex index = currentIndex();
    bool hasSelection = index.isValid();
    bool isDir = hasSelection && isDirectory(index);
    bool isFile = hasSelection && !isDir;

    // 更新动作状态
    m_newFileAction->setEnabled(hasSelection && isDir);
    m_newDirAction->setEnabled(hasSelection && isDir);
    m_openFileAction->setEnabled(hasSelection && isFile);
    m_renameAction->setEnabled(hasSelection);
    m_deleteAction->setEnabled(hasSelection);
    m_copyPathAction->setEnabled(hasSelection);
    m_showInExplorerAction->setEnabled(hasSelection);
    m_refreshAction->setEnabled(hasSelection);
    m_expandAction->setEnabled(hasSelection && isDir);
    // 历史菜单只对文件启用
    m_historyAction->setEnabled(hasSelection && isFile);
}

QString ProjectPanel::getFilePath(const QModelIndex &index) const
{
    if (!index.isValid()) return QString();
    if (!m_proxyModel) {
        // 如果没有代理，直接使用文件模型（兼容旧代码）
        return m_fileModel->filePath(index);
    }
    QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
    if (!sourceIndex.isValid()) {
        qDebug() << "mapToSource failed for index:" << index;
        return QString();
    }
    return m_fileModel->filePath(sourceIndex);
}

bool ProjectPanel::isDirectory(const QModelIndex &index) const
{
    if (!index.isValid()) return false;
    QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
    return m_fileModel->isDir(sourceIndex);
}

bool ProjectPanel::isFile(const QModelIndex &index) const
{
    return !isDirectory(index);
}

QString ProjectPanel::selectedParentDir() const
{
    QModelIndex index = currentIndex();
    if (index.isValid()) {
        QString path = getFilePath(index);  // 使用 getFilePath 已处理转换
        if (isDirectory(index)) {
            return path;
        } else {
            return QFileInfo(path).absolutePath();
        }
    }
    return m_projectPath;
}

void ProjectPanel::startRename(const QModelIndex &index)
{
    if (index.isValid()) {
        scrollTo(index, QAbstractItemView::EnsureVisible);
        // 直接编辑代理索引
        edit(index);
    }
}

bool ProjectPanel::validateFileName(const QString& fileName) const
{
    if (fileName.isEmpty() || fileName == "." || fileName == "..") {
        return false;
    }

    // 检查非法字符（Windows和Linux兼容）
    static const QString illegalChars = "<>:\"|?*\\/";
    for (const QChar& ch : illegalChars) {
        if (fileName.contains(ch)) {
            return false;
        }
    }

    // 检查保留名称（Windows）
    static const QStringList reservedNames = {
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };

    QString upperName = fileName.toUpper();
    for (const QString& reserved : reservedNames) {
        if (upperName == reserved) {
            return false;
        }
    }

    // 检查结尾空格或点
    if (fileName.endsWith(' ') || fileName.endsWith('.')) {
        return false;
    }

    return true;
}

QString ProjectPanel::getNewFileName(const QString& parentDir, const QString& baseName, const QString& extension) const
{
    QDir dir(parentDir);
    QString fullExtension = extension.isEmpty() ? "" : "." + extension;

    // 尝试从 baseName 开始
    for (int i = 0; i < 100; ++i) {
        QString fileName;
        if (i == 0) {
            fileName = baseName + fullExtension;
        } else {
            fileName = QString("%1%2%3").arg(baseName).arg(i).arg(fullExtension);
        }

        if (!dir.exists(fileName)) {
            return fileName;
        }
    }

    // 如果所有尝试都失败，使用时间戳
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    return QString("%1_%2%3").arg(baseName).arg(timestamp).arg(fullExtension);
}

} // namespace SprintIDE

#include "project_panel.moc"
