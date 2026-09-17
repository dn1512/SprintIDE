// project_selection_dialog.cpp
#include "project_selection_dialog.h"

#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QFont>
#include <QPalette>
#include <QMessageBox>
#include <QDateTime>
#include <QInputDialog>
#include <QPushButton>
#include <QMenu>
#include <QShortcut>
#include <QKeySequence>

#include "project_manager.h"   // 用于持久化移除操作

namespace SprintIDE {

// ==================== 构造函数 / 析构函数 ====================

ProjectSelectionDialog::ProjectSelectionDialog(const QMap<QString, QDateTime>& recentProjects,
                                               QWidget *parent)
    : QDialog(parent),
    m_selectedProjectPath(""),
    m_createNewProject(false)
{
    setWindowTitle("选择或创建项目");
    setMinimumSize(600, 400);
    setModal(true);
    setStyleSheet("QDialog { background-color: #1e1e1e; } "
                  "QLabel { color: #ffffff; } "
                  "QPushButton { color: #ffffff; } "
                  "QComboBox { color: #ffffff; }");

    initUI();
    loadRecentProjects(recentProjects);
}

ProjectSelectionDialog::~ProjectSelectionDialog()
{
}

// ==================== 公共接口 ====================

QString ProjectSelectionDialog::selectedProjectPath() const
{
    return m_selectedProjectPath;
}

bool ProjectSelectionDialog::isCreateNewProject() const
{
    return m_createNewProject;
}

// ==================== 槽函数：原有交互 ====================

void ProjectSelectionDialog::onProjectItemDoubleClicked(QListWidgetItem* item)
{
    if (!item) return;
    // 占位项不可操作
    if (!(item->flags() & Qt::ItemIsSelectable)) return;

    m_selectedProjectPath = item->data(Qt::UserRole).toString();
    accept();
}

void ProjectSelectionDialog::onOpenExistingProject()
{
    QString dirPath = QFileDialog::getExistingDirectory(this, "选择项目目录");
    if (!dirPath.isEmpty()) {
        m_selectedProjectPath = dirPath;
        accept();
    }
}

void ProjectSelectionDialog::onCreateNewProject()
{
    m_createNewProject = true;
    accept();
}

void ProjectSelectionDialog::onProjectSelectionChanged(QListWidgetItem* current,
                                                       QListWidgetItem* previous)
{
    Q_UNUSED(previous);

    if (!current || !(current->flags() & Qt::ItemIsSelectable)) {
        m_infoLabel->clear();
        m_selectedProjectPath.clear();
        return;
    }

    const QString projectPath = current->data(Qt::UserRole).toString();
    showProjectInfo(projectPath);
    m_selectedProjectPath = projectPath;
}

// 从历史中移除

void ProjectSelectionDialog::onProjectListContextMenuRequested(const QPoint& pos)
{
    QListWidgetItem* item = m_projectList->itemAt(pos);

    // 空处右键：只提供"打开现有项目 / 创建新项目"
    if (!item || !(item->flags() & Qt::ItemIsSelectable)) {
        QMenu menu(this);
        QAction* openAct   = menu.addAction("打开现有项目...");
        QAction* createAct = menu.addAction("创建新项目...");
        QAction* chosen = menu.exec(m_projectList->viewport()->mapToGlobal(pos));

        if (chosen == openAct)   onOpenExistingProject();
        if (chosen == createAct) onCreateNewProject();
        return;
    }

    // 有效项右键
    m_projectList->setCurrentItem(item);

    QMenu menu(this);
    QAction* openAct   = menu.addAction("打开项目");
    menu.addSeparator();
    QAction* removeAct = menu.addAction("从历史记录中移除");
    removeAct->setShortcut(QKeySequence::Delete);
    menu.addSeparator();
    QAction* createAct = menu.addAction("创建新项目...");
    QAction* browseAct = menu.addAction("打开其他项目...");

    QAction* chosen = menu.exec(m_projectList->viewport()->mapToGlobal(pos));

    if (chosen == openAct) {
        m_selectedProjectPath = item->data(Qt::UserRole).toString();
        accept();
    } else if (chosen == removeAct) {
        removeProjectItem(item);
    } else if (chosen == createAct) {
        onCreateNewProject();
    } else if (chosen == browseAct) {
        onOpenExistingProject();
    }
}

void ProjectSelectionDialog::onRemoveSelectedProject()
{
    QListWidgetItem* item = m_projectList->currentItem();
    if (!item) return;
    if (!(item->flags() & Qt::ItemIsSelectable)) return;

    removeProjectItem(item);
}

void ProjectSelectionDialog::removeProjectItem(QListWidgetItem* item)
{
    if (!item) return;

    const QString projectPath = item->data(Qt::UserRole).toString();
    if (projectPath.isEmpty()) return;

    // ---------- 1. 弹出确认框 ----------
    const QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "从历史记录中移除",
        QString("确定要把以下项目从历史记录中移除吗？\n\n"
                "%1\n\n"
                "· 只移除历史记录\n"
                "· 磁盘上的项目文件不会被删除")
            .arg(projectPath),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    // ---------- 2. 通过 ProjectManager 持久化移除 ----------
    // 注意：ProjectManager 需要提供 removeRecentProject() 方法
    ProjectManager* pm = ProjectManager::instance();
    if (pm) {
        pm->removeRecentProject(projectPath);
    }

    // ---------- 3. 从 UI 列表移除 ----------
    const int row = m_projectList->row(item);
    delete m_projectList->takeItem(row);

    // ---------- 4. 清空 info 面板 / 选中状态 ----------
    if (m_selectedProjectPath == projectPath) {
        m_selectedProjectPath.clear();
    }
    m_infoLabel->clear();

    // ---------- 5. 如果列表空了，补一个占位项 ----------
    ensureEmptyPlaceholder();
}

// ==================== 私有辅助 ====================

void ProjectSelectionDialog::initUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // ---------- 标题 ----------
    QLabel* titleLabel = new QLabel("请选择要打开的项目，或创建新项目", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("QLabel { color: #ffffff; }");
    mainLayout->addWidget(titleLabel);

    mainLayout->addSpacing(20);

    // ---------- 最近项目标签 ----------
    QLabel* recentProjectsLabel = new QLabel("最近使用的项目：", this);
    recentProjectsLabel->setStyleSheet("QLabel { color: #ffffff; }");
    mainLayout->addWidget(recentProjectsLabel);

    // ---------- 列表 ----------
    m_projectList = new QListWidget(this);
    m_projectList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_projectList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_projectList->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // 启用自定义右键菜单
    m_projectList->setContextMenuPolicy(Qt::CustomContextMenu);

    m_projectList->setStyleSheet(
        "QListWidget { "
        "    background-color: #2c2c2c; "
        "    border: 1px solid #444444; "
        "    border-radius: 4px; "
        "    padding: 4px; "
        "}"
        "QListWidget::item { "
        "    padding: 8px; "
        "    border-bottom: 1px solid #444444; "
        "    color: #ffffff; "
        "}"
        "QListWidget::item:hover { "
        "    background-color: #3a3a3a; "
        "}"
        "QListWidget::item:selected { "
        "    background-color: #1890ff; "
        "    color: #ffffff; "
        "}"
        );

    mainLayout->addWidget(m_projectList);

    // ---------- 项目信息面板 ----------
    m_infoLabel = new QLabel(this);
    m_infoLabel->setWordWrap(true);
    m_infoLabel->setStyleSheet("QLabel { background-color: #2c2c2c; "
                               "color: #ffffff; padding: 8px; "
                               "border: 1px solid #444444; border-radius: 4px; }");
    mainLayout->addWidget(m_infoLabel);

    mainLayout->addSpacing(20);

    // ---------- 中部按钮 ----------
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    m_openButton = new QPushButton("打开现有项目", this);
    m_openButton->setIcon(QIcon(":/icons/icons/open_project.svg"));
    buttonLayout->addWidget(m_openButton);

    m_createButton = new QPushButton("创建新项目", this);
    m_createButton->setIcon(QIcon(":/icons/icons/new_project.svg"));
    buttonLayout->addWidget(m_createButton);

    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    // ---------- 底部按钮 ----------
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel, Qt::Horizontal, this);
    mainLayout->addWidget(m_buttonBox);

    // ==================== 信号连接 ====================

    connect(m_projectList, &QListWidget::itemDoubleClicked,
            this, &ProjectSelectionDialog::onProjectItemDoubleClicked);
    connect(m_projectList, &QListWidget::currentItemChanged,
            this, &ProjectSelectionDialog::onProjectSelectionChanged);
    connect(m_openButton, &QPushButton::clicked,
            this, &ProjectSelectionDialog::onOpenExistingProject);
    connect(m_createButton, &QPushButton::clicked,
            this, &ProjectSelectionDialog::onCreateNewProject);
    connect(m_buttonBox, &QDialogButtonBox::rejected,
            this, &ProjectSelectionDialog::reject);

    // 右键菜单
    connect(m_projectList, &QListWidget::customContextMenuRequested,
            this, &ProjectSelectionDialog::onProjectListContextMenuRequested);

    // Delete 键快捷方式（作用于整个对话框，但只在列表有选中项时生效）
    m_deleteShortcut = new QShortcut(QKeySequence::Delete, m_projectList);
    m_deleteShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(m_deleteShortcut, &QShortcut::activated,
            this, &ProjectSelectionDialog::onRemoveSelectedProject);
}

void ProjectSelectionDialog::loadRecentProjects(const QMap<QString, QDateTime>& recentProjects)
{
    m_projectList->clear();

    // ---------- 排序：按时间降序 ----------
    QList<QPair<QString, QDateTime>> projectList;
    for (auto it = recentProjects.constBegin(); it != recentProjects.constEnd(); ++it) {
        projectList.append(qMakePair(it.key(), it.value()));
    }
    std::sort(projectList.begin(), projectList.end(),
              [](const QPair<QString, QDateTime>& a,
                 const QPair<QString, QDateTime>& b) {
                  return a.second > b.second;
              });

    // ---------- 插入项 ----------
    for (const QPair<QString, QDateTime>& project : projectList) {
        QFileInfo fileInfo(project.first);
        QString projectName = fileInfo.dir().dirName();
        QString displayText = QString("%1 (%2)").arg(projectName,
                                                     fileInfo.dir().absolutePath());

        QListWidgetItem* item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, project.first);

        // 提示
        QString toolTip = QString("项目路径: %1\n上次打开: %2\n\n"
                                  "右键可移除历史记录")
                              .arg(project.first)
                              .arg(project.second.toString("yyyy-MM-dd HH:mm:ss"));
        item->setToolTip(toolTip);

        // 图标
        if (fileInfo.isFile() && fileInfo.suffix() == "lightcppproj") {
            item->setIcon(QIcon(":/icons/icons/project_file.svg"));
        } else {
            item->setIcon(QIcon(":/icons/icons/project_folder.svg"));
        }

        m_projectList->addItem(item);
    }

    // ---------- 空列表占位 ----------
    ensureEmptyPlaceholder();
}

void ProjectSelectionDialog::showProjectInfo(const QString& projectPath)
{
    if (projectPath.isEmpty()) {
        m_infoLabel->clear();
        return;
    }

    QFileInfo fileInfo(projectPath);
    QString projectName   = fileInfo.dir().dirName();
    QString projectDir    = fileInfo.dir().absolutePath();
    QString fileSize      = QString::number(fileInfo.size() / 1024.0, 'f', 2) + " KB";
    QString lastModified  = fileInfo.lastModified().toString("yyyy-MM-dd HH:mm:ss");

    QString infoText = QString("项目名称: <b>%1</b><br>").arg(projectName)
                       + QString("项目路径: <b>%1</b><br>").arg(projectDir)
                       + QString("配置文件大小: <b>%1</b><br>").arg(fileSize)
                       + QString("上次修改时间: <b>%1</b>").arg(lastModified);

    m_infoLabel->setText(infoText);
}

// 确保空列表有占位项
void ProjectSelectionDialog::ensureEmptyPlaceholder()
{
    if (m_projectList->count() > 0) return;

    QListWidgetItem* emptyItem = new QListWidgetItem("没有最近使用的项目");
    emptyItem->setFlags(Qt::NoItemFlags);   // 禁用选择
    emptyItem->setForeground(QBrush(QColor("#888888")));
    m_projectList->addItem(emptyItem);
}

} // namespace SprintIDE
