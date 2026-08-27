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

namespace SprintIDE {

ProjectSelectionDialog::ProjectSelectionDialog(const QMap<QString, QDateTime>& recentProjects,
                                               QWidget *parent)
    : QDialog(parent),
    m_selectedProjectPath(""),
    m_createNewProject(false)
{
    setWindowTitle("选择或创建项目");
    setMinimumSize(600, 400);
    setModal(true);
    setStyleSheet("QDialog { background-color: #1e1e1e; } QLabel { color: #ffffff; } QPushButton { color: #ffffff; } QComboBox { color: #ffffff; }");

    initUI();
    loadRecentProjects(recentProjects);
}

ProjectSelectionDialog::~ProjectSelectionDialog()
{
}

QString ProjectSelectionDialog::selectedProjectPath() const
{
    return m_selectedProjectPath;
}

bool ProjectSelectionDialog::isCreateNewProject() const
{
    return m_createNewProject;
}

void ProjectSelectionDialog::onProjectItemDoubleClicked(QListWidgetItem* item)
{
    if (item) {
        m_selectedProjectPath = item->data(Qt::UserRole).toString();
        accept();
    }
}

void ProjectSelectionDialog::onOpenExistingProject()
{
    // 使用QFileDialog选择项目文件或目录
    QString filePath = QFileDialog::getExistingDirectory(this, "选择项目目录");
    if (!filePath.isEmpty()) {
        // 检查是否是项目目录（包含.lightcppproj文件）
        QDir projectDir(filePath);
        QStringList filters;
        filters << "*.lightcppproj";
        projectDir.setNameFilters(filters);
        QStringList projectFiles = projectDir.entryList();

        if (!projectFiles.isEmpty()) {
            // 找到项目配置文件
            m_selectedProjectPath = projectDir.absoluteFilePath(projectFiles.first());
            accept();
        } else {
            // 目录中没有项目配置文件，提示用户创建
            int ret = QMessageBox::question(this, "创建项目",
                                            "该目录中未找到项目配置文件，是否要创建新项目？",
                                            QMessageBox::Yes | QMessageBox::No);
            if (ret == QMessageBox::Yes) {
                // 进入创建新项目流程
                m_createNewProject = true;
                accept();
            }
        }
    }
}

void ProjectSelectionDialog::onCreateNewProject()
{
    m_createNewProject = true;
    accept();
}

void ProjectSelectionDialog::onProjectSelectionChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
    Q_UNUSED(previous);

    if (current) {
        QString projectPath = current->data(Qt::UserRole).toString();
        showProjectInfo(projectPath);
        m_selectedProjectPath = projectPath;
    } else {
        m_infoLabel->clear();
        m_selectedProjectPath.clear();
    }
}

void ProjectSelectionDialog::initUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 标题
    QLabel* titleLabel = new QLabel("请选择要打开的项目，或创建新项目", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("QLabel { color: #ffffff; }");  // 设置标题文字颜色为白色
    mainLayout->addWidget(titleLabel);

    mainLayout->addSpacing(20);

    // 项目列表
    QLabel* recentProjectsLabel = new QLabel("最近使用的项目：", this);
    recentProjectsLabel->setStyleSheet("QLabel { color: #ffffff; }");  // 设置标签文字颜色为白色
    mainLayout->addWidget(recentProjectsLabel);

    m_projectList = new QListWidget(this);
    m_projectList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_projectList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_projectList->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // 设置列表项样式
    m_projectList->setStyleSheet(
        "QListWidget { "
        "    background-color: #2c2c2c; "  // 深色背景
        "    border: 1px solid #444444; "
        "    border-radius: 4px; "
        "    padding: 4px; "
        "}"
        "QListWidget::item { "
        "    padding: 8px; "
        "    border-bottom: 1px solid #444444; "
        "    color: #ffffff; "  // 设置默认文字颜色为白色
        "}"
        "QListWidget::item:hover { "
        "    background-color: #3a3a3a; "  // 稍微浅一点的灰色
        "}"
        "QListWidget::item:selected { "
        "    background-color: #1890ff; "  // 蓝色背景
        "    color: #ffffff; "
        "}"
        );

    mainLayout->addWidget(m_projectList);

    // 项目信息
    m_infoLabel = new QLabel(this);
    m_infoLabel->setWordWrap(true);
    m_infoLabel->setStyleSheet("QLabel { background-color: #2c2c2c; color: #ffffff; padding: 8px; border: 1px solid #444444; border-radius: 4px; }");
    mainLayout->addWidget(m_infoLabel);

    mainLayout->addSpacing(20);

    // 按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    m_openButton = new QPushButton("打开现有项目", this);
    m_openButton->setIcon(QIcon(":/icons/icons/open_project.svg"));
    buttonLayout->addWidget(m_openButton);

    m_createButton = new QPushButton("创建新项目", this);
    m_createButton->setIcon(QIcon(":/icons/icons/new_project.svg"));
    buttonLayout->addWidget(m_createButton);

    buttonLayout->addStretch();

    mainLayout->addLayout(buttonLayout);

    // 底部按钮组
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel, Qt::Horizontal, this);
    mainLayout->addWidget(m_buttonBox);

    // 连接信号槽
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
}

void ProjectSelectionDialog::loadRecentProjects(const QMap<QString, QDateTime>& recentProjects)
{
    m_projectList->clear();

    // 将项目按最近使用时间排序
    QMap<QString, QDateTime> sortedProjects = recentProjects;
    // 转换为列表并排序
    QList<QPair<QString, QDateTime>> projectList;
    for (auto it = sortedProjects.constBegin(); it != sortedProjects.constEnd(); ++it) {
        projectList.append(qMakePair(it.key(), it.value()));
    }

    // 按时间降序排序
    std::sort(projectList.begin(), projectList.end(),
              [](const QPair<QString, QDateTime>& a, const QPair<QString, QDateTime>& b) {
                  return a.second > b.second;
              });

    // 添加到列表视图
    for (const QPair<QString, QDateTime>& project : projectList) {
        QFileInfo fileInfo(project.first);
        QString projectName = fileInfo.dir().dirName();
        QString displayText = QString("%1 (%2)").arg(projectName, fileInfo.dir().absolutePath());

        QListWidgetItem* item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, project.first);

        // 设置工具提示
        QString toolTip = QString("项目路径: %1\n上次打开: %2")
                              .arg(project.first)
                              .arg(project.second.toString("yyyy-MM-dd HH:mm:ss"));
        item->setToolTip(toolTip);

        // 设置图标
        if (fileInfo.isFile() && fileInfo.suffix() == "lightcppproj") {
            item->setIcon(QIcon(":/icons/icons/project_file.svg"));
        } else {
            item->setIcon(QIcon(":/icons/icons/project_folder.svg"));
        }

        m_projectList->addItem(item);
    }

    if (m_projectList->count() == 0) {
        // 没有最近使用的项目
        QListWidgetItem* emptyItem = new QListWidgetItem("没有最近使用的项目");
        emptyItem->setFlags(Qt::NoItemFlags); // 禁用选择
        m_projectList->addItem(emptyItem);
    }
}

void ProjectSelectionDialog::showProjectInfo(const QString& projectPath)
{
    if (projectPath.isEmpty()) {
        m_infoLabel->clear();
        return;
    }

    QFileInfo fileInfo(projectPath);
    QString projectName = fileInfo.dir().dirName();
    QString projectDir = fileInfo.dir().absolutePath();
    QString fileSize = QString::number(fileInfo.size() / 1024.0, 'f', 2) + " KB";
    QString lastModified = fileInfo.lastModified().toString("yyyy-MM-dd HH:mm:ss");

    QString infoText = QString("项目名称: <b>%1</b><br>")
                           .arg(projectName)
                       + QString("项目路径: <b>%1</b><br>")
                             .arg(projectDir)
                       + QString("配置文件大小: <b>%1</b><br>")
                             .arg(fileSize)
                       + QString("上次修改时间: <b>%1</b>")
                             .arg(lastModified);

    m_infoLabel->setText(infoText);
}

} // namespace SprintIDE
