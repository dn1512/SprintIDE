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
#include <QDirIterator>
#include <QUrl>
#include <QDebug>
#include <QHeaderView>
#include <QSettings>
#include <QEvent>
#include <QDropEvent>
#include <QShortcut>

#include "python/python_test_runner.h"
#include "project_manager.h"
#include "python/python_project_model.h"
#include "git/git_manager.h"
#include "git/git_diff_dialog.h"
#include "git/git_file_history_dialog.h"
#include "theme_manager.h"

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

// ============================================================
//  自定义排序逻辑
// ------------------------------------------------------------
//  规则：
//    1. 文件夹永远排在文件前面；
//    2. 同类之间按文件名 A-Z 升序排列；
//    3. 名称比较不区分大小写（跨平台体验一致）。
// ============================================================
bool ExcludeDirProxyModel::lessThan(const QModelIndex& source_left,
                                    const QModelIndex& source_right) const
{
    // ---------- 1. 拿到源模型，并断言它是 QFileSystemModel ----------
    auto* fsModel = qobject_cast<QFileSystemModel*>(sourceModel());
    if (!fsModel) {
        // 理论上不会发生；如果不是文件系统模型，退回默认比较
        return QSortFilterProxyModel::lessThan(source_left, source_right);
    }

    // ---------- 2. 只关心第 0 列（文件名列） ----------
    //    其他列（大小 / 类型 / 修改时间）已被隐藏，但保险起见还是判断一下。
    if (source_left.column() != 0 || source_right.column() != 0) {
        return source_left.column() < source_right.column();
    }

    // ---------- 3. 取得两侧的路径与文件信息 ----------
    const QString leftPath  = fsModel->filePath(source_left);
    const QString rightPath = fsModel->filePath(source_right);

    QFileInfo leftInfo(leftPath);
    QFileInfo rightInfo(rightPath);

    const bool leftIsDir  = leftInfo.isDir();
    const bool rightIsDir = rightInfo.isDir();

    // ---------- 4. 文件夹优先 ----------
    if (leftIsDir != rightIsDir) {
        return leftIsDir;   // 左边是目录 → 返回 true → 排在前面
    }

    // ---------- 5. 同类之间：按文件名升序（不区分大小写） ----------
    return QString::compare(leftInfo.fileName(),
                            rightInfo.fileName(),
                            Qt::CaseInsensitive) < 0;
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
                    QModelIndex proxyRoot = toProxy(rootIndex);
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
    if (m_projectPath.isEmpty()) return;

    // 记录根路径；清空再设回是让 QFileSystemModel 丢弃内部缓存、
    // 真正重新扫描文件系统的可靠做法。
    const QString rootPath = m_projectPath;

    m_fileModel->setRootPath(QString());                  // 清空
    QModelIndex sourceRoot = m_fileModel->setRootPath(rootPath);  // 重新扫描

    if (sourceRoot.isValid()) {
        QModelIndex proxyRoot = toProxy(sourceRoot);
        if (proxyRoot.isValid()) {
            m_fileTreeView->setRootIndex(proxyRoot);
            m_fileTreeView->expand(proxyRoot);            // 展开根
        }
    }

    refreshTestTree();
    qDebug() << "[ProjectPanel] refresh 完成:" << rootPath;
}

void ProjectPanel::locateFile(const QString& filePath)
{
    if (filePath.isEmpty() || !QFileInfo::exists(filePath)) return;
    QModelIndex sourceIndex = m_fileModel->index(filePath);
    if (sourceIndex.isValid()) {
        QModelIndex proxyIndex = toProxy(sourceIndex);
        if (!proxyIndex.isValid()) return;
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
            QMessageBox::critical(this, tr("创建失败"), tr("无法创建目录: ") + fileName);
            return false;
        }
    } else {
        QFile file(fullPath);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, tr("创建失败"),
                                  tr("无法创建文件: ") + fileName + tr("\n错误: ") + file.errorString());
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
        QMessageBox::warning(this, tr("重命名失败"), tr("目标文件已存在: ") + newName);
        return false;
    }

    if (!QFile::rename(oldPath, newPath)) {
        QMessageBox::critical(this, tr("重命名失败"), tr("无法重命名文件"));
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
        this, tr("确认删除"),
        QString(tr("确定要删除 \"%1\" 吗？")).arg(info.fileName()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reply != QMessageBox::Yes) return false;

    bool success = info.isDir() ? QDir(filePath).removeRecursively() : QFile::remove(filePath);
    if (success) {
        m_fileModel->setRootPath(info.dir().absolutePath());
        m_fileTreeView->setCurrentIndex(QModelIndex());
        emit fileDeleted(filePath);
    } else {
        QMessageBox::critical(this, tr("删除失败"), tr("无法删除: ") + info.fileName());
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

QModelIndex ProjectPanel::toProxy(const QModelIndex& sourceIndex) const
{
    if (!sourceIndex.isValid() || !m_proxyModel || !m_gitOverlay) {
        return QModelIndex();
    }
    QModelIndex mid = m_proxyModel->mapFromSource(sourceIndex);
    return mid.isValid() ? m_gitOverlay->mapFromSource(mid) : QModelIndex();
}

QModelIndex ProjectPanel::toSource(const QModelIndex& proxyIndex) const
{
    if (!proxyIndex.isValid() || !m_proxyModel || !m_gitOverlay) {
        return QModelIndex();
    }
    QModelIndex mid = m_gitOverlay->mapToSource(proxyIndex);
    return mid.isValid() ? m_proxyModel->mapToSource(mid) : QModelIndex();
}

QString ProjectPanel::getFilePath(const QModelIndex &index) const
{
    if (!index.isValid()) return QString();
    QModelIndex sourceIndex = toSource(index);
    return sourceIndex.isValid() ? m_fileModel->filePath(sourceIndex) : QString();
}

QModelIndex ProjectPanel::findIndexByPath(const QString &path) const
{
    if (path.isEmpty()) return QModelIndex();
    QModelIndex sourceIndex = m_fileModel->index(path);
    return toProxy(sourceIndex);
}

// ==================== 公共槽 ====================

void ProjectPanel::newFile()
{
    QString parentDir = selectedParentDir();
    if (parentDir.isEmpty()) parentDir = m_projectPath;
    bool ok;
    QString name = QInputDialog::getText(this, tr("新建文件"), tr("文件名:"), QLineEdit::Normal,
                                         getNewFileName(parentDir, "new_file", "cpp"), &ok);
    if (ok && !name.isEmpty()) addNewFile(parentDir, name, false);
}

void ProjectPanel::newDirectory()
{
    QString parentDir = selectedParentDir();
    if (parentDir.isEmpty()) parentDir = m_projectPath;
    bool ok;
    QString name = QInputDialog::getText(this, tr("新建目录"), tr("目录名:"), QLineEdit::Normal,
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
    if (!index.isValid()) return;

    const QString oldPath = getFilePath(index);
    if (oldPath.isEmpty()) return;

    const QFileInfo oldInfo(oldPath);

    bool ok = false;
    const QString newName = QInputDialog::getText(
        this,
        tr("重命名"),
        tr("新名称:"),
        QLineEdit::Normal,
        oldInfo.fileName(),
        &ok);

    if (!ok || newName.isEmpty() || newName == oldInfo.fileName()) {
        return;
    }

    // 直接走 QFile::rename（renameFile 内部实现）
    if (renameFile(oldPath, newName)) {
        const QString newPath = oldInfo.dir().absoluteFilePath(newName);
        QTimer::singleShot(100, [this, newPath]() {
            locateFile(newPath);
        });
    }
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

bool ProjectPanel::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_fileTreeView->viewport()) {
        switch (event->type()) {
        case QEvent::DragEnter:
        case QEvent::DragMove: {
            auto* e = static_cast<QDragMoveEvent*>(event);
            if (e->mimeData()->hasUrls()) {
                e->setDropAction(Qt::MoveAction);
                e->accept();
                return true;   // 阻止 view 默认处理（避免触发 Copy）
            }
            break;
        }
        case QEvent::Drop: {
            auto* e = static_cast<QDropEvent*>(event);
            if (handleDrop(e)) {
                e->setDropAction(Qt::MoveAction);
                e->accept();
                return true;
            }
            break;
        }
        default:
            break;
        }
    }
    return QWidget::eventFilter(watched, event);
}

bool ProjectPanel::handleDrop(QDropEvent* event)
{
    if (!event->mimeData()->hasUrls()) return false;

    // ---------- 1. 确定目标目录 ----------
    // Qt6 用 position()，Qt5 用 pos()
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QPoint dropPos = event->position().toPoint();
#else
    const QPoint dropPos = event->pos();
#endif

    const QModelIndex idx = m_fileTreeView->indexAt(dropPos);

    QString dstDir;
    if (idx.isValid()) {
        const QString path = getFilePath(idx);
        QFileInfo info(path);
        dstDir = info.isDir() ? path : info.absolutePath();
    } else {
        // 拖到空白区域 → 项目根目录
        dstDir = m_projectPath;
    }

    if (dstDir.isEmpty()) return false;

    // ---------- 2. 逐个移动 ----------
    bool anyMoved = false;

    const QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl& url : urls) {
        const QString srcPath = url.toLocalFile();
        if (srcPath.isEmpty()) continue;

        const QFileInfo srcInfo(srcPath);
        const QString targetPath =
            QDir(dstDir).absoluteFilePath(srcInfo.fileName());

        // 目标与源相同 → 跳过
        if (QDir::cleanPath(srcPath) == QDir::cleanPath(targetPath)) {
            continue;
        }

        // 目标已存在 → 跳过（可在这里弹提示，暂不弹）
        if (QFileInfo::exists(targetPath)) {
            qWarning() << "[Drop] 目标已存在，跳过:" << targetPath;
            continue;
        }

        // 移动文件或目录
        if (movePath(srcPath, targetPath)) {
            anyMoved = true;
            qDebug() << "[Drop] 已移动:" << srcPath << "→" << targetPath;
        } else {
            qWarning() << "[Drop] 移动失败:" << srcPath << "→" << targetPath;
        }
    }

    // ---------- 3. 刷新模型 ----------
    if (anyMoved) {
        // 通知文件系统模型：源和目标目录都变了
        m_fileModel->setRootPath(dstDir);
        emit fileRenamed(QString(), QString()); // 触发外部刷新（可选）
        QTimer::singleShot(150, this, [this, dstDir]() {
            QModelIndex srcRoot = m_fileModel->index(dstDir);
            if (srcRoot.isValid()) {
                m_fileModel->fetchMore(srcRoot);
            }
        });
    }

    return anyMoved;
}

// ============================================================
// 移动文件 / 目录
//   优先 QFile::rename（同盘原子操作）
//   跨文件系统失败时回退：copy + remove
// ============================================================
bool ProjectPanel::movePath(const QString& src, const QString& dst)
{
    // 同盘 rename
    if (QFile::rename(src, dst)) {
        return true;
    }

    // 跨盘 fallback
    const QFileInfo srcInfo(src);
    if (srcInfo.isDir()) {
        // 目录：递归复制 + 删除源
        QDir srcDir(src);
        QDir dstDir(dst);
        if (!dstDir.exists() && !QDir().mkpath(dst)) {
            return false;
        }
        // 递归复制
        QDirIterator it(src, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString filePath = it.next();
            const QFileInfo fileInfo(filePath);
            const QString relPath = srcDir.relativeFilePath(filePath);
            const QString target = QDir(dst).absoluteFilePath(relPath);

            if (fileInfo.isDir()) {
                QDir().mkpath(target);
            } else {
                QDir().mkpath(QFileInfo(target).absolutePath());
                if (!QFile::copy(filePath, target)) {
                    qWarning() << "[movePath] 复制失败:" << filePath;
                    // 失败时清理
                    QDir(dst).removeRecursively();
                    return false;
                }
            }
        }
        // 删除源
        if (!QDir(src).removeRecursively()) {
            qWarning() << "[movePath] 删除源目录失败:" << src;
            return false;
        }
        return true;
    } else {
        // 文件：copy + remove
        if (!QFile::copy(src, dst)) {
            return false;
        }
        if (!QFile::remove(src)) {
            qWarning() << "[movePath] 删除源文件失败:" << src;
            QFile::remove(dst);   // 回滚
            return false;
        }
        return true;
    }
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
        blankMenu.addAction(tr("新建文件"), this, &ProjectPanel::newFile);
        blankMenu.addAction(tr("新建目录"), this, &ProjectPanel::newDirectory);
        blankMenu.addSeparator();
        blankMenu.addAction(tr("刷新"), this, &ProjectPanel::refresh);
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
    layout->setSpacing(0);

    setupToolbar(layout);

    m_splitter = new QSplitter(Qt::Vertical, this);
    m_splitter->setChildrenCollapsible(false);
    m_splitter->setHandleWidth(4);

    createFileTree();
    createTestTree();

    m_splitter->setStretchFactor(0, 7);
    m_splitter->setStretchFactor(1, 3);

    layout->addWidget(m_splitter, /*stretch=*/1);
}

// ============================================================
//  顶部工具栏：定位文件 / 刷新
//  - 按钮风格跟随主题（QSS 全局规则 + autoRaise）
//  - 定位：发信号给 MainWindow，由它决定"当前文件"是谁
//  - 刷新：直接调 onRefreshRequested()，无参数
// ============================================================
void ProjectPanel::setupToolbar(QVBoxLayout* parentLayout)
{
    QWidget* toolbar = new QWidget(this);
    toolbar->setObjectName("ProjectPanelToolbar");
    toolbar->setFixedHeight(22);                              // 26 → 22，更紧凑
    toolbar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QHBoxLayout* tl = new QHBoxLayout(toolbar);
    tl->setContentsMargins(6, 2, 6, 2);
    tl->setSpacing(4);

    // ★ 关键 1：stretch 放最前面 → 后续控件全部靠右
    tl->addStretch(1);

    // ============================================================
    //  定位按钮
    // ============================================================
    m_locateButton = new QToolButton(toolbar);

    m_locateButton->setObjectName("ProjectPanelLocateButton"); // 供 QSS 精确命中
    m_locateButton->setToolTip(tr("定位到当前正在编辑的文件"));
    m_locateButton->setAutoRaise(true);
    m_locateButton->setFocusPolicy(Qt::NoFocus);
    m_locateButton->setCursor(Qt::PointingHandCursor);

    // 只显示图标
    m_locateButton->setToolButtonStyle(Qt::ToolButtonIconOnly);

    // 图标 12×12，按钮 12×12
    m_locateButton->setIconSize(QSize(14, 14));
    m_locateButton->setFixedSize(14, 14);

    m_locateButton->setIcon(QIcon(":icons/icons/locate.svg"));

    connect(m_locateButton, &QToolButton::clicked,
            this, &ProjectPanel::locateCurrentFileRequested);
    tl->addWidget(m_locateButton);

    // ============================================================
    //  刷新按钮
    // ============================================================
    m_refreshButton = new QToolButton(toolbar);
    // m_refreshButton->setIcon(QIcon(":icons/icons/refresh.svg"));
    m_refreshButton->setObjectName("ProjectPanelRefreshButton");
    m_refreshButton->setToolTip(tr("刷新项目面板（重新加载根目录下的所有内容）"));
    m_refreshButton->setAutoRaise(true);
    m_refreshButton->setFocusPolicy(Qt::NoFocus);
    m_refreshButton->setCursor(Qt::PointingHandCursor);

    m_refreshButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_refreshButton->setIconSize(QSize(14, 14));
    m_refreshButton->setFixedSize(14, 14);

    m_refreshButton->setIcon(
        style()->standardIcon(QStyle::SP_BrowserReload));

    connect(m_refreshButton, &QToolButton::clicked,
            this, &ProjectPanel::onRefreshRequested);
    tl->addWidget(m_refreshButton);

    parentLayout->addWidget(toolbar);
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
    m_fileTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_fileTreeView->viewport()->installEventFilter(this);
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

    // 最外层：叠加 Git 状态圆点
    m_gitOverlay = new Git::GitOverlayProxyModel(this);
    m_gitOverlay->setSourceModel(m_proxyModel);
    m_gitOverlay->setFileSystemModel(m_fileModel);
    m_gitOverlay->setIntermediateProxy(m_proxyModel);

    m_fileTreeView->setModel(m_gitOverlay);

    // 显式触发一次按第 0 列升序排序
    //   - 表头隐藏时 QTreeView 不会自动排序，必须手动调用
    //   - 立即生效，避免首帧乱序闪烁
    m_fileTreeView->sortByColumn(0, Qt::AscendingOrder);

    // 隐藏所有列，只保留第一列
    for (int i = 1; i < m_fileModel->columnCount(); ++i) {
        m_fileTreeView->hideColumn(i);
    }

    // 让第一列（文件名）填满可用宽度
    m_fileTreeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_fileTreeView->header()->setStretchLastSection(false);

    m_fileTreeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_fileWatcher = new QFileSystemWatcher(this);

    auto* renameShortcut = new QShortcut(QKeySequence(Qt::Key_F2), m_fileTreeView);
    renameShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(renameShortcut, &QShortcut::activated,
            this, &ProjectPanel::renameSelectedFile);
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
    m_testTreeView->setObjectName("TestTreeView");
    m_testTreeView->setHeaderHidden(true);
    m_testTreeView->setIndentation(15);
    m_testTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_testTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

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
    m_newFileAction = m_contextMenu->addAction(tr("新建文件"), this, &ProjectPanel::newFile);
    m_newDirAction  = m_contextMenu->addAction(tr("新建目录"), this, &ProjectPanel::newDirectory);
    m_contextMenu->addSeparator();
    m_openFileAction = m_contextMenu->addAction(tr("打开"), this, &ProjectPanel::openSelectedFile);
    m_renameAction   = m_contextMenu->addAction(tr("重命名"), this, &ProjectPanel::renameSelectedFile);
    m_deleteAction   = m_contextMenu->addAction(tr("删除"), this, &ProjectPanel::deleteSelectedFile);
    m_contextMenu->addSeparator();
    m_copyPathAction = m_contextMenu->addAction(tr("复制路径"), this, &ProjectPanel::copyFilePath);
    m_showInExplorerAction = m_contextMenu->addAction(tr("在文件管理器中显示"), this, &ProjectPanel::showInFileManager);
    m_contextMenu->addSeparator();
    m_historyAction = m_contextMenu->addAction(tr("查看本地历史"), this, &ProjectPanel::showFileHistory);

    // ---- Git 子菜单（插在"刷新"之前，与 PyCharm 位置一致） ----
    buildGitSubMenu();

    m_contextMenu->addSeparator();
    m_refreshAction = m_contextMenu->addAction(tr("刷新"), this, &ProjectPanel::refreshSelected);
    m_expandAction  = m_contextMenu->addAction(tr("展开"), this, &ProjectPanel::expandToSelected);
}

void ProjectPanel::buildGitSubMenu()
{
    m_gitSubMenu = m_contextMenu->addMenu(QStringLiteral("Git"));
    m_gitSubMenu->setIcon(
        QIcon(QStringLiteral(":/icons/icons/git_branch.svg")));

    // 辅助：创建菜单项并连接动作
    auto addItem = [this](QAction*& out,
                          const QString& text,
                          GitContextAction action,
                          const QString& iconPath = QString(),
                          bool enabled = true) {
        if (iconPath.isEmpty()) {
            out = m_gitSubMenu->addAction(text);
        } else {
            out = m_gitSubMenu->addAction(QIcon(iconPath), text);
        }
        out->setEnabled(enabled);
        if (enabled) {
            QObject::connect(out, &QAction::triggered, this, [this, action]() {
                // 触发时用最近一次右键选中的路径
                emit gitActionRequested(action, m_lastSelectedPaths);
            });
        } else {
            out->setToolTip(QStringLiteral("将在后续版本实现"));
        }
        return out;
    };

    // ---------- 文件操作 ----------
    addItem(m_gitCommitAct,        QStringLiteral("提交..."),
            GitContextAction::Commit,
            QStringLiteral(":/icons/icons/git_commit.svg"));

    addItem(m_gitAddAct,           QStringLiteral("添加"),
            GitContextAction::Add);

    m_gitSubMenu->addSeparator();

    addItem(m_gitAnnotateAct,      QStringLiteral("使用 Git 追溯注解"),
            GitContextAction::Annotate, QString(), /*enabled=*/false);

    addItem(m_gitShowDiffAct,      QStringLiteral("显示差异"),
            GitContextAction::ShowDiff);

    addItem(m_gitCompareRevAct,    QStringLiteral("与修订比较..."),
            GitContextAction::CompareWithRevision, QString(), /*enabled=*/false);

    addItem(m_gitCompareBranchAct, QStringLiteral("与分支比较..."),
            GitContextAction::CompareWithBranch, QString(), /*enabled=*/false);

    addItem(m_gitShowHistoryAct,   QStringLiteral("显示历史记录"),
            GitContextAction::ShowHistory);

    addItem(m_gitShowRevisionAct,  QStringLiteral("显示当前修订"),
            GitContextAction::ShowCurrentRevision);

    addItem(m_gitRollbackAct,      QStringLiteral("回滚..."),
            GitContextAction::Rollback);

    m_gitSubMenu->addSeparator();

    // ---------- 仓库操作 ----------
    addItem(m_gitPushAct,          QStringLiteral("推送..."),
            GitContextAction::Push,
            QStringLiteral(":/icons/icons/git_push.svg"));

    addItem(m_gitPullAct,          QStringLiteral("拉取..."),
            GitContextAction::Pull,
            QStringLiteral(":/icons/icons/git_pull.svg"));

    addItem(m_gitFetchAct,         QStringLiteral("提取"),
            GitContextAction::Fetch,
            QStringLiteral(":/icons/icons/git_refresh.svg"));

    m_gitSubMenu->addSeparator();

    addItem(m_gitMergeAct,         QStringLiteral("合并..."),
            GitContextAction::Merge, QString(), /*enabled=*/false);

    addItem(m_gitRebaseAct,        QStringLiteral("变基..."),
            GitContextAction::Rebase, QString(), /*enabled=*/false);

    addItem(m_gitBranchesAct,      QStringLiteral("分支..."),
            GitContextAction::Branches, QString(), /*enabled=*/false);

    addItem(m_gitNewBranchAct,     QStringLiteral("新建分支..."),
            GitContextAction::NewBranch,
            QStringLiteral(":/icons/icons/git_branch.svg"));

    addItem(m_gitNewTagAct,        QStringLiteral("新建标记..."),
            GitContextAction::NewTag, QString(), /*enabled=*/false);

    addItem(m_gitResetHeadAct,     QStringLiteral("重置 HEAD..."),
            GitContextAction::ResetHead, QString(), /*enabled=*/false);

    m_gitSubMenu->addSeparator();

    addItem(m_gitStashChangesAct,  QStringLiteral("存储变更..."),
            GitContextAction::StashChanges);

    addItem(m_gitUnstashAct,       QStringLiteral("取消存储变更..."),
            GitContextAction::UnstashChanges, QString(), /*enabled=*/false);

    m_gitSubMenu->addSeparator();

    addItem(m_gitManageRemotesAct, QStringLiteral("管理远程..."),
            GitContextAction::ManageRemotes, QString(), /*enabled=*/false);
}

void ProjectPanel::updateContextMenu()
{
    QModelIndex idx = m_fileTreeView->currentIndex();
    bool hasSel = idx.isValid();
    bool isDir = hasSel && isDirectory(idx);
    bool isFil = hasSel && !isDir;

    m_newFileAction->setEnabled(hasSel);
    m_newDirAction->setEnabled(hasSel);
    m_openFileAction->setEnabled(hasSel && isFil);
    m_renameAction->setEnabled(hasSel);
    m_deleteAction->setEnabled(hasSel);
    m_copyPathAction->setEnabled(hasSel);
    m_showInExplorerAction->setEnabled(hasSel);
    m_refreshAction->setEnabled(hasSel);
    m_expandAction->setEnabled(hasSel && isDir);
    m_historyAction->setEnabled(hasSel && isFil);

    updateGitSubMenuState();
}

void ProjectPanel::updateGitSubMenuState()
{
    if (!m_gitSubMenu) return;

    // 整个 Git 子菜单只在当前项目是 Git 仓库时显示
    const auto& gm = Git::GitManager::instance();
    if (!gm.isActive()) {
        m_gitSubMenu->menuAction()->setVisible(false);
        m_lastSelectedPaths.clear();
        return;
    }
    m_gitSubMenu->menuAction()->setVisible(true);

    // 记录当前选中的文件（绝对路径）
    m_lastSelectedPaths = selectedFilePaths();

    // 状态判定
    const QModelIndex idx = m_fileTreeView->currentIndex();
    const bool hasSel  = idx.isValid();
    const bool isDir   = hasSel && isDirectory(idx);
    const bool isFile  = hasSel && !isDir;

    // 判断选中文件是否已被 git 跟踪
    bool isTracked = false;
    bool isUntracked = false;
    if (isFile && !m_lastSelectedPaths.isEmpty()) {
        const QString absPath = m_lastSelectedPaths.first();
        for (const auto& s : gm.currentStatus()) {
            if (s.absolutePath == absPath) {
                if (s.isUntracked()) isUntracked = true;
                else isTracked = true;
                break;
            }
        }
        // 未出现在 status 里 → 已跟踪但当前干净（已提交）
        if (!isUntracked && !isTracked) isTracked = true;
    }

    // ---- 文件级动作 ----
    if (m_gitAddAct) {
        // 目录也可"添加"（git add <dir>）
        m_gitAddAct->setEnabled(hasSel);
    }
    if (m_gitShowDiffAct) {
        // 目录 / 未跟踪文件无 diff
        m_gitShowDiffAct->setEnabled(isFile && isTracked);
    }
    if (m_gitShowHistoryAct) {
        m_gitShowHistoryAct->setEnabled(isFile && isTracked);
    }
    if (m_gitShowRevisionAct) {
        m_gitShowRevisionAct->setEnabled(isFile && isTracked);
    }
    if (m_gitRollbackAct) {
        // 有未提交的更改才允许回滚
        bool hasChanges = false;
        if (isFile && !m_lastSelectedPaths.isEmpty()) {
            const QString absPath = m_lastSelectedPaths.first();
            for (const auto& s : gm.currentStatus()) {
                if (s.absolutePath == absPath
                    && (s.isStaged() || s.hasUnstagedChanges() || s.isUntracked())) {
                    hasChanges = true;
                    break;
                }
            }
        }
        m_gitRollbackAct->setEnabled(hasChanges);
    }

    // ---- 仓库级动作 ----
    const bool hasRemote = gm.hasRemote();
    if (m_gitCommitAct)   m_gitCommitAct->setEnabled(true);
    if (m_gitPushAct)     m_gitPushAct->setEnabled(hasRemote);
    if (m_gitPullAct)     m_gitPullAct->setEnabled(hasRemote);
    if (m_gitFetchAct)    m_gitFetchAct->setEnabled(hasRemote);
    if (m_gitNewBranchAct) m_gitNewBranchAct->setEnabled(true);
    if (m_gitStashChangesAct) {
        // 有未提交更改才允许 stash
        bool dirty = false;
        for (const auto& s : gm.currentStatus()) {
            if (s.isStaged() || s.hasUnstagedChanges() || s.isUntracked()) {
                dirty = true;
                break;
            }
        }
        m_gitStashChangesAct->setEnabled(dirty);
    }
}

bool ProjectPanel::isDirectory(const QModelIndex &index) const
{
    if (!index.isValid()) return false;
    QModelIndex src = toSource(index);
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
        if (item && item->text() == tr("测试 (pytest)")) {
            return item;
        }
    }
    QStandardItem* newRoot = new QStandardItem(tr("测试 (pytest)"));
    newRoot->setIcon(QIcon(":/icons/test.svg"));
    newRoot->setEditable(false);
    root->appendRow(newRoot);
    return newRoot;
}

// ============================================================
//  外部请求刷新（按钮 / MainWindow 信号）→ 统一走这里
// ============================================================
void ProjectPanel::onRefreshRequested()
{
    refresh();
}

// ==================== Git 状态叠加 ====================

void ProjectPanel::onGitStatusUpdated(const QList<Git::GitFileStatus>& status)
{
    if (!m_gitOverlay) return;

    // 组装 absolutePath → FileStatus 的映射
    //   优先级：工作区状态 > 暂存区状态（因为用户更关心"当前文件有没有变"）
    //   注意：map 的 value 是枚举 FileStatus，不是结构体 GitFileStatus
    QMap<QString, Git::FileStatus> map;
    for (const Git::GitFileStatus& s : status) {
        if (s.absolutePath.isEmpty()) continue;

        Git::FileStatus shown = s.workTreeStatus;
        if (shown == Git::FileStatus::Unmodified) {
            shown = s.indexStatus;
        }
        if (shown == Git::FileStatus::Unmodified) continue;

        map.insert(s.absolutePath, shown);
    }

    m_gitOverlay->setStatusMap(map);
    // 触发重绘
    if (m_fileTreeView) {
        m_fileTreeView->viewport()->update();
    }
}

} // namespace SprintIDE

#include "project_panel.moc"
