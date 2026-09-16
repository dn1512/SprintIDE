// ============================================================================
//  project_creation_dialog.cpp
//  多语言版本项目创建向导
//
//  设计要点：
//    1. 尽量不修改 .ui 文件——通过代码动态往 layout 里插入"语言下拉框"
//       和"语言特有选项组"。
//    2. 语言与模板分离：语言下拉框决定显示哪些模板；模板下拉框只列出
//       当前语言下的模板。
//    3. 每种语言走各自的生成器函数（generatePythonProject 等），C++ 沿
//       用原有 generateProjectFiles()。
// ============================================================================
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
#include <QFileInfo>
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
#include <QDialogButtonBox>
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

// ============================================================================
//  一、语言枚举的辅助函数
// ============================================================================

QString projectLanguageToString(ProjectLanguage lang)
{
    switch (lang) {
    case ProjectLanguage::Cpp:       return QStringLiteral("C++");
    case ProjectLanguage::Python:    return QStringLiteral("Python");
    case ProjectLanguage::Go:        return QStringLiteral("Go");
    case ProjectLanguage::Rust:      return QStringLiteral("Rust");
    case ProjectLanguage::Node:      return QStringLiteral("Node.js");
    case ProjectLanguage::GenericAI: return QStringLiteral("AI 通用适配器");
    }
    return QStringLiteral("未知");
}

QString projectLanguageToId(ProjectLanguage lang)
{
    switch (lang) {
    case ProjectLanguage::Cpp:       return QStringLiteral("cpp");
    case ProjectLanguage::Python:    return QStringLiteral("python");
    case ProjectLanguage::Go:        return QStringLiteral("go");
    case ProjectLanguage::Rust:      return QStringLiteral("rust");
    case ProjectLanguage::Node:      return QStringLiteral("node");
    case ProjectLanguage::GenericAI: return QStringLiteral("generic_ai");
    }
    return QStringLiteral("unknown");
}

ProjectLanguage projectLanguageFromId(const QString& id)
{
    const QString s = id.toLower();
    if (s == "cpp" || s == "c" || s == "c++") return ProjectLanguage::Cpp;
    if (s == "python" || s == "py")            return ProjectLanguage::Python;
    if (s == "go" || s == "golang")            return ProjectLanguage::Go;
    if (s == "rust" || s == "rs")              return ProjectLanguage::Rust;
    if (s == "node" || s == "js" || s == "ts") return ProjectLanguage::Node;
    if (s == "generic_ai" || s == "ai")        return ProjectLanguage::GenericAI;
    return ProjectLanguage::Cpp;
}

// ============================================================================
//  二、静态辅助函数与内置示例代码
// ============================================================================

// 返回默认的"项目目录"位置
static QString getDefaultProjectsPath()
{
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (!documentsPath.isEmpty()) {
        return QDir(documentsPath).absoluteFilePath("SprintIDE Projects");
    }
    return QDir::current().absoluteFilePath("Projects");
}

// ------- C++ 示例代码（保持原样） -------
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
    "        QWidget *centralWidget = new QWidget(this);\n"
    "        setCentralWidget(centralWidget);\n\n"
    "        QVBoxLayout *layout = new QVBoxLayout(centralWidget);\n\n"
    "        QLabel *label = new QLabel(\"欢迎使用SprintIDE！\", this);\n"
    "        label->setAlignment(Qt::AlignCenter);\n"
    "        layout->addWidget(label);\n\n"
    "        QPushButton *button = new QPushButton(\"点击我\", this);\n"
    "        layout->addWidget(button);\n\n"
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

// ------- Python / Go / Rust / Node 示例代码 -------
static const QString PYTHON_MAIN_EXAMPLE =
    "def main():\n"
    "    print(\"Hello from SprintIDE!\")\n\n"
    "if __name__ == \"__main__\":\n"
    "    main()\n";

static const QString GO_MAIN_EXAMPLE =
    "package main\n\n"
    "import \"fmt\"\n\n"
    "func main() {\n"
    "    fmt.Println(\"Hello from SprintIDE!\")\n"
    "}\n";

static const QString RUST_MAIN_EXAMPLE =
    "fn main() {\n"
    "    println!(\"Hello from SprintIDE!\");\n"
    "}\n";

static const QString RUST_LIB_EXAMPLE =
    "pub fn hello() -> &'static str {\n"
    "    \"Hello from SprintIDE!\"\n"
    "}\n";

static const QString NODE_JS_EXAMPLE =
    "console.log('Hello from SprintIDE!');\n";

static const QString NODE_TS_EXAMPLE =
    "const greeting: string = 'Hello from SprintIDE!';\n"
    "console.log(greeting);\n";

// ============================================================================
//  三、构造函数 / 析构函数
// ============================================================================

ProjectCreationDialog::ProjectCreationDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ProjectCreationDialog)
    , m_autoOpenProject(true)
    , m_createGitRepo(false)
    , m_initializeGitIgnore(true)
    , m_initialized(false)
{
    qDebug() << "[PCD] 构造开始";

    // ---- 基本窗口属性 ----
    setWindowTitle("创建新项目 - SprintIDE");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // ---- 加载 .ui ----
    ui->setupUi(this);
    qDebug() << "[PCD] setupUi 完成, templateComboBox ="
             << static_cast<void*>(ui->templateComboBox);

    resize(820, 680);

    // ---- 初始化内部数据结构 ----
    initTemplates();          // 里面对 C++ 模板写入 language = Cpp，并新增其他语言的模板
    initCompilerSettings();
    qDebug() << "[PCD] initTemplates/initCompilerSettings 完成";

    // ---- 配置界面 + 插入动态控件 ----
    setupUI();                // 里面会创建语言下拉框并插入，创建语言特有选项组
    qDebug() << "[PCD] setupUI 完成";

    setupConnections();       // 里面会连接语言下拉框信号
    qDebug() << "[PCD] setupConnections 完成";

    // ---- 恢复上一次的对话框状态 ----
    loadDialogState();
    qDebug() << "[PCD] loadDialogState 完成";

    // ---- 初始刷新 ----
    qDebug() << "[PCD] → 即将执行初始刷新, m_languageComboBox ="
             << static_cast<void*>(m_languageComboBox);
    if (m_languageComboBox && m_languageComboBox->count() > 0) {
        const ProjectLanguage lang = currentLanguage();
        qDebug() << "[PCD]   当前语言索引:" << static_cast<int>(lang);

        refreshTemplateList(lang);
        qDebug() << "[PCD]   refreshTemplateList 完成";

        refreshLanguageSpecificUI();
        qDebug() << "[PCD]   refreshLanguageSpecificUI 完成";
    }

    // ---- 应用第一个模板作为默认 ----
    qDebug() << "[PCD] → 即将应用默认模板, ui->templateComboBox->count() ="
             << ui->templateComboBox->count();
    if (!m_templates.isEmpty() && ui->templateComboBox->count() > 0) {
        ui->templateComboBox->setCurrentIndex(0);
        qDebug() << "[PCD]   setCurrentIndex(0) 完成";

        applyCurrentTemplate();
        qDebug() << "[PCD]   applyCurrentTemplate 完成";

        updateTemplateDescription();
        qDebug() << "[PCD]   updateTemplateDescription 完成";
    }

    updatePathPreview();
    qDebug() << "[PCD] → updatePathPreview 完成";

    m_initialized = true;
    qDebug() << "[PCD] 构造函数正常结束";
}

ProjectCreationDialog::~ProjectCreationDialog()
{
    saveDialogState();
    delete ui;
}

// ============================================================================
//  四、公共方法
// ============================================================================

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
    return !m_templates.isEmpty();
}

QList<ProjectTemplate> ProjectCreationDialog::availableTemplates() const
{
    return m_templates;
}

// ============================================================================
//  五、accept() —— 核心：按语言分发
// ============================================================================

void ProjectCreationDialog::accept()
{
    // ------------------------------------------------------------
    // 1. 输入校验
    // ------------------------------------------------------------
    QStringList errors;
    if (!validateInput(errors)) {
        QString errorMsg = "请修正以下错误：\n\n";
        for (const QString& error : errors) {
            errorMsg += "• " + error + "\n";
        }
        QMessageBox::warning(this, "输入错误", errorMsg);
        return;
    }

    // ------------------------------------------------------------
    // 2. 重置结果对象，并记录当前语言
    // ------------------------------------------------------------
    m_result.reset();
    m_result.language = currentLanguage();   // ★ 关键：把语言写进结果

    // ------------------------------------------------------------
    // 3. 创建项目目录结构（所有语言共用）
    // ------------------------------------------------------------
    if (!createProjectStructure()) {
        m_result.errorMessage = "无法创建项目目录结构";
        QMessageBox::critical(this, "创建失败", m_result.errorMessage);
        return;
    }

    // ------------------------------------------------------------
    // 4. 按语言分发到不同的生成器
    //    - C++ 与 GenericAI 走原 generateProjectFiles()
    //    - 其余语言走各自的生成器
    // ------------------------------------------------------------
    bool ok = false;
    switch (m_result.language) {
    case ProjectLanguage::Python:
        ok = generatePythonProject();
        break;
    case ProjectLanguage::Go:
        ok = generateGoProject();
        break;
    case ProjectLanguage::Rust:
        ok = generateRustProject();
        break;
    case ProjectLanguage::Node:
        ok = generateNodeProject();
        break;
    case ProjectLanguage::Cpp:
    case ProjectLanguage::GenericAI:
    default:
        ok = generateProjectFiles();
        break;
    }

    if (!ok) {
        if (m_result.errorMessage.isEmpty()) {
            m_result.errorMessage = "无法生成项目文件";
        }
        QMessageBox::critical(this, "创建失败", m_result.errorMessage);
        return;
    }

    // ------------------------------------------------------------
    // 5. 统一记录结果
    // ------------------------------------------------------------
    m_result.success     = true;
    m_result.projectPath = m_projectConfig.projectPath();
    m_result.projectName = m_projectConfig.projectName();
    m_result.displayName = m_projectConfig.displayName();
    m_result.projectType = m_projectConfig.projectType();
    m_result.createdTime = QDateTime::currentDateTime();

    // ------------------------------------------------------------
    // 6. Git 初始化（保持原有逻辑）
    // ------------------------------------------------------------
    if (m_createGitRepo) {
        QProcess git;
        git.setWorkingDirectory(m_projectConfig.projectPath());
        git.start("git", QStringList() << "init");
        if (git.waitForStarted(3000)) {
            git.waitForFinished(3000);

            if (m_initializeGitIgnore) {
                QFile gitignore(QDir(m_projectConfig.projectPath()).absoluteFilePath(".gitignore"));
                if (gitignore.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QString content =
                        "# 编译输出\n"
                        "bin/\n"
                        "build/\n"
                        "dist/\n"
                        "target/\n"
                        "*.o\n"
                        "*.obj\n"
                        "*.exe\n"
                        "*.so\n"
                        "*.dylib\n"
                        "*.dll\n"
                        "__pycache__/\n"
                        ".venv/\n"
                        "node_modules/\n\n"
                        "# IDE 配置文件\n"
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

            git.start("git", QStringList() << "add" << ".");
            git.waitForFinished(3000);

            git.start("git", QStringList() << "commit" << "-m" << "Initial commit");
            git.waitForFinished(3000);
        }
    }

    QDialog::accept();
}

// ============================================================================
//  六、用户交互槽函数
// ============================================================================

void ProjectCreationDialog::onBrowseLocation()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, "选择项目位置", ui->locationLineEdit->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        ui->locationLineEdit->setText(dir);
        updatePathPreview();
    }
}

void ProjectCreationDialog::onProjectNameChanged(const QString& name)
{
    QString error;
    bool valid = validateProjectName(name, error);

    if (!valid && !name.isEmpty()) {
        ui->projectNameLineEdit->setStyleSheet("QLineEdit { border: 2px solid red; }");
        ui->projectNameErrorLabel->setText(error);
        ui->projectNameErrorLabel->setVisible(true);
    } else {
        ui->projectNameLineEdit->setStyleSheet("");
        ui->projectNameErrorLabel->setVisible(false);
        if (ui->displayNameLineEdit->text().isEmpty() && !name.isEmpty()) {
            ui->displayNameLineEdit->setText(name);
        }
    }
    updatePathPreview();
}

void ProjectCreationDialog::onDisplayNameChanged(const QString& name)
{
    Q_UNUSED(name);
}

void ProjectCreationDialog::onLocationChanged(const QString& path)
{
    Q_UNUSED(path);
    updatePathPreview();
}

// ------------------------------------------------------------
// ★ 新增：语言切换槽函数
//   1. 刷新模板下拉框（只显示当前语言的模板）
//   2. 刷新"语言特有选项"组
// ------------------------------------------------------------
void ProjectCreationDialog::onLanguageChanged(int /*index*/)
{
    if (!m_initialized) {
        // 初始化阶段忽略，避免访问未就绪的控件
        return;
    }
    const ProjectLanguage lang = currentLanguage();
    refreshTemplateList(lang);
    refreshLanguageSpecificUI();
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
    // 仅 C++ 语言才显示高级编译配置
    if (currentLanguage() == ProjectLanguage::Cpp) {
        ui->advancedConfigGroupBox->setVisible(checked);
    } else {
        ui->advancedConfigGroupBox->setVisible(false);
    }

    m_projectConfig.setProjectConfigMode(
        checked ? ProjectConfigMode::AdvancedMode : ProjectConfigMode::SimpleMode);
}

void ProjectCreationDialog::onCompilerChanged(int index)
{
    Q_UNUSED(index);
    updateCompilerOptions();
}

void ProjectCreationDialog::onCppStandardChanged(int index)
{
    Q_UNUSED(index);
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

    // C++ 走原有分支
    if (currentTemplate.language == ProjectLanguage::Cpp) {
        if (currentTemplate.type == ProjectType::ConsoleApp)   exampleCode = CONSOLE_APP_EXAMPLE;
        else if (currentTemplate.type == ProjectType::SimpleGame) exampleCode = SIMPLE_GAME_EXAMPLE;
        else if (currentTemplate.type == ProjectType::GuiApp)  exampleCode = GUI_APP_EXAMPLE;
        else                                                   exampleCode = EMPTY_PROJECT_EXAMPLE;
    } else if (!currentTemplate.exampleFiles.isEmpty()) {
        exampleCode = currentTemplate.exampleFiles.first().content;
    } else {
        exampleCode = QStringLiteral("（暂无示例代码）");
    }

    QDialog previewDialog(this);
    previewDialog.setWindowTitle("示例代码预览 - " + currentTemplate.name);
    previewDialog.resize(700, 460);

    QVBoxLayout* layout = new QVBoxLayout(&previewDialog);
    QTextEdit* textEdit = new QTextEdit(&previewDialog);
    textEdit->setPlainText(exampleCode);
    textEdit->setReadOnly(true);
    textEdit->setFont(QFont("Consolas", 11));
    layout->addWidget(textEdit);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, &previewDialog);
    layout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::rejected, &previewDialog, &QDialog::reject);

    previewDialog.exec();
}

void ProjectCreationDialog::onResetToDefaults()
{
    ui->projectNameLineEdit->clear();
    ui->displayNameLineEdit->clear();

    if (!m_defaultProjectPath.isEmpty()) {
        ui->locationLineEdit->setText(m_defaultProjectPath);
    } else {
        ui->locationLineEdit->setText(getDefaultProjectsPath());
    }

    // 语言回到 C++（会触发 onLanguageChanged）
    if (m_languageComboBox && m_languageComboBox->count() > 0) {
        m_languageComboBox->setCurrentIndex(0);
    }

    // 模板回到当前语言的第一个
    if (ui->templateComboBox->count() > 0) {
        ui->templateComboBox->setCurrentIndex(0);
    }

    ui->simpleModeRadioButton->setChecked(true);
    ui->advancedConfigGroupBox->setVisible(false);

    // C++ 编译选项恢复默认
    ui->compilerComboBox->setCurrentIndex(0);
    ui->cppStandardComboBox->setCurrentText("c++17");
    ui->optimizationComboBox->setCurrentText("none");
    ui->warningLevelComboBox->setCurrentText("all");
    ui->debugInfoCheckBox->setChecked(true);

    ui->autoOpenCheckBox->setChecked(true);
    ui->createGitRepoCheckBox->setChecked(false);

    m_projectConfig = ProjectConfig();
    applyCurrentTemplate();
    updatePathPreview();
}

void ProjectCreationDialog::setDisplayName(const QString& displayName)
{
    m_projectConfig.setDisplayName(displayName);
}

void ProjectCreationDialog::setProjectType(ProjectType type)
{
    m_projectConfig.setProjectType(type);
}

void ProjectCreationDialog::setCompilerType(ProjectCompilerType type)
{
    m_projectConfig.setProjectCompilerType(type);
}

// ============================================================================
//  七、初始化方法
// ============================================================================

void ProjectCreationDialog::setupUI()
{
    // ---------- 静态控件基础配置（原有） ----------
    ui->projectNameLineEdit->setPlaceholderText("例如: MyFirstGame");
    ui->displayNameLineEdit->setPlaceholderText("例如: 我的第一个游戏");
    ui->locationLineEdit->setPlaceholderText("选择项目保存的位置");

    // ---------- 模板下拉框（先空着，后面 refreshTemplateList 会填） ----------
    ui->templateComboBox->clear();

    // ---------- C++ 编译器下拉框 ----------
    ui->compilerComboBox->clear();
    ui->compilerComboBox->addItem("自动检测", "auto");
    ui->compilerComboBox->addItem("GCC (g++)", "gcc");
    ui->compilerComboBox->addItem("Clang (clang++)", "clang");
#ifdef Q_OS_WIN
    ui->compilerComboBox->addItem("MSVC (cl.exe)", "msvc");
#endif

    // ---------- C++ 标准 ----------
    ui->cppStandardComboBox->clear();
    ui->cppStandardComboBox->addItems({"c++11", "c++14", "c++17", "c++20", "c++23"});
    ui->cppStandardComboBox->setCurrentText("c++17");

    // ---------- 优化级别 ----------
    ui->optimizationComboBox->clear();
    ui->optimizationComboBox->addItems({"none", "fast", "faster", "fastest", "size"});
    ui->optimizationComboBox->setCurrentText("none");

    // ---------- 警告级别 ----------
    ui->warningLevelComboBox->clear();
    ui->warningLevelComboBox->addItems({"none", "low", "medium", "high", "all"});
    ui->warningLevelComboBox->setCurrentText("all");

    // ---------- 默认项目路径 ----------
    if (m_defaultProjectPath.isEmpty()) {
        m_defaultProjectPath = getDefaultProjectsPath();
    }
    ui->locationLineEdit->setText(m_defaultProjectPath);

    // ---------- 隐藏高级配置 ----------
    ui->advancedConfigGroupBox->setVisible(false);

    // ---------- 底部按钮 ----------
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("创建");
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText("取消");
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    // ---------- tab 顺序 ----------
    setTabOrder(ui->projectNameLineEdit, ui->displayNameLineEdit);
    setTabOrder(ui->displayNameLineEdit, ui->locationLineEdit);
    setTabOrder(ui->locationLineEdit, ui->browseButton);
    setTabOrder(ui->browseButton, ui->templateComboBox);

    ui->projectNameLineEdit->setFocus();

    // ============================================================
    //  ★ 关键：创建并插入"语言选择器"与"语言特有选项组"
    // ------------------------------------------------------------
    //  1) 创建语言下拉框
    //  2) 把它插入到 templateComboBox 上方
    //  3) 创建"语言特有选项"分组
    // ============================================================
    createLanguageComboBox();
    insertLanguageComboBoxIntoLayout();
    createLanguageSpecificGroup();
}

void ProjectCreationDialog::setupConnections()
{
    // ---------- 项目基本信息 ----------
    connect(ui->projectNameLineEdit, &QLineEdit::textChanged,
            this, &ProjectCreationDialog::onProjectNameChanged);
    connect(ui->displayNameLineEdit, &QLineEdit::textChanged,
            this, &ProjectCreationDialog::onDisplayNameChanged);
    connect(ui->locationLineEdit, &QLineEdit::textChanged,
            this, &ProjectCreationDialog::onLocationChanged);
    connect(ui->browseButton, &QPushButton::clicked,
            this, &ProjectCreationDialog::onBrowseLocation);

    // ---------- 模板下拉框 ----------
    connect(ui->templateComboBox,
            static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ProjectCreationDialog::onTemplateChanged);

    // ---------- 编译模式 ----------
    connect(ui->simpleModeRadioButton, &QRadioButton::toggled,
            this, &ProjectCreationDialog::onCompileModeChanged);
    connect(ui->advancedModeRadioButton, &QRadioButton::toggled,
            this, &ProjectCreationDialog::onCompileModeChanged);

    // ---------- C++ 编译器相关 ----------
    connect(ui->compilerComboBox,
            static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ProjectCreationDialog::onCompilerChanged);
    connect(ui->cppStandardComboBox,
            static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ProjectCreationDialog::onCppStandardChanged);
    connect(ui->optimizationComboBox,
            static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ProjectCreationDialog::onOptimizationChanged);
    connect(ui->warningLevelComboBox,
            static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ProjectCreationDialog::onWarningLevelChanged);

    // ---------- 高级配置 / 示例 / 重置 ----------
    connect(ui->advancedConfigButton, &QPushButton::clicked,
            this, &ProjectCreationDialog::onShowAdvancedConfig);
    connect(ui->exampleCodeButton, &QPushButton::clicked,
            this, &ProjectCreationDialog::onShowExampleCode);
    connect(ui->resetButton, &QPushButton::clicked,
            this, &ProjectCreationDialog::onResetToDefaults);

    // ---------- 对话框按钮 ----------
    connect(ui->buttonBox, &QDialogButtonBox::accepted,
            this, &ProjectCreationDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);

    // ============================================================
    //  ★ 关键：语言下拉框信号
    // ============================================================
    if (m_languageComboBox) {
        connect(m_languageComboBox,
                static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this, &ProjectCreationDialog::onLanguageChanged);
    }
}

// ============================================================================
//  八、模板系统
// ============================================================================

void ProjectCreationDialog::initTemplates()
{
    m_templates.clear();
    m_templateMap.clear();

    // ============================================================
    //  C++ 模板（与原版本一致，只是补上 language = Cpp）
    // ============================================================
    {
        ProjectTemplate t;
        t.id = "console_app";
        t.name = "控制台应用程序";
        t.description = "创建一个简单的命令行程序，适合学习基础 C++ 语法。\n"
                        "包含基本的 main 函数结构和标准库使用示例。";
        t.language = ProjectLanguage::Cpp;
        t.type = ProjectType::ConsoleApp;
        t.icon = ":/icons/icons/template_console.svg";
        t.defaultFiles = {"src/main.cpp"};
        t.directories = {"src", "include", "bin", "docs"};

        ProjectTemplate::ExampleFile f;
        f.path = "src/main.cpp";
        f.content = CONSOLE_APP_EXAMPLE;
        f.language = "cpp";
        t.exampleFiles.append(f);

        m_templates.append(t);
        m_templateMap[t.id] = t;
    }
    {
        ProjectTemplate t;
        t.id = "simple_game";
        t.name = "简单游戏";
        t.description = "创建一个有趣的小游戏项目，如猜数字、井字棋等。\n"
                        "增加编程的趣味性，适合有一定基础的学习者。";
        t.language = ProjectLanguage::Cpp;
        t.type = ProjectType::SimpleGame;
        t.icon = ":/icons/icons/template_game.svg";
        t.defaultFiles = {"src/main.cpp", "src/game.cpp", "include/game.h"};
        t.directories = {"src", "include", "assets", "bin", "docs"};

        ProjectTemplate::ExampleFile f;
        f.path = "src/main.cpp";
        f.content = SIMPLE_GAME_EXAMPLE;
        f.language = "cpp";
        t.exampleFiles.append(f);

        m_templates.append(t);
        m_templateMap[t.id] = t;
    }
    {
        ProjectTemplate t;
        t.id = "gui_app";
        t.name = "图形界面程序";
        t.description = "创建一个带窗口界面的应用程序，使用 Qt 框架。\n"
                        "学习图形界面编程，创建更直观的用户界面。";
        t.language = ProjectLanguage::Cpp;
        t.type = ProjectType::GuiApp;
        t.icon = ":/icons/icons/template_gui.svg";
        t.defaultFiles = {"src/main.cpp", "src/mainwindow.cpp", "include/mainwindow.h"};
        t.directories = {"src", "include", "ui", "resources", "bin", "docs"};

        ProjectTemplate::ExampleFile f;
        f.path = "src/main.cpp";
        f.content = GUI_APP_EXAMPLE;
        f.language = "cpp";
        t.exampleFiles.append(f);

        m_templates.append(t);
        m_templateMap[t.id] = t;
    }
    {
        ProjectTemplate t;
        t.id = "empty_project";
        t.name = "空白项目";
        t.description = "创建一个完全自定义的项目结构，没有预置代码。\n"
                        "适合有经验的学习者，或需要特殊结构的项目。";
        t.language = ProjectLanguage::Cpp;
        t.type = ProjectType::EmptyProject;
        t.icon = ":/icons/icons/template_empty.svg";
        t.defaultFiles = {};
        t.directories = {"src", "include"};

        ProjectTemplate::ExampleFile f;
        f.path = "src/main.cpp";
        f.content = EMPTY_PROJECT_EXAMPLE;
        f.language = "cpp";
        t.exampleFiles.append(f);

        m_templates.append(t);
        m_templateMap[t.id] = t;
    }

    // ============================================================
    //  Python 模板
    // ============================================================
    {
        ProjectTemplate t;
        t.id = "python_blank";
        t.name = "Python 空白项目";
        t.description = "创建一个空白 Python 项目。\n"
                        "包含 main.py、requirements.txt 和可选的虚拟环境 (.venv)。";
        t.language = ProjectLanguage::Python;
        t.type = ProjectType::EmptyProject;
        t.icon = ":/icons/icons/python.svg";
        t.defaultFiles = {"main.py", "requirements.txt"};
        t.directories = {"src", "tests"};

        ProjectTemplate::ExampleFile f;
        f.path = "main.py";
        f.content = PYTHON_MAIN_EXAMPLE;
        f.language = "python";
        t.exampleFiles.append(f);

        m_templates.append(t);
        m_templateMap[t.id] = t;
    }

    // ============================================================
    //  Go 模板
    // ============================================================
    {
        ProjectTemplate t;
        t.id = "go_blank";
        t.name = "Go 空白项目";
        t.description = "创建一个 Go 模块项目。\n"
                        "包含 main.go 和 go.mod，可直接用 go build 构建。";
        t.language = ProjectLanguage::Go;
        t.type = ProjectType::EmptyProject;
        t.icon = ":/icons/icons/go.svg";
        t.defaultFiles = {"main.go", "go.mod"};
        t.directories = {"cmd", "internal", "pkg"};

        ProjectTemplate::ExampleFile f;
        f.path = "main.go";
        f.content = GO_MAIN_EXAMPLE;
        f.language = "go";
        t.exampleFiles.append(f);

        m_templates.append(t);
        m_templateMap[t.id] = t;
    }

    // ============================================================
    //  Rust 模板
    // ============================================================
    {
        ProjectTemplate t;
        t.id = "rust_blank";
        t.name = "Rust 项目";
        t.description = "创建一个 Rust 项目（可执行或库）。\n"
                        "包含 Cargo.toml 与 src/ 目录，可直接用 cargo build 构建。";
        t.language = ProjectLanguage::Rust;
        t.type = ProjectType::EmptyProject;
        t.icon = ":/icons/icons/rust.svg";
        t.defaultFiles = {"Cargo.toml", "src/main.rs"};
        t.directories = {"src", "tests"};

        ProjectTemplate::ExampleFile f;
        f.path = "src/main.rs";
        f.content = RUST_MAIN_EXAMPLE;
        f.language = "rust";
        t.exampleFiles.append(f);

        m_templates.append(t);
        m_templateMap[t.id] = t;
    }

    // ============================================================
    //  Node.js 模板 —— JavaScript
    // ============================================================
    {
        ProjectTemplate t;
        t.id = "node_js";
        t.name = "Node.js 项目 (JavaScript)";
        t.description = "创建一个 Node.js 项目，使用 JavaScript。\n"
                        "包含 package.json 与 index.js。";
        t.language = ProjectLanguage::Node;
        t.type = ProjectType::EmptyProject;
        t.icon = ":/icons/icons/node.svg";
        t.defaultFiles = {"package.json", "index.js"};
        t.directories = {"src"};

        ProjectTemplate::ExampleFile f;
        f.path = "index.js";
        f.content = NODE_JS_EXAMPLE;
        f.language = "javascript";
        t.exampleFiles.append(f);

        m_templates.append(t);
        m_templateMap[t.id] = t;
    }

    // ============================================================
    //  Node.js 模板 —— TypeScript
    // ============================================================
    {
        ProjectTemplate t;
        t.id = "node_ts";
        t.name = "Node.js 项目 (TypeScript)";
        t.description = "创建一个 Node.js 项目，使用 TypeScript。\n"
                        "包含 package.json、tsconfig.json 与 src/index.ts。";
        t.language = ProjectLanguage::Node;
        t.type = ProjectType::EmptyProject;
        t.icon = ":/icons/icons/typescript.svg";
        t.defaultFiles = {"package.json", "tsconfig.json", "src/index.ts"};
        t.directories = {"src"};

        ProjectTemplate::ExampleFile f;
        f.path = "src/index.ts";
        f.content = NODE_TS_EXAMPLE;
        f.language = "typescript";
        t.exampleFiles.append(f);

        m_templates.append(t);
        m_templateMap[t.id] = t;
    }

    // ============================================================
    //  GenericAI 模板 —— 交由 AI 服务端下发命令
    // ============================================================
    {
        ProjectTemplate t;
        t.id = "generic_ai";
        t.name = "AI 通用适配器项目";
        t.description = "适用于任意语言 / 工具链的空项目。\n"
                        "可在下方指定 build / run / clean 命令，或让 AI 自动补全。\n"
                        "命令会被写入 .lightcppproj 文件。";
        t.language = ProjectLanguage::GenericAI;
        t.type = ProjectType::EmptyProject;
        t.icon = ":/icons/icons/ai.svg";
        t.defaultFiles = {"README.md"};
        t.directories = {};

        ProjectTemplate::ExampleFile f;
        f.path = "README.md";
        f.content = "# ${DISPLAY_NAME}\n\n"
                    "由 SprintIDE 的 AI 通用适配器创建。\n\n"
                    "请在 `.lightcppproj` 的 `generic_build` 字段中配置命令。\n";
        f.language = "markdown";
        t.exampleFiles.append(f);

        m_templates.append(t);
        m_templateMap[t.id] = t;
    }
}

void ProjectCreationDialog::initCompilerSettings()
{
    QStringList compilers = ProjectConfig::detectAvailableCompilers();
    if (!compilers.isEmpty()) {
        QString tooltip = "检测到以下编译器:\n";
        for (const QString& compiler : compilers) {
            tooltip += "• " + compiler + "\n";
        }
        ui->compilerComboBox->setToolTip(tooltip);
    }
}

// ============================================================================
//  九、★ 新增：多语言支持相关方法
// ============================================================================

// ------------------------------------------------------------
// 创建语言下拉框
// ------------------------------------------------------------
void ProjectCreationDialog::createLanguageComboBox()
{
    m_languageComboBox = new QComboBox(this);
    m_languageComboBox->setObjectName("languageComboBox");
    m_languageComboBox->setMinimumWidth(180);

    // 顺序即显示顺序。这里刻意不列出 Java。
    m_languageComboBox->addItem(QIcon(":/icons/icons/cpp.svg"),
                                "C++",
                                QVariant::fromValue(int(ProjectLanguage::Cpp)));
    m_languageComboBox->addItem(QIcon(":/icons/icons/python.svg"),
                                "Python",
                                QVariant::fromValue(int(ProjectLanguage::Python)));
    m_languageComboBox->addItem(QIcon(":/icons/icons/go.svg"),
                                "Go",
                                QVariant::fromValue(int(ProjectLanguage::Go)));
    m_languageComboBox->addItem(QIcon(":/icons/icons/rust.svg"),
                                "Rust",
                                QVariant::fromValue(int(ProjectLanguage::Rust)));
    m_languageComboBox->addItem(QIcon(":/icons/icons/node.svg"),
                                "Node.js / TypeScript",
                                QVariant::fromValue(int(ProjectLanguage::Node)));
    m_languageComboBox->addItem(QIcon(":/icons/icons/ai.svg"),
                                "AI 通用适配器",
                                QVariant::fromValue(int(ProjectLanguage::GenericAI)));

    // 默认选中 C++
    m_languageComboBox->setCurrentIndex(0);
}

// ------------------------------------------------------------
// 把语言下拉框插入到 templateComboBox 上方
//
// 逻辑：
//   1. 找到 templateComboBox 的父 widget 及其 layout
//   2. 按 layout 类型分别处理：
//        QFormLayout  → form->insertRow(...)
//        QVBoxLayout  → 用 QHBoxLayout 把 label + combo 打包后插入
//        QGridLayout  → 打印警告，提示用户手动调整布局
// ------------------------------------------------------------
void ProjectCreationDialog::insertLanguageComboBoxIntoLayout()
{
    // 控件和布局任意缺失就静默跳过，绝不崩溃
    if (!ui) {
        qWarning() << "[ProjectCreationDialog] ui 未初始化";
        return;
    }
    if (!ui->templateComboBox) {
        qWarning() << "[ProjectCreationDialog] ui->templateComboBox 为空，"
                      "检查 .ui 文件是否包含该控件，或 uic 输出是否过期。";
        return;
    }

    QWidget* anchor = ui->templateComboBox;
    QWidget* parentWidget = anchor->parentWidget();
    if (!parentWidget) {
        qWarning() << "[ProjectCreationDialog] templateComboBox 没有父 widget，"
                      "无法自动插入语言选择器。";
        return;
    }

    QLayout* layout = parentWidget->layout();
    if (!layout) {
        qWarning() << "[ProjectCreationDialog] templateComboBox 的父 widget 没有布局，"
                      "无法自动插入语言选择器。";
        return;
    }

    // ---------- 情况 1：QFormLayout（最理想） ----------
    if (QFormLayout* form = qobject_cast<QFormLayout*>(layout)) {
        for (int row = 0; row < form->rowCount(); ++row) {
            QLayoutItem* item = form->itemAt(row, QFormLayout::FieldRole);
            if (item && item->widget() == anchor) {
                QLabel* label = new QLabel("编程语言:", this);
                // 在模板下拉框所在行插入新的一行
                form->insertRow(row, label, m_languageComboBox);
                return;
            }
        }
    }

    // ---------- 情况 2：QVBoxLayout ----------
    if (QVBoxLayout* vbox = qobject_cast<QVBoxLayout*>(layout)) {
        int idx = vbox->indexOf(anchor);
        if (idx >= 0) {
            QHBoxLayout* rowLayout = new QHBoxLayout;
            QLabel* label = new QLabel("编程语言:", this);
            label->setMinimumWidth(80);
            rowLayout->addWidget(label);
            rowLayout->addWidget(m_languageComboBox, 1);
            vbox->insertLayout(idx, rowLayout);
            return;
        }
    }

    // ---------- 情况 3：QGridLayout —— 无法自动插入，提示用户 ----------
    if (qobject_cast<QGridLayout*>(layout)) {
        qWarning() << "[ProjectCreationDialog] templateComboBox 位于 QGridLayout 中，"
                      "无法自动插入语言选择器。\n"
                      "请在 Qt Designer 中把 templateComboBox 所在的父布局改为 "
                      "QFormLayout 或 QVBoxLayout。";
        // 兜底：直接把语言下拉框塞到父 widget 里（视觉上会靠底部）
        m_languageComboBox->setParent(parentWidget);
        return;
    }

    qWarning() << "[ProjectCreationDialog] 未知的布局类型："
               << layout->metaObject()->className();
}

// ------------------------------------------------------------
// 创建"语言特有选项"分组
// ------------------------------------------------------------
void ProjectCreationDialog::createLanguageSpecificGroup()
{
    m_languageSpecificGroup = new QGroupBox("语言选项", this);
    m_languageSpecificForm  = new QFormLayout(m_languageSpecificGroup);
    m_languageSpecificForm->setLabelAlignment(Qt::AlignRight);

    // ---- Python ----
    m_pythonVenvCheckBox = new QCheckBox("创建虚拟环境 (.venv)", this);
    m_pythonVenvCheckBox->setChecked(true);
    m_pythonInterpreterEdit = new QLineEdit(this);
    m_pythonInterpreterEdit->setPlaceholderText("留空则自动探测 python3 / py");
    m_pythonEntryLabel = new QLabel("main.py", this);

    // ---- Go ----
    m_goModuleEdit = new QLineEdit(this);
    m_goModuleEdit->setPlaceholderText("例如 example.com/myapp");

    // ---- Rust ----
    m_rustKindCombo = new QComboBox(this);
    m_rustKindCombo->addItem("可执行程序 (bin)", "bin");
    m_rustKindCombo->addItem("库 (lib)",         "lib");

    // ---- Node ----
    m_nodeVariantCombo = new QComboBox(this);
    m_nodeVariantCombo->addItem("JavaScript", "js");
    m_nodeVariantCombo->addItem("TypeScript", "ts");

    m_nodePmCombo = new QComboBox(this);
    m_nodePmCombo->addItems({"npm", "pnpm", "yarn"});

    m_nodeEntryEdit = new QLineEdit("index.js", this);
    connect(m_nodeVariantCombo,
            static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int) {
                const bool ts = (m_nodeVariantCombo->currentData().toString() == "ts");
                m_nodeEntryEdit->setText(ts ? "src/index.ts" : "index.js");
            });

    // ---- GenericAI ----
    m_genericBuildEdit = new QLineEdit(this);
    m_genericBuildEdit->setPlaceholderText("例如 make -j4");
    m_genericRunEdit = new QLineEdit(this);
    m_genericRunEdit->setPlaceholderText("例如 ./my_app");
    m_genericCleanEdit = new QLineEdit(this);
    m_genericCleanEdit->setPlaceholderText("例如 rm -rf build/");

    // ---- 把 group 插到对话框里 ----
    // 优先插到 templateFilesLabel 之后，否则插到 templateDescriptionLabel 之后
    QWidget* anchor = ui->templateFilesLabel;
    if (!anchor) anchor = ui->templateDescriptionLabel;

    bool inserted = false;
    if (anchor && anchor->parentWidget() && anchor->parentWidget()->layout()) {
        QLayout* lay = anchor->parentWidget()->layout();
        if (QVBoxLayout* vbox = qobject_cast<QVBoxLayout*>(lay)) {
            int idx = vbox->indexOf(anchor);
            if (idx >= 0) {
                vbox->insertWidget(idx + 1, m_languageSpecificGroup);
                inserted = true;
            }
        } else if (QFormLayout* form = qobject_cast<QFormLayout*>(lay)) {
            // 如果锚点在 QFormLayout 里，直接 addRow
            form->addRow(m_languageSpecificGroup);
            inserted = true;
        }
    }

    if (!inserted) {
        if (layout()) {
            layout()->addWidget(m_languageSpecificGroup);
        } else {
            // 极端情况：连对话框都没有布局，就直接挂在 this 上
            m_languageSpecificGroup->setParent(this);
        }
        qWarning() << "[ProjectCreationDialog] 未能确定语言特有选项组的插入点，"
                      "将作为顶层控件独立存在。";
    }

    // 初始隐藏，语言为 C++ 时不显示
    m_languageSpecificGroup->setVisible(false);
}

// ------------------------------------------------------------
// 按语言刷新模板列表
// ------------------------------------------------------------
void ProjectCreationDialog::refreshTemplateList(ProjectLanguage lang)
{
    if (!ui || !ui->templateComboBox) {
        qWarning() << "[ProjectCreationDialog] refreshTemplateList: "
                      "templateComboBox 不可用";
        return;
    }
    if (!m_languageComboBox) {
        qWarning() << "[ProjectCreationDialog] refreshTemplateList: "
                      "languageComboBox 尚未创建";
        return;
    }

    // 暂时屏蔽信号，避免 clear/add 时反复触发 onTemplateChanged
    ui->templateComboBox->blockSignals(true);
    ui->templateComboBox->clear();

    for (const ProjectTemplate& tmpl : m_templates) {
        if (tmpl.language != lang) continue;
        ui->templateComboBox->addItem(QIcon(tmpl.icon), tmpl.name, tmpl.id);
    }

    // 恢复信号
    ui->templateComboBox->blockSignals(false);

    // 手动触发一次模板变更，保证描述 / 配置同步
    if (ui->templateComboBox->count() > 0) {
        ui->templateComboBox->setCurrentIndex(0);
        onTemplateChanged(0);
    }
}

// ------------------------------------------------------------
// 读取当前语言（从下拉框读取）
// ------------------------------------------------------------
ProjectLanguage ProjectCreationDialog::currentLanguage() const
{
    if (!m_languageComboBox) return ProjectLanguage::Cpp;
    return static_cast<ProjectLanguage>(
        m_languageComboBox->currentData().toInt());
}

// ------------------------------------------------------------
// 按语言刷新"语言特有选项"组
// ------------------------------------------------------------
void ProjectCreationDialog::refreshLanguageSpecificUI()
{
    if (!m_languageSpecificGroup || !m_languageSpecificForm) return;

    // 1. 清空所有行（QFormLayout 会移除对 widget 的引用，但不会 delete）
    // while (m_languageSpecificForm->rowCount() > 0) {
    //     m_languageSpecificForm->removeRow(0);
    // }
    QList<QLayoutItem*> allItems;
    for (int i = 0; i < m_languageSpecificForm->rowCount(); ++i) {
        if (QLayoutItem* li = m_languageSpecificForm->itemAt(i, QFormLayout::LabelRole)) {
            allItems.append(li);
        }
        if (QLayoutItem* li = m_languageSpecificForm->itemAt(i, QFormLayout::FieldRole)) {
            allItems.append(li);
        }
    }
    for (QLayoutItem* item : allItems) {
        if (!item) continue;
        m_languageSpecificForm->removeItem(item);
        if (QWidget* w = item->widget()) {
            // 保留 widget 的生命周期：把它挂回 group 名下并隐藏
            // 注意：不要 delete，成员指针仍指向它
            w->setParent(m_languageSpecificGroup);
            w->hide();
        }
        delete item;  // 只删除 QLayoutItem 容器，widget 存活
    }

    const ProjectLanguage lang = currentLanguage();

    // 2. 处理 C++：隐藏语言特有 group，启用 C++ 编译器相关控件
    if (lang == ProjectLanguage::Cpp) {
        m_languageSpecificGroup->setVisible(false);

        // 每一项都判空，绝不直接解引用
        if (ui->compilerComboBox)        ui->compilerComboBox->setEnabled(true);
        if (ui->cppStandardComboBox)     ui->cppStandardComboBox->setEnabled(true);
        if (ui->advancedConfigGroupBox && ui->advancedModeRadioButton) {
            ui->advancedConfigGroupBox->setVisible(
                ui->advancedModeRadioButton->isChecked());
        }
        return;
    }

    // 3. 非 C++：禁用并隐藏 C++ 编译器相关控件
    if (ui->advancedConfigGroupBox)  ui->advancedConfigGroupBox->setVisible(false);
    if (ui->compilerComboBox)        ui->compilerComboBox->setEnabled(false);
    if (ui->cppStandardComboBox)     ui->cppStandardComboBox->setEnabled(false);

    m_languageSpecificGroup->setVisible(true);

    switch (lang) {
    case ProjectLanguage::Python:
        m_languageSpecificForm->addRow("入口文件:", m_pythonEntryLabel);
        m_pythonEntryLabel->show();

        m_languageSpecificForm->addRow(QString(), m_pythonVenvCheckBox);
        m_pythonVenvCheckBox->show();

        m_languageSpecificForm->addRow("解释器路径:", m_pythonInterpreterEdit);
        m_pythonInterpreterEdit->show();
        break;

    case ProjectLanguage::Go:
        m_languageSpecificForm->addRow("Module 路径:", m_goModuleEdit);
        m_goModuleEdit->show();
        break;

    case ProjectLanguage::Rust:
        m_languageSpecificForm->addRow("目标类型:", m_rustKindCombo);
        m_rustKindCombo->show();
        break;

    case ProjectLanguage::Node:
        m_languageSpecificForm->addRow("语言变体:", m_nodeVariantCombo);
        m_nodeVariantCombo->show();

        m_languageSpecificForm->addRow("包管理器:", m_nodePmCombo);
        m_nodePmCombo->show();

        m_languageSpecificForm->addRow("入口文件:", m_nodeEntryEdit);
        m_nodeEntryEdit->show();
        break;

    case ProjectLanguage::GenericAI:
        m_languageSpecificForm->addRow("Build 命令:", m_genericBuildEdit);
        m_genericBuildEdit->show();

        m_languageSpecificForm->addRow("Run 命令:", m_genericRunEdit);
        m_genericRunEdit->show();

        m_languageSpecificForm->addRow("Clean 命令:", m_genericCleanEdit);
        m_genericCleanEdit->show();
        break;

    default:
        break;
    }
}

// ============================================================================
//  十、输入验证
// ============================================================================

bool ProjectCreationDialog::validateInput(QStringList& errors) const
{
    errors.clear();

    QString projectName = ui->projectNameLineEdit->text().trimmed();
    QString nameError;
    if (!validateProjectName(projectName, nameError)) {
        errors.append("项目名称: " + nameError);
    }

    QString displayName = ui->displayNameLineEdit->text().trimmed();
    if (displayName.isEmpty()) {
        errors.append("显示名称不能为空");
    }

    QString projectPath = ui->locationLineEdit->text().trimmed();
    QString pathError;
    if (!validateProjectPath(projectPath, pathError)) {
        errors.append("项目路径: " + pathError);
    } else {
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

    static const QString illegalChars = "<>:\"|?*";
    for (const QChar& ch : illegalChars) {
        if (path.contains(ch)) {
            error = "路径包含非法字符";
            return false;
        }
    }

    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            error = "无法创建目录，请检查权限";
            return false;
        }
    } else {
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

// ============================================================================
//  十一、目录结构创建（所有语言共用）
// ============================================================================

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

    // 按模板创建子目录
    ProjectTemplate currentTemplate = getCurrentTemplate();
    for (const QString& dirName : currentTemplate.directories) {
        QString dirPath = projectDir.absoluteFilePath(dirName);
        if (projectDir.mkpath(dirName)) {
            m_result.createdDirectories.append(dirPath);
        } else {
            m_result.errorMessage = "无法创建目录: " + dirPath;
            return false;
        }
    }

    // 更新 ProjectConfig 的路径
    m_projectConfig.setProjectPath(fullProjectPath);

    return true;
}

// ============================================================================
//  十二、C++ / GenericAI 项目生成（原有流程，稍作整理）
// ============================================================================

bool ProjectCreationDialog::generateProjectFiles()
{
    if (!generateConfigFile())   return false;
    if (!generateExampleFiles()) return false;
    return true;
}

bool ProjectCreationDialog::generateConfigFile()
{
    // 更新基础信息
    m_projectConfig.setProjectName(ui->projectNameLineEdit->text().trimmed());
    m_projectConfig.setDisplayName(ui->displayNameLineEdit->text().trimmed());

    // 项目类型
    ProjectTemplate currentTemplate = getCurrentTemplate();
    m_projectConfig.setProjectType(currentTemplate.type);

    // 编译模式
    m_projectConfig.setProjectConfigMode(
        ui->advancedModeRadioButton->isChecked()
            ? ProjectConfigMode::AdvancedMode
            : ProjectConfigMode::SimpleMode);

    // 编译器类型（仅 C++ 有意义；其他语言默认 AutoDetect）
    if (currentLanguage() == ProjectLanguage::Cpp) {
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

        m_projectConfig.setCppStandard(ui->cppStandardComboBox->currentText());
        m_projectConfig.setOptimizationLevel(ui->optimizationComboBox->currentText());
        m_projectConfig.setWarningLevel(ui->warningLevelComboBox->currentText());
        m_projectConfig.setEnableDebugInfo(ui->debugInfoCheckBox->isChecked());
    }

    // 输出目录：C++ 为 bin；其他语言由各自的生成器设置
    if (currentLanguage() == ProjectLanguage::Cpp) {
        m_projectConfig.setOutputPath("bin");
    }

    // 应用模板
    m_projectConfig.applyTemplate(currentTemplate.type);

    // 保存 .lightcppproj
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

    for (const ProjectTemplate::ExampleFile& example : currentTemplate.exampleFiles) {
        QString filePath = projectDir.absoluteFilePath(example.path);
        QFileInfo fileInfo(filePath);

        // 确保目录存在
        if (!projectDir.mkpath(fileInfo.absolutePath())) {
            m_result.errorMessage = "无法创建目录: " + fileInfo.absolutePath();
            return false;
        }

        // 变量替换
        QMap<QString, QString> variables;
        variables["PROJECT_NAME"] = m_projectConfig.projectName();
        variables["DISPLAY_NAME"] = m_projectConfig.displayName();
        variables["CREATE_DATE"]  = QDateTime::currentDateTime().toString("yyyy-MM-dd");

        QString content = replaceTemplateVariables(example.content, variables);

        if (!writeTextFile(filePath, content)) {
            m_result.errorMessage = "无法写入文件: " + filePath;
            return false;
        }
        m_result.createdFiles.append(filePath);
    }

    return true;
}

// ============================================================================
//  十三、★ 新增：Python / Go / Rust / Node 生成器
// ============================================================================

// ------------------------------------------------------------
// Python 项目生成器
// ------------------------------------------------------------
bool ProjectCreationDialog::generatePythonProject()
{
    QDir projDir(m_projectConfig.projectPath());

    // ---- 1. main.py ----
    QString mainPath = projDir.absoluteFilePath("main.py");
    if (!writeTextFile(mainPath, PYTHON_MAIN_EXAMPLE)) {
        m_result.errorMessage = "无法创建 main.py";
        return false;
    }
    m_result.createdFiles.append(mainPath);

    // ---- 2. requirements.txt（空文件） ----
    QString reqPath = projDir.absoluteFilePath("requirements.txt");
    QFile(reqPath).open(QIODevice::WriteOnly);   // 立刻析构，创建空文件
    m_result.createdFiles.append(reqPath);

    // ---- 3. .lightcppproj ----
    m_projectConfig.setProjectName(ui->projectNameLineEdit->text().trimmed());
    m_projectConfig.setDisplayName(ui->displayNameLineEdit->text().trimmed());
    m_projectConfig.setProjectType(ProjectType::EmptyProject);
    m_projectConfig.setOutputPath("__pycache__");

    QString cfgPath = projDir.absoluteFilePath(
        m_projectConfig.projectName() + ".lightcppproj");
    QStringList cfgErrors;
    if (!m_projectConfig.saveToFile(cfgPath, cfgErrors)) {
        m_result.errorMessage = "无法写入配置文件: " + cfgErrors.join("; ");
        return false;
    }
    m_result.createdFiles.append(cfgPath);

    // ---- 4. 创建虚拟环境（如果用户勾选） ----
    if (m_pythonVenvCheckBox && m_pythonVenvCheckBox->isChecked()) {
        QString py = m_pythonInterpreterEdit->text().trimmed();
        if (py.isEmpty()) py = "python3";

        QProcess p;
        p.setWorkingDirectory(m_projectConfig.projectPath());
        p.start(py, {"-m", "venv", ".venv"});
        if (!p.waitForFinished(60000)) {
            // 超时不视为致命错误，用户可稍后手动创建
            qWarning() << "[ProjectCreationDialog] venv 创建超时（可稍后手动创建）";
        }
    }

    return true;
}

// ------------------------------------------------------------
// Go 项目生成器
// ------------------------------------------------------------
bool ProjectCreationDialog::generateGoProject()
{
    QDir projDir(m_projectConfig.projectPath());

    QString moduleName = m_goModuleEdit->text().trimmed();
    if (moduleName.isEmpty()) {
        moduleName = m_projectConfig.projectName().toLower();
    }

    // ---- 1. main.go ----
    QString mainPath = projDir.absoluteFilePath("main.go");
    if (!writeTextFile(mainPath, GO_MAIN_EXAMPLE)) {
        m_result.errorMessage = "无法创建 main.go";
        return false;
    }
    m_result.createdFiles.append(mainPath);

    // ---- 2. go.mod ----
    QString modContent = QString("module %1\n\ngo 1.21\n").arg(moduleName);
    QString modPath = projDir.absoluteFilePath("go.mod");
    if (!writeTextFile(modPath, modContent)) {
        m_result.errorMessage = "无法创建 go.mod";
        return false;
    }
    m_result.createdFiles.append(modPath);

    // ---- 3. .lightcppproj ----
    m_projectConfig.setProjectName(ui->projectNameLineEdit->text().trimmed());
    m_projectConfig.setDisplayName(ui->displayNameLineEdit->text().trimmed());
    m_projectConfig.setProjectType(ProjectType::EmptyProject);
    m_projectConfig.setOutputPath("bin");

    QString cfgPath = projDir.absoluteFilePath(
        m_projectConfig.projectName() + ".lightcppproj");
    QStringList cfgErrors;
    if (!m_projectConfig.saveToFile(cfgPath, cfgErrors)) {
        m_result.errorMessage = "无法写入配置文件: " + cfgErrors.join("; ");
        return false;
    }
    m_result.createdFiles.append(cfgPath);

    return true;
}

// ------------------------------------------------------------
// Rust 项目生成器
// ------------------------------------------------------------
bool ProjectCreationDialog::generateRustProject()
{
    QDir projDir(m_projectConfig.projectPath());

    const QString kind = m_rustKindCombo
                             ? m_rustKindCombo->currentData().toString()
                             : QStringLiteral("bin");

    // ---- 1. Cargo.toml ----
    QString tomlContent = QString(
                              "[package]\n"
                              "name = \"%1\"\n"
                              "version = \"0.1.0\"\n"
                              "edition = \"2021\"\n\n"
                              "[dependencies]\n"
                              ).arg(m_projectConfig.projectName().toLower());

    QString tomlPath = projDir.absoluteFilePath("Cargo.toml");
    if (!writeTextFile(tomlPath, tomlContent)) {
        m_result.errorMessage = "无法创建 Cargo.toml";
        return false;
    }
    m_result.createdFiles.append(tomlPath);

    // ---- 2. src/main.rs 或 src/lib.rs ----
    if (!projDir.mkpath("src")) {
        m_result.errorMessage = "无法创建 src 目录";
        return false;
    }
    const QString srcName = (kind == "lib") ? "lib.rs" : "main.rs";
    const QString srcContent = (kind == "lib") ? RUST_LIB_EXAMPLE : RUST_MAIN_EXAMPLE;
    QString srcPath = projDir.absoluteFilePath("src/" + srcName);
    if (!writeTextFile(srcPath, srcContent)) {
        m_result.errorMessage = "无法创建 " + srcName;
        return false;
    }
    m_result.createdFiles.append(srcPath);

    // ---- 3. .lightcppproj ----
    m_projectConfig.setProjectName(ui->projectNameLineEdit->text().trimmed());
    m_projectConfig.setDisplayName(ui->displayNameLineEdit->text().trimmed());
    m_projectConfig.setProjectType(ProjectType::EmptyProject);
    m_projectConfig.setOutputPath("target");

    QString cfgPath = projDir.absoluteFilePath(
        m_projectConfig.projectName() + ".lightcppproj");
    QStringList cfgErrors;
    if (!m_projectConfig.saveToFile(cfgPath, cfgErrors)) {
        m_result.errorMessage = "无法写入配置文件: " + cfgErrors.join("; ");
        return false;
    }
    m_result.createdFiles.append(cfgPath);

    return true;
}

// ------------------------------------------------------------
// Node.js 项目生成器（支持 JS / TS）
// ------------------------------------------------------------
bool ProjectCreationDialog::generateNodeProject()
{
    QDir projDir(m_projectConfig.projectPath());

    // 判断变体
    const bool isTs = m_nodeVariantCombo
                      && m_nodeVariantCombo->currentData().toString() == "ts";

    // ---- 1. package.json ----
    QJsonObject pkg;
    pkg["name"]    = m_projectConfig.projectName().toLower();
    pkg["version"] = "1.0.0";
    pkg["main"]    = isTs ? "dist/index.js" : "index.js";

    QJsonObject scripts;
    scripts["start"] = isTs ? "node dist/index.js" : "node index.js";
    scripts["build"] = isTs ? "tsc" : "echo \"No build step\"";
    pkg["scripts"] = scripts;

    if (isTs) {
        QJsonObject devDeps;
        devDeps["typescript"] = "^5.0.0";
        pkg["devDependencies"] = devDeps;
    }

    QString pkgPath = projDir.absoluteFilePath("package.json");
    if (!writeTextFile(pkgPath,
                       QString::fromUtf8(QJsonDocument(pkg).toJson(QJsonDocument::Indented)))) {
        m_result.errorMessage = "无法创建 package.json";
        return false;
    }
    m_result.createdFiles.append(pkgPath);

    // ---- 2. 源文件 ----
    const QString srcRel = isTs ? "src/index.ts" : "index.js";
    const QString srcAbs = projDir.absoluteFilePath(srcRel);
    if (!projDir.mkpath(QFileInfo(srcAbs).absolutePath())) {
        m_result.errorMessage = "无法创建源码目录";
        return false;
    }
    const QString srcContent = isTs ? NODE_TS_EXAMPLE : NODE_JS_EXAMPLE;
    if (!writeTextFile(srcAbs, srcContent)) {
        m_result.errorMessage = "无法创建入口文件";
        return false;
    }
    m_result.createdFiles.append(srcAbs);

    // ---- 3. TypeScript 额外生成 tsconfig.json ----
    if (isTs) {
        QString tsConfigContent =
            "{\n"
            "  \"compilerOptions\": {\n"
            "    \"target\": \"ES2020\",\n"
            "    \"module\": \"commonjs\",\n"
            "    \"outDir\": \"dist\",\n"
            "    \"strict\": true,\n"
            "    \"esModuleInterop\": true\n"
            "  },\n"
            "  \"include\": [\"src/**/*\"]\n"
            "}\n";
        QString tsCfgPath = projDir.absoluteFilePath("tsconfig.json");
        if (!writeTextFile(tsCfgPath, tsConfigContent)) {
            m_result.errorMessage = "无法创建 tsconfig.json";
            return false;
        }
        m_result.createdFiles.append(tsCfgPath);
    }

    // ---- 4. .lightcppproj ----
    m_projectConfig.setProjectName(ui->projectNameLineEdit->text().trimmed());
    m_projectConfig.setDisplayName(ui->displayNameLineEdit->text().trimmed());
    m_projectConfig.setProjectType(ProjectType::EmptyProject);
    m_projectConfig.setOutputPath(isTs ? "dist" : ".");

    QString cfgPath = projDir.absoluteFilePath(
        m_projectConfig.projectName() + ".lightcppproj");
    QStringList cfgErrors;
    if (!m_projectConfig.saveToFile(cfgPath, cfgErrors)) {
        m_result.errorMessage = "无法写入配置文件: " + cfgErrors.join("; ");
        return false;
    }
    m_result.createdFiles.append(cfgPath);

    return true;
}

// ============================================================================
//  十四、UI 更新辅助函数
// ============================================================================

void ProjectCreationDialog::updatePathPreview()
{
    // 早退：ui 或关键控件缺失
    if (!ui || !ui->projectNameLineEdit || !ui->locationLineEdit
        || !ui->pathPreviewLabel || !ui->buttonBox) {
        return;
    }

    QPushButton* okBtn = ui->buttonBox->button(QDialogButtonBox::Ok);
    if (!okBtn) return;

    QString projectName = ui->projectNameLineEdit->text().trimmed();
    QString projectPath = ui->locationLineEdit->text().trimmed();

    if (projectName.isEmpty() || projectPath.isEmpty()) {
        ui->pathPreviewLabel->setText("请填写项目名称和路径");
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        return;
    }

    QString fullPath = QDir(projectPath).absoluteFilePath(projectName);

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

    QString nameError, pathError;
    bool canCreate = !projectName.isEmpty()
                     && !projectPath.isEmpty()
                     && validateProjectName(projectName, nameError)
                     && validateProjectPath(projectPath, pathError)
                     && (empty || !exists);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(canCreate);
}

void ProjectCreationDialog::updateTemplateDescription()
{
    ProjectTemplate currentTemplate = getCurrentTemplate();

    ui->templateDescriptionLabel->setText(currentTemplate.description);

    QString fileList = "将创建以下文件:\n";
    for (const QString& file : currentTemplate.defaultFiles) {
        fileList += "• " + file + "\n";
    }

    fileList += "\n目录结构:\n";
    for (const QString& dir : currentTemplate.directories) {
        fileList += "• " + dir + "/\n";
    }

    ui->templateFilesLabel->setText(fileList);
}

void ProjectCreationDialog::updateCompilerOptions()
{
    QString compilerType = ui->compilerComboBox->currentData().toString();

    if (compilerType == "msvc") {
        ui->cppStandardComboBox->clear();
        ui->cppStandardComboBox->addItems({"c++14", "c++17", "c++20", "c++23"});
    } else {
        ui->cppStandardComboBox->clear();
        ui->cppStandardComboBox->addItems({"c++11", "c++14", "c++17", "c++20", "c++23"});
    }
}

void ProjectCreationDialog::applyCurrentTemplate()
{
    ProjectTemplate currentTemplate = getCurrentTemplate();
    m_projectConfig.applyTemplate(currentTemplate.type);
}

ProjectTemplate ProjectCreationDialog::getCurrentTemplate() const
{
    if (ui->templateComboBox->count() == 0) {
        // 防御性返回：模板尚未加载时返回空模板
        ProjectTemplate empty;
        empty.language = ProjectLanguage::Cpp;
        return empty;
    }
    QString templateId = ui->templateComboBox->currentData().toString();
    return m_templateMap.value(templateId, m_templates.first());
}

QString ProjectCreationDialog::replaceTemplateVariables(
    const QString& content,
    const QMap<QString, QString>& variables) const
{
    QString result = content;
    for (auto it = variables.constBegin(); it != variables.constEnd(); ++it) {
        QString placeholder = "${" + it.key() + "}";
        result.replace(placeholder, it.value());
    }
    return result;
}

// ============================================================================
//  十五、状态持久化
// ============================================================================

void ProjectCreationDialog::saveDialogState()
{
    QSettings settings("SprintIDE", "ProjectCreationDialog");

    settings.setValue("lastUsedPath", ui->locationLineEdit->text());
    settings.setValue("lastTemplate", ui->templateComboBox->currentIndex());
    settings.setValue("compileMode",  ui->advancedModeRadioButton->isChecked());
    settings.setValue("compilerType", ui->compilerComboBox->currentIndex());
    settings.setValue("cppStandard",  ui->cppStandardComboBox->currentIndex());
    settings.setValue("optimization", ui->optimizationComboBox->currentIndex());
    settings.setValue("warningLevel", ui->warningLevelComboBox->currentIndex());
    settings.setValue("debugInfo",    ui->debugInfoCheckBox->isChecked());
    settings.setValue("autoOpen",     ui->autoOpenCheckBox->isChecked());
    settings.setValue("createGitRepo", ui->createGitRepoCheckBox->isChecked());

    // ★ 保存语言选择
    if (m_languageComboBox) {
        settings.setValue("lastLanguage",
                          static_cast<int>(currentLanguage()));
    }
}

void ProjectCreationDialog::loadDialogState()
{
    QSettings settings("SprintIDE", "ProjectCreationDialog");

    QString lastPath = settings.value("lastUsedPath").toString();
    if (!lastPath.isEmpty() && QDir(lastPath).exists()) {
        ui->locationLineEdit->setText(lastPath);
    }

    // 先恢复语言，再恢复模板（顺序重要）
    if (m_languageComboBox) {
        int langIdx = settings.value("lastLanguage", 0).toInt();
        if (langIdx >= 0 && langIdx < m_languageComboBox->count()) {
            m_languageComboBox->blockSignals(true);
            m_languageComboBox->setCurrentIndex(langIdx);
            m_languageComboBox->blockSignals(false);
        }
        // 语言选定后刷新模板列表
        refreshTemplateList(currentLanguage());
        refreshLanguageSpecificUI();
    }

    // int lastTemplate = settings.value("lastTemplate", 0).toInt();
    // if (lastTemplate >= 0 && lastTemplate < ui->templateComboBox->count()) {
    //     ui->templateComboBox->setCurrentIndex(lastTemplate);
    // }
    settings.setValue("lastTemplateId",
                      ui->templateComboBox->currentData().toString());

    // 读取时：
    const QString lastId = settings.value("lastTemplateId").toString();
    if (!lastId.isEmpty()) {
        int idx = ui->templateComboBox->findData(lastId);
        if (idx >= 0) ui->templateComboBox->setCurrentIndex(idx);
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

    int optimizationIndex = settings.value("optimization", 0).toInt();
    if (optimizationIndex >= 0 && optimizationIndex < ui->optimizationComboBox->count()) {
        ui->optimizationComboBox->setCurrentIndex(optimizationIndex);
    }

    int warningLevelIndex = settings.value("warningLevel", 4).toInt();
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

// ============================================================================
//  十六、辅助工具函数
// ============================================================================

bool ProjectCreationDialog::writeTextFile(const QString& filePath, const QString& content)
{
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    const bool ok = (f.write(content.toUtf8()) != -1);
    f.close();
    return ok;
}

void ProjectCreationDialog::appendCreatedFile(const QString& filePath)
{
    if (!filePath.isEmpty() && !m_result.createdFiles.contains(filePath)) {
        m_result.createdFiles.append(filePath);
    }
}

bool ProjectCreationDialog::saveProjectConfigFile()
{
    QString cfgPath = QDir(m_projectConfig.projectPath())
    .absoluteFilePath(m_projectConfig.projectName() + ".lightcppproj");
    QStringList errors;
    if (!m_projectConfig.saveToFile(cfgPath, errors)) {
        m_result.errorMessage = "无法写入配置文件: " + errors.join("; ");
        return false;
    }
    m_result.createdFiles.append(cfgPath);
    return true;
}

} // namespace SprintIDE
