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

#include "python/python_test_runner.h"
#include "project_manager.h"
#include "python/python_project_model.h"

namespace SprintIDE {

// ==================== 静态常量 ====================
static const QStringList DEFAULT_EXCLUDE_DIRS = {
    ".git", ".svn", ".hg", ".idea", ".vs", "docs",
    "build", "bin", "obj", "Debug", "Release",
    "__pycache__", "node_modules", "CMakeFiles",
    ".dSYM"
};

static const QStringList SUPPORTED_EXTENSIONS = {
    "cpp", "c", "cc", "cxx", "c++", "xls", "xlsx",
    "h", "hpp", "hh", "hxx", "h++", "doc", "docx",
    "txt", "md", "json", "xml", "html", "css", "js",
    "ini", "cfg", "conf", "pro", "pri", "yml",
    "lightcppproj", "cmake", "makefile",
    "py", "go", "rs", "java", "cs", "php"
};

// ==================== ExcludeDirProxyModel 实现 ====================
ExcludeDirProxyModel::ExcludeDirProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent) {}

void ExcludeDirProxyModel::setExcludeDirs(const QStringList& dirs)
{
    m_excludeDirs = dirs;
    invalidateFilter();
}

void ExcludeDirProxyModel::setAllowedExtensions(const QStringList& exts)
{
    m_allowedExtensions = exts;
    invalidateFilter();
}

bool ExcludeDirProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);
    if (!idx.isValid()) return true;

    auto* fsModel = qobject_cast<QFileSystemModel*>(sourceModel());
    if (!fsModel) return true;

    QString filePath = fsModel->filePath(idx);
    QFileInfo info(filePath);

    if (info.isDir()) {
        QString dirName = info.fileName();
        if (m_excludeDirs.contains(dirName) || dirName.endsWith(".dSYM"))
            return false;
        return true;
    } else {
        QString ext = info.suffix().toLower();
        if (m_allowedExtensions.isEmpty()) return true;
        return m_allowedExtensions.contains(ext);
    }
}

// ==================== ProjectPanel 实现 ====================

ProjectPanel::ProjectPanel(QWidget *parent)
    : QWidget(parent)
    , m_splitter(new QSplitter(this))
    , m_fileTreeView(nullptr)
    , m_fileWatcher(nullptr)
    , m_fileModel(nullptr)
    , m_proxyModel(nullptr)
    , m_projectConfig(nullptr)
    , m_testTreeView(nullptr)
    , m_testModel(nullptr)
    , m_contextMenu(nullptr)
{
    setupUI();
    createContextMenu();
}

ProjectPanel::~ProjectPanel()
{
    if (m_projectConfig) delete m_projectConfig;
}

// ==================== 公共方法 ====================

bool ProjectPanel::openProject(const QString& projectPath, const ProjectConfig* config)
{
    if (projectPath.isEmpty() || !QDir(projectPath).exists()) {
        qWarning() << "Project path invalid:" << projectPath;
        return false;
    }

    closeProject();

    m_projectPath = QDir(projectPath).absolutePath();

    if (config) {
        if (m_projectConfig) delete m_projectConfig;
        m_projectConfig = new ProjectConfig(*config);
    }

    QModelIndex rootIndex = m_fileModel->setRootPath(m_projectPath);
    if (!rootIndex.isValid()) {
        qWarning() << "Invalid root index for:" << m_projectPath;
        return false;
    }

    connect(m_fileModel, &QFileSystemModel::directoryLoaded,
            this, [this, rootIndex](const QString& loadedPath) {
                if (loadedPath == m_projectPath) {
                    QModelIndex proxyRoot = m_proxyModel->mapFromSource(rootIndex);
                    if (proxyRoot.isValid()) {
                        m_fileTreeView->setRootIndex(proxyRoot);
                        m_fileTreeView->expand(proxyRoot);
                    }
                    disconnect(m_fileModel, &QFileSystemModel::directoryLoaded, this, nullptr);
                }
            });

    refreshTestTree();
    m_fileWatcher->addPath(m_projectPath);
    return true;
}

void ProjectPanel::closeProject()
{
    m_fileModel->setRootPath("");
    m_fileTreeView->setRootIndex(QModelIndex());

    if (m_projectConfig) {
        delete m_projectConfig;
        m_projectConfig = nullptr;
    }

    m_projectPath.clear();
    m_testModel->clear();
    emit projectClosed();
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
    m_fileModel->setNameFilters(filters);
    m_fileModel->setNameFilterDisables(false);
}

QString ProjectPanel::selectedFilePath() const
{
    QModelIndexList selected = m_fileTreeView->selectionModel()->selectedIndexes();
    if (!selected.isEmpty()) {
        QModelIndex first = selected.first();
        if (first.column() != 0) {
            QModelIndex idx0 = m_fileTreeView->model()->index(first.row(), 0, first.parent());
            if (idx0.isValid()) first = idx0;
        }
        return getFilePath(first);
    }
    return QString();
}

QStringList ProjectPanel::selectedFilePaths() const
{
    QStringList paths;
    QModelIndexList selected = m_fileTreeView->selectionModel()->selectedIndexes();
    for (const QModelIndex& index : selected) {
        if (index.column() == 0) {
            QString path = getFilePath(index);
            if (!path.isEmpty()) paths.append(path);
        }
    }
    return paths;
}

void ProjectPanel::refresh()
{
    if (!m_projectPath.isEmpty()) {
        m_fileModel->setRootPath(m_projectPath);
        QModelIndex sourceRoot = m_fileModel->index(m_projectPath);
        if (sourceRoot.isValid()) {
            QModelIndex proxyRoot = m_proxyModel->mapFromSource(sourceRoot);
            m_fileTreeView->setRootIndex(proxyRoot);
        }
    }
    refreshTestTree();
}

void ProjectPanel::locateFile(const QString& filePath)
{
    if (filePath.isEmpty() || !QFileInfo::exists(filePath)) return;
    QModelIndex sourceIndex = m_fileModel->index(filePath);
    if (sourceIndex.isValid()) {
        QModelIndex proxyIndex = m_proxyModel->mapFromSource(sourceIndex);
        m_fileTreeView->setCurrentIndex(proxyIndex);
        m_fileTreeView->scrollTo(proxyIndex, QAbstractItemView::PositionAtCenter);
        QModelIndex parent = proxyIndex.parent();
        while (parent.isValid()) {
            m_fileTreeView->expand(parent);
            parent = parent.parent();
        }
    }
}

bool ProjectPanel::addNewFile(const QString& parentDir, const QString& fileName, bool isDirectory)
{
    if (parentDir.isEmpty() || fileName.isEmpty() || !validateFileName(fileName)) return false;

    QString fullPath = QDir(parentDir).absoluteFilePath(fileName);

    if (isDirectory) {
        if (!QDir(parentDir).mkdir(fileName)) {
            QMessageBox::critical(this, "创建失败", "无法创建目录: " + fileName);
            return false;
        }
    } else {
        QFile file(fullPath);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, "创建失败",
                                  "无法创建文件: " + fileName + "\n错误: " + file.errorString());
            return false;
        }
        file.close();
    }

    m_fileModel->setRootPath(parentDir);
    emit fileCreated(fullPath);

    QTimer::singleShot(200, [this, fullPath]() {
        QModelIndex idx = m_fileModel->index(fullPath);
        if (idx.isValid()) {
            locateFile(fullPath);
            m_fileTreeView->edit(m_proxyModel->mapFromSource(idx));
        }
    });

    return true;
}

bool ProjectPanel::renameFile(const QString& oldPath, const QString& newName)
{
    if (oldPath.isEmpty() || newName.isEmpty() || !validateFileName(newName)) return false;

    QFileInfo oldInfo(oldPath);
    QString newPath = oldInfo.dir().absoluteFilePath(newName);
    if (QFileInfo::exists(newPath)) {
        QMessageBox::warning(this, "重命名失败", "目标文件已存在: " + newName);
        return false;
    }

    if (!QFile::rename(oldPath, newPath)) {
        QMessageBox::critical(this, "重命名失败", "无法重命名文件");
        return false;
    }

    m_fileModel->setRootPath(oldInfo.dir().absolutePath());
    emit fileRenamed(oldPath, newPath);
    return true;
}

bool ProjectPanel::deleteFile(const QString& filePath, bool moveToTrash)
{
    Q_UNUSED(moveToTrash);
    if (filePath.isEmpty() || !QFileInfo::exists(filePath)) return false;

    QFileInfo info(filePath);
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "确认删除",
        QString("确定要删除 \"%1\" 吗？").arg(info.fileName()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reply != QMessageBox::Yes) return false;

    bool success = info.isDir() ? QDir(filePath).removeRecursively() : QFile::remove(filePath);
    if (success) {
        m_fileModel->setRootPath(info.dir().absolutePath());
        m_fileTreeView->setCurrentIndex(QModelIndex());
        emit fileDeleted(filePath);
    } else {
        QMessageBox::critical(this, "删除失败", "无法删除: " + info.fileName());
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
    m_excludeDirs = excludeDirs;
    if (m_proxyModel) m_proxyModel->setExcludeDirs(excludeDirs);
}

QIcon ProjectPanel::getFileIcon(const QString& filePath) const
{
    QFileInfo info(filePath);
    if (info.isDir()) return QIcon(":/icons/icons/folder.svg");
    QString ext = info.suffix().toLower();
    static QMap<QString, QString> iconMap = {
        {"cpp", ":/icons/icons/file_.svg"},
        {"c", ":/icons/icons/file_c.svg"},
        {"h", ":/icons/icons/file_h.svg"},
        {"hpp", ":/icons/icons/file_hpp.svg"},
        {"txt", ":/icons/icons/file_text.svg"},
        {"md", ":/icons/icons/file_markdown.svg"},
        {"json", ":/icons/icons/file_json.svg"},
        {"xml", ":/icons/icons/file_xml.svg"},
        {"ini", ":/icons/icons/file_config.svg"},
        {"lightcppproj", ":/icons/icons/file_project.svg"},
        {"cmake", ":/icons/icons/file_cmake.svg"},
        {"py", ":/icons/icons/file_python.svg"},
        {"java", ":/icons/icons/file_java.svg"},
        {"go", ":/icons/icons/file_go.svg"},
        {"rs", ":/icons/icons/file_rust.svg"},
        {"js", ":/icons/icons/file_js.svg"},
        {"html", ":/icons/icons/file_html.svg"},
        {"css", ":/icons/icons/file_css.svg"}
    };
    return QIcon(iconMap.value(ext, ":/icons/icons/file_default.svg"));
}

QString ProjectPanel::getFilePath(const QModelIndex &index) const
{
    if (!index.isValid()) return QString();
    QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
    return sourceIndex.isValid() ? m_fileModel->filePath(sourceIndex) : QString();
}

QModelIndex ProjectPanel::findIndexByPath(const QString &path) const
{
    if (path.isEmpty()) return QModelIndex();
    QModelIndex sourceIndex = m_fileModel->index(path);
    return sourceIndex.isValid() ? m_proxyModel->mapFromSource(sourceIndex) : QModelIndex();
}

// ==================== 公共槽 ====================

void ProjectPanel::newFile()
{
    QString parentDir = selectedParentDir();
    if (parentDir.isEmpty()) parentDir = m_projectPath;
    bool ok;
    QString name = QInputDialog::getText(this, "新建文件", "文件名:", QLineEdit::Normal,
                                         getNewFileName(parentDir, "new_file", "cpp"), &ok);
    if (ok && !name.isEmpty()) addNewFile(parentDir, name, false);
}

void ProjectPanel::newDirectory()
{
    QString parentDir = selectedParentDir();
    if (parentDir.isEmpty()) parentDir = m_projectPath;
    bool ok;
    QString name = QInputDialog::getText(this, "新建目录", "目录名:", QLineEdit::Normal,
                                         getNewFileName(parentDir, "NewFolder", ""), &ok);
    if (ok && !name.isEmpty()) addNewFile(parentDir, name, true);
}

void ProjectPanel::openSelectedFile()
{
    QString path = selectedFilePath();
    if (!path.isEmpty() && isFile(m_fileTreeView->currentIndex()))
        emit fileOpenRequested(path);
}

void ProjectPanel::renameSelectedFile()
{
    QModelIndex index = m_fileTreeView->currentIndex();
    if (index.isValid()) m_fileTreeView->edit(index);
}

void ProjectPanel::deleteSelectedFile()
{
    QStringList files = selectedFilePaths();
    if (files.isEmpty()) return;
    emit deleteRequested(files);
}

void ProjectPanel::copyFilePath()
{
    QString path = selectedFilePath();
    if (!path.isEmpty()) QApplication::clipboard()->setText(path);
}

void ProjectPanel::showInFileManager()
{
    QString path = selectedFilePath();
    if (!path.isEmpty()) {
        QFileInfo info(path);
        QDesktopServices::openUrl(QUrl::fromLocalFile(info.isDir() ? path : info.absolutePath()));
    }
}

void ProjectPanel::refreshSelected()
{
    QModelIndex idx = m_fileTreeView->currentIndex();
    if (idx.isValid()) {
        QString path = getFilePath(idx);
        if (!path.isEmpty()) m_fileModel->setRootPath(path);
    } else refresh();
}

void ProjectPanel::expandToSelected()
{
    QModelIndex idx = m_fileTreeView->currentIndex();
    while (idx.isValid()) {
        m_fileTreeView->expand(idx);
        idx = idx.parent();
    }
}

void ProjectPanel::showFileHistory()
{
    QString path = selectedFilePath();
    if (!path.isEmpty()) emit requestFileHistory(path);
}

// ==================== 事件处理 ====================

void ProjectPanel::contextMenuEvent(QContextMenuEvent *event)
{
    QPoint pos = m_fileTreeView->mapFromGlobal(event->globalPos());
    QModelIndex index = m_fileTreeView->indexAt(pos);
    if (index.isValid()) {
        m_fileTreeView->setCurrentIndex(index);
        updateContextMenu();
        m_contextMenu->exec(event->globalPos());
    } else {
        QMenu blankMenu(this);
        blankMenu.addAction("新建文件", this, &ProjectPanel::newFile);
        blankMenu.addAction("新建目录", this, &ProjectPanel::newDirectory);
        blankMenu.addSeparator();
        blankMenu.addAction("刷新", this, &ProjectPanel::refresh);
        blankMenu.exec(event->globalPos());
    }
}

// ==================== 文件树信号槽 ====================

void ProjectPanel::onFileTreeSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected);
    if (!selected.isEmpty()) {
        QModelIndex idx = selected.indexes().first();
        QString path = getFilePath(idx);
        emit selectionChanged(path);
    } else {
        emit selectionChanged(QString());
    }
}

void ProjectPanel::onFileTreeDoubleClicked(const QModelIndex &index)
{
    if (index.isValid() && isFile(index))
        emit fileOpenRequested(getFilePath(index));
}

void ProjectPanel::onDirectoryChanged(const QString &path)
{
    Q_UNUSED(path); // 可选择性刷新
}

void ProjectPanel::onFileRenamed(const QString &path, const QString &oldName, const QString &newName)
{
    Q_UNUSED(path); Q_UNUSED(oldName); Q_UNUSED(newName);
}

void ProjectPanel::onWatchedDirectoryChanged(const QString &path)
{
    QModelIndex idx = m_fileModel->index(path);
    if (idx.isValid()) {
        m_fileModel->fetchMore(idx);
        m_fileTreeView->viewport()->update();
    }
}

void ProjectPanel::onWatchedFileChanged(const QString &path)
{
    QFileInfo info(path);
    if (info.exists()) {
        QString parentDir = info.absolutePath();
        onWatchedDirectoryChanged(parentDir);
    }
}

// ==================== 测试树信号槽 ====================

void ProjectPanel::onTestTreeDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    QStandardItem* item = m_testModel->itemFromIndex(index);
    if (!item) return;

    QString fullName = item->data(Qt::UserRole + 2).toString();
    QString filePath = item->data(Qt::UserRole + 1).toString();
    if (!fullName.isEmpty() && !filePath.isEmpty()) {
        emit runTestRequested(filePath, fullName);
    }
}

// ==================== 私有方法 ====================

void ProjectPanel::setupUI()
{
    // 创建分割器（垂直方向）
    m_splitter = new QSplitter(Qt::Vertical, this);
    m_splitter->setChildrenCollapsible(false);
    m_splitter->setHandleWidth(4);

    createFileTree();
    createTestTree();

    // 将两个树添加到分割器
    m_splitter->addWidget(m_fileTreeView);
    m_splitter->addWidget(m_testTreeView);

    // 初始大小比例：文件树占 70%，测试树占 30%（但测试树默认隐藏，所以文件树占满）
    m_splitter->setStretchFactor(0, 7);
    m_splitter->setStretchFactor(1, 3);

    // 用 QVBoxLayout 包裹分割器，填满整个 ProjectPanel
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_splitter);
}

void ProjectPanel::createFileTree()
{
    m_fileTreeView = new QTreeView(this);
    m_fileTreeView->setIndentation(20);
    m_fileTreeView->setSortingEnabled(true);
    m_fileTreeView->setHeaderHidden(true);
    m_fileTreeView->setAlternatingRowColors(false);
    m_fileTreeView->setDragEnabled(true);
    m_fileTreeView->setAcceptDrops(true);
    m_fileTreeView->setDropIndicatorShown(true);
    m_fileTreeView->setDragDropMode(QAbstractItemView::DragDrop);
    m_fileTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_fileTreeView->setEditTriggers(QAbstractItemView::EditKeyPressed);
    // m_fileTreeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // 关键：让文件树水平方向尽量扩展
    m_fileTreeView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_fileModel = new QFileSystemModel(this);
    m_fileModel->setReadOnly(false);
    m_fileModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);

    m_proxyModel = new ExcludeDirProxyModel(this);
    m_proxyModel->setSourceModel(m_fileModel);
    m_proxyModel->setExcludeDirs(DEFAULT_EXCLUDE_DIRS);
    m_proxyModel->setAllowedExtensions(SUPPORTED_EXTENSIONS);

    m_fileTreeView->setModel(m_proxyModel);

    // 隐藏所有列，只保留第一列
    for (int i = 1; i < m_fileModel->columnCount(); ++i) {
        m_fileTreeView->hideColumn(i);
    }

    // 让第一列（文件名）填满可用宽度
    m_fileTreeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_fileTreeView->header()->setStretchLastSection(false);

    m_fileTreeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_fileWatcher = new QFileSystemWatcher(this);
    connect(m_fileWatcher, &QFileSystemWatcher::directoryChanged,
            this, &ProjectPanel::onWatchedDirectoryChanged);
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged,
            this, &ProjectPanel::onWatchedFileChanged);

    connect(m_fileTreeView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &ProjectPanel::onFileTreeSelectionChanged);
    connect(m_fileTreeView, &QTreeView::doubleClicked,
            this, &ProjectPanel::onFileTreeDoubleClicked);

    m_splitter->addWidget(m_fileTreeView);
}

void ProjectPanel::createTestTree()
{
    m_testTreeView = new QTreeView(this);
    m_testTreeView->setHeaderHidden(true);
    m_testTreeView->setIndentation(15);
    m_testTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_testTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_testTreeView->setStyleSheet("QTreeView { background-color: #2b2b2b; color: #cccccc; }");

    // 默认隐藏
    m_testTreeView->setVisible(false);

    m_testModel = new QStandardItemModel(this);
    m_testTreeView->setModel(m_testModel);

    connect(m_testTreeView, &QTreeView::doubleClicked,
            this, &ProjectPanel::onTestTreeDoubleClicked);

    m_splitter->addWidget(m_testTreeView);
}

void ProjectPanel::createContextMenu()
{
    m_contextMenu = new QMenu(this);
    m_newFileAction = m_contextMenu->addAction("新建文件", this, &ProjectPanel::newFile);
    m_newDirAction  = m_contextMenu->addAction("新建目录", this, &ProjectPanel::newDirectory);
    m_contextMenu->addSeparator();
    m_openFileAction = m_contextMenu->addAction("打开", this, &ProjectPanel::openSelectedFile);
    m_renameAction   = m_contextMenu->addAction("重命名", this, &ProjectPanel::renameSelectedFile);
    m_deleteAction   = m_contextMenu->addAction("删除", this, &ProjectPanel::deleteSelectedFile);
    m_contextMenu->addSeparator();
    m_copyPathAction = m_contextMenu->addAction("复制路径", this, &ProjectPanel::copyFilePath);
    m_showInExplorerAction = m_contextMenu->addAction("在文件管理器中显示", this, &ProjectPanel::showInFileManager);
    m_contextMenu->addSeparator();
    m_historyAction = m_contextMenu->addAction("查看历史", this, &ProjectPanel::showFileHistory);
    m_contextMenu->addSeparator();
    m_refreshAction = m_contextMenu->addAction("刷新", this, &ProjectPanel::refreshSelected);
    m_expandAction  = m_contextMenu->addAction("展开", this, &ProjectPanel::expandToSelected);
}

void ProjectPanel::updateContextMenu()
{
    QModelIndex idx = m_fileTreeView->currentIndex();
    bool hasSel = idx.isValid();
    bool isDir = hasSel && isDirectory(idx);
    bool isFil = hasSel && !isDir;

    m_newFileAction->setEnabled(hasSel && isDir);
    m_newDirAction->setEnabled(hasSel && isDir);
    m_openFileAction->setEnabled(hasSel && isFil);
    m_renameAction->setEnabled(hasSel);
    m_deleteAction->setEnabled(hasSel);
    m_copyPathAction->setEnabled(hasSel);
    m_showInExplorerAction->setEnabled(hasSel);
    m_refreshAction->setEnabled(hasSel);
    m_expandAction->setEnabled(hasSel && isDir);
    m_historyAction->setEnabled(hasSel && isFil);
}

bool ProjectPanel::isDirectory(const QModelIndex &index) const
{
    if (!index.isValid()) return false;
    QModelIndex src = m_proxyModel->mapToSource(index);
    return m_fileModel->isDir(src);
}

bool ProjectPanel::isFile(const QModelIndex &index) const
{
    return !isDirectory(index);
}

QString ProjectPanel::selectedParentDir() const
{
    QModelIndex idx = m_fileTreeView->currentIndex();
    if (idx.isValid()) {
        QString path = getFilePath(idx);
        if (isDirectory(idx)) return path;
        else return QFileInfo(path).absolutePath();
    }
    return m_projectPath;
}

void ProjectPanel::startRename(const QModelIndex &index)
{
    if (index.isValid()) {
        m_fileTreeView->scrollTo(index, QAbstractItemView::EnsureVisible);
        m_fileTreeView->edit(index);
    }
}

bool ProjectPanel::validateFileName(const QString& fileName) const
{
    if (fileName.isEmpty() || fileName == "." || fileName == "..") return false;
    static const QString illegalChars = "<>:\"|?*\\/";
    for (const QChar& ch : illegalChars) if (fileName.contains(ch)) return false;
    static const QStringList reserved = {"CON","PRN","AUX","NUL","COM1","COM2","COM3","COM4","COM5","COM6","COM7","COM8","COM9","LPT1","LPT2","LPT3","LPT4","LPT5","LPT6","LPT7","LPT8","LPT9"};
    if (reserved.contains(fileName.toUpper())) return false;
    if (fileName.endsWith(' ') || fileName.endsWith('.')) return false;
    return true;
}

QString ProjectPanel::getNewFileName(const QString& parentDir, const QString& baseName, const QString& extension) const
{
    QDir dir(parentDir);
    QString ext = extension.isEmpty() ? "" : "." + extension;
    for (int i = 0; i < 100; ++i) {
        QString name = (i == 0) ? baseName + ext : QString("%1%2%3").arg(baseName).arg(i).arg(ext);
        if (!dir.exists(name)) return name;
    }
    return QString("%1_%2%3").arg(baseName, QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"), ext);
}

// ==================== 测试树相关 ====================

void ProjectPanel::refreshTestTree()
{
    m_testModel->clear();

    m_testTreeView->setVisible(false);

    if (!m_projectPath.isEmpty()) {
        addPythonTestNodes(m_projectPath);
    }
}

void ProjectPanel::addPythonTestNodes(const QString& projectPath)
{
    auto model = ProjectManager::instance()->pythonProjectModel();
    if (!model) return;

    QList<PythonTestInfo> tests = PythonTestRunner::discoverTests(projectPath);
    if (tests.isEmpty()) {
        m_testTreeView->setVisible(false);
        return;
    }

    // 有测试，显示测试树
    m_testTreeView->setVisible(true);

    QStandardItem* testRoot = findOrCreateTestRoot();
    if (!testRoot) return;

    QMap<QString, QList<PythonTestInfo>> fileMap;
    for (const PythonTestInfo& test : tests) {
        fileMap[test.filePath].append(test);
    }

    testRoot->removeRows(0, testRoot->rowCount());

    for (auto it = fileMap.begin(); it != fileMap.end(); ++it) {
        QString filePath = it.key();
        QStandardItem* fileItem = new QStandardItem(QFileInfo(filePath).fileName());
        fileItem->setData(filePath, Qt::UserRole + 1);
        fileItem->setIcon(QIcon(":/icons/file.svg"));
        fileItem->setEditable(false);

        for (const PythonTestInfo& test : it.value()) {
            QStandardItem* methodItem = new QStandardItem(test.methodName);
            methodItem->setData(filePath, Qt::UserRole + 1);
            methodItem->setData(test.fullName, Qt::UserRole + 2);
            methodItem->setIcon(QIcon(":/icons/run.svg"));
            methodItem->setEditable(false);
            fileItem->appendRow(methodItem);
        }
        testRoot->appendRow(fileItem);
    }

    QModelIndex rootIndex = m_testModel->indexFromItem(testRoot);
    if (rootIndex.isValid()) m_testTreeView->expand(rootIndex);
}

QStandardItem* ProjectPanel::findOrCreateTestRoot()
{
    QStandardItem* root = m_testModel->invisibleRootItem();
    for (int i = 0; i < root->rowCount(); ++i) {
        QStandardItem* item = root->child(i);
        if (item && item->text() == "测试 (pytest)") {
            return item;
        }
    }
    QStandardItem* newRoot = new QStandardItem("测试 (pytest)");
    newRoot->setIcon(QIcon(":/icons/test.svg"));
    newRoot->setEditable(false);
    root->appendRow(newRoot);
    return newRoot;
}

} // namespace SprintIDE

#include "project_panel.moc"
