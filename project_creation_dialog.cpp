// project_creation_dialog.cpp
#include "project_creation_dialog.h"
#include "ui_project_creation_dialog.h"

#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QGroupBox>
#include <QLabel>
#include <QTextEdit>
#include <QTreeWidget>
#include <QTabWidget>
#include <QStackedWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QTimer>
#include <QToolTip>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QApplication>
#include <QScreen>
#include <QFontMetrics>
#include <QDebug>

namespace SprintIDE {

// ==================== 静态辅助函数和常量 ====================

// 默认项目路径
static QString getDefaultProjectsPath()
{
    // 尝试使用Qt的标准路径
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (!documentsPath.isEmpty()) {
        return QDir(documentsPath).absoluteFilePath("SprintIDE Projects");
    }

    // 备用方案：使用当前目录
    return QDir::current().absoluteFilePath("Projects");
}

// 模板示例代码
static const QString CONSOLE_APP_EXAMPLE =
    "#include <iostream>\n\n"
    "int main() {\n"
    "    std::cout << \"Hello, SprintIDE!\" << std::endl;\n"
    "    return 0;\n"
    "}\n";

static const QString SIMPLE_GAME_EXAMPLE =
    "#include <iostream>\n"
    "#include <cstdlib>\n"
    "#include <ctime>\n\n"
    "int main() {\n"
    "    std::srand(std::time(nullptr));\n"
    "    int secretNumber = std::rand() % 100 + 1;\n"
    "    int guess = 0;\n"
    "    int attempts = 0;\n\n"
    "    std::cout << \"猜数字游戏！\\n\";\n"
    "    std::cout << \"我想了一个1-100之间的数字，猜猜看！\\n\\n\";\n\n"
    "    do {\n"
    "        std::cout << \"请输入你的猜测: \";\n"
    "        std::cin >> guess;\n"
    "        attempts++;\n\n"
    "        if (guess < secretNumber) {\n"
    "            std::cout << \"太小了！\\n\";\n"
    "        } else if (guess > secretNumber) {\n"
    "            std::cout << \"太大了！\\n\";\n"
    "        } else {\n"
    "            std::cout << \"恭喜你！猜对了！\\n\";\n"
    "            std::cout << \"你用了 \" << attempts << \" 次尝试。\\n\";\n"
    "        }\n"
    "    } while (guess != secretNumber);\n\n"
    "    return 0;\n"
    "}\n";

static const QString GUI_APP_EXAMPLE =
    "#include <QApplication>\n"
    "#include <QMainWindow>\n"
    "#include <QPushButton>\n"
    "#include <QVBoxLayout>\n"
    "#include <QWidget>\n"
    "#include <QLabel>\n\n"
    "class MainWindow : public QMainWindow {\n"
    "    Q_OBJECT\n\n"
    "public:\n"
    "    MainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {\n"
    "        setWindowTitle(\"我的第一个GUI程序\");\n"
    "        resize(400, 300);\n\n"
    "        // 创建中央部件\n"
    "        QWidget *centralWidget = new QWidget(this);\n"
    "        setCentralWidget(centralWidget);\n\n"
    "        // 创建布局\n"
    "        QVBoxLayout *layout = new QVBoxLayout(centralWidget);\n\n"
    "        // 创建标签\n"
    "        QLabel *label = new QLabel(\"欢迎使用SprintIDE！\", this);\n"
    "        label->setAlignment(Qt::AlignCenter);\n"
    "        layout->addWidget(label);\n\n"
    "        // 创建按钮\n"
    "        QPushButton *button = new QPushButton(\"点击我\", this);\n"
    "        layout->addWidget(button);\n\n"
    "        // 连接按钮信号\n"
    "        connect(button, &QPushButton::clicked, [label]() {\n"
    "            label->setText(\"按钮被点击了！\");\n"
    "        });\n"
    "    }\n\n"
    "    virtual ~MainWindow() {}\n"
    "};\n\n"
    "int main(int argc, char *argv[]) {\n"
    "    QApplication app(argc, argv);\n\n"
    "    MainWindow window;\n"
    "    window.show();\n\n"
    "    return app.exec();\n"
    "}\n";

static const QString EMPTY_PROJECT_EXAMPLE =
    "// 这是一个空白项目\n"
    "// 从这里开始编写你的代码\n\n"
    "int main() {\n"
    "    // TODO: 在这里添加你的代码\n"
    "    return 0;\n"
    "}\n";

// ==================== 构造函数和析构函数 ====================

ProjectCreationDialog::ProjectCreationDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ProjectCreationDialog)
    , m_autoOpenProject(true)
    , m_createGitRepo(false)
    , m_initializeGitIgnore(true)
    , m_initialized(false)
{
    // 设置窗口属性
    setWindowTitle("创建新项目 - SprintIDE");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // 初始化UI
    ui->setupUi(this);

    // 设置对话框大小（适应内容）
    resize(800, 600);

    // 初始化模板系统
    initTemplates();

    // 初始化编译器设置
    initCompilerSettings();

    // 初始化界面状态
    setupUI();
    setupConnections();

    // 加载保存的状态
    loadDialogState();

    // 应用第一个模板作为默认
    if (!m_templates.isEmpty()) {
        ui->templateComboBox->setCurrentIndex(0);
        applyCurrentTemplate();
    }

    // 更新路径预览
    updatePathPreview();

    m_initialized = true;
}

ProjectCreationDialog::~ProjectCreationDialog()
{
    // 保存对话框状态
    saveDialogState();

    delete ui;
}

// ==================== 公共方法 ====================

ProjectCreationResult ProjectCreationDialog::getResult() const
{
    return m_result;
}

ProjectConfig ProjectCreationDialog::getProjectConfig() const
{
    return m_projectConfig;
}

void ProjectCreationDialog::setDefaultProjectPath(const QString& defaultPath)
{
    m_defaultProjectPath = defaultPath;
    if (ui->locationLineEdit->text().isEmpty()) {
        ui->locationLineEdit->setText(defaultPath);
        updatePathPreview();
    }
}

QString ProjectCreationDialog::defaultProjectPath() const
{
    return m_defaultProjectPath;
}

bool ProjectCreationDialog::loadTemplates()
{
    // 目前使用内置模板，未来可以从文件加载
    return !m_templates.isEmpty();
}

QList<ProjectTemplate> ProjectCreationDialog::availableTemplates() const
{
    return m_templates;
}

// ==================== 公共槽函数 ====================

void ProjectCreationDialog::accept()
{
    // 验证输入
    QStringList errors;
    if (!validateInput(errors)) {
        QString errorMsg = "请修正以下错误：\n\n";
        for (const QString& error : errors) {
            errorMsg += "• " + error + "\n";
        }

        QMessageBox::warning(this, "输入错误", errorMsg);
        return;
    }

    // 准备创建结果
    m_result.reset();

    // 创建项目目录结构
    if (!createProjectStructure()) {
        m_result.errorMessage = "无法创建项目目录结构";
        QMessageBox::critical(this, "创建失败", m_result.errorMessage);
        return;
    }

    // 生成项目文件
    if (!generateProjectFiles()) {
        m_result.errorMessage = "无法生成项目文件";
        QMessageBox::critical(this, "创建失败", m_result.errorMessage);
        return;
    }

    // 更新创建结果
    m_result.success = true;
    m_result.projectPath = m_projectConfig.projectPath();
    m_result.projectName = m_projectConfig.projectName();
    m_result.displayName = m_projectConfig.displayName();
    m_result.projectType = m_projectConfig.projectType();
    m_result.createdTime = QDateTime::currentDateTime();

    // 保存配置
    QStringList configErrors;
    QString configPath = QDir(m_projectConfig.projectPath()).absoluteFilePath(m_projectConfig.projectName() + ".lightcppproj");
    if (!m_projectConfig.saveToFile(configPath, configErrors)) {
        qWarning() << "保存配置文件时出错:" << configErrors;
    }

    // 创建Git仓库（如果启用）
    if (m_createGitRepo) {
        QProcess git;
        git.setWorkingDirectory(m_projectConfig.projectPath());
        git.start("git", QStringList() << "init");

        if (git.waitForStarted(3000)) {
            git.waitForFinished(3000);

            // 添加.gitignore文件
            if (m_initializeGitIgnore) {
                QFile gitignore(QDir(m_projectConfig.projectPath()).absoluteFilePath(".gitignore"));
                if (gitignore.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QString content = "# 编译输出\n"
                                      "bin/\n"
                                      "build/\n"
                                      "*.o\n"
                                      "*.obj\n"
                                      "*.exe\n"
                                      "*.so\n"
                                      "*.dylib\n"
                                      "*.dll\n\n"
                                      "# IDE配置文件\n"
                                      ".idea/\n"
                                      "*.user\n"
                                      "*.sln\n"
                                      "*.vcxproj\n\n"
                                      "# 系统文件\n"
                                      ".DS_Store\n"
                                      "Thumbs.db\n";
                    gitignore.write(content.toUtf8());
                    gitignore.close();

                    git.start("git", QStringList() << "add" << ".gitignore");
                    git.waitForFinished(3000);
                }
            }

            // 添加所有文件并提交
            git.start("git", QStringList() << "add" << ".");
            git.waitForFinished(3000);

            git.start("git", QStringList() << "commit" << "-m" << "Initial commit");
            git.waitForFinished(3000);
        }
    }

    // 调用基类accept关闭对话框
    QDialog::accept();
}

// ==================== 私有槽函数 ====================

void ProjectCreationDialog::onBrowseLocation()
{
    QString dir = QFileDialog::getExistingDirectory(this,
                                                    "选择项目位置",
                                                    ui->locationLineEdit->text(),
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        ui->locationLineEdit->setText(dir);
        updatePathPreview();
    }
}

void ProjectCreationDialog::onProjectNameChanged(const QString& name)
{
    // 验证项目名称
    QString error;
    bool valid = validateProjectName(name, error);

    // 更新UI状态
    if (!valid && !name.isEmpty()) {
        ui->projectNameLineEdit->setStyleSheet("QLineEdit { border: 2px solid red; }");
        ui->projectNameErrorLabel->setText(error);
        ui->projectNameErrorLabel->setVisible(true);
    } else {
        ui->projectNameLineEdit->setStyleSheet("");
        ui->projectNameErrorLabel->setVisible(false);

        // 如果显示名称为空，使用项目名称作为显示名称
        if (ui->displayNameLineEdit->text().isEmpty() && !name.isEmpty()) {
            ui->displayNameLineEdit->setText(name);
        }
    }

    updatePathPreview();
}

void ProjectCreationDialog::onDisplayNameChanged(const QString& name)
{
    Q_UNUSED(name);
    // 目前只是存储，不需要特殊处理
}

void ProjectCreationDialog::onLocationChanged(const QString& path)
{
    Q_UNUSED(path);
    updatePathPreview();
}

void ProjectCreationDialog::onTemplateChanged(int index)
{
    Q_UNUSED(index);
    if (m_initialized) {
        applyCurrentTemplate();
        updateTemplateDescription();
    }
}

void ProjectCreationDialog::onCompileModeChanged(bool checked)
{
    // 显示或隐藏高级配置选项
    ui->advancedConfigGroupBox->setVisible(checked);

    // 更新项目配置
    m_projectConfig.setProjectConfigMode(checked ? ProjectConfigMode::AdvancedMode : ProjectConfigMode::SimpleMode);
}

void ProjectCreationDialog::onCompilerChanged(int index)
{
    Q_UNUSED(index);
    updateCompilerOptions();
}

void ProjectCreationDialog::onCppStandardChanged(int index)
{
    Q_UNUSED(index);
    // 存储设置到项目配置
    m_projectConfig.setCppStandard(ui->cppStandardComboBox->currentText());
}

void ProjectCreationDialog::onOptimizationChanged(int index)
{
    Q_UNUSED(index);
    m_projectConfig.setOptimizationLevel(ui->optimizationComboBox->currentText());
}

void ProjectCreationDialog::onWarningLevelChanged(int index)
{
    Q_UNUSED(index);
    m_projectConfig.setWarningLevel(ui->warningLevelComboBox->currentText());
}

void ProjectCreationDialog::onShowAdvancedConfig()
{
    // 临时实现：显示消息框
    QMessageBox::information(this, "高级配置",
                             "高级编译配置功能正在开发中...\n"
                             "未来版本将允许您设置：\n"
                             "- 自定义包含路径\n"
                             "- 库链接设置\n"
                             "- 预处理器定义\n"
                             "- 自定义编译标志");
}

void ProjectCreationDialog::onShowExampleCode()
{
    ProjectTemplate currentTemplate = getCurrentTemplate();
    QString exampleCode;

    // 获取示例代码
    if (currentTemplate.type == ProjectType::ConsoleApp) {
        exampleCode = CONSOLE_APP_EXAMPLE;
    } else if (currentTemplate.type == ProjectType::SimpleGame) {
        exampleCode = SIMPLE_GAME_EXAMPLE;
    } else if (currentTemplate.type == ProjectType::GuiApp) {
        exampleCode = GUI_APP_EXAMPLE;
    } else {
        exampleCode = EMPTY_PROJECT_EXAMPLE;
    }

    // 创建预览窗口
    QDialog previewDialog(this);
    previewDialog.setWindowTitle("示例代码预览 - " + currentTemplate.name);
    previewDialog.resize(600, 400);

    QVBoxLayout* layout = new QVBoxLayout(&previewDialog);

    QTextEdit* textEdit = new QTextEdit(&previewDialog);
    textEdit->setPlainText(exampleCode);
    textEdit->setReadOnly(true);
    textEdit->setFont(QFont("Consolas", 10));

    layout->addWidget(textEdit);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, &previewDialog);
    layout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::rejected, &previewDialog, &QDialog::reject);

    previewDialog.exec();
}

void ProjectCreationDialog::onResetToDefaults()
{
    // 重置所有设置到默认值
    ui->projectNameLineEdit->clear();
    ui->displayNameLineEdit->clear();

    if (!m_defaultProjectPath.isEmpty()) {
        ui->locationLineEdit->setText(m_defaultProjectPath);
    } else {
        ui->locationLineEdit->setText(getDefaultProjectsPath());
    }

    ui->templateComboBox->setCurrentIndex(0);
    ui->simpleModeRadioButton->setChecked(true);
    ui->advancedConfigGroupBox->setVisible(false);

    // 重置编译器设置
    ui->compilerComboBox->setCurrentIndex(0);  // 自动检测
    ui->cppStandardComboBox->setCurrentText("c++17");
    ui->optimizationComboBox->setCurrentText("none");
    ui->warningLevelComboBox->setCurrentText("all");
    ui->debugInfoCheckBox->setChecked(true);

    // 重置高级选项
    ui->autoOpenCheckBox->setChecked(true);
    ui->createGitRepoCheckBox->setChecked(false);

    // 更新配置
    m_projectConfig = ProjectConfig();
    applyCurrentTemplate();
    updatePathPreview();
}

void ProjectCreationDialog::setDisplayName(const QString& displayName)
{
    // 如果需要，可以在这里添加逻辑来更新 UI
    // 例如：ui->displayNameLineEdit->setText(displayName);
    m_projectConfig.setDisplayName(displayName);
}

void ProjectCreationDialog::setProjectType(ProjectType type)
{
    m_projectConfig.setProjectType(type);
    // 如果需要，可以在这里更新 UI 以反映项目类型变化
}

void ProjectCreationDialog::setCompilerType(ProjectCompilerType type)
{
    m_projectConfig.setProjectCompilerType(type);
    // 如果需要，可以在这里更新 UI 以反映编译器类型变化
}

// ==================== 私有方法 ====================

void ProjectCreationDialog::setupUI()
{
    // 设置控件属性
    ui->projectNameLineEdit->setPlaceholderText("例如: MyFirstGame");
    ui->displayNameLineEdit->setPlaceholderText("例如: 我的第一个游戏");
    ui->locationLineEdit->setPlaceholderText("选择项目保存的位置");

    // 设置按钮图标（如果有）
    // ui->browseButton->setIcon(QIcon(":/icons/folder.png"));

    // 设置模板组合框
    ui->templateComboBox->clear();
    for (const ProjectTemplate& tmpl : m_templates) {
        ui->templateComboBox->addItem(tmpl.name, tmpl.id);
    }

    // 设置编译器组合框
    ui->compilerComboBox->clear();
    ui->compilerComboBox->addItem("自动检测", "auto");
    ui->compilerComboBox->addItem("GCC (g++)", "gcc");
    ui->compilerComboBox->addItem("Clang (clang++)", "clang");
#ifdef Q_OS_WIN
    ui->compilerComboBox->addItem("MSVC (cl.exe)", "msvc");
#endif

    // 设置C++标准组合框
    ui->cppStandardComboBox->clear();
    ui->cppStandardComboBox->addItems({"c++11", "c++14", "c++17", "c++20", "c++23"});
    ui->cppStandardComboBox->setCurrentText("c++17");

    // 设置优化级别组合框
    ui->optimizationComboBox->clear();
    ui->optimizationComboBox->addItems({"none", "fast", "faster", "fastest", "size"});
    ui->optimizationComboBox->setCurrentText("none");

    // 设置警告级别组合框
    ui->warningLevelComboBox->clear();
    ui->warningLevelComboBox->addItems({"none", "low", "medium", "high", "all"});
    ui->warningLevelComboBox->setCurrentText("all");

    // 设置默认路径
    if (m_defaultProjectPath.isEmpty()) {
        m_defaultProjectPath = getDefaultProjectsPath();
    }
    ui->locationLineEdit->setText(m_defaultProjectPath);

    // 隐藏高级配置（默认使用简单模式）
    ui->advancedConfigGroupBox->setVisible(false);

    // 设置按钮
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("创建");
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText("取消");

    // 启用/禁用创建按钮
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    // 设置tab顺序
    setTabOrder(ui->projectNameLineEdit, ui->displayNameLineEdit);
    setTabOrder(ui->displayNameLineEdit, ui->locationLineEdit);
    setTabOrder(ui->locationLineEdit, ui->browseButton);
    setTabOrder(ui->browseButton, ui->templateComboBox);

    // 设置焦点
    ui->projectNameLineEdit->setFocus();
}

void ProjectCreationDialog::setupConnections()
{
    // 项目信息
    connect(ui->projectNameLineEdit, &QLineEdit::textChanged,
            this, &ProjectCreationDialog::onProjectNameChanged);
    connect(ui->displayNameLineEdit, &QLineEdit::textChanged,
            this, &ProjectCreationDialog::onDisplayNameChanged);
    connect(ui->locationLineEdit, &QLineEdit::textChanged,
            this, &ProjectCreationDialog::onLocationChanged);
    connect(ui->browseButton, &QPushButton::clicked,
            this, &ProjectCreationDialog::onBrowseLocation);

    // 模板选择 - 使用更兼容的语法，避免QOverload问题
    connect(ui->templateComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ProjectCreationDialog::onTemplateChanged);

    // 编译模式
    connect(ui->simpleModeRadioButton, &QRadioButton::toggled,
            this, &ProjectCreationDialog::onCompileModeChanged);
    connect(ui->advancedModeRadioButton, &QRadioButton::toggled,
            this, &ProjectCreationDialog::onCompileModeChanged);

    // 编译器设置 - 使用更兼容的语法
    connect(ui->compilerComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ProjectCreationDialog::onCompilerChanged);
    connect(ui->cppStandardComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ProjectCreationDialog::onCppStandardChanged);
    connect(ui->optimizationComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ProjectCreationDialog::onOptimizationChanged);
    connect(ui->warningLevelComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ProjectCreationDialog::onWarningLevelChanged);

    // 高级配置
    connect(ui->advancedConfigButton, &QPushButton::clicked,
            this, &ProjectCreationDialog::onShowAdvancedConfig);

    // 示例代码和重置
    connect(ui->exampleCodeButton, &QPushButton::clicked,
            this, &ProjectCreationDialog::onShowExampleCode);
    connect(ui->resetButton, &QPushButton::clicked,
            this, &ProjectCreationDialog::onResetToDefaults);

    // 按钮框
    connect(ui->buttonBox, &QDialogButtonBox::accepted,
            this, &ProjectCreationDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
}

void ProjectCreationDialog::initTemplates()
{
    m_templates.clear();
    m_templateMap.clear();

    // 控制台应用程序模板
    ProjectTemplate consoleTemplate;
    consoleTemplate.id = "console_app";
    consoleTemplate.name = "控制台应用程序";
    consoleTemplate.description = "创建一个简单的命令行程序，适合学习基础C++语法。\n"
                                  "包含基本的main函数结构和标准库使用示例。";
    consoleTemplate.type = ProjectType::ConsoleApp;
    consoleTemplate.icon = ":/icons/icons/template_console.svg";
    consoleTemplate.defaultFiles = {"src/main.cpp"};
    consoleTemplate.directories = {"src", "include", "bin", "docs"};

    // 示例文件
    ProjectTemplate::ExampleFile consoleMain;
    consoleMain.path = "src/main.cpp";
    consoleMain.content = CONSOLE_APP_EXAMPLE;
    consoleMain.language = "cpp";
    consoleTemplate.exampleFiles.append(consoleMain);

    m_templates.append(consoleTemplate);
    m_templateMap[consoleTemplate.id] = consoleTemplate;

    // 简单游戏模板
    ProjectTemplate gameTemplate;
    gameTemplate.id = "simple_game";
    gameTemplate.name = "简单游戏";
    gameTemplate.description = "创建一个有趣的小游戏项目，如猜数字、井字棋等。\n"
                               "增加编程的趣味性，适合有一定基础的学习者。";
    gameTemplate.type = ProjectType::SimpleGame;
    gameTemplate.icon = ":/icons/icons/template_game.svg";
    gameTemplate.defaultFiles = {"src/main.cpp", "src/game.cpp", "include/game.h"};
    gameTemplate.directories = {"src", "include", "assets", "bin", "docs"};

    ProjectTemplate::ExampleFile gameMain;
    gameMain.path = "src/main.cpp";
    gameMain.content = SIMPLE_GAME_EXAMPLE;
    gameMain.language = "cpp";
    gameTemplate.exampleFiles.append(gameMain);

    m_templates.append(gameTemplate);
    m_templateMap[gameTemplate.id] = gameTemplate;

    // GUI应用程序模板
    ProjectTemplate guiTemplate;
    guiTemplate.id = "gui_app";
    guiTemplate.name = "图形界面程序";
    guiTemplate.description = "创建一个带窗口界面的应用程序，使用Qt框架。\n"
                              "学习图形界面编程，创建更直观的用户界面。";
    guiTemplate.type = ProjectType::GuiApp;
    guiTemplate.icon = ":/icons/icons/template_gui.svg";
    guiTemplate.defaultFiles = {"src/main.cpp", "src/mainwindow.cpp", "include/mainwindow.h"};
    guiTemplate.directories = {"src", "include", "ui", "resources", "bin", "docs"};

    ProjectTemplate::ExampleFile guiMain;
    guiMain.path = "src/main.cpp";
    guiMain.content = GUI_APP_EXAMPLE;
    guiMain.language = "cpp";
    guiTemplate.exampleFiles.append(guiMain);

    m_templates.append(guiTemplate);
    m_templateMap[guiTemplate.id] = guiTemplate;

    // 空白项目模板
    ProjectTemplate emptyTemplate;
    emptyTemplate.id = "empty_project";
    emptyTemplate.name = "空白项目";
    emptyTemplate.description = "创建一个完全自定义的项目结构，没有预置代码。\n"
                                "适合有经验的学习者，或需要特殊结构的项目。";
    emptyTemplate.type = ProjectType::EmptyProject;
    emptyTemplate.icon = ":/icons/icons/template_empty.svg";
    emptyTemplate.defaultFiles = {};
    emptyTemplate.directories = {"src", "include"};

    ProjectTemplate::ExampleFile emptyMain;
    emptyMain.path = "src/main.cpp";
    emptyMain.content = EMPTY_PROJECT_EXAMPLE;
    emptyMain.language = "cpp";
    emptyTemplate.exampleFiles.append(emptyMain);

    m_templates.append(emptyTemplate);
    m_templateMap[emptyTemplate.id] = emptyTemplate;
}

void ProjectCreationDialog::initCompilerSettings()
{
    // 检测可用编译器
    QStringList compilers = ProjectConfig::detectAvailableCompilers();

    // 如果检测到编译器，更新UI提示
    if (!compilers.isEmpty()) {
        QString tooltip = "检测到以下编译器:\n";
        for (const QString& compiler : compilers) {
            tooltip += "• " + compiler + "\n";
        }
        ui->compilerComboBox->setToolTip(tooltip);
    }
}

bool ProjectCreationDialog::validateInput(QStringList& errors) const
{
    errors.clear();

    // 验证项目名称
    QString projectName = ui->projectNameLineEdit->text().trimmed();
    QString nameError;
    if (!validateProjectName(projectName, nameError)) {
        errors.append("项目名称: " + nameError);
    }

    // 验证显示名称
    QString displayName = ui->displayNameLineEdit->text().trimmed();
    if (displayName.isEmpty()) {
        errors.append("显示名称不能为空");
    }

    // 验证项目路径
    QString projectPath = ui->locationLineEdit->text().trimmed();
    QString pathError;
    if (!validateProjectPath(projectPath, pathError)) {
        errors.append("项目路径: " + pathError);
    } else {
        // 检查项目目录是否已存在
        QString fullProjectPath = QDir(projectPath).absoluteFilePath(projectName);
        if (QDir(fullProjectPath).exists()) {
            QDir projectDir(fullProjectPath);
            if (!projectDir.isEmpty()) {
                errors.append("项目目录已存在且不为空");
            }
        }
    }

    return errors.isEmpty();
}

bool ProjectCreationDialog::validateProjectName(const QString& name, QString& error) const
{
    if (name.isEmpty()) {
        error = "项目名称不能为空";
        return false;
    }

    if (!ProjectConfig::isValidProjectName(name)) {
        error = "项目名称只能包含字母、数字和下划线，且不能以数字开头";
        return false;
    }

    // 检查是否为保留名称
    static const QStringList reservedNames = {
        "con", "prn", "aux", "nul", "com1", "com2", "com3", "com4",
        "com5", "com6", "com7", "com8", "com9", "lpt1", "lpt2",
        "lpt3", "lpt4", "lpt5", "lpt6", "lpt7", "lpt8", "lpt9"
    };

    if (reservedNames.contains(name.toLower())) {
        error = "项目名称是系统保留名称，请选择其他名称";
        return false;
    }

    error.clear();
    return true;
}

bool ProjectCreationDialog::validateProjectPath(const QString& path, QString& error) const
{
    if (path.isEmpty()) {
        error = "项目路径不能为空";
        return false;
    }

    QDir dir(path);
    if (!dir.isAbsolute()) {
        error = "项目路径必须是绝对路径";
        return false;
    }

    // 检查路径是否包含非法字符
    static const QString illegalChars = "<>:\"|?*";
    for (const QChar& ch : illegalChars) {
        if (path.contains(ch)) {
            error = "路径包含非法字符";
            return false;
        }
    }

    // 尝试创建目录（如果不存在）
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            error = "无法创建目录，请检查权限";
            return false;
        }
    } else {
        // 检查目录是否可写
        QFile testFile(dir.absoluteFilePath(".sprintide_test"));
        if (!testFile.open(QIODevice::WriteOnly)) {
            error = "目录不可写，请检查权限";
            return false;
        }
        testFile.remove();
    }

    error.clear();
    return true;
}

bool ProjectCreationDialog::createProjectStructure()
{
    QString projectName = ui->projectNameLineEdit->text().trimmed();
    QString projectPath = ui->locationLineEdit->text().trimmed();
    QString fullProjectPath = QDir(projectPath).absoluteFilePath(projectName);

    // 创建项目根目录
    QDir projectDir(fullProjectPath);
    if (!projectDir.mkpath(".")) {
        m_result.errorMessage = "无法创建项目目录: " + fullProjectPath;
        return false;
    }

    m_result.createdDirectories.append(fullProjectPath);

    // 获取当前模板
    ProjectTemplate currentTemplate = getCurrentTemplate();

    // 创建模板指定的目录
    for (const QString& dirName : currentTemplate.directories) {
        QString dirPath = projectDir.absoluteFilePath(dirName);
        if (projectDir.mkpath(dirName)) {
            m_result.createdDirectories.append(dirPath);
        } else {
            m_result.errorMessage = "无法创建目录: " + dirPath;
            return false;
        }
    }

    // 更新项目配置的路径
    m_projectConfig.setProjectPath(fullProjectPath);

    return true;
}

bool ProjectCreationDialog::generateProjectFiles()
{
    if (!generateConfigFile()) {
        return false;
    }

    if (!generateExampleFiles()) {
        return false;
    }

    return true;
}

bool ProjectCreationDialog::generateConfigFile()
{
    // 更新项目配置
    m_projectConfig.setProjectName(ui->projectNameLineEdit->text().trimmed());
    m_projectConfig.setDisplayName(ui->displayNameLineEdit->text().trimmed());

    // 设置项目类型
    ProjectTemplate currentTemplate = getCurrentTemplate();
    m_projectConfig.setProjectType(currentTemplate.type);

    // 设置编译模式
    m_projectConfig.setProjectConfigMode(ui->advancedModeRadioButton->isChecked() ?
                                       ProjectConfigMode::AdvancedMode : ProjectConfigMode::SimpleMode);

    // 设置编译器类型
    QString compilerType = ui->compilerComboBox->currentData().toString();
    if (compilerType == "gcc") {
        m_projectConfig.setProjectCompilerType(ProjectCompilerType::GCC);
    } else if (compilerType == "clang") {
        m_projectConfig.setProjectCompilerType(ProjectCompilerType::Clang);
    } else if (compilerType == "msvc") {
        m_projectConfig.setProjectCompilerType(ProjectCompilerType::MSVC);
    } else {
        m_projectConfig.setProjectCompilerType(ProjectCompilerType::AutoDetect);
    }

    // 设置其他编译选项
    m_projectConfig.setCppStandard(ui->cppStandardComboBox->currentText());
    m_projectConfig.setOptimizationLevel(ui->optimizationComboBox->currentText());
    m_projectConfig.setWarningLevel(ui->warningLevelComboBox->currentText());
    m_projectConfig.setEnableDebugInfo(ui->debugInfoCheckBox->isChecked());

    // 设置输出路径
    m_projectConfig.setOutputPath("bin");

    // 应用模板
    m_projectConfig.applyTemplate(currentTemplate.type);

    // 保存配置文件
    QString configPath = QDir(m_projectConfig.projectPath())
                             .absoluteFilePath(m_projectConfig.projectName() + ".lightcppproj");

    QStringList errors;
    if (!m_projectConfig.saveToFile(configPath, errors)) {
        m_result.errorMessage = "无法生成配置文件:\n" + errors.join("\n");
        return false;
    }

    m_result.createdFiles.append(configPath);
    return true;
}

bool ProjectCreationDialog::generateExampleFiles()
{
    ProjectTemplate currentTemplate = getCurrentTemplate();
    QDir projectDir(m_projectConfig.projectPath());

    // 生成模板的示例文件
    for (const ProjectTemplate::ExampleFile& example : currentTemplate.exampleFiles) {
        QString filePath = projectDir.absoluteFilePath(example.path);
        QFileInfo fileInfo(filePath);

        // 确保目录存在
        if (!projectDir.mkpath(fileInfo.absolutePath())) {
            m_result.errorMessage = "无法创建目录: " + fileInfo.absolutePath();
            return false;
        }

        // 写入文件内容
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            m_result.errorMessage = "无法创建文件: " + filePath;
            return false;
        }

        // 替换模板变量
        QMap<QString, QString> variables;
        variables["PROJECT_NAME"] = m_projectConfig.projectName();
        variables["DISPLAY_NAME"] = m_projectConfig.displayName();
        variables["CREATE_DATE"] = QDateTime::currentDateTime().toString("yyyy-MM-dd");

        QString content = replaceTemplateVariables(example.content, variables);

        if (file.write(content.toUtf8()) == -1) {
            file.close();
            m_result.errorMessage = "无法写入文件: " + filePath;
            return false;
        }

        file.close();
        m_result.createdFiles.append(filePath);
    }

    return true;
}

void ProjectCreationDialog::updatePathPreview()
{
    QString projectName = ui->projectNameLineEdit->text().trimmed();
    QString projectPath = ui->locationLineEdit->text().trimmed();

    if (projectName.isEmpty() || projectPath.isEmpty()) {
        ui->pathPreviewLabel->setText("请填写项目名称和路径");
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        return;
    }

    QString fullPath = QDir(projectPath).absoluteFilePath(projectName);

    // 检查目录是否存在
    bool exists = QDir(fullPath).exists();
    bool empty = !exists || QDir(fullPath).isEmpty();

    QString status;
    if (exists) {
        if (empty) {
            status = "(目录已存在，为空)";
            ui->pathPreviewLabel->setStyleSheet("color: orange;");
        } else {
            status = "(目录已存在，不为空)";
            ui->pathPreviewLabel->setStyleSheet("color: red;");
        }
    } else {
        status = "(将创建新目录)";
        ui->pathPreviewLabel->setStyleSheet("color: green;");
    }

    ui->pathPreviewLabel->setText(fullPath + "\n" + status);

    // 启用/禁用创建按钮
    // 修复：创建本地变量来存储错误信息，避免传递临时对象给非const引用参数
    QString nameError, pathError;
    bool canCreate = !projectName.isEmpty() &&
                     !projectPath.isEmpty() &&
                     validateProjectName(projectName, nameError) &&
                     validateProjectPath(projectPath, pathError) &&
                     (empty || !exists);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(canCreate);
}

void ProjectCreationDialog::updateTemplateDescription()
{
    ProjectTemplate currentTemplate = getCurrentTemplate();

    // 更新描述
    ui->templateDescriptionLabel->setText(currentTemplate.description);

    // 更新文件列表预览
    QString fileList = "将创建以下文件:\n";
    for (const QString& file : currentTemplate.defaultFiles) {
        fileList += "• " + file + "\n";
    }

    // 添加目录结构
    fileList += "\n目录结构:\n";
    for (const QString& dir : currentTemplate.directories) {
        fileList += "• " + dir + "/\n";
    }

    ui->templateFilesLabel->setText(fileList);
}

void ProjectCreationDialog::updateCompilerOptions()
{
    QString compilerType = ui->compilerComboBox->currentData().toString();

    // 根据编译器类型更新可用选项
    if (compilerType == "msvc") {
        // MSVC不支持某些选项
        ui->cppStandardComboBox->clear();
        ui->cppStandardComboBox->addItems({"c++14", "c++17", "c++20", "c++23"});
    } else {
        // GCC/Clang支持所有标准
        ui->cppStandardComboBox->clear();
        ui->cppStandardComboBox->addItems({"c++11", "c++14", "c++17", "c++20", "c++23"});
    }
}

void ProjectCreationDialog::applyCurrentTemplate()
{
    ProjectTemplate currentTemplate = getCurrentTemplate();

    // 更新项目配置
    m_projectConfig.applyTemplate(currentTemplate.type);
}

ProjectTemplate ProjectCreationDialog::getCurrentTemplate() const
{
    QString templateId = ui->templateComboBox->currentData().toString();
    return m_templateMap.value(templateId, m_templates.first());
}

QString ProjectCreationDialog::replaceTemplateVariables(const QString& content,
                                                        const QMap<QString, QString>& variables) const
{
    QString result = content;

    for (auto it = variables.constBegin(); it != variables.constEnd(); ++it) {
        QString placeholder = "${" + it.key() + "}";
        result.replace(placeholder, it.value());
    }

    return result;
}

void ProjectCreationDialog::saveDialogState()
{
    QSettings settings("SprintIDE", "ProjectCreationDialog");

    settings.setValue("lastUsedPath", ui->locationLineEdit->text());
    settings.setValue("lastTemplate", ui->templateComboBox->currentIndex());
    settings.setValue("compileMode", ui->advancedModeRadioButton->isChecked());
    settings.setValue("compilerType", ui->compilerComboBox->currentIndex());
    settings.setValue("cppStandard", ui->cppStandardComboBox->currentIndex());
    settings.setValue("optimization", ui->optimizationComboBox->currentIndex());
    settings.setValue("warningLevel", ui->warningLevelComboBox->currentIndex());
    settings.setValue("debugInfo", ui->debugInfoCheckBox->isChecked());
    settings.setValue("autoOpen", ui->autoOpenCheckBox->isChecked());
    settings.setValue("createGitRepo", ui->createGitRepoCheckBox->isChecked());
}

void ProjectCreationDialog::loadDialogState()
{
    QSettings settings("SprintIDE", "ProjectCreationDialog");

    QString lastPath = settings.value("lastUsedPath").toString();
    if (!lastPath.isEmpty() && QDir(lastPath).exists()) {
        ui->locationLineEdit->setText(lastPath);
    }

    int lastTemplate = settings.value("lastTemplate", 0).toInt();
    if (lastTemplate >= 0 && lastTemplate < ui->templateComboBox->count()) {
        ui->templateComboBox->setCurrentIndex(lastTemplate);
    }

    bool advancedMode = settings.value("compileMode", false).toBool();
    if (advancedMode) {
        ui->advancedModeRadioButton->setChecked(true);
    } else {
        ui->simpleModeRadioButton->setChecked(true);
    }

    int compilerIndex = settings.value("compilerType", 0).toInt();
    if (compilerIndex >= 0 && compilerIndex < ui->compilerComboBox->count()) {
        ui->compilerComboBox->setCurrentIndex(compilerIndex);
    }

    int cppStandardIndex = settings.value("cppStandard", 2).toInt(); // c++17
    if (cppStandardIndex >= 0 && cppStandardIndex < ui->cppStandardComboBox->count()) {
        ui->cppStandardComboBox->setCurrentIndex(cppStandardIndex);
    }

    int optimizationIndex = settings.value("optimization", 0).toInt(); // none
    if (optimizationIndex >= 0 && optimizationIndex < ui->optimizationComboBox->count()) {
        ui->optimizationComboBox->setCurrentIndex(optimizationIndex);
    }

    int warningLevelIndex = settings.value("warningLevel", 4).toInt(); // all
    if (warningLevelIndex >= 0 && warningLevelIndex < ui->warningLevelComboBox->count()) {
        ui->warningLevelComboBox->setCurrentIndex(warningLevelIndex);
    }

    bool debugInfo = settings.value("debugInfo", true).toBool();
    ui->debugInfoCheckBox->setChecked(debugInfo);

    bool autoOpen = settings.value("autoOpen", true).toBool();
    ui->autoOpenCheckBox->setChecked(autoOpen);

    bool createGitRepo = settings.value("createGitRepo", false).toBool();
    ui->createGitRepoCheckBox->setChecked(createGitRepo);

    m_autoOpenProject = autoOpen;
    m_createGitRepo = createGitRepo;
}

} // namespace SprintIDE
