#include "mainwindow.h"

// 先包含私有类的头文件
#include "mainwindow_p.h"

// 包含必要的Qt头文件
#include <QFile>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QMenu>
#include <QDebug>
#include <QInputDialog>
#include <QPushButton>
#include <QTemporaryFile>
#include <QStandardItemModel>
#include <QTreeView>
#include <QTextEdit>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QIcon>
#include <QFont>
#include <QStatusBar>
#include <QApplication>
#include <QStandardPaths>
#include <QProcess>
#include <QProcessEnvironment>
#include <QThread>
#include <QTime>
#include <QElapsedTimer>
#include <QRegularExpression>
#include <QTextCursor>
#include <QTextBlock>
#include <QScrollBar>
#include <QStack>
#include <QString>
#include <QDirIterator>
#include <QShortcut>
#include <QListWidgetItem>
#include <QDialog>
#include <QtCore/qjsonarray.h>
#include <QUuid>
#include "builder/java_maven_builder.h"
#include "java/maven_project_model.h"

// 最后包含TextEditor和TabWidget的头文件（它们依赖Qt头文件）
#include "text_editor.h"
#include "tab_widget.h"
#include "theme_manager.h"
#include "project_creation_dialog.h"
#include "project_config.h"

// 包含编译系统头文件
#include "compile_manager.h"
#include "compile_output_parser.h"
#include "process_manager.h"
#include "path_utils.h"

// 包含调试系统头文件
#include "debug_manager.h"
#include "debug_panel.h"
#include "find_dialog.h"
#include "replace_dialog.h"

//LSP服务
// #include "lsp/lsp_client.h"
#include "lsp/lsp_manager.h"

//ai
#include "ai/ai_action_executor.h"
#include "ai/ai_integration.h"
#include "ai/ai_server.h"
#include "ai/chat_panel.h"

#include "language/language_type.h"

#include "Application.h"
#include "config_manager.h"
#include "project_manager.h"
#include "settings_dialog.h"
#include "python/python_debugger.h"
#include "python/python_formatter.h"
#include "python/python_dependency_manager.h"
#include "python/python_virtual_env.h"
#include "python/python_test_runner.h"
#include "python/python_project_model.h"
#include "cpp/cpp_project_model.h"

#include "cpp/cpp_debugger.h"
#include "deps/toolchain_manager.h"
#include "deps/toolchain_types.h"
#include "deps/toolchain_dialog.h"


using namespace SprintIDE;

namespace SprintIDE {

// ========== 辅助函数：根据文件路径检测语言类型 ==========
static SprintIDE::LanguageType detectLanguageTypeFromPath(const QString& filePath) {
    if (filePath.isEmpty()) {
        return SprintIDE::LanguageType::Unknown;
    }

    // 使用现有的 LanguageDetector 获取语言 ID
    SprintIDE::LanguageInfo info = SprintIDE::LanguageDetector::instance().detectLanguage(filePath);
    QString id = info.id;

    // 映射到我们的 LanguageType 枚举
    if (id == "cpp" || id == "c" || id == "cxx" || id == "cc") {
        return SprintIDE::LanguageType::Cpp;
    }
    if (id == "python") {
        return SprintIDE::LanguageType::Python;
    }
    if (id == "java") {
        return SprintIDE::LanguageType::Java;
    }
    if (id == "go") {
        return SprintIDE::LanguageType::Go;
    }
    if (id == "rust") {
        return SprintIDE::LanguageType::Rust;
    }
    if (id == "javascript") {
        return SprintIDE::LanguageType::JavaScript;
    }
    if (id == "typescript") {
        return SprintIDE::LanguageType::TypeScript;
    }
    if (id == "csharp") {
        return SprintIDE::LanguageType::CSharp;
    }
    if (id == "ruby") {
        return SprintIDE::LanguageType::Ruby;
    }
    if (id == "php") {
        return SprintIDE::LanguageType::Php;
    }
    if (id == "shell") {
        return SprintIDE::LanguageType::Shell;
    }
    if (id == "html") {
        return SprintIDE::LanguageType::Html;
    }
    if (id == "css") {
        return SprintIDE::LanguageType::Css;
    }
    if (id == "json") {
        return SprintIDE::LanguageType::Json;
    }
    if (id == "xml") {
        return SprintIDE::LanguageType::Xml;
    }
    if (id == "markdown") {
        return SprintIDE::LanguageType::Markdown;
    }

    // 如果未识别，则返回 Unknown（后续可在编译时根据配置决定是否使用 Generic_AI）
    return SprintIDE::LanguageType::Unknown;
}

static SprintIDE::LanguageType detectProjectLanguage(const QString& projectPath)
{
    if (projectPath.isEmpty()) {
        return SprintIDE::LanguageType::Unknown;
    }

    QDir dir(projectPath);

    // Java
    if (dir.exists("pom.xml") || dir.exists("build.gradle") || dir.exists("gradlew")) {
        return SprintIDE::LanguageType::Java;
    }

    // C++ (CMake 或 Makefile)
    if (dir.exists("CMakeLists.txt") || dir.exists("Makefile") || dir.exists("configure")) {
        return SprintIDE::LanguageType::Cpp;
    }

    // Python
    if (dir.exists("requirements.txt") || dir.exists("setup.py") || dir.exists("pyproject.toml")) {
        return SprintIDE::LanguageType::Python;
    }

    // Go
    if (dir.exists("go.mod")) {
        return SprintIDE::LanguageType::Go;
    }

    // Rust
    if (dir.exists("Cargo.toml")) {
        return SprintIDE::LanguageType::Rust;
    }

    // Node.js
    if (dir.exists("package.json")) {
        return SprintIDE::LanguageType::JavaScript;  // 或 TypeScript
    }

    // 无法识别，返回 Unknown
    return SprintIDE::LanguageType::Unknown;
}

// ========== Diff 辅助函数（用于历史差异对比） ==========

enum DiffType { CONTEXT, ADD, DELETE };

struct DiffLine {
    DiffType type;
    QString text;
};

static QList<DiffLine> computeDiff(const QStringList& oldLines, const QStringList& newLines)
{
    // 使用 LCS 算法计算差异
    int m = oldLines.size(), n = newLines.size();
    QVector<QVector<int>> dp(m + 1, QVector<int>(n + 1, 0));
    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            if (oldLines[i-1] == newLines[j-1])
                dp[i][j] = dp[i-1][j-1] + 1;
            else
                dp[i][j] = qMax(dp[i-1][j], dp[i][j-1]);
        }
    }
    QList<DiffLine> result;
    int i = m, j = n;
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && oldLines[i-1] == newLines[j-1]) {
            result.prepend({CONTEXT, oldLines[i-1]});
            --i; --j;
        } else if (j > 0 && (i == 0 || dp[i][j-1] >= dp[i-1][j])) {
            result.prepend({ADD, newLines[j-1]});
            --j;
        } else if (i > 0 && (j == 0 || dp[i][j-1] < dp[i-1][j])) {
            result.prepend({DELETE, oldLines[i-1]});
            --i;
        }
    }
    return result;
}

static QString diffToHtml(const QList<DiffLine>& diff)
{
    QString html;
    html += "<pre style='font-family: monospace; font-size: 11pt; line-height: 1.3; color: #e0e0e0; background-color: #1e1e1e; padding: 8px;'>";
    for (const auto& line : diff) {
        QString escaped = line.text.toHtmlEscaped();
        if (line.type == ADD)
            html += "<span style='background-color: #1a4d1a; color: #d0f0d0;'>+ " + escaped + "</span>\n";
        else if (line.type == DELETE)
            html += "<span style='background-color: #4d1a1a; color: #f0d0d0;'>- " + escaped + "</span>\n";
        else
            html += "  " + escaped + "\n";
    }
    html += "</pre>";
    return html;
}

SprintIDE::ErrorMarker parseErrorLine(const QString& line)
{
    SprintIDE::ErrorMarker marker;
    marker.line = 0;
    marker.column = -1;

    // 支持中文和英文的错误/警告标识
    static QRegularExpression regex(
        "^(.*?):(\\d+):(\\d+):\\s*(错误|警告|error|warning|fatal error|note):\\s*(.*)$",
        QRegularExpression::CaseInsensitiveOption
        );

    QRegularExpressionMatch match = regex.match(line);
    if (match.hasMatch()) {
        marker.filePath = match.captured(1).trimmed();
        marker.line = match.captured(2).toInt();
        marker.column = match.captured(3).toInt();
        marker.severity = match.captured(4).trimmed();
        marker.message = match.captured(5).trimmed();
    } else {
        // qDebug() << "parseErrorLine: 无法解析行:" << line;
    }

    return marker;
}

// ==================== 构造函数和析构函数 ====================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    d(new MainWindowPrivate(this))
{
    setWindowIcon(QIcon(":/icons/icons/app_icon.svg"));
    setWindowTitle("SprintIDE");
    resize(1600, 1000);

    // 初始化编译管理器
    m_compileManager = &CompileManager::instance();
    m_isCompiling = false;
    connectCompileManagerSignals();

    // 设置深色主题
    ThemeManager& tm = ThemeManager::instance();
    tm.setTheme(ThemeManager::ThemeType::Dark);

    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupDockWidgets();
    setupConnections();
    setupDebugPanel();
    setupTerminalPanel();

    connect(&tm, &ThemeManager::themeChanged,
            this, &MainWindow::onThemeChanged);
    applyCurrentTheme();

    // 初始化 LSP 管理器
    m_lspManager = new LSPManager(this);
    m_lspManager->initialize();

    connect(m_lspManager, &LSPManager::clientCreated,
            this, [this](const QString &languageId, const QString &projectRoot) {
                Q_UNUSED(languageId);
                Q_UNUSED(projectRoot);
            });
    connect(m_lspManager, &LSPManager::clientError,
            this, [this](const QString &languageId, const QString &error) {
                d->outputPanel->append(QString("LSP (%1) 错误: %2")
                                           .arg(languageId).arg(error));
            });

    connect(d->projectPanel, &ProjectPanel::fileOpenRequested, this, &MainWindow::openFile);

    // 初始化调试管理器
    m_debugManager = &DebugManager::instance();
    m_debugManager->initialize();

    // 连接调试管理器信号
    connect(m_debugManager, &DebugManager::stateChanged,
            this, &MainWindow::onDebugStateChanged);
    // ... 其余调试信号连接（保持不变） ...

    // 初始化 AI 模块
    if (initializeAIIntegration(this, m_aiServer, m_aiExecutor, m_aiChatPanel)) {
        setupAIMenu(this, m_aiChatPanel);
        restoreChatPanelState(m_aiChatPanel);
        if (m_aiChatPanel) {
            m_aiChatPanel->setVisible(true);
            m_aiChatPanel->raise();
        }
        qDebug() << "AI 模块已集成到 MainWindow";
    } else {
        qDebug() << "AI 模块初始化失败（可能是端口被占用）";
    }

    // 初始化本地历史
    m_localHistory = new LocalHistory(this);
    if (!d->currentProjectPath.isEmpty()) {
        m_localHistory->setProjectRoot(d->currentProjectPath);
    }

    // 连接项目面板的历史请求信号

    if (d->projectPanel) {
        connect(d->projectPanel, &ProjectPanel::requestFileHistory,
                this, &MainWindow::showFileHistoryDialog);
    }
}

MainWindow::~MainWindow()
{
    qDebug() << "[MainWindow] 析构开始";

    // ========== 1. 断开可能产生回调的信号 ==========
    if (m_compileManager) {
        disconnect(m_compileManager, nullptr, this, nullptr);
    }
    if (m_lspManager) {
        disconnect(m_lspManager, nullptr, this, nullptr);
    }
    if (m_pythonDebugger) {
        disconnect(m_pythonDebugger, nullptr, this, nullptr);
    }

    // ========== 2. Python 调试器停止（不 delete，交给 Qt parent）==========
    if (m_pythonDebugger) {
        m_pythonDebugger->stopDebugging();
    }

    // ========== 2.5 C++ 调试器停止 ==========
    if (m_cppDebugger) {
        m_cppDebugger->stopDebugging();
        delete m_cppDebugger;
        m_cppDebugger = nullptr;
    }

    // ========== 3. 显式关闭 LSP（它有 parent = this，会随后被 Qt 删除）==========
    if (m_lspManager) {
        m_lspManager->shutdownAll();
    }

    // ========== 4. ★ 不要在 MainWindow 析构里 shutdown DebugManager ★ ==========
    // 因为此时可能已经晚于 QApplication 的清理时机。
    // 全部移到 main.cpp 里做。

    // ========== 5. 清理编辑器标签 ==========
    if (d && d->editorTabs) {
        while (d->editorTabs->count() > 0) {
            QWidget* w = d->editorTabs->widget(0);
            d->editorTabs->removeTab(0);
            if (w) {
                delete w;
            }
        }
    }

    // ========== 6. 断开 definition 连接 ==========
    for (auto it = m_definitionConnections.begin(); it != m_definitionConnections.end(); ++it) {
        disconnect(it.value());
    }
    m_definitionConnections.clear();

    for (auto it = m_pendingDefinitionConnections.begin(); it != m_pendingDefinitionConnections.end(); ++it) {
        disconnect(it.value());
    }
    m_pendingDefinitionConnections.clear();

    // ========== 7. 删除私有实现 ==========
    delete d;
    d = nullptr;

    qDebug() << "[MainWindow] 析构完成";
}

// ==================== 主题相关方法 ====================

void MainWindow::onThemeChanged(ThemeManager::ThemeType theme)
{
    Q_UNUSED(theme);
    applyCurrentTheme();
}

void MainWindow::applyCurrentTheme()
{
    ThemeManager& tm = ThemeManager::instance();
    bool isDark = tm.isDarkTheme();

    // qDebug() << "开始应用主题: " << (isDark ? "深色" : "浅色");

    if (isDark) {
        QColor mainWindowBackground("#2D2D2D");
        QColor editorBackground("#1E1E1E");
        QColor textColor("#E0E0E0");
        QColor selectionBackground("#3A3D41");
        QColor alternateBase("#3C3C3C");
        QColor menuBackground("#3C3C3C");
        QColor buttonBackground("#4A4A4A");

        QPalette windowPalette = palette();
        windowPalette.setColor(QPalette::Window, mainWindowBackground);
        windowPalette.setColor(QPalette::WindowText, textColor);
        setPalette(windowPalette);

        if (menuBar()) {
            QPalette menuPalette = menuBar()->palette();
            menuPalette.setColor(QPalette::Button, menuBackground);
            menuPalette.setColor(QPalette::ButtonText, textColor);
            menuBar()->setPalette(menuPalette);
        }

        for (QToolBar* toolBar : findChildren<QToolBar*>()) {
            QPalette toolPalette = toolBar->palette();
            toolPalette.setColor(QPalette::Button, buttonBackground);
            toolPalette.setColor(QPalette::ButtonText, textColor);
            toolBar->setPalette(toolPalette);
        }

        if (statusBar()) {
            QPalette statusPalette = statusBar()->palette();
            statusPalette.setColor(QPalette::Window, mainWindowBackground);
            statusPalette.setColor(QPalette::WindowText, textColor);
            statusBar()->setPalette(statusPalette);
        }

        if (d->projectPanel) {
            QPalette treePalette = d->projectPanel->palette();
            treePalette.setColor(QPalette::Base, mainWindowBackground);
            treePalette.setColor(QPalette::Text, textColor);
            treePalette.setColor(QPalette::Highlight, selectionBackground);
            treePalette.setColor(QPalette::HighlightedText, textColor);
            d->projectPanel->setPalette(treePalette);
        }

        if (d->outputPanel) {
            QPalette outputPalette = d->outputPanel->palette();
            outputPalette.setColor(QPalette::Base, editorBackground);
            outputPalette.setColor(QPalette::Text, QColor("#CCCCCC"));
            d->outputPanel->setPalette(outputPalette);
        }
    } else {
        QColor mainWindowBackground("#F8F9FA");
        QColor editorBackground("#FFFFFF");
        QColor textColor("#333333");
        QColor selectionBackground("#D3E9F7");
        QColor alternateBase("#E9ECEF");
        QColor menuBackground("#FFFFFF");
        QColor buttonBackground("#E9ECEF");

        QPalette windowPalette = palette();
        windowPalette.setColor(QPalette::Window, mainWindowBackground);
        windowPalette.setColor(QPalette::WindowText, textColor);
        setPalette(windowPalette);

        if (menuBar()) {
            QPalette menuPalette = menuBar()->palette();
            menuPalette.setColor(QPalette::Button, menuBackground);
            menuPalette.setColor(QPalette::ButtonText, textColor);
            menuBar()->setPalette(menuPalette);
        }

        for (QToolBar* toolBar : findChildren<QToolBar*>()) {
            QPalette toolPalette = toolBar->palette();
            toolPalette.setColor(QPalette::Button, buttonBackground);
            toolPalette.setColor(QPalette::ButtonText, textColor);
            toolBar->setPalette(toolPalette);
        }

        if (statusBar()) {
            QPalette statusPalette = statusBar()->palette();
            statusPalette.setColor(QPalette::Window, mainWindowBackground);
            statusPalette.setColor(QPalette::WindowText, textColor);
            statusBar()->setPalette(statusPalette);
        }

        if (d->projectPanel) {
            QPalette treePalette = d->projectPanel->palette();
            treePalette.setColor(QPalette::Base, mainWindowBackground);
            treePalette.setColor(QPalette::Text, textColor);
            treePalette.setColor(QPalette::Highlight, selectionBackground);
            treePalette.setColor(QPalette::HighlightedText, textColor);
            d->projectPanel->setPalette(treePalette);
        }

        if (d->outputPanel) {
            QPalette outputPalette = d->outputPanel->palette();
            outputPalette.setColor(QPalette::Base, editorBackground);
            outputPalette.setColor(QPalette::Text, textColor);
            d->outputPanel->setPalette(outputPalette);
        }
    }

    applyThemeToTextEditors(isDark);
    // updateWindowTitle();
    update();
    repaint();

    // qDebug() << "主题应用完成";
}

void MainWindow::applyThemeToDockWidget(QDockWidget* dockWidget, bool isDark)
{
    if (!dockWidget) return;

    QString titleStyle;
    if (isDark) {
        titleStyle = QString(
            "QDockWidget {"
            "  background-color: #2D2D2D;"
            "  color: #E0E0E0;"
            "  border: 1px solid #555555;"
            "}"
            "QDockWidget::title {"
            "  background-color: #3C3C3C;"
            "  color: #E0E0E0;"
            "  padding: 4px;"
            "  text-align: left;"
            "}"
            "QDockWidget::close-button, QDockWidget::float-button {"
            "  background-color: #4A4A4A;"
            "  border: 1px solid #555555;"
            "}"
            );
    } else {
        titleStyle = QString(
            "QDockWidget {"
            "  background-color: #F8F9FA;"
            "  color: #333333;"
            "  border: 1px solid #CCCCCC;"
            "}"
            "QDockWidget::title {"
            "  background-color: #E9ECEF;"
            "  color: #333333;"
            "  padding: 4px;"
            "  text-align: left;"
            "}"
            "QDockWidget::close-button, QDockWidget::float-button {"
            "  background-color: #FFFFFF;"
            "  border: 1px solid #CCCCCC;"
            "}"
            );
    }

    dockWidget->setStyleSheet(titleStyle);
}

void MainWindow::applyThemeToProjectTree(bool isDark)
{
    if (!d->projectPanel) return;

    ThemeManager& tm = ThemeManager::instance();
    tm.applyThemeToTreeView(d->projectPanel->getFileTreeView());

    if (d->projectDock) {
        QString styleSheet = QString(
                                 "QDockWidget {"
                                 "  background-color: %1;"
                                 "  color: %2;"
                                 "  border: 1px solid %3;"
                                 "}"
                                 "QDockWidget::title {"
                                 "  background-color: %4;"
                                 "  color: %5;"
                                 "  padding: 4px;"
                                 "}"
                                 ).arg(
                                     tm.color(ThemeManager::UIElement::MainWindowBackground).name(),
                                     tm.color(ThemeManager::UIElement::TextColor).name(),
                                     tm.color(ThemeManager::UIElement::MainWindowBackground).darker(120).name(),
                                     tm.color(ThemeManager::UIElement::ButtonBackground).name(),
                                     tm.color(ThemeManager::UIElement::TextColor).name()
                                     );
        d->projectDock->setStyleSheet(styleSheet);
    }

    d->projectPanel->setStyle(QApplication::style());
}

void MainWindow::applyThemeToAll()
{
    ThemeManager& tm = ThemeManager::instance();
    applyThemeToWidgetRecursive(this, tm.isDarkTheme());
    applyThemeToTextEditors(tm.isDarkTheme());
    applyThemeToTabWidget(tm.isDarkTheme());
}

void MainWindow::applyThemeToWidgetRecursive(QWidget* widget, bool isDark)
{
    if (!widget) return;

    if (QTextEdit* textEdit = qobject_cast<QTextEdit*>(widget)) {
        ThemeManager::instance().applyThemeToTextEdit(textEdit);
    } else if (QPlainTextEdit* plainTextEdit = qobject_cast<QPlainTextEdit*>(widget)) {
        ThemeManager::instance().applyThemeToPlainTextEdit(plainTextEdit);
    } else if (QTreeView* treeView = qobject_cast<QTreeView*>(widget)) {
        ThemeManager::instance().applyThemeToTreeView(treeView);
    } else {
        ThemeManager::instance().applyThemeToWidget(widget);
    }

    for (QObject* child : widget->children()) {
        if (QWidget* childWidget = qobject_cast<QWidget*>(child)) {
            applyThemeToWidgetRecursive(childWidget, isDark);
        }
    }
}

void MainWindow::applyThemeToTextEditors(bool isDark)
{
    for (int i = 0; i < d->editorTabs->count(); ++i) {
        TextEditor* editor = qobject_cast<TextEditor*>(d->editorTabs->widget(i));
        if (editor) {
            editor->applyTheme(isDark ? "dark" : "light");
        }
    }
}

void MainWindow::applyThemeToTabWidget(bool isDark)
{
    if (TabWidget* tabWidget = qobject_cast<TabWidget*>(d->editorTabs)) {
        tabWidget->updateTheme();
    }
}

// ==================== UI设置方法 ====================

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    d->editorTabs = new TabWidget(centralWidget);
    d->editorTabs->setTabsClosable(true);
    d->editorTabs->setMovable(true);

    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(d->editorTabs);
}

void MainWindow::setupMenuBar()
{

    QMenuBar *menuBar = this->menuBar();

    // ==================== 文件菜单 ====================
    QMenu *fileMenu = menuBar->addMenu("文件(&F)");

    QMenu *newMenu = fileMenu->addMenu(QIcon(":/icons/icons/new_file.svg"), "新建(&N)");

    // 通用新建文件（弹出对话框）
    QAction *newFileAction = newMenu->addAction(QIcon(":/icons/icons/text.svg"), "文件...");
    newFileAction->setShortcut(QKeySequence::New);
    connect(newFileAction, &QAction::triggered, this, &MainWindow::onNewFile);

    newMenu->addSeparator();

    // 快速创建 C++ 文件
    QAction *newCppAction = newMenu->addAction(QIcon(":/icons/icons/cpp.svg"), "C++ 源文件");
    connect(newCppAction, &QAction::triggered, this, [this]() {
        createNewFileWithName("main.cpp", "新建 C++ 文件", QString());
    });

    // 快速创建 C++ 头文件
    QAction *newHppAction = newMenu->addAction(QIcon(":/icons/icons/header.svg"), "C++ 头文件");
    connect(newHppAction, &QAction::triggered, this, [this]() {
        createNewFileWithName("main.h", "新建 C++ 头文件", QString());
    });

    newMenu->addSeparator();

    // 快速创建 Python 文件
    QAction *newPythonAction = newMenu->addAction("Python 文件");
    connect(newPythonAction, &QAction::triggered, this, [this]() {
        createNewFileWithName("main.py", "新建 Python 文件", QString());
    });

    // 快速创建 JavaScript 文件
    QAction *newJsAction = newMenu->addAction("JavaScript 文件");
    connect(newJsAction, &QAction::triggered, this, [this]() {
        createNewFileWithName("main.js", "新建 JavaScript 文件", QString());
    });

    // 快速创建 HTML 文件
    QAction *newHtmlAction = newMenu->addAction("HTML 文件");
    connect(newHtmlAction, &QAction::triggered, this, [this]() {
        createNewFileWithName("index.html", "新建 HTML 文件", QString());
    });

    // 快速创建纯文本文件
    QAction *newTxtAction = newMenu->addAction("纯文本文件");
    connect(newTxtAction, &QAction::triggered, this, [this]() {
        createNewFileWithName("untitled.txt", "新建文本文件", QString());
    });

    QAction *newProjectAction = newMenu->addAction(QIcon(":/icons/icons/new_project.svg"), "项目");
    connect(newProjectAction, &QAction::triggered, this, &MainWindow::onNewProject);

    QMenu *openMenu = fileMenu->addMenu(QIcon(":/icons/icons/open_file.svg"), "打开(&O)");

    QAction *openFileAction = openMenu->addAction(QIcon(":/icons/icons/open_file.svg"), "文件");
    openFileAction->setShortcut(QKeySequence::Open);
    connect(openFileAction, &QAction::triggered, this, &MainWindow::onOpenFile);

    QAction *openProjectAction = openMenu->addAction(QIcon(":/icons/icons/open_project.svg"), "项目");
    openProjectAction->setShortcut(QKeySequence("Ctrl+Shift+O"));
    connect(openProjectAction, &QAction::triggered, this, &MainWindow::onOpenProject);

    fileMenu->addSeparator();

    QAction *saveFileAction = fileMenu->addAction(QIcon(":/icons/icons/save_file.svg"), "保存(&S)");
    saveFileAction->setShortcut(QKeySequence::Save);
    connect(saveFileAction, &QAction::triggered, this, &MainWindow::onSaveFile);

    QAction *saveAllAction = fileMenu->addAction(QIcon(":/icons/icons/save_all.svg"), "保存全部(&A)");
    saveAllAction->setShortcut(QKeySequence("Ctrl+Shift+S"));
    connect(saveAllAction, &QAction::triggered, this, [this]() {
        for (int i = 0; i < d->editorTabs->count(); ++i) {
            TextEditor* editor = qobject_cast<TextEditor*>(d->editorTabs->widget(i));
            if (editor && editor->isModified()) {
                editor->saveFile();
            }
        }
        d->outputPanel->append("已保存所有文件");
    });

    QAction *saveAsAction = fileMenu->addAction("另存为(&A)");
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::onSaveAs);

    fileMenu->addSeparator();

    QAction *closeFileAction = fileMenu->addAction(QIcon(":/icons/icons/close_tab.svg"), "关闭(&C)");
    closeFileAction->setShortcut(QKeySequence::Close);
    connect(closeFileAction, &QAction::triggered, this, &MainWindow::onCloseFile);

    QAction *exitAction = fileMenu->addAction("退出(&X)");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &MainWindow::onExit);

    // ==================== 编辑菜单 ====================
    QMenu *editMenu = menuBar->addMenu("编辑(&E)");

    QAction *undoAction = editMenu->addAction(QIcon(":/icons/icons/undo.svg"), "撤销(&U)");
    undoAction->setShortcut(QKeySequence::Undo);
    connect(undoAction, &QAction::triggered, this, &MainWindow::onUndo);

    QAction *redoAction = editMenu->addAction(QIcon(":/icons/icons/redo.svg"), "重做(&R)");
    redoAction->setShortcut(QKeySequence::Redo);
    connect(redoAction, &QAction::triggered, this, &MainWindow::onRedo);

    editMenu->addSeparator();

    QAction *cutAction = editMenu->addAction(QIcon(":/icons/icons/cut.svg"), "剪切(&T)");
    cutAction->setShortcut(QKeySequence::Cut);
    connect(cutAction, &QAction::triggered, this, &MainWindow::onCut);

    QAction *copyAction = editMenu->addAction(QIcon(":/icons/icons/copy.svg"), "复制(&C)");
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, this, &MainWindow::onCopy);

    QAction *pasteAction = editMenu->addAction(QIcon(":/icons/icons/paste.svg"), "粘贴(&P)");
    pasteAction->setShortcut(QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, this, &MainWindow::onPaste);

    editMenu->addSeparator();

    QAction *copyLineAction = editMenu->addAction("复制行(&D)");
    copyLineAction->setShortcut(QKeySequence("Ctrl+D"));
    connect(copyLineAction, &QAction::triggered, this, [this]() {
        TextEditor* editor = currentEditor();
        if (editor) {
            editor->duplicateLine();
        }
    });

    QAction *deleteLineAction = editMenu->addAction("删除行(&Y)");
    deleteLineAction->setShortcut(QKeySequence("Ctrl+Y"));
    connect(deleteLineAction, &QAction::triggered, this, [this]() {
        TextEditor* editor = currentEditor();
        if (editor) {
            editor->deleteLine();
        }
    });

    // ========== Python 菜单 ==========
    QMenu* pythonMenu = menuBar->addMenu("Python(&P)");

    QAction* installDepsAction = pythonMenu->addAction("安装依赖");
    connect(installDepsAction, &QAction::triggered, this, [this]() {
        auto model = ProjectManager::instance()->pythonProjectModel();
        if (!model) {
            QMessageBox::information(this, "提示", "未打开 Python 项目");
            return;
        }
        QString python = model->interpreterPath();
        if (python.isEmpty()) python = "python3";
        d->outputPanel->append("正在安装依赖...");
        PythonDependencyManager::installDependencies(
            model->projectPath(), python,
            [this](const QString& out) { d->outputPanel->append(out); }
            );
        d->outputPanel->append("依赖安装完成");
    });

    QAction* createVenvAction = pythonMenu->addAction("创建虚拟环境");
    connect(createVenvAction, &QAction::triggered, this, [this]() {
        auto model = ProjectManager::instance()->pythonProjectModel();
        if (!model) {
            QMessageBox::information(this, "提示", "未打开 Python 项目");
            return;
        }
        if (PythonVirtualEnv::createVenv(model->projectPath())) {
            d->outputPanel->append("虚拟环境创建成功");
            model->load(); // 重新加载
        } else {
            d->outputPanel->append("虚拟环境创建失败");
        }
    });

    QAction* formatPythonAction = pythonMenu->addAction("格式化代码");
    connect(formatPythonAction, &QAction::triggered, this, [this]() {
        TextEditor* editor = currentEditor();
        if (!editor) return;
        QString filePath = editor->filePath();
        if (!filePath.endsWith(".py")) {
            QMessageBox::information(this, "提示", "当前文件不是 Python 文件");
            return;
        }
        QString error;
        if (PythonFormatter::formatFile(filePath, error)) {
            editor->reloadFile();
            d->outputPanel->append("代码格式化完成");
        } else {
            d->outputPanel->append("格式化失败: " + error);
        }
    });

    // 设置动作（原编译配置）
    editMenu->addSeparator();

    QAction *settingsAction = editMenu->addAction(QIcon(":/icons/icons/setting.svg"), "设置...");
    settingsAction->setShortcut(QKeySequence("Ctrl+,"));
    settingsAction->setMenuRole(QAction::PreferencesRole);  // 关键：让其在 macOS 下进入应用程序菜单
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onCompileSettings);


    QAction *findAction = editMenu->addAction("查找(&F)");
    findAction->setShortcut(QKeySequence::Find);
    connect(findAction, &QAction::triggered, this, &MainWindow::onFind);

    QAction *replaceAction = editMenu->addAction("替换(&R)");
    replaceAction->setShortcut(QKeySequence("Ctrl+Shift+H"));
    connect(replaceAction, &QAction::triggered, this, &MainWindow::onReplace);

    QAction *formatCodeAction = editMenu->addAction(QIcon(":/icons/icons/format_code.svg"), "格式化代码(&O)");
    formatCodeAction->setShortcut(QKeySequence("Ctrl+Alt+L"));
    connect(formatCodeAction, &QAction::triggered, this, &MainWindow::onFormatCode);

    QAction *commentAction = editMenu->addAction(QIcon(":/icons/icons/comment_code.svg"), "注释(&C)");
    commentAction->setShortcut(QKeySequence("Ctrl+/"));
    connect(commentAction, &QAction::triggered, this, &MainWindow::onComment);

    // ==================== 构建菜单 ====================
    QMenu *buildMenu = menuBar->addMenu("构建(&B)");

    QAction *compileFileAction = buildMenu->addAction(QIcon(":/icons/icons/build.svg"), "编译当前文件(&C)");
    compileFileAction->setShortcut(QKeySequence("Ctrl+B"));
    connect(compileFileAction, &QAction::triggered, this, &MainWindow::onCompile);

    QAction *compileProjectAction = buildMenu->addAction(QIcon(":/icons/icons/build.svg"), "编译项目(&P)");
    compileProjectAction->setShortcut(QKeySequence("Ctrl+Shift+B"));
    connect(compileProjectAction, &QAction::triggered, this, &MainWindow::onBuildProject);

    QAction *cleanBuildAction = buildMenu->addAction("清理构建(&O)");
    connect(cleanBuildAction, &QAction::triggered, this, &MainWindow::onCleanBuild);

    // QAction *compileSettingsAction = buildMenu->addAction(QIcon(":/icons/icons/setting.svg"), "编译配置(&C)");
    // connect(compileSettingsAction, &QAction::triggered, this, &MainWindow::onCompileSettings);

    // ==================== 调试菜单 ====================
    QMenu *debugMenu = menuBar->addMenu("调试(&D)");

    m_startDebugAction = debugMenu->addAction(QIcon(":/icons/icons/debug.svg"), "启动调试(&S)");
    m_startDebugAction->setShortcut(QKeySequence("Ctrl+Shift+D"));
    connect(m_startDebugAction, &QAction::triggered, this, &MainWindow::onStartDebug);

    m_stopDebugAction = debugMenu->addAction(QIcon(":/icons/icons/stop.svg"),"停止调试(&T)");
    m_stopDebugAction->setShortcut(QKeySequence("Ctrl+."));
    m_stopDebugAction->setEnabled(false);
    connect(m_stopDebugAction, &QAction::triggered, this, &MainWindow::onStopDebug);

    debugMenu->addSeparator();

    m_stepOverAction = debugMenu->addAction(QIcon(":/icons/icons/stop_over.svg"), "步过(&N)");
    m_stepOverAction->setShortcut(QKeySequence("F10"));
    m_stepOverAction->setEnabled(false);
    connect(m_stepOverAction, &QAction::triggered, this, &MainWindow::onStepOver);

    m_stepIntoAction = debugMenu->addAction(QIcon(":/icons/icons/step_into.svg"), "步入(&S)");
    m_stepIntoAction->setShortcut(QKeySequence("F11"));
    m_stepIntoAction->setEnabled(false);
    connect(m_stepIntoAction, &QAction::triggered, this, &MainWindow::onStepInto);

    m_stepOutAction = debugMenu->addAction(QIcon(":/icons/icons/step_out.svg"), "步出(&O)");
    m_stepOutAction->setShortcut(QKeySequence("Shift+F11"));
    m_stepOutAction->setEnabled(false);
    connect(m_stepOutAction, &QAction::triggered, this, &MainWindow::onStepOut);

    debugMenu->addSeparator();

    m_continueAction = debugMenu->addAction(QIcon(":/icons/icons/run.svg"), "继续(&C)");
    m_continueAction->setShortcut(QKeySequence("F5"));
    m_continueAction->setEnabled(false);
    connect(m_continueAction, &QAction::triggered, this, &MainWindow::onContinueDebug);

    debugMenu->addSeparator();

    m_addBreakpointAction = debugMenu->addAction(QIcon(":/icons/icons/toggle_break_point.svg"), "添加断点(&B)");
    m_addBreakpointAction->setShortcut(QKeySequence("Ctrl+F8"));
    connect(m_addBreakpointAction, &QAction::triggered, this, &MainWindow::onAddBreakpoint);

    // ==================== 工具菜单 ====================
    QMenu *toolsMenu = menuBar->addMenu("工具(&T)");

    QMenu *themeMenu = toolsMenu->addMenu(QIcon(":/icons/icons/setting.svg"), "主题设置(&H)");

    QAction *lightThemeAction = themeMenu->addAction("浅色主题(&L)");
    connect(lightThemeAction, &QAction::triggered, this, &MainWindow::onThemeLight);

    QAction *darkThemeAction = themeMenu->addAction("深色主题(&D)");
    connect(darkThemeAction, &QAction::triggered, this, &MainWindow::onThemeDark);

    QAction *pluginManagerAction = toolsMenu->addAction(QIcon(":/icons/icons/services.svg"), "插件管理(&P)");
    connect(pluginManagerAction, &QAction::triggered, this, [this]() {
        d->outputPanel->append("插件管理功能开发中...");
    });

    QAction *performanceMonitorAction = toolsMenu->addAction("性能监控(&M)");
    connect(performanceMonitorAction, &QAction::triggered, this, [this]() {
        d->outputPanel->append("性能监控功能开发中...");
    });

    QAction *terminalAction = toolsMenu->addAction(QIcon(":/icons/terminal.svg"), "终端(&T)");
    terminalAction->setShortcut(QKeySequence("Ctrl+`"));  // Cmd+`
    connect(terminalAction, &QAction::triggered, this, [this]() {
        if (m_terminalPanel) {
            m_terminalPanel->setVisible(!m_terminalPanel->isVisible());
        }
    });

    // ==================== 帮助菜单 ====================
    QMenu *helpMenu = menuBar->addMenu("帮助(&H)");

    QAction *quickStartAction = helpMenu->addAction(QIcon(":/icons/icons/run.svg"), "快速指南(&G)");
    connect(quickStartAction, &QAction::triggered, this, &MainWindow::onQuickStart);

    QAction *documentationAction = helpMenu->addAction(QIcon(":/icons/icons/text.svg"), "文档中心(&D)");
    connect(documentationAction, &QAction::triggered, this, &MainWindow::onDocumentation);

    QAction *aboutAction = helpMenu->addAction(QIcon(":/icons/icons/app_icon.svg"), "关于(&A)");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::setupToolBar()
{
    QToolBar *toolBar = addToolBar("主工具栏");
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(20, 20));

    QAction *newAction = toolBar->addAction(QIcon(":/icons/icons/new_file.svg"), "新建");
    connect(newAction, &QAction::triggered, this, &MainWindow::onNewFile);

    QAction *openAction = toolBar->addAction(QIcon(":/icons/icons/open_file.svg"), "打开");
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenFile);

    QAction *saveAction = toolBar->addAction(QIcon(":/icons/icons/save_file.svg"), "保存");
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveFile);

    toolBar->addSeparator();

    QAction *buildAction = toolBar->addAction(QIcon(":/icons/icons/build.svg"),"编译");
    connect(buildAction, &QAction::triggered, this, &MainWindow::onCompile);

    QAction *runAction = toolBar->addAction(QIcon(":/icons/icons/run.svg"),"运行");
    runAction->setShortcut(QKeySequence("Ctrl+R"));
    connect(runAction, &QAction::triggered, this, &MainWindow::onRun);

    QAction *debugAction = toolBar->addAction(QIcon(":/icons/icons/debug.svg"),"调试");
    connect(debugAction, &QAction::triggered, this, &MainWindow::onStartDebug);
}

void MainWindow::setupStatusBar()
{
    QStatusBar *statusBar = this->statusBar();

    // 主状态标签
    m_statusLabel = new QLabel("就绪", statusBar);
    statusBar->addWidget(m_statusLabel, 1);  // stretch = 1

    // 分隔线
    statusBar->addPermanentWidget(new QLabel(" | "));

    // 行号/列号
    m_cursorPositionLabel = new QLabel("行: 1  列: 1", statusBar);
    statusBar->addPermanentWidget(m_cursorPositionLabel);

    statusBar->addPermanentWidget(new QLabel(" | "));

    // 编码
    m_encodingLabel = new QLabel("UTF-8", statusBar);
    statusBar->addPermanentWidget(m_encodingLabel);

    statusBar->addPermanentWidget(new QLabel(" | "));

    // 语言
    m_languageLabel = new QLabel("C++", statusBar);
    statusBar->addPermanentWidget(m_languageLabel);

    statusBar->addPermanentWidget(new QLabel(" | "));

    // 缩进
    m_indentLabel = new QLabel("空格: 4", statusBar);
    statusBar->addPermanentWidget(m_indentLabel);

    // 编译进度条（默认隐藏）
    m_buildProgressBar = new QProgressBar(statusBar);
    m_buildProgressBar->setMaximumSize(150, 20);
    m_buildProgressBar->setMinimum(0);
    m_buildProgressBar->setMaximum(100);
    m_buildProgressBar->setValue(0);
    m_buildProgressBar->setVisible(false);
    statusBar->addPermanentWidget(m_buildProgressBar);

    // 更新光标位置（初始）
    onTabCurrentChanged(0);
}

void MainWindow::setupDockWidgets()
{
    // ========== 项目面板（左侧） ==========
    d->projectDock = new QDockWidget("项目面板", this);

    // 创建 ProjectPanel 实例
    d->projectPanel = new ProjectPanel(d->projectDock);
    d->projectDock->setWidget(d->projectPanel);
    addDockWidget(Qt::LeftDockWidgetArea, d->projectDock);
    d->projectPanel->setShowHiddenFiles(false);

    QColor mainWindowBackground("#2D2D2D");
    QColor editorBackground("#1E1E1E");
    QColor textColor("#E0E0E0");
    QColor selectionBackground("#3A3D41");
    QColor alternateBase("#3C3C3C");

    QPalette treePalette = d->projectPanel->palette();
    treePalette.setColor(QPalette::Base, mainWindowBackground);
    treePalette.setColor(QPalette::Text, textColor);
    treePalette.setColor(QPalette::Highlight, selectionBackground);
    treePalette.setColor(QPalette::HighlightedText, textColor);
    treePalette.setColor(QPalette::Window, mainWindowBackground);
    treePalette.setColor(QPalette::WindowText, textColor);
    treePalette.setColor(QPalette::Button, mainWindowBackground.darker(110));
    treePalette.setColor(QPalette::ButtonText, textColor);
    treePalette.setColor(QPalette::AlternateBase, alternateBase);

    d->projectPanel->setPalette(treePalette);
    d->projectPanel->setAutoFillBackground(true);

    QString treeStyleSheet = QString(
                                 "QTreeView {"
                                 "  background-color: %1;"
                                 "  color: %2;"
                                 "}"
                                 "QTreeView::item:selected {"
                                 "  background-color: %4;"
                                 "  color: %5;"
                                 "}"
                                 "QTreeView::item:hover {"
                                 "  background-color: %6;"
                                 "}"
                                 ).arg(
                                     mainWindowBackground.name(),
                                     textColor.name(),
                                     alternateBase.name(),
                                     selectionBackground.name(),
                                     textColor.name(),
                                     selectionBackground.lighter(120).name()
                                     );

    d->projectPanel->setStyleSheet(treeStyleSheet);
    d->projectPanel->setAlternatingRowColors(false);

    QString dockStyle = QString(
                            "QDockWidget {"
                            "  background-color: %1;"
                            "  color: %2;"
                            "  border: 1px solid #555555;"
                            "}"
                            "QDockWidget::title {"
                            "  background-color: %3;"
                            "  color: %2;"
                            "  padding: 4px;"
                            "  text-align: left;"
                            "}"
                            "QDockWidget::close-button, QDockWidget::float-button {"
                            "  background-color: %4;"
                            "  border: 1px solid #555555;"
                            "}"
                            ).arg(
                                mainWindowBackground.name(),
                                textColor.name(),
                                alternateBase.name(),
                                mainWindowBackground.lighter(110).name()
                                );

    d->projectDock->setStyleSheet(dockStyle);

    QStandardItemModel *model = new QStandardItemModel(this);
    QStandardItem *rootItem = new QStandardItem("项目");
    rootItem->setForeground(QBrush(textColor));
    model->appendRow(rootItem);
    // d->projectPanel->setModel(model);

    d->projectDock->setWidget(d->projectPanel);
    addDockWidget(Qt::LeftDockWidgetArea, d->projectDock);

    // ========== 底部面板 ==========
    m_bottomDockWidget = new QDockWidget("面板", this);
    m_bottomDockWidget->setObjectName("BottomPanel");
    m_bottomDockWidget->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    m_bottomDockWidget->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    // 确保没有最大尺寸限制
    // m_bottomDockWidget->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    // m_bottomDockWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_bottomTabWidget = new QTabWidget(m_bottomDockWidget);
    m_bottomTabWidget->setTabPosition(QTabWidget::South);
    m_bottomTabWidget->setDocumentMode(true);
    m_bottomTabWidget->setTabsClosable(false);
    // 确保 TabWidget 填满 DockWidget
    m_bottomTabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // ========== 输出面板（第一个标签） ==========
    d->outputPanel = new QTextEdit(this);
    d->outputPanel->setReadOnly(true);
    d->outputPanel->setFont(QFont("Consolas", 9));
    // ========== 确保支持键盘选择（从而支持 Ctrl+C 复制） ==========
    d->outputPanel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    // 确保可以接收焦点
    d->outputPanel->setFocusPolicy(Qt::StrongFocus);
    d->outputPanel->setStyleSheet(
        "QTextEdit {"
        "  background-color: #1E1E1E;"
        "  color: #CCCCCC;"
        "  border: none;"
        "  font-family: 'Monaco', 'Consolas', monospace;"
        "  font-size: 10pt;"
        "}"
        );
    // ========== 使用 QShortcut 绑定复制快捷键 ==========
    QShortcut* copyShortcut = new QShortcut(QKeySequence::Copy, this);
    connect(copyShortcut, &QShortcut::activated, this, [this]() {
        if (d->outputPanel && d->outputPanel->textCursor().hasSelection()) {
            d->outputPanel->copy();
        }
    });

    QPalette outputPalette = d->outputPanel->palette();
    outputPalette.setColor(QPalette::Base, QColor("#1E1E1E"));
    outputPalette.setColor(QPalette::Text, QColor("#CCCCCC"));
    d->outputPanel->setPalette(outputPalette);

    d->outputPanel->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(d->outputPanel, &QTextEdit::customContextMenuRequested,
            this, [this](const QPoint& pos) {
                QMenu* menu = new QMenu(this);

                // 复制（如果选中了文本才启用）
                QAction* copyAction = new QAction("复制", this);
                copyAction->setShortcut(QKeySequence::Copy);
                copyAction->setEnabled(d->outputPanel->textCursor().hasSelection());
                connect(copyAction, &QAction::triggered, d->outputPanel, &QTextEdit::copy);
                menu->addAction(copyAction);

                menu->addSeparator();

                // 清空
                QAction* clearAction = new QAction("清空", this);
                connect(clearAction, &QAction::triggered, d->outputPanel, &QTextEdit::clear);
                menu->addAction(clearAction);

                menu->addSeparator();

                // 全选
                QAction* selectAllAction = new QAction("全选", this);
                selectAllAction->setShortcut(QKeySequence::SelectAll);
                connect(selectAllAction, &QAction::triggered, d->outputPanel, &QTextEdit::selectAll);
                menu->addAction(selectAllAction);

                menu->exec(d->outputPanel->viewport()->mapToGlobal(pos));
                delete menu;
            });

    m_bottomTabWidget->addTab(d->outputPanel, QIcon(":/icons/run_view.svg"), "输出");

    m_bottomDockWidget->setWidget(m_bottomTabWidget);
    addDockWidget(Qt::BottomDockWidgetArea, m_bottomDockWidget);

    // 设置初始高度
    m_bottomDockWidget->resize(width(), 200);
    m_bottomDockWidget->setVisible(true);

    // 确保不浮动
    m_bottomDockWidget->setFloating(false);
    // 重置几何位置
    m_bottomDockWidget->setGeometry(0, height() - 200, width(), 200);
    // 确保停靠
    m_bottomDockWidget->show();

    // 连接项目面板的删除请求
    connect(d->projectPanel, &ProjectPanel::deleteRequested,
            this, [this](const QStringList& filePaths) {
                // 第一步：尝试关闭所有打开的文件
                bool canDelete = true;
                for (const QString& path : filePaths) {
                    if (!closeEditorForFile(path)) {
                        canDelete = false; // 用户取消关闭，中止删除
                        break;
                    }
                }

                if (!canDelete) {
                    d->outputPanel->append("删除操作已取消");
                    return;
                }

                // 第二步：执行实际删除
                for (const QString& path : filePaths) {
                    // deleteFile 内部仍有确认对话框，保留双重确认
                    d->projectPanel->deleteFile(path, true);
                    updateWindowTitle();
                }
            });
}

void MainWindow::setupConnections()
{
    connect(d->editorTabs, &TabWidget::tabCloseRequested,
            this, &MainWindow::onTabCloseRequested);
    connect(d->editorTabs, &TabWidget::currentChanged,
            this, &MainWindow::onTabCurrentChanged);
    connect(d->projectPanel, &ProjectPanel::runTestRequested,
            this, [this](const QString& filePath, const QString& testFullName) {
                // 执行测试
                PythonTestInfo test;
                test.filePath = filePath;
                test.fullName = testFullName;
                // 解析 fullName 获取 className, methodName（可选）
                // 调用 PythonTestRunner::runTest
                PythonTestRunner::runTest(d->projectPanel->currentProjectPath(), test,
                                          [this](const QString& output) {
                                              d->outputPanel->append(output);
                                          });
            });
    // ========== 连接测试运行请求 ==========
    connect(d->projectPanel, &ProjectPanel::runTestRequested,
            this, [this](const QString& filePath, const QString& testFullName) {
                if (filePath.isEmpty() || testFullName.isEmpty()) {
                    d->outputPanel->append("测试信息不完整");
                    return;
                }

                QString projectPath = d->projectPanel->currentProjectPath();
                if (projectPath.isEmpty()) {
                    d->outputPanel->append("未打开项目，无法运行测试");
                    return;
                }

                // 构建测试信息
                PythonTestInfo test;
                test.filePath = filePath;
                test.fullName = testFullName;

                // 尝试解析类名和方法名（从 fullName 中提取）
                // fullName 格式: "test_file.py::TestClass::test_method"
                QStringList parts = testFullName.split("::");
                if (parts.size() >= 2) {
                    test.className = parts[parts.size() - 2];
                    test.methodName = parts.last();
                } else {
                    test.methodName = testFullName;
                }

                d->outputPanel->append(QString("▶ 运行测试: %1").arg(testFullName));

                // 执行测试（异步，输出实时显示）
                PythonTestRunner::runTest(projectPath, test,
                                          [this](const QString& output) {
                                              // 将输出显示到输出面板
                                              if (!output.isEmpty()) {
                                                  d->outputPanel->append(output);
                                              }
                                          });
            });
}

// ========== 调试面板设置 ==========

void MainWindow::setupDebugPanel()
{
    m_debugPanel = new DebugPanel(this);
    int index = m_bottomTabWidget->addTab(m_debugPanel, QIcon(":/icons/debug.svg"), "调试");
    m_bottomTabWidget->setTabVisible(index, false);

    // 连接调试面板信号到 MainWindow 的槽函数
    connect(m_debugPanel, &DebugPanel::continueRequested,
            this, &MainWindow::onContinueDebug);
    connect(m_debugPanel, &DebugPanel::stepOverRequested,
            this, &MainWindow::onStepOver);
    connect(m_debugPanel, &DebugPanel::stepIntoRequested,
            this, &MainWindow::onStepInto);
    connect(m_debugPanel, &DebugPanel::stepOutRequested,
            this, &MainWindow::onStepOut);
    connect(m_debugPanel, &DebugPanel::stopRequested,
            this, &MainWindow::onStopDebug);

    // 连接调试面板到调试管理器
    m_debugPanel->connectToDebugManager(m_debugManager);
}

void MainWindow::setupTerminalPanel()
{
    m_terminalPanel = new TerminalPanel(this);
    m_bottomTabWidget->addTab(m_terminalPanel, QIcon(":/icons/terminal.svg"), "终端");

    if (!d->currentProjectPath.isEmpty()) {
        m_terminalPanel->setWorkingDirectory(d->currentProjectPath);
    }
}

void MainWindow::applyTheme()
{
    // 主题应用留空，后续实现
}

// ==================== 窗口事件处理 ====================

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!confirmUnsavedChanges()) {
        event->ignore();
        return;
    }

    if (m_isCompiling) {
        int reply = QMessageBox::question(this, "正在编译",
                                          "当前正在编译，是否停止编译并退出？",
                                          QMessageBox::Yes | QMessageBox::No,
                                          QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            stopCompile();
        } else {
            event->ignore();
            return;
        }
    }

    saveSession();

    // 保存 AI 面板状态
    if (m_aiChatPanel) {
        saveChatPanelState(m_aiChatPanel);
    }

    event->accept();
}

bool MainWindow::confirmUnsavedChanges()
{
    bool hasUnsaved = false;

    for (int i = 0; i < d->editorTabs->count(); ++i) {
        TextEditor* editor = qobject_cast<TextEditor*>(d->editorTabs->widget(i));
        if (editor && editor->isModified()) {
            hasUnsaved = true;
            break;
        }
    }

    if (hasUnsaved) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("未保存的更改");
        msgBox.setText("有未保存的更改，是否保存？");
        msgBox.setStandardButtons(QMessageBox::SaveAll | QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::SaveAll);

        int ret = msgBox.exec();

        if (ret == QMessageBox::SaveAll) {
            for (int i = 0; i < d->editorTabs->count(); ++i) {
                TextEditor* editor = qobject_cast<TextEditor*>(d->editorTabs->widget(i));
                if (editor && editor->isModified()) {
                    editor->saveFile();
                }
            }
            return true;
        } else if (ret == QMessageBox::Discard) {
            return true;
        } else {
            return false;
        }
    }
    return true;
}

void MainWindow::updateStatusBarPosition(TextEditor* editor)
{
    if (!editor) return;
    m_cursorPositionLabel->setText(
        QString("行: %1  列: %2")
            .arg(editor->currentLine())
            .arg(editor->currentColumn())
        );
}

void MainWindow::updateWindowTitle()
{
    QString title = "SprintIDE";

    TextEditor* editor = currentEditor();
    if (editor) {
        QString fileName = editor->fileName();
        if (!fileName.isEmpty()) {
            title += " - " + fileName;
        }
    }

    // 遍历所有标签页检查是否有未保存
    bool hasUnsaved = false;
    for (int i = 0; i < d->editorTabs->count(); ++i) {
        TextEditor* ed = qobject_cast<TextEditor*>(d->editorTabs->widget(i));
        if (ed && ed->isModified()) {
            hasUnsaved = true;
            break;
        }
    }

    if (hasUnsaved) {
        title += " *";
    }

    setWindowTitle(title);
}

void MainWindow::saveWindowState()
{
    // 窗口状态保存留空，后续实现
}

// ==================== 项目操作 ====================
void MainWindow::openProject(const QString &path) {
    // 确定实际项目根目录（现有逻辑）
    QString actualRoot = path;
    QDir dir(path);
    while (!dir.isRoot()) {
        if (dir.exists("pom.xml") || dir.exists("build.gradle") ||
            dir.exists("settings.gradle") || dir.exists(".git")) {
            actualRoot = dir.absolutePath();
            break;
        }
        dir.cdUp();
    }

    d->currentProjectPath = actualRoot;

    // 使用 ProjectManager 打开项目
    ProjectManager* pm = ProjectManager::instance();
    if (!pm->openProject(path)) {
        QMessageBox::warning(this, "打开项目失败", "无法打开项目：" + path);
        return;
    }
    d->currentProjectName = pm->currentProjectName();

    // 更新面板
    if (d->projectPanel) {
        ProjectConfig config;
        QString configFile = d->currentProjectPath + "/.lightcppproj";
        QStringList errors;
        bool hasConfig = config.loadFromFile(configFile, errors);
        d->projectPanel->openProject(d->currentProjectPath, hasConfig ? &config : nullptr);
    }

    if (m_terminalPanel) {
        m_terminalPanel->setWorkingDirectory(d->currentProjectPath);
    }

    if (m_localHistory) {
        m_localHistory->setProjectRoot(d->currentProjectPath);
    }

    // 加载项目文件列表（可选）
    loadAllProjectFiles(actualRoot);

    ProjectModel* model = pm->javaProjectModel();
    if (model) {
        QString typeStr;
        switch (model->type()) {
        case JavaProjectType::Maven: typeStr = "Maven"; break;
        case JavaProjectType::Gradle: typeStr = "Gradle"; break;
        default: typeStr = "Plain Java"; break;
        }
        d->outputPanel->append(QString("检测到 Java 项目类型: %1").arg(typeStr));
        d->outputPanel->append(QString("  主类: %1").arg(model->defaultMainClass()));
        d->outputPanel->append(QString("  依赖数量: %1").arg(model->dependencyJars().size()));
        d->outputPanel->append(QString("  类路径条目: %1").arg(model->classpath().size()));
    }

    d->outputPanel->append(QString("项目已打开: %1").arg(d->currentProjectPath));
    // 延迟恢复上次会话
    // 用 singleShot 确保当前 openProject 流程走完、UI 已就绪后再恢复，
    // 避免在 MainWindow 构造期间被 main.cpp 提前调用导致 tab 错乱。
    QTimer::singleShot(150, this, &MainWindow::restoreSession);
}

void MainWindow::closeProject()
{
    d->currentProjectPath.clear();
    // updateWindowTitle();
}

void MainWindow::openFile(const QString &path)
{
    // ★ 空路径 / 不存在的路径：直接返回，不创建编辑器，不弹窗
    if (path.isEmpty()) {
        qWarning() << "[openFile] 空路径，跳过";
        return;
    }
    if (!QFile::exists(path)) {
        qWarning() << "[openFile] 文件不存在:" << path;
        return;
    }
    // ============================================================
    // 1. 检查文件是否已打开
    // ============================================================
    for (int i = 0; i < d->editorTabs->count(); ++i) {
        TextEditor* editor = qobject_cast<TextEditor*>(d->editorTabs->widget(i));
        if (editor && editor->filePath() == path) {
            d->editorTabs->setCurrentIndex(i);
            return;
        }
    }

    // ============================================================
    // 2. 创建编辑器并加载文件
    // ============================================================
    TextEditor* editor = new TextEditor(this);

    // ---- 代码补全信号（保持不变） ----
    connect(editor, &TextEditor::completionRequested,
            this, [this, editor](const QString& filePath, int line, int column) {
                if (!m_lspManager) return;
                QString projectRoot = d->currentProjectPath.isEmpty() ?
                                          QFileInfo(filePath).absolutePath() :
                                          d->currentProjectPath;
                LSPClient* client = m_lspManager->getClientForFile(filePath, projectRoot);
                if (client) {
                    static QSet<LSPClient*> connectedClients;
                    if (!connectedClients.contains(client)) {
                        connect(client, &LSPClient::completionReceived,
                                this, [this, editor](const QString& filePath,
                                               const QList<LSPCompletionItem>& items) {
                                    if (editor && editor->filePath() == filePath) {
                                        editor->updateCompletionItems(items);
                                    }
                                });
                        connectedClients.insert(client);
                    }
                    client->requestCompletion(filePath, line, column);
                }
            });

    // ============================================================
    // 跳转定义连接（核心修复部分）
    // ============================================================
    // 跳转定义连接
    connect(editor, &TextEditor::definitionRequested,
            this, [this, editor](const QString& filePath, int line, int column) {

                if (!m_lspManager) return;
                QString projectRoot = d->currentProjectPath.isEmpty() ?
                                          QFileInfo(filePath).absolutePath() :
                                          d->currentProjectPath;
                LSPClient* client = m_lspManager->getClientForFile(filePath, projectRoot);
                if (!client) {
                    qWarning() << "[MainWindow] No LSP client for" << filePath;
                    return;
                }

                // ---- 关键修改：只建立一次连接，之后复用 ----
                if (!m_definitionConnections.contains(client)) {
                    QMetaObject::Connection conn = connect(client, &LSPClient::definitionReceived,
                                                           this, [this](const QString& targetFile, int targetLine, int targetColumn) {
                                                               openFile(targetFile);
                                                               TextEditor* targetEditor = currentEditor();
                                                               if (targetEditor) {
                                                                   targetEditor->gotoLine(targetLine);
                                                                   targetEditor->setFocus();
                                                                   targetEditor->highlightLine(targetLine);
                                                               }
                                                           });
                    m_definitionConnections[client] = conn;
                } else {
                    qDebug() << "[MainWindow] definitionReceived connection already exists for client" << client;
                }

                // ---- 等待 LSP 就绪（如果有） ----
                if (!client->isReady()) {
                    QPointer<LSPClient> clientPtr = client;
                    auto connPtr = std::make_shared<QMetaObject::Connection>();
                    *connPtr = connect(client, &LSPClient::serverReady, this,
                                       [this, clientPtr, filePath, line, column, connPtr]() {
                                           if (!clientPtr) {
                                               qWarning() << "[MainWindow] client destroyed before serverReady";
                                               return;
                                           }
                                           clientPtr->requestDefinition(filePath, line, column);
                                           disconnect(*connPtr);
                                       });
                    QTimer::singleShot(5000, this, [connPtr]() {
                        if (*connPtr) {
                            qWarning() << "[MainWindow] LSP ready timeout";
                            disconnect(*connPtr);
                        }
                    });
                    return;
                }

                // ---- 客户端已就绪，直接发送请求 ----
                client->requestDefinition(filePath, line, column);
            });

    // ---- 本地历史（原有代码） ----
    if (m_localHistory) {
        QString rootPath = d->currentProjectPath.isEmpty() ?
                               QFileInfo(path).absolutePath() :
                               d->currentProjectPath;
        m_localHistory->setProjectRoot(rootPath);
        editor->setLocalHistory(m_localHistory);
    }

    // ---- 加载文件 ----
    if (!editor->loadFile(path)) {
        delete editor;
        QMessageBox::warning(this, "打开错误",
                             QString("无法打开文件:\n%1").arg(path));
        return;
    }

    if (m_lspManager && path.endsWith(".py", Qt::CaseInsensitive)) {
        QString projectRoot = d->currentProjectPath.isEmpty() ?
                                  QFileInfo(path).absolutePath() :
                                  d->currentProjectPath;
        LSPClient* client = m_lspManager->getClientForFile(path, projectRoot);
        if (client && client->isReady()) {
            client->openDocument(path, editor->toPlainText());
        }
    }

    int index = d->editorTabs->addTab(editor, editor->fileName());
    d->editorTabs->setCurrentIndex(index);
    d->outputPanel->append(QString("已打开: %1").arg(path));

    // ---- 其他信号连接（原有） ----
    connect(editor, &TextEditor::modificationChanged,
            this, &MainWindow::onEditorModificationChanged);
    connect(editor, &TextEditor::cursorPositionChanged,
            this, &MainWindow::onEditorCursorPositionChanged);
    connect(editor, &TextEditor::fileLoaded,
            this, &MainWindow::onFileLoaded);
    connect(editor, &TextEditor::fileSaved,
            this, &MainWindow::onFileSaved);
    connect(editor->document(), &QTextDocument::contentsChanged,
            this, [this, editor]() {
                onEditorTextChanged(editor);
            });
    connect(editor, &TextEditor::breakpointToggled,
            this, &MainWindow::onBreakpointToggled);

    // ---- 应用主题 ----
    bool isDark = ThemeManager::instance().isDarkTheme();
    editor->applyTheme(isDark ? "dark" : "light");

    // ---- 更新状态栏语言 ----
    QFileInfo info(path);
    QString ext = info.suffix().toLower();
    if (ext == "cpp" || ext == "cxx" || ext == "cc" || ext == "c") {
        m_languageLabel->setText("C++");
    } else if (ext == "h" || ext == "hpp") {
        m_languageLabel->setText("C++ Header");
    } else if (ext == "py") {
        m_languageLabel->setText("Python");
    } else if (ext == "js" || ext == "ts") {
        m_languageLabel->setText("JavaScript");
    } else if (ext == "go") {
        m_languageLabel->setText("Go");
    } else if (ext == "rs") {
        m_languageLabel->setText("Rust");
    } else if (ext == "java") {
        m_languageLabel->setText("Java");
    } else if (ext == "cs") {
        m_languageLabel->setText("C#");
    } else if (ext == "php") {
        m_languageLabel->setText("PHP");
    } else if (ext == "html" || ext == "htm") {
        m_languageLabel->setText("HTML");
    } else if (ext == "css") {
        m_languageLabel->setText("CSS");
    } else if (ext == "json") {
        m_languageLabel->setText("JSON");
    } else if (ext == "xml") {
        m_languageLabel->setText("XML");
    } else if (ext == "md") {
        m_languageLabel->setText("Markdown");
    } else if (ext == "sh" || ext == "bash") {
        m_languageLabel->setText("Shell");
    } else {
        m_languageLabel->setText("Unknown");
    }

    // ---- LSP 打开文档（原有） ----
    if (m_lspManager) {
        QString projectPath = d->currentProjectPath.isEmpty() ?
                                  QFileInfo(path).absolutePath() :
                                  d->currentProjectPath;

        LSPClient *client = m_lspManager->getClientForFile(path, projectPath);
        if (client) {
            static QSet<LSPClient*> connectedClients;
            if (!connectedClients.contains(client)) {
                connect(client, &LSPClient::diagnosticsReceived,
                        this, &MainWindow::onLSPDiagnostics);
                connectedClients.insert(client);
                qDebug() << "LSP: diagnostics signal connected for client";
            }

            if (client->isReady()) {
                client->openDocument(path, editor->toPlainText());
            } else {
                QMetaObject::Connection *conn = new QMetaObject::Connection;
                *conn = connect(client, &LSPClient::serverReady, this,
                                [this, client, path, editor, conn]() {
                                    if (client && client->isReady()) {
                                        QString content = editor->toPlainText();
                                        client->openDocument(path, content);
                                        qDebug() << "LSP: openDocument (delayed) for" << path;
                                    }
                                    disconnect(*conn);
                                    delete conn;
                                });
            }
        }
    }

    // ============================================================
    //    打开文件时，把 DebugManager 中属于该文件的所有断点同步到编辑器
    //    用途：会话恢复时未必把所有断点所在文件都打开，
    //          但用户后续打开文件时应该看到红点。
    // ============================================================
    if (m_debugManager) {
        const QString absPath = QFileInfo(path).absoluteFilePath();
        const QList<Breakpoint> allBps = m_debugManager->getBreakpoints();
        for (const Breakpoint& bp : allBps) {
            if (QFileInfo(bp.filePath).absoluteFilePath() == absPath) {
                editor->setBreakpoint(bp.lineNumber, bp.enabled);
            }
        }
        editor->viewport()->update();
    }
}

void MainWindow::openFiles(const QStringList &paths)
{
    for (const QString &path : paths) {
        openFile(path);
    }
}

void MainWindow::closeFile(const QString &path)
{
    for (int i = 0; i < d->editorTabs->count(); ++i) {
        TextEditor* editor = qobject_cast<TextEditor*>(d->editorTabs->widget(i));
        if (editor && editor->filePath() == path) {
            closeEditorTab(i);
            break;
        }
    }
}

TextEditor* MainWindow::currentEditor() const
{
    return qobject_cast<TextEditor*>(d->editorTabs->currentWidget());
}

// ==================== 标签页管理 ====================

void MainWindow::createNewEditorTab(const QString& title)
{
    TextEditor* editor = new TextEditor(this);
    // 传递本地历史管理器
    if (m_localHistory) {
        editor->setLocalHistory(m_localHistory);
    }

    int index = d->editorTabs->addTab(editor, title);
    d->editorTabs->setCurrentIndex(index);

    connect(editor, &TextEditor::modificationChanged,
            this, &MainWindow::onEditorModificationChanged);
    connect(editor, &TextEditor::cursorPositionChanged,
            this, &MainWindow::onEditorCursorPositionChanged);
    connect(editor, &TextEditor::fileLoaded,
            this, &MainWindow::onFileLoaded);
    connect(editor, &TextEditor::fileSaved,
            this, &MainWindow::onFileSaved);
    connect(editor->document(), &QTextDocument::contentsChanged, this, [this, editor]() {
        onEditorTextChanged(editor);  // 直接传入 editor
    });
    connect(editor, &TextEditor::breakpointToggled, this, &MainWindow::onBreakpointToggled);
    // qDebug() << "breakpointToggled signal connected for editor:" << editor;
    bool isDark = ThemeManager::instance().isDarkTheme();
    editor->applyTheme(isDark ? "dark" : "light");
}

bool MainWindow::closeEditorTab(int index)
{
    TextEditor* editor = qobject_cast<TextEditor*>(d->editorTabs->widget(index));
    if (!editor) return true; // 无效标签，视为已关闭

    if (editor->isModified()) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("未保存的更改");
        msgBox.setText(QString("文件 '%1' 有未保存的更改。\n是否保存更改？")
                           .arg(editor->fileName()));
        msgBox.setIcon(QMessageBox::Question);
        QPushButton *saveButton = msgBox.addButton("保存(&S)", QMessageBox::AcceptRole);
        QPushButton *discardButton = msgBox.addButton("不保存(&D)", QMessageBox::RejectRole);
        QPushButton *cancelButton = msgBox.addButton("取消(&C)", QMessageBox::NoRole);
        msgBox.setDefaultButton(saveButton);
        msgBox.exec();

        QAbstractButton* clicked = msgBox.clickedButton();
        if (clicked == saveButton) {
            if (!editor->saveFile()) {
                return false; // 保存失败，视为取消
            }
            d->editorTabs->removeTab(index);
            return true;
        } else if (clicked == discardButton) {
            d->editorTabs->removeTab(index);
            return true;
        } else { // 取消
            return false;
        }
    } else {
        d->editorTabs->removeTab(index);
        return true;
    }
}

bool MainWindow::closeEditorForFile(const QString& filePath)
{
    for (int i = 0; i < d->editorTabs->count(); ++i) {
        TextEditor* editor = qobject_cast<TextEditor*>(d->editorTabs->widget(i));
        if (editor && editor->filePath() == filePath) {
            return closeEditorTab(i); // 返回是否成功关闭
        }
    }
    return true; // 编辑器未打开，视为可删除
}

void MainWindow::updateTabTitle(int index, const QString& title)
{
    if (index >= 0 && index < d->editorTabs->count()) {
        d->editorTabs->setTabText(index, title);
        // updateWindowTitle();
    }
}

// ==================== 标签页信号处理 ====================

void MainWindow::onTabCloseRequested(int index)
{
    closeEditorTab(index);
}

void MainWindow::onTabCurrentChanged(int index)
{
    if (index >= 0) {
        TextEditor* editor = qobject_cast<TextEditor*>(d->editorTabs->widget(index));
        if (editor) {
            updateStatusBarPosition(editor);

            // updateWindowTitle();

            if (!editor->filePath().isEmpty()) {
                m_statusLabel->setText(QString("正在编辑: %1").arg(editor->fileName()));
            }
        }
    } else {
        m_cursorPositionLabel->setText("行: -  列: -");
        m_statusLabel->setText("就绪");
    }
}

void MainWindow::onTabContextMenuRequested(int index, const QPoint &pos)
{
    Q_UNUSED(index);
    Q_UNUSED(pos);
}

// ==================== 编辑器信号处理 ====================

void MainWindow::onEditorModificationChanged(bool modified)
{
    Q_UNUSED(modified);
    updateWindowTitle();
}

void MainWindow::onEditorCursorPositionChanged(int line, int column)
{
    if (m_cursorPositionLabel) {
        m_cursorPositionLabel->setText(QString("行: %1  列: %2").arg(line).arg(column));
    }
}

void MainWindow::onFileLoaded(const QString &filePath)
{
    d->outputPanel->append(QString("已加载文件: %1").arg(filePath));
    m_statusLabel->setText(QString("已加载: %1").arg(QFileInfo(filePath).fileName()));
}

void MainWindow::onFileSaved(const QString &filePath)
{
    // qDebug() << "onFileSaved called, filePath:" << filePath;
    // qDebug() << "m_statusLabel:" << m_statusLabel;

    // d->outputPanel->append(QString("已保存文件: %1").arg(filePath));

    TextEditor* editor = qobject_cast<TextEditor*>(sender());
    if (editor) {
        int index = d->editorTabs->indexOf(editor);
        if (index >= 0) {
            d->editorTabs->setTabText(index, editor->fileName());
        }
    }

    if (m_lspManager && !filePath.isEmpty()) {
        LSPClient *client = m_lspManager->getClientForFile(filePath, d->currentProjectPath);
        if (client && client->isReady()) {
            client->saveDocument(filePath);
        }
    }

    // qDebug() << "Before setText";
    if (m_statusLabel) {
        m_statusLabel->setText("文件已保存");
    }
    // qDebug() << "After setText";

    updateWindowTitle();

    // LSP 保存通知
    if (m_lspManager && !filePath.isEmpty()) {
        QString projectPath = d->currentProjectPath.isEmpty() ?
                                  QFileInfo(filePath).absolutePath() :
                                  d->currentProjectPath;
        LSPClient *client = m_lspManager->getClientForFile(filePath, projectPath);
        if (client && client->isReady()) {
            client->saveDocument(filePath);
        }
    }
}

void MainWindow::onEditorTextChanged(TextEditor* editor)
{

    if (!editor) return;
    // 获取发出信号的 QTextDocument
    QTextDocument* doc = qobject_cast<QTextDocument*>(sender());
    if (!doc) return;

    d->hasUnsavedChanges = true;
    updateWindowTitle();

    // LSP 实时更新
    if (m_lspManager && !editor->filePath().isEmpty()) {
        QString projectPath = d->currentProjectPath.isEmpty() ?
                                  QFileInfo(editor->filePath()).absolutePath() :
                                  d->currentProjectPath;
        LSPClient *client = m_lspManager->getClientForFile(editor->filePath(), projectPath);
        if (client && client->isReady()) {
            client->changeDocument(editor->filePath(), editor->toPlainText());
        }
    }
}

void MainWindow::onBreakpointToggled(const QString& filePath, int line)
{
    if (!m_debugManager) return;

    // 切换断点
    m_debugManager->toggleBreakpoint(filePath, line);

    // 更新当前编辑器的断点显示
    TextEditor* editor = currentEditor();
    if (editor && editor->filePath() == filePath) {
        bool hasBreakpoint = m_debugManager->hasBreakpoint(filePath, line);
        editor->setBreakpoint(line, hasBreakpoint);
    }

    // ========== C++ 调试断点同步 ==========
    if (m_cppDebugger && m_isDebugging) {
        QList<Breakpoint> breakpoints = m_debugManager->getBreakpoints();
        m_cppDebugger->setBreakpoints(breakpoints);
        qDebug() << "[Cpp Debug] 断点已同步，共" << breakpoints.size() << "个";
    }

    // ========== Python 调试断点同步 ==========
    if (m_pythonDebugger && m_isDebugging) {
        QList<Breakpoint> breakpoints = m_debugManager->getBreakpoints();
        m_pythonDebugger->setBreakpoints(breakpoints);
        qDebug() << "[Python Debug] 断点已同步，共" << breakpoints.size() << "个";
    }
}

void MainWindow::onPythonDebugFinished()
{
    // 幂等：避免 exited + terminated 双触发时重复清理
    if (!m_isDebugging && !m_debugPanel)
        return;

    m_isDebugging = false;

    // 1. 清除当前调试行高亮
    if (TextEditor* editor = currentEditor()) {
        editor->clearCurrentDebugLine();
    }

    // 2. 更新调试动作的启用/禁用
    updateDebugActions();

    // 3. 清空调试面板
    if (m_debugPanel) {
        m_debugPanel->setDebuggingState(false);
        m_debugPanel->updateVariables(QList<DebugVariable>());
        m_debugPanel->updateCallStack(QList<StackFrame>());
    }

    // 4. 状态栏提示
    if (m_statusLabel) {
        m_statusLabel->setText("调试已结束");
    }

    // 5. ★ 关键：切回输出面板
    switchToOutputTab();

    d->outputPanel->append(QString("<span style='color:#33CC33;'>调试会话已结束</span>"));
}

// ==================== 项目面板信号处理 ====================

void MainWindow::onProjectItemDoubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index);
    d->outputPanel->append("项目面板开发中...");
}

// ==================== 编译系统 ====================

void MainWindow::connectCompileManagerSignals()
{
    if (!m_compileManager) {
        qWarning() << "编译管理器未初始化";
        return;
    }

    connect(m_compileManager, &CompileManager::compileStarted,
            this, &MainWindow::onCompileStarted);
    connect(m_compileManager, &CompileManager::compileProgress,
            this, &MainWindow::onCompileProgress);
    connect(m_compileManager, &CompileManager::compileOutput,
            this, &MainWindow::onCompileOutput);
    connect(m_compileManager, &CompileManager::compileFinished,
            this, &MainWindow::onCompileFinished);
    connect(m_compileManager, &CompileManager::compileError,
            this, &MainWindow::onCompileError);
    connect(m_compileManager, &CompileManager::compilersDetected,
            this, &MainWindow::onCompilersDetected);
}

void MainWindow::onCompileStarted(const QString& taskId, const QString& sourceFile)
{
    m_currentCompileTaskId = taskId;
    m_isCompiling = true;
    m_compileTimer.start();
    m_compilerOutputCache[taskId] = QString();

    // 切换到输出标签
    switchToOutputTab();

    d->outputPanel->clear();

    QString timeStr = QTime::currentTime().toString("hh:mm:ss");
    QString htmlMessage = QString("<span style='color:#3399FF;'>[%1] 开始编译: %2</span>")
                              .arg(timeStr)
                              .arg(sourceFile);
    d->outputPanel->append(htmlMessage);

    m_statusLabel->setText(QString("正在编译 %1...").arg(QFileInfo(sourceFile).fileName()));

    for (int i = 0; i < d->editorTabs->count(); ++i) {
        TextEditor* editor = qobject_cast<TextEditor*>(d->editorTabs->widget(i));
        if (editor) {
            editor->clearAllMarkers();
        }
    }

    // 显示进度条
    m_buildProgressBar->setVisible(true);
    m_buildProgressBar->setValue(0);
    // qDebug() << "编译开始，任务ID:" << taskId << "源文件:" << sourceFile;
}

void MainWindow::onCompileProgress(const QString& taskId, int progress, const QString& currentFile)
{
    if (taskId != m_currentCompileTaskId) {
        return;
    }

    QString progressStr;
    if (!currentFile.isEmpty()) {
        progressStr = QString("正在编译... %1% - %2").arg(progress).arg(currentFile);
    } else {
        progressStr = QString("正在编译... %1%").arg(progress);
    }
    m_statusLabel->setText(progressStr);

    if (progress % 25 == 0) {
        QString timeStr = QTime::currentTime().toString("hh:mm:ss");
        QString htmlMessage = QString("<span style='color:#9966FF;'>[%1] 编译进度: %2%</span>")
                                  .arg(timeStr)
                                  .arg(progress);
        d->outputPanel->append(htmlMessage);
    }
    m_buildProgressBar->setValue(progress);
}

void MainWindow::onCompileOutput(const QString& taskId, const QString& output, bool isError)
{
    if (taskId != m_currentCompileTaskId) {
        return;
    }

    // 编译输出关键字的分类正则
    static QRegularExpression errorRegex(
        "(^|\\s)(fatal\\s+)?error(\\s|:)|错误",
        QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression warningRegex(
        "(^|\\s)warning(\\s|:)|警告",
        QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression noteRegex(
        "(^|\\s)note(\\s|:)|说明",
        QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression progressRegex(
        "^(\\[\\s*\\d+%\\]|--|==>|\\[\\d+/\\d+\\])",
        QRegularExpression::CaseInsensitiveOption);

    const QStringList lines = output.split('\n');
    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;

        QString color;
        if (errorRegex.match(trimmed).hasMatch()) {
            color = "#FF5555";
        } else if (warningRegex.match(trimmed).hasMatch()) {
            color = "#FFAA00";
        } else if (noteRegex.match(trimmed).hasMatch()) {
            color = "#9999FF";
        } else if (progressRegex.match(trimmed).hasMatch()) {
            color = "#88DDFF";
        } else if (isError) {
            color = "#FF5555";   // 编译失败时，所有输出默认红色
        } else {
            color = "#CCCCCC";   // 正常输出：浅灰
        }

        d->outputPanel->append(QString("<span style='color:%1;'>%2</span>")
                                   .arg(color, line.toHtmlEscaped()));
    }
}

void MainWindow::onCompileFinished(const QString& taskId, const CompileResult& result)
{
    if (taskId != m_currentCompileTaskId) {
        return;
    }

    m_isCompiling = false;
    qint64 elapsedMs = m_compileTimer.elapsed();

    QString timeStr = QTime::currentTime().toString("hh:mm:ss");

    if (result.success) {
        QString htmlMessage = QString("<span style='color:#33CC33;'>[%1] 编译成功! 耗时: %2ms</span>")
                                  .arg(timeStr)
                                  .arg(elapsedMs);
        d->outputPanel->append(htmlMessage);

        if (!result.executablePath.isEmpty()) {
            d->outputPanel->append(QString("<span style='color:#33CC33;'>可执行文件: %1</span>")
                                       .arg(result.executablePath));
        }

        // 编译成功的细节统计
        QString statsLine = QString("<span style='color:#66CC66;'>  错误: %1, 警告: %2</span>")
                                .arg(result.errors.size())
                                .arg(result.warnings.size());
        d->outputPanel->append(statsLine);

        m_statusLabel->setText("编译成功");

        // ========== 编译后自动运行（新增） ==========
        if (m_autoRunAfterCompile) {
            m_autoRunAfterCompile = false;
            if (!result.executablePath.isEmpty()) {
                d->outputPanel->append("编译成功，正在运行...");
                // runProgram(result.executablePath, false);
                runExecutableAsync(result.executablePath);
            } else {
                d->outputPanel->append("编译成功，但未找到可执行文件，无法自动运行。");
            }
        }

        // ========== 编译后自动启动调试（原有） ==========
        if (m_compileForDebug) {
            m_compileForDebug = false;

            if (result.executablePath.isEmpty()) {
                d->outputPanel->append("编译成功，但未找到可执行文件，无法启动调试。");
            } else {
                // 判断当前编辑器语言
                TextEditor* editor = currentEditor();
                LanguageType lang = editor ? detectLanguageTypeFromPath(editor->filePath())
                                           : LanguageType::Unknown;

                if (lang == LanguageType::Cpp) {
                    // ★ C++：用 DAP 路径。重新进 onStartDebug()，这次 exePath 已存在
                    d->outputPanel->append("编译成功，正在启动 C++ 调试...");
                    QTimer::singleShot(100, this, &MainWindow::onStartDebug);
                } else {
                    // 其他语言：保留旧逻辑
                    d->outputPanel->append("编译成功，正在启动调试...");
                    syncBreakpointsToDebugger();
                    startDebuggingWithExecutable(result.executablePath);
                }
            }
        }

    } else {
        QString htmlMessage = QString("<span style='color:#FF3333;'>[%1] 编译失败! 耗时: %2ms</span>")
                                  .arg(timeStr)
                                  .arg(elapsedMs);
        d->outputPanel->append(htmlMessage);

        m_statusLabel->setText("编译失败");

        int errorCount = result.errors.size();
        int warningCount = result.warnings.size();

        if (errorCount > 0 || warningCount > 0) {
            QString statsMessage = QString("<span style='color:#FF3333;'>错误: %1, 警告: %2</span>")
                                       .arg(errorCount)
                                       .arg(warningCount);
            d->outputPanel->append(statsMessage);
        }

        if (!result.errors.isEmpty()) {
            d->outputPanel->append("<span style='color:#FF3333;font-weight:bold;'>错误列表:</span>");
            for (int i = 0; i < qMin(5, result.errors.size()); i++) {
                d->outputPanel->append(QString("<span style='color:#FF3333;'>  %1. %2</span>")
                                           .arg(i + 1)
                                           .arg(result.errors[i].toHtmlEscaped()));
            }
            if (result.errors.size() > 5) {
                d->outputPanel->append(QString("<span style='color:#FF9900;'>... 还有 %1 个错误未显示</span>")
                                           .arg(result.errors.size() - 5));
            }
        }

        // 失败时清除自动运行/调试标志
        if (m_compileForDebug) {
            m_compileForDebug = false;
            d->outputPanel->append("编译失败，无法启动调试。请修复错误后重试。");
        }
        if (m_autoRunAfterCompile) {
            m_autoRunAfterCompile = false;
            d->outputPanel->append("编译失败，无法自动运行。");
        }
    }

    // ========== 显示错误/警告标记（原有逻辑，保持不变） ==========
    if (!result.success) {
        // 清除所有编辑器的旧标记
        for (int i = 0; i < d->editorTabs->count(); ++i) {
            TextEditor* editor = qobject_cast<TextEditor*>(d->editorTabs->widget(i));
            if (editor) {
                editor->clearAllMarkers();
            }
        }

        // 收集每个文件的错误和警告
        QMap<QString, QList<SprintIDE::ErrorMarker>> fileErrors;
        QMap<QString, QList<SprintIDE::ErrorMarker>> fileWarnings;

        for (const QString& errorLine : result.errors) {
            SprintIDE::ErrorMarker marker = parseErrorLine(errorLine);
            if (marker.line > 0) {
                for (int i = 0; i < d->editorTabs->count(); ++i) {
                    TextEditor* editor = qobject_cast<TextEditor*>(d->editorTabs->widget(i));
                    if (!editor) continue;
                    if (QFileInfo(editor->filePath()).fileName() == QFileInfo(marker.filePath).fileName()) {
                        editor->setErrorMarker(marker.filePath, marker.line, marker.column, marker.message);
                        editor->viewport()->update();
                        break;
                    }
                }
            }
        }

        for (const QString& warningLine : result.warnings) {
            SprintIDE::ErrorMarker marker = parseErrorLine(warningLine);
            if (marker.line > 0) {
                for (int i = 0; i < d->editorTabs->count(); ++i) {
                    TextEditor* editor = qobject_cast<TextEditor*>(d->editorTabs->widget(i));
                    if (!editor) continue;
                    if (QFileInfo(editor->filePath()).fileName() == QFileInfo(marker.filePath).fileName()) {
                        editor->setWarningMarker(marker.filePath, marker.line, marker.column, marker.message);
                        editor->viewport()->update();
                        break;
                    }
                }
            }
        }

        // 将标记添加到对应的编辑器
        for (int i = 0; i < d->editorTabs->count(); ++i) {
            TextEditor* editor = qobject_cast<TextEditor*>(d->editorTabs->widget(i));
            if (!editor) continue;

            QString filePath = editor->filePath();
            if (fileErrors.contains(filePath)) {
                for (const SprintIDE::ErrorMarker& marker : fileErrors[filePath]) {
                    editor->setErrorMarker(marker.filePath, marker.line, marker.column, marker.message);
                }
            }
            if (fileWarnings.contains(filePath)) {
                for (const SprintIDE::ErrorMarker& marker : fileWarnings[filePath]) {
                    editor->setWarningMarker(marker.filePath, marker.line, marker.column, marker.message);
                }
            }
        }
    }

    if (result.errors.isEmpty() && !result.success) {
        d->outputPanel->append("=== 编译输出（完整） ===");
        d->outputPanel->append(result.output);
        d->outputPanel->append("=== 编译输出结束 ===");
    }

    // ========== 清理工作 ==========
    m_currentCompileTaskId.clear();
    m_buildProgressBar->setVisible(false);
    m_buildProgressBar->setValue(0);
}

void MainWindow::onCompileError(const QString& taskId, const QString& errorMessage)
{
    if (taskId != m_currentCompileTaskId) {
        return;
    }

    m_isCompiling = false;

    QString timeStr = QTime::currentTime().toString("hh:mm:ss");
    QString htmlMessage = QString("<span style='color:#FF3333;font-weight:bold;'>[%1] 编译错误: %2</span>")
                              .arg(timeStr)
                              .arg(errorMessage.toHtmlEscaped());
    d->outputPanel->append(htmlMessage);

    m_statusLabel->setText("编译错误");

    m_currentCompileTaskId.clear();
}

void MainWindow::onCompilersDetected(const QStringList& compilers)
{
    if (compilers.isEmpty()) {
        return;
    }

    QString timeStr = QTime::currentTime().toString("hh:mm:ss");
    d->outputPanel->append(QString("<span style='color:#3399FF;'>[%1] 检测到的编译器:</span>").arg(timeStr));

    for (const QString& compiler : compilers) {
        QString info = m_compileManager->getCompilerInfo(compiler);
        d->outputPanel->append(QString("<span style='color:#CCCCCC;'>  - %1: %2</span>")
                                   .arg(compiler)
                                   .arg(info.toHtmlEscaped()));
    }
}

// ==================== 程序运行 ====================

void MainWindow::askRunProgram(const QString& executablePath)
{
    if (executablePath.isEmpty()) {
        return;
    }

    runProgram(executablePath, false);
}

void MainWindow::runProgram(const QString& executablePath, bool inTerminal)
{
    if (!QFile::exists(executablePath)) {
        QMessageBox::warning(this, "运行错误",
                             QString("可执行文件不存在:\n%1").arg(executablePath));
        return;
    }

    QString timeStr = QTime::currentTime().toString("hh:mm:ss");
    d->outputPanel->append(QString("<span style='color:#33CC33;'>[%1] 正在运行程序...</span>").arg(timeStr));

    if (inTerminal) {
        QString program;
        QStringList arguments;

#ifdef Q_OS_WIN
        program = "cmd.exe";
        arguments << "/c" << "start" << "cmd" << "/c" << executablePath + " & pause";
#elif defined(Q_OS_MAC)
        program = "osascript";
        arguments << "-e" << QString("tell application \"Terminal\" to do script \"%1\"").arg(executablePath);
#else
        program = "xterm";
        arguments << "-e" << executablePath + "; echo '程序执行完毕，按任意键退出...'; read -n1";
#endif

        QProcess::startDetached(program, arguments);

    } else {
        QProcess* process = new QProcess(this);
        process->setProgram(executablePath);
        process->setWorkingDirectory(QFileInfo(executablePath).path());

        connect(process, &QProcess::readyReadStandardOutput, [=]() {
            QString output = QString::fromLocal8Bit(process->readAllStandardOutput());
            QString timeStr = QTime::currentTime().toString("hh:mm:ss");
            d->outputPanel->append(QString("<span style='color:#CCCCCC;'>[%1] 程序输出: %2</span>")
                                       .arg(timeStr)
                                       .arg(output.toHtmlEscaped()));
        });

        connect(process, &QProcess::readyReadStandardError, [=]() {
            QString error = QString::fromLocal8Bit(process->readAllStandardError());
            QString timeStr = QTime::currentTime().toString("hh:mm:ss");
            d->outputPanel->append(QString("<span style='color:#FF9900;'>[%1] 程序错误: %2</span>")
                                       .arg(timeStr)
                                       .arg(error.toHtmlEscaped()));
        });

        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                [=](int exitCode, QProcess::ExitStatus exitStatus) {
                    QString timeStr = QTime::currentTime().toString("hh:mm:ss");
                    QString message;

                    if (exitStatus == QProcess::NormalExit) {
                        message = QString("<span style='color:#33CC33;'>[%1] 程序执行完毕，退出码: %2</span>")
                                      .arg(timeStr)
                                      .arg(exitCode);
                    } else {
                        message = QString("<span style='color:#FF3333;'>[%1] 程序异常退出</span>").arg(timeStr);
                    }

                    d->outputPanel->append(message);
                    m_statusLabel->setText("程序执行完成");

                    process->deleteLater();
                });

        process->start();

        if (!process->waitForStarted(3000)) {
            QMessageBox::warning(this, "运行错误", "无法启动程序");
            process->deleteLater();
        } else {
            m_statusLabel->setText("程序运行中...");
        }
    }
}

void MainWindow::syncBreakpointsToDebugger()
{
    if (!m_debugManager) return;

    // 获取所有打开的文件及其断点
    for (int i = 0; i < d->editorTabs->count(); ++i) {
        TextEditor* editor = qobject_cast<TextEditor*>(d->editorTabs->widget(i));
        if (!editor) continue;

        QString filePath = editor->filePath();
        if (filePath.isEmpty()) continue;

        // 获取编辑器中的所有断点（需要 TextEditor 提供接口）
        // 这里简化处理：通过 DebugManager 的断点列表同步
        // 因为断点已经在 toggleBreakpoint 时添加到了 DebugManager
        // 所以只需要确保 DebugManager 中的断点有效
    }

    // 实际上，断点已经在 toggleBreakpoint 时添加到 DebugManager 了
    // 编译后重新加载可执行文件时，需要重新同步断点到调试器
    // 这会在 DebugManager::startDebugging 中调用 syncBreakpoints 完成

    // qDebug() << "断点已同步到调试器";
}


void MainWindow::onLSPDiagnostics(const QString& filePath, const QList<LSPDiagnostic>& diagnostics)
{
    qDebug() << "MainWindow: onLSPDiagnostics called, file:" << filePath << "count:" << diagnostics.size();
    // 查找对应的编辑器
    TextEditor* editor = findEditorByFilePath(filePath);
    if (!editor) return;

    // 清除旧的 LSP 标记（不删除编译标记）
    editor->clearAllMarkers();
    // 需要 TextEditor 支持区分标记类型
    for (const LSPDiagnostic& diag : diagnostics) {
        if (diag.severity == "error") {
            editor->setErrorMarker(filePath, diag.line, diag.column, diag.message);
        } else if (diag.severity == "warning") {
            editor->setWarningMarker(filePath, diag.line, diag.column, diag.message);
        }
    }
}

// 辅助方法
TextEditor* MainWindow::findEditorByFilePath(const QString& filePath)
{
    for (int i = 0; i < d->editorTabs->count(); ++i) {
        TextEditor* editor = qobject_cast<TextEditor*>(d->editorTabs->widget(i));
        if (editor && editor->filePath() == filePath) {
            return editor;
        }
    }
    return nullptr;
}
// ==================== 调试菜单槽函数 ====================

void MainWindow::onStartDebug() {
    TextEditor* editor = currentEditor();
    if (!editor) {
        QMessageBox::information(this, "提示", "请先打开一个文件再开始调试。");
        return;
    }

    QString sourceFile = editor->filePath();
    if (sourceFile.isEmpty()) {
        QMessageBox::warning(this, "错误", "请先保存文件。");
        return;
    }

    if (editor->isModified() && !editor->saveFile()) {
        QMessageBox::warning(this, "保存错误", "无法保存文件，调试中止。");
        return;
    }

    if (m_isCompiling) {
        QMessageBox::information(this, "提示", "当前正在编译，请等待编译完成后再调试。");
        return;
    }

    if (m_isDebugging) {
        QMessageBox::information(this, "提示", "当前正在调试中，请先停止调试。");
        return;
    }

    LanguageType lang = detectLanguageTypeFromPath(sourceFile);

    // ========== Python 调试 ==========
    if (lang == LanguageType::Python || sourceFile.endsWith(".py")) {

        // ---- 获取解释器（与 onRun 逻辑一致） ----
        auto model = ProjectManager::instance()->pythonProjectModel();
        QString projectPath = d->currentProjectPath.isEmpty() ? QFileInfo(sourceFile).absolutePath() : d->currentProjectPath;
        QString python;

        // 1. 优先使用虚拟环境中的解释器
        if (model && !model->interpreterPath().isEmpty()) {
            python = model->interpreterPath();
        }
        // 2. 如果模型未提供，从 SDK 环境变量 PYTHON_HOME 读取
        else if (python.isEmpty() || python == "python3") {
            ConfigManager* cfg = Application::instance().configManager();
            if (cfg) {
                QVariantMap sdkEnv = cfg->value("Sdk/Environment").toMap();
                if (sdkEnv.contains("PYTHON_HOME")) {
                    QString pyHome = sdkEnv["PYTHON_HOME"].toString();
                    if (!pyHome.isEmpty()) {
#ifdef Q_OS_WIN
                        QString pyExe = pyHome + "/python.exe";
#else
                        QString pyExe = pyHome; // macOS/Linux 通常直接是 bin/python3 或链接
                        // macOS 特定路径检查
#ifdef Q_OS_MAC
                        if (pyExe.isEmpty() || pyExe == "python3") {
                            if (QFile::exists("/opt/homebrew/bin/python3")) python = "/opt/homebrew/bin/python3";
                            else if (QFile::exists("/usr/local/bin/python3")) python = "/usr/local/bin/python3";
                        }
#endif
#endif
                        if (QFile::exists(pyExe)) {
                            python = pyExe;
                        }
                    }
                }
            }
        }
        // 3. 最终 fallback
        if (python.isEmpty()) {
            python = "python3";
        }

        // 构建调试配置
        DebugConfig config;
        config.executablePath = sourceFile;
        config.debuggerPath = python;
        config.workingDirectory = projectPath;
        config.stopOnEntry = true;

        d->outputPanel->append("调试目标文件: " + sourceFile);

        // --- 关键修改：复用调试器实例 ---
        if (!m_pythonDebugger) {
            m_pythonDebugger = new PythonDebugger(this);

            // 连接信号
            connect(m_pythonDebugger, &PythonDebugger::stopped, this,
                    [this](const QString& file, int line, const QString& reason) {

                        d->outputPanel->append(QString("调试暂停: %1:%2 (%3)")
                                                   .arg(file).arg(line).arg(reason));

                        // 1. 找到（或打开）命中的文件
                        TextEditor* editor = findEditorByFilePath(file);
                        if (!editor) {
                            openFile(file);
                            editor = currentEditor();
                        } else {
                            d->editorTabs->setCurrentWidget(editor);
                        }

                        // 2. 定位并高亮
                        if (editor) {
                            editor->gotoLine(line);
                            editor->setCurrentDebugLine(line);   // 你的 TextEditor 提供的方法
                            editor->setFocus();
                            updateStatusBarPosition(editor);
                        }

                        // 3. 状态更新
                        m_isDebugging = true;
                        updateDebugActions();
                        switchToDebugTab();                       // 自动切到调试面板
                    });

            // connect(m_pythonDebugger, &PythonDebugger::continued, this, [this]() {
            //     d->outputPanel->append("继续执行");
            //     TextEditor* ed = currentEditor();
            //     if (ed) { ed->clearCurrentDebugLine(); }
            // });

            connect(m_pythonDebugger, &PythonDebugger::exited, this, [this](int code) {
                d->outputPanel->append(QString("程序退出，退出码: %1").arg(code));
                onPythonDebugFinished();
            });
            connect(m_pythonDebugger, &PythonDebugger::terminated,
                    this, &MainWindow::onPythonDebugFinished);


            connect(m_pythonDebugger, &PythonDebugger::continued, this, &MainWindow::onDebuggerContinued);

            connect(m_pythonDebugger, &PythonDebugger::outputReceived, this, [this](const QString& out, bool err) {
                if (err) { d->outputPanel->append("<span style='color:#FF6B6B;'>" + out.toHtmlEscaped() + "</span>"); }
                else { d->outputPanel->append(out); }
            });

            connect(m_pythonDebugger, &PythonDebugger::errorOccurred, this, [this](const QString& error) {
                d->outputPanel->append("调试错误: " + error);
                m_isDebugging = false;
                updateDebugActions();
                if (m_debugPanel) {
                    m_debugPanel->setDebuggingState(false);
                    m_debugPanel->updateVariables(QList<DebugVariable>());
                    m_debugPanel->updateCallStack(QList<StackFrame>());
                }
                TextEditor* ed = currentEditor();
                if (ed) { ed->clearCurrentDebugLine(); }
            });

            connect(m_pythonDebugger, &PythonDebugger::variablesUpdated, this, [this](const QList<DebugVariable>& vars) {
                if (m_debugPanel) { m_debugPanel->updateVariables(vars); }
            });

            connect(m_pythonDebugger, &PythonDebugger::stackUpdated, this, [this](const QList<StackFrame>& frames) {
                if (m_debugPanel) { m_debugPanel->updateCallStack(frames); }
            });
        }

        // 启动调试
        if (m_pythonDebugger->startDebugging(config)) {
            d->outputPanel->append("Python 调试已启动");
            m_isDebugging = true;

            QList<Breakpoint> allBreakpoints = m_debugManager->getBreakpoints();
            m_pythonDebugger->setBreakpoints(allBreakpoints);
            d->outputPanel->append(QString("已同步 %1 个断点").arg(allBreakpoints.size()));

            // 切换到调试面板
            switchToDebugTab();
            // 更新调试动作状态
            updateDebugActions();

            // --- 关键修改：不再手动发送断点 ---
            // 断点将在 debugpy 发送 "initialized" 事件后，
            // 由 PythonDebugger 内部的 applyBreakpoints() 自动同步
            d->outputPanel->append("断点将在调试器初始化后自动同步");

            // 初始化调试面板
            if (m_debugPanel) {
                m_debugPanel->setDebuggingState(true);
                m_debugPanel->updateVariables(QList<DebugVariable>());
                m_debugPanel->updateCallStack(QList<StackFrame>());
            }
        } else {
            d->outputPanel->append("Python 调试启动失败");
            // 如果启动失败，确保状态重置
            if (m_pythonDebugger) {
                m_pythonDebugger->stopDebugging();
                // 注意：这里也不删除实例，以便下次重试
            }
        }
        return;
    }

    // ==========  C++ 调试（DAP）==========
    if (lang == LanguageType::Cpp) {
        qDebug() << "[onStartDebug] 进入 C++ DAP 分支";

        // 1. 获取可执行文件路径
        BuildTask task;
        task.taskId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        task.language = LanguageType::Cpp;
        task.projectPath = d->currentProjectPath.isEmpty()
                               ? QFileInfo(sourceFile).absolutePath()
                               : d->currentProjectPath;
        task.sourceFiles << sourceFile;
        task.outputName = QString();       // 留空，让 CppBuilder 从 CMake 取
        task.outputDir = "build";          // CMake 项目默认 build
        task.enableDebug = true;

        if (!d->currentProjectPath.isEmpty()) {
            ProjectConfig projConfig;
            QString configFile = d->currentProjectPath + "/.lightcppproj";
            QStringList errors;
            if (projConfig.loadFromFile(configFile, errors)) {
                task.sdkConfig = projConfig.sdkEnvironment();
            }
        }

        IBuilder* builder = BuilderFactory::createBuilder(LanguageType::Cpp);
        QString exePath;
        if (builder) exePath = builder->getExecutablePath(task);

        qDebug() << "[onStartDebug] exePath =" << exePath;

        if (exePath.isEmpty() || !QFile::exists(exePath) ||
            QFileInfo(sourceFile).lastModified() > QFileInfo(exePath).lastModified()) {
            int reply = QMessageBox::question(this, "编译并调试",
                                              "文件尚未编译或已过期，是否现在编译并调试?",
                                              QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                m_compileForDebug = true;
                onBuildProject();      // ★ 用 onBuildProject，不是 onCompile
            }
            return;
        }

        // 2. 解析 DAP adapter
        auto adapterRes = ToolchainManager::instance().resolve(ToolchainId::LldbDap);
        if (!adapterRes.found) {
            ToolchainManager::instance().resolveOrPrompt(ToolchainId::LldbDap, this, &adapterRes);
            if (!adapterRes.found) {
                d->outputPanel->append("未找到 C++ 调试适配器，调试中止。");
                return;
            }
        }

        d->outputPanel->append(QString("使用调试适配器: %1").arg(adapterRes.executablePath));

        // 3. 构建配置
        CppDebugConfig cfg;
        cfg.adapterPath = adapterRes.executablePath;
        cfg.program = exePath;
        cfg.workingDirectory = QFileInfo(exePath).absolutePath();
        cfg.stopOnEntry = false;

        // 4. 首次创建 CppDebugger
        if (!m_cppDebugger) {
            m_cppDebugger = new CppDebugger(this);

            connect(m_cppDebugger, &CppDebugger::stopped, this,
                    [this](const QString& file, int line, const QString& reason) {
                        d->outputPanel->append(
                            QString("调试暂停: %1:%2 (%3)").arg(file).arg(line).arg(reason));

                        // 空路径防护：DAP 有时不返回 source.path
                        if (file.isEmpty()) {
                            d->outputPanel->append(
                                "<span style='color:#FF9900;'>警告：调试器未返回源文件路径，无法跳转高亮</span>");
                            m_isDebugging = true;
                            updateDebugActions();
                            switchToDebugTab();
                            return;
                        }

                        TextEditor* ed = findEditorByFilePath(file);
                        if (!ed) { openFile(file); ed = currentEditor(); }
                        else { d->editorTabs->setCurrentWidget(ed); }

                        if (ed) {
                            ed->gotoLine(line);
                            ed->setCurrentDebugLine(line);
                            ed->setFocus();
                            updateStatusBarPosition(ed);
                        }
                        m_isDebugging = true;
                        updateDebugActions();
                        switchToDebugTab();
                    });

            connect(m_cppDebugger, &CppDebugger::continued,
                    this, &MainWindow::onDebuggerContinued);

            connect(m_cppDebugger, &CppDebugger::exited, this,
                    [this](int code) {
                        d->outputPanel->append(QString("程序退出，退出码: %1").arg(code));
                        onPythonDebugFinished();
                    });

            connect(m_cppDebugger, &CppDebugger::terminated,
                    this, &MainWindow::onPythonDebugFinished);

            connect(m_cppDebugger, &CppDebugger::outputReceived, this,
                    [this](const QString& out, bool err) {
                        if (err) {
                            d->outputPanel->append("<span style='color:#FF6B6B;'>"
                                                   + out.toHtmlEscaped() + "</span>");
                        } else {
                            d->outputPanel->append(out);
                        }
                    });

            connect(m_cppDebugger, &CppDebugger::errorOccurred, this,
                    [this](const QString& e) {
                        d->outputPanel->append("调试错误: " + e);
                        m_isDebugging = false;
                        updateDebugActions();
                        if (m_debugPanel) m_debugPanel->setDebuggingState(false);
                    });

            connect(m_cppDebugger, &CppDebugger::variablesUpdated, this,
                    [this](const QList<DebugVariable>& vars) {
                        if (m_debugPanel) m_debugPanel->updateVariables(vars);
                    });

            connect(m_cppDebugger, &CppDebugger::stackUpdated, this,
                    [this](const QList<StackFrame>& frames) {
                        if (m_debugPanel) m_debugPanel->updateCallStack(frames);
                    });
        }

        // 5. 启动
        if (m_cppDebugger->startDebugging(cfg)) {
            d->outputPanel->append("C++ 调试已启动");
            m_isDebugging = true;
            updateDebugActions();
            switchToDebugTab();
            if (m_debugPanel) {
                m_debugPanel->setDebuggingState(true);
                m_debugPanel->updateVariables({});
                m_debugPanel->updateCallStack({});
            }

            QList<Breakpoint> allBps = m_debugManager->getBreakpoints();
            m_cppDebugger->setBreakpoints(allBps);
            d->outputPanel->append(QString("已同步 %1 个断点").arg(allBps.size()));
        } else {
            d->outputPanel->append("C++ 调试启动失败");
            m_isDebugging = false;
            updateDebugActions();
        }
        return;
    }

    // ========== C++ / Java / 其他调试（原有逻辑） ==========
    BuildTask task;
    task.taskId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    task.language = detectLanguageTypeFromPath(sourceFile);
    task.projectPath = d->currentProjectPath.isEmpty() ? QFileInfo(sourceFile).absolutePath() : d->currentProjectPath;
    task.sourceFiles << sourceFile;
    task.outputName = QFileInfo(sourceFile).completeBaseName();
    task.outputDir = "bin";
    task.enableDebug = true;
    task.debugPort = 5005;
    task.suspendOnStart = true;

    if (!d->currentProjectPath.isEmpty()) {
        ProjectConfig projConfig;
        QString configFile = d->currentProjectPath + "/.lightcppproj";
        QStringList errors;
        if (projConfig.loadFromFile(configFile, errors)) {
            task.sdkConfig = projConfig.sdkEnvironment();
        }
    }

    if (task.language == LanguageType::Java) {
        ProjectManager* pm = ProjectManager::instance();
        ProjectModel* model = pm->javaProjectModel();
        if (model) {
            task.classpath = model->classpath();
            task.dependencyJars = model->dependencyJars();
            task.aiCustomPayload["mainClass"] = model->defaultMainClass();
            task.aiCustomPayload["isFatJar"] = model->isFatJar();
            task.aiCustomPayload["fatJarPath"] = model->fatJarPath();
            task.outputDir = model->outputDirectory();
            task.projectPath = model->projectPath();
        }
    }

    IBuilder* builder = BuilderFactory::createBuilder(task.language);
    QString executablePath;
    if (builder) {
        executablePath = builder->getExecutablePath(task);
    }

    if (executablePath.isEmpty() || !QFile::exists(executablePath) || QFileInfo(sourceFile).lastModified() > QFileInfo(executablePath).lastModified()) {
        int reply = QMessageBox::question(this, "编译并调试", "文件尚未编译或已过期，是否现在编译并调试?", QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            m_compileForDebug = true;
            onCompile();
        }
        return;
    }

    BuildResult dummyResult;
    dummyResult.executablePath = executablePath;
    QString output;
    if (m_compileManager->runTask(task, dummyResult, output)) {
        d->outputPanel->append("调试已启动，等待连接...");
        m_isDebugging = true;
        updateDebugActions();
    } else {
        d->outputPanel->append("启动调试失败: " + output);
    }
}

void MainWindow::onStopDebug() {
    if (isCppDebugging()) {
            m_cppDebugger->stopDebugging();
            delete m_cppDebugger;
            m_cppDebugger = nullptr;

            m_isDebugging = false;
            updateDebugActions();
            if (m_debugPanel) {
                m_debugPanel->setDebuggingState(false);
                m_debugPanel->updateVariables(QList<DebugVariable>());
                m_debugPanel->updateCallStack(QList<StackFrame>());
            }
            TextEditor* editor = currentEditor();
            if (editor) editor->clearCurrentDebugLine();
            switchToOutputTab();
            return;
        }

    if (isPythonDebugging()) {
        // 1. 停止调试进程
        m_pythonDebugger->stopDebugging();

        // 2. 【关键修复】删除调试器对象，释放资源
        delete m_pythonDebugger;
        m_pythonDebugger = nullptr;

        // 3. 更新 UI 状态
        m_isDebugging = false;
        updateDebugActions();
        if (m_debugPanel) {
            m_debugPanel->setDebuggingState(false);
            m_debugPanel->updateVariables(QList<DebugVariable>());
            m_debugPanel->updateCallStack(QList<StackFrame>());
        }
        TextEditor* editor = currentEditor();
        if (editor) editor->clearCurrentDebugLine();
        return;
    }

    // ... 其他调试器（如 C++）的停止逻辑保持不变 ...
    if (m_debugManager && m_isDebugging) {
        m_debugManager->stopDebugging();
    }
    switchToOutputTab();
}

void MainWindow::onStepOver()
{
    if (isCppDebugging()) {
        m_cppDebugger->stepOver();
        return;
    }
    if (isPythonDebugging()) {
        m_pythonDebugger->stepOver();
        return;
    }
    if (m_debugManager && m_isDebugging) {
        m_debugManager->stepOver();
    }
}

void MainWindow::onStepInto()
{
    if (isCppDebugging()) {
        m_cppDebugger->stepInto();
        return;
    }
    if (isPythonDebugging()) {
        m_pythonDebugger->stepInto();
        return;
    }
    if (m_debugManager && m_isDebugging) {
        m_debugManager->stepInto();
    }
}

void MainWindow::onStepOut()
{
    if (isCppDebugging()) {
        m_cppDebugger->stepOut();
        return;
    }
    if (isPythonDebugging()) {
        m_pythonDebugger->stepOut();
        return;
    }
    if (m_debugManager && m_isDebugging) {
        m_debugManager->stepOut();
    }
}

void MainWindow::onOutputPanelContextMenu(const QPoint& pos)
{
    QMenu* menu = new QMenu(this);

    // 复制（如果选中了文本才启用）
    QAction* copyAction = new QAction("复制", this);
    copyAction->setShortcut(QKeySequence::Copy);
    copyAction->setEnabled(d->outputPanel->textCursor().hasSelection());
    connect(copyAction, &QAction::triggered, d->outputPanel, &QTextEdit::copy);
    menu->addAction(copyAction);

    menu->addSeparator();

    // 清空
    QAction* clearAction = new QAction("清空", this);
    connect(clearAction, &QAction::triggered, this, &MainWindow::clearOutputPanel);
    menu->addAction(clearAction);

    // 全选
    menu->addSeparator();
    QAction* selectAllAction = new QAction("全选", this);
    selectAllAction->setShortcut(QKeySequence::SelectAll);
    connect(selectAllAction, &QAction::triggered, d->outputPanel, &QTextEdit::selectAll);
    menu->addAction(selectAllAction);

    menu->exec(d->outputPanel->viewport()->mapToGlobal(pos));
    delete menu;
}

void MainWindow::clearOutputPanel()
{
    d->outputPanel->clear();
}

void MainWindow::onContinueDebug()
{
    if (isCppDebugging()) {
        m_cppDebugger->continueExecution();
        return;
    }
    if (isPythonDebugging()) {
        m_pythonDebugger->continueExecution();
        return;
    }
    if (m_debugManager && m_isDebugging) {
        m_debugManager->continueExecution();
    }
}

void MainWindow::onAddBreakpoint()
{
    TextEditor* editor = currentEditor();
    if (!editor) {
        return;
    }

    QString filePath = editor->filePath();
    if (filePath.isEmpty()) {
        d->outputPanel->append("请先保存文件后再添加断点");
        return;
    }

    int line = editor->currentLine();
    m_debugManager->toggleBreakpoint(filePath, line);
}

// ==================== 调试信号处理 ====================

void MainWindow::onDebugStateChanged(DebugState state)
{
    switch (state) {
    case DebugState::Stopped:
        m_isDebugging = false;
        if (m_statusLabel) {
            m_statusLabel->setText("调试已停止");
        }
        if (m_bottomTabWidget) {
            int debugIndex = m_bottomTabWidget->indexOf(m_debugPanel);
            if (debugIndex >= 0) {
                m_bottomTabWidget->setTabVisible(debugIndex, false);
            }
        }
        switchToOutputTab();
        break;
    case DebugState::Starting:
        m_isDebugging = true;
        if (m_statusLabel) {
            m_statusLabel->setText("正在启动调试...");
        }
        if (m_bottomTabWidget) {
            int debugIndex = m_bottomTabWidget->indexOf(m_debugPanel);
            if (debugIndex >= 0) {
                m_bottomTabWidget->setTabVisible(debugIndex, true);
                m_bottomTabWidget->setCurrentIndex(debugIndex);
            }
        }
        break;
    case DebugState::Running:
        m_isDebugging = true;
        if (m_statusLabel) {
            m_statusLabel->setText("调试运行中");
        }
        break;
    case DebugState::Paused:
        m_isDebugging = true;
        if (m_statusLabel) {
            m_statusLabel->setText("调试暂停（断点命中）");
        }
        switchToDebugTab();
        break;
    case DebugState::Stepping:
        if (m_statusLabel) {
            m_statusLabel->setText("单步执行中...");
        }
        break;
    case DebugState::Finished:
        m_isDebugging = false;
        if (m_statusLabel) {
            m_statusLabel->setText("程序执行完成");
        }
        // 清除高亮
        if (TextEditor* editor = currentEditor()) {
            editor->clearCurrentDebugLine();
        }
        updateDebugActions();
        break;
    case DebugState::Error:
        m_isDebugging = false;
        if (m_statusLabel) {
            m_statusLabel->setText("调试错误");
        }
        if (TextEditor* editor = currentEditor()) {
            editor->clearCurrentDebugLine();
        }
        break;
    }

    updateDebugActions();
}

void MainWindow::onBreakpointHit(const QString& filePath, int lineNumber)
{
    // qDebug() << "=== onBreakpointHit called ===";
    // qDebug() << "filePath:" << filePath << "lineNumber:" << lineNumber;

    QString absoluteFilePath = resolveDebuggerPath(filePath);

    // d->outputPanel->append(QString("断点命中: %1:%2")
    //                            .arg(absoluteFilePath)
    //                            .arg(lineNumber));

    openFile(absoluteFilePath);
    TextEditor* editor = currentEditor();
    if (editor) {
        editor->gotoLine(lineNumber);
        editor->setCurrentDebugLine(lineNumber);
        editor->setFocus();
    }

    updateDebugPanels();
}

void MainWindow::onProgramStopped(const QString& filePath, int lineNumber, const QString& reason)
{
    // qDebug() << "=== onProgramStopped called ===";
    // qDebug() << "filePath:" << filePath << "lineNumber:" << lineNumber << "reason:" << reason;

    QString absoluteFilePath = resolveDebuggerPath(filePath);

    // d->outputPanel->append(QString("程序停止: %1 (%2:%3)")
    //                            .arg(reason)
    //                            .arg(absoluteFilePath)
    //                            .arg(lineNumber));

    if (!absoluteFilePath.isEmpty() && QFile::exists(absoluteFilePath)) {
        openFile(absoluteFilePath);
        TextEditor* editor = currentEditor();
        if (editor && lineNumber > 0) {
            editor->gotoLine(lineNumber);
            editor->setCurrentDebugLine(lineNumber);
            editor->setFocus();
        }
    }

    updateDebugPanels();
}

void MainWindow::onProgramExited(int exitCode)
{
    d->outputPanel->append(QString("程序退出，退出码: %1").arg(exitCode));
    m_isDebugging = false;

    // 清除高亮
    TextEditor* editor = currentEditor();
    if (editor) {
        editor->clearCurrentDebugLine();
    }

    updateDebugActions();
}

void MainWindow::onDebugError(const QString& error)
{
    d->outputPanel->append(QString("调试错误: %1").arg(error));
    m_isDebugging = false;
    updateDebugActions();
}

// ==================== 调试控制方法 ====================

bool MainWindow::startDebugging()
{
    // qDebug() << "=== startDebugging called ===";

    TextEditor* editor = currentEditor();
    if (!editor) {
        d->outputPanel->append("请先打开一个文件再开始调试");
        return false;
    }

    QString sourceFile = editor->filePath();
    if (sourceFile.isEmpty()) {
        d->outputPanel->append("请先保存文件再开始调试");
        return false;
    }
    // qDebug() << "Source file:" << sourceFile;

    if (editor->isModified()) {
        d->outputPanel->append("文件已修改，正在编译...");
        onCompile();
    }

    QString executablePath = findExecutableForSource(sourceFile);
    // qDebug() << "Executable path:" << executablePath;

    if (executablePath.isEmpty()) {
        d->outputPanel->append("未找到可执行文件，请等待编译");
        onCompile();
        // return false;
    }

    QFileInfo exeInfo(executablePath);
    if (!exeInfo.exists()) {
        d->outputPanel->append("可执行文件不存在: " + executablePath);
        return false;
    }
    if (!exeInfo.isExecutable()) {
        d->outputPanel->append("文件不可执行: " + executablePath);
        return false;
    }

    QList<Breakpoint> breakpoints = m_debugManager->getBreakpoints();
    // qDebug() << "Current breakpoints count:" << breakpoints.size();
    // for (const Breakpoint& bp : breakpoints) {
        // qDebug() << "  -" << bp.filePath << ":" << bp.lineNumber;
    // }

    DebugConfig config;
    config.executablePath = executablePath;
    config.workingDirectory = QFileInfo(executablePath).absolutePath();
    config.stopOnEntry = true;

    // qDebug() << "Starting debugger with executable:" << config.executablePath;

    bool success = m_debugManager->startDebugging(config);
    if (success) {
        m_isDebugging = true;
        d->outputPanel->append("调试已启动");
        updateDebugActions();
    } else {
        d->outputPanel->append("启动调试失败");
    }

    return success;
}

void MainWindow::stopDebugging()
{
    if (m_debugManager && m_isDebugging) {
        m_debugManager->stopDebugging();
    }
}

// ==================== 检查编译文件与当前代码是否相符 ====================
bool MainWindow::isSourceNewerThanExecutable(const QString& sourceFile, const QString& executablePath)
{
    QFileInfo sourceInfo(sourceFile);
    QFileInfo exeInfo(executablePath);

    if (!sourceInfo.exists()) {
        qWarning() << "源文件不存在:" << sourceFile;
        return false;
    }

    if (!exeInfo.exists()) {
        // 可执行文件不存在，需要编译
        return true;
    }

    // 比较最后修改时间
    return sourceInfo.lastModified() > exeInfo.lastModified();
}

bool MainWindow::needsRecompile(const QString& sourceFile, const QString& executablePath)
{
    // 可执行文件不存在，需要编译
    if (executablePath.isEmpty() || !QFile::exists(executablePath)) {
        return true;
    }

    // 源文件比可执行文件新，需要重新编译
    if (isSourceNewerThanExecutable(sourceFile, executablePath)) {
        return true;
    }

    return false;
}

// ==================== 调试辅助方法 ====================

void MainWindow::updateDebugActions()
{
    bool isDebugging = m_isDebugging;

    // 更新菜单动作
    if (m_startDebugAction) {
        m_startDebugAction->setEnabled(!isDebugging);
    }
    if (m_stopDebugAction) {
        m_stopDebugAction->setEnabled(isDebugging);
    }
    if (m_stepOverAction) {
        m_stepOverAction->setEnabled(isDebugging);
    }
    if (m_stepIntoAction) {
        m_stepIntoAction->setEnabled(isDebugging);
    }
    if (m_stepOutAction) {
        m_stepOutAction->setEnabled(isDebugging);
    }
    if (m_continueAction) {
        m_continueAction->setEnabled(isDebugging);
    }

    // 更新调试面板状态
    if (m_debugPanel) {
        m_debugPanel->setDebuggingState(isDebugging);
    }
}

void MainWindow::updateDebugPanels()
{
    if (!m_debugPanel) return;

    // 更新调用栈和变量
    if (!m_callStack.isEmpty()) {
        m_debugPanel->updateCallStack(m_callStack);
    }
    if (!m_localVariables.isEmpty()) {
        m_debugPanel->updateVariables(m_localVariables);
    }

    // 同时输出到输出面板（方便查看）
    if (!m_callStack.isEmpty()) {
        d->outputPanel->append("=== 调用栈 ===");
        for (const StackFrame& frame : m_callStack) {
            d->outputPanel->append(QString("  #%1 %2 at %3:%4")
                                       .arg(frame.level)
                                       .arg(frame.functionName)
                                       .arg(frame.filePath)
                                       .arg(frame.lineNumber));
        }
    }

    if (!m_localVariables.isEmpty()) {
        d->outputPanel->append("=== 局部变量 ===");
        for (const DebugVariable& var : m_localVariables) {
            d->outputPanel->append(QString("  %1 (%2) = %3")
                                       .arg(var.name)
                                       .arg(var.type)
                                       .arg(var.value));
        }
    }
}

void MainWindow::switchToDebugTab()
{
    if (m_bottomTabWidget && m_debugPanel) {
        int debugIndex = m_bottomTabWidget->indexOf(m_debugPanel);
        if (debugIndex >= 0) {
            m_bottomTabWidget->setCurrentIndex(debugIndex);
        }
    }
}

void MainWindow::switchToOutputTab()
{
    if (m_bottomTabWidget && d->outputPanel) {
        int index = m_bottomTabWidget->indexOf(d->outputPanel);
        if (index >= 0) {
            m_bottomTabWidget->setCurrentIndex(index);
        }
    }
}

void MainWindow::switchToTerminalTab()
{
    if (m_bottomTabWidget && m_terminalPanel) {
        int terminalIndex = m_bottomTabWidget->indexOf(m_terminalPanel);
        if (terminalIndex >= 0) {
            m_bottomTabWidget->setCurrentIndex(terminalIndex);
        }
    }
}

void MainWindow::showFileHistoryDialog(const QString &filePath)
{
    if (!m_localHistory || filePath.isEmpty()) {
        return;
    }

    // 获取历史版本列表
    QList<HistoryVersion> versions = m_localHistory->getVersions(filePath);
    if (versions.isEmpty()) {
        QMessageBox::information(this, "本地历史",
                                 "该文件没有历史记录。\n"
                                 "保存文件时会自动创建历史版本。");
        return;
    }

    // 创建对话框
    QDialog dialog(this);
    dialog.setWindowTitle(QString("本地历史 - %1").arg(QFileInfo(filePath).fileName()));
    dialog.resize(900, 500);

    // 主布局：水平分割
    QHBoxLayout *mainLayout = new QHBoxLayout(&dialog);

    // ---------- 左侧：版本列表 ----------
    QWidget *leftWidget = new QWidget;
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *infoLabel = new QLabel(QString("共 %1 个版本").arg(versions.size()));
    leftLayout->addWidget(infoLabel);

    QListWidget *listWidget = new QListWidget;
    listWidget->setAlternatingRowColors(false);
    // 按版本号从高到低显示（最新的在前）
    for (int i = versions.size() - 1; i >= 0; --i) {
        const HistoryVersion &v = versions[i];
        QString displayText = QString("v%1  %2  %3  %4")
                                  .arg(v.version)
                                  .arg(v.displayTime())
                                  .arg(v.displaySize())
                                  .arg(v.comment);
        QListWidgetItem *item = new QListWidgetItem(displayText, listWidget);
        item->setData(Qt::UserRole, v.version);
    }
    leftLayout->addWidget(listWidget);

    // 按钮：恢复、关闭
    QHBoxLayout *btnLayout = new QHBoxLayout;
    QPushButton *restoreBtn = new QPushButton("恢复到选中版本");
    QPushButton *closeBtn = new QPushButton("关闭");
    btnLayout->addWidget(restoreBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    leftLayout->addLayout(btnLayout);

    mainLayout->addWidget(leftWidget, 1); // 左侧占 1 份

    // ---------- 右侧：差异显示 ----------
    QWidget *rightWidget = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *diffLabel = new QLabel("差异对比（选中版本 vs 当前文件）");
    rightLayout->addWidget(diffLabel);

    QTextEdit *diffEdit = new QTextEdit;
    diffEdit->setReadOnly(true);
    diffEdit->setFont(QFont("Consolas", 11));
    diffEdit->setStyleSheet("QTextEdit { background-color: #1e1e1e; color: #e0e0e0; border: none; }");
    rightLayout->addWidget(diffEdit);

    mainLayout->addWidget(rightWidget, 2); // 右侧占 2 份

    // ---------- 信号连接 ----------
    // 获取当前文件内容（从编辑器或磁盘）
    auto getCurrentContent = [this, filePath]() -> QString {
        // 先查找是否在编辑器中打开
        for (int i = 0; i < d->editorTabs->count(); ++i) {
            TextEditor *editor = qobject_cast<TextEditor*>(d->editorTabs->widget(i));
            if (editor && editor->filePath() == filePath) {
                return editor->toPlainText();
            }
        }
        // 否则从磁盘读取
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            return stream.readAll();
        }
        return QString();
    };

    // 更新差异显示
    auto updateDiff = [&](int version) {
        QString content = m_localHistory->getVersionContent(filePath, version);
        if (content.isNull()) {
            diffEdit->setPlainText("无法读取版本内容");
            return;
        }
        QString currentContent = getCurrentContent();
        QStringList oldLines = content.split('\n');
        QStringList newLines = currentContent.split('\n');
        QList<DiffLine> diff = computeDiff(oldLines, newLines);
        QString html = diffToHtml(diff);
        diffEdit->setHtml(html);
    };

    // 列表项选中时更新差异
    connect(listWidget, &QListWidget::currentItemChanged, [&](QListWidgetItem *item, QListWidgetItem *) {
        if (!item) return;
        int ver = item->data(Qt::UserRole).toInt();
        updateDiff(ver);
    });

    // 默认选中第一个（最新版本）
    if (listWidget->count() > 0) {
        listWidget->setCurrentRow(0);
    }

    // 恢复按钮
    connect(restoreBtn, &QPushButton::clicked, [&]() {
        QListWidgetItem *item = listWidget->currentItem();
        if (!item) {
            QMessageBox::information(&dialog, "提示", "请先选择一个版本");
            return;
        }
        int version = item->data(Qt::UserRole).toInt();
        QString content = m_localHistory->restoreVersion(filePath, version);
        if (content.isNull()) {
            QMessageBox::warning(&dialog, "恢复失败", "无法读取版本内容");
            return;
        }

        // 更新编辑器或文件
        for (int i = 0; i < d->editorTabs->count(); ++i) {
            TextEditor *editor = qobject_cast<TextEditor*>(d->editorTabs->widget(i));
            if (editor && editor->filePath() == filePath) {
                editor->setPlainText(content);
                editor->document()->setModified(true);
                QMessageBox::information(&dialog, "恢复成功", QString("已恢复到版本 %1").arg(version));
                dialog.accept();
                return;
            }
        }
        // 若未打开，直接写入文件
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << content;
            file.close();
            openFile(filePath);
            QMessageBox::information(&dialog, "恢复成功", QString("已恢复到版本 %1").arg(version));
            dialog.accept();
        } else {
            QMessageBox::warning(&dialog, "恢复失败", "无法写入文件");
        }
    });

    connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    // 双击列表项也恢复（可选）
    connect(listWidget, &QListWidget::itemDoubleClicked, [&](QListWidgetItem *item) {
        restoreBtn->click();
    });

    dialog.exec();
}

void MainWindow::saveSession()
{
    if (d->currentProjectPath.isEmpty()) {
        qWarning() << "[Session] 当前项目路径为空，跳过保存";
        return;
    }

    // ---------- 1. 确保 .SprintIDE 目录存在 ----------
    const QString sprintDirPath =
        QDir(d->currentProjectPath).absoluteFilePath(".SprintIDE");

    if (!QDir(sprintDirPath).exists()) {
        if (!QDir().mkpath(sprintDirPath)) {
            qWarning() << "[Session] 无法创建目录:" << sprintDirPath;
            return;
        }
    }

    // ---------- 2. 组装 JSON ----------
    QJsonObject session;

    // 2.1 打开的文件列表
    QJsonArray openFiles;
    for (int i = 0; i < d->editorTabs->count(); ++i) {
        TextEditor* editor = qobject_cast<TextEditor*>(d->editorTabs->widget(i));
        if (editor && !editor->filePath().isEmpty()) {
            QJsonObject fileObj;
            fileObj["path"]   = editor->filePath();
            fileObj["line"]   = editor->currentLine();
            fileObj["column"] = editor->currentColumn();
            openFiles.append(fileObj);
        }
    }
    session["openFiles"]   = openFiles;
    session["activeIndex"] = d->editorTabs->currentIndex();

    // 2.2 项目树展开的路径
    QJsonArray expandedPaths;
    if (d->projectPanel) {
        QModelIndex root = d->projectPanel->rootIndex();
        saveExpandedPaths(expandedPaths, root);
    }
    session["expandedPaths"] = expandedPaths;

    // ============================================================
    // 2.3 ★ 保存断点
    // ------------------------------------------------------------
    // Breakpoint 有 filePath / lineNumber / enabled / condition 四个
    // 关键字段，全部写入。hitCount 是运行时统计，不用持久化。
    // ============================================================
    QJsonArray breakpointsArr;
    if (m_debugManager) {
        const QList<Breakpoint> breakpoints = m_debugManager->getBreakpoints();
        for (const Breakpoint& bp : breakpoints) {
            QJsonObject bpObj;
            bpObj["file"]      = bp.filePath;
            bpObj["line"]      = bp.lineNumber;
            bpObj["enabled"]   = bp.enabled;
            bpObj["condition"] = bp.condition;
            breakpointsArr.append(bpObj);
        }
    }
    session["breakpoints"] = breakpointsArr;

    // ---------- 3. 写入文件 ----------
    const QString sessionFile = QDir(sprintDirPath).absoluteFilePath("session.json");

    QFile file(sessionFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[Session] 无法写入会话文件:" << sessionFile
                   << "错误:" << file.errorString();
        return;
    }

    const QByteArray json = QJsonDocument(session).toJson(QJsonDocument::Indented);
    if (file.write(json) == -1) {
        qWarning() << "[Session] 写入失败:" << file.errorString();
    }
    file.close();

    qDebug() << "[Session] 已保存到:" << sessionFile
             << "打开文件数:" << openFiles.size()
             << "断点数:"   << breakpointsArr.size();
}

void MainWindow::restoreSession()
{
    if (d->currentProjectPath.isEmpty()) {
        qWarning() << "[Session] 当前项目路径为空，跳过恢复";
        return;
    }

    const QString sprintDirPath =
        QDir(d->currentProjectPath).absoluteFilePath(".SprintIDE");
    const QString sessionFile =
        QDir(sprintDirPath).absoluteFilePath("session.json");

    if (!QFile::exists(sessionFile)) {
        qDebug() << "[Session] 未找到会话文件:" << sessionFile;
        return;
    }

    QFile file(sessionFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[Session] 无法读取会话文件:" << sessionFile;
        return;
    }

    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[Session] 解析会话文件失败:" << parseError.errorString();
        return;
    }

    QJsonObject session = doc.object();
    qDebug() << "[Session] 恢复会话，打开文件数:"
             << session["openFiles"].toArray().size()
             << "断点数:"
             << session["breakpoints"].toArray().size();

    // ---------- 1. 恢复打开的文件 ----------
    const QJsonArray openFiles = session["openFiles"].toArray();
    for (const QJsonValue &val : openFiles) {
        const QJsonObject fileObj = val.toObject();
        const QString path = fileObj["path"].toString();
        if (!QFile::exists(path)) continue;

        openFile(path);

        TextEditor* editor = currentEditor();
        if (editor) {
            const int line   = fileObj["line"].toInt(1);
            const int column = fileObj["column"].toInt(1);
            editor->gotoLine(line);
            Q_UNUSED(column);
        }
    }

    // ============================================================
    // 2. ★ 恢复断点
    // ------------------------------------------------------------
    //  顺序很重要：
    //    1) addBreakpoint(...) → 写入 DebugManager（断点的权威数据源）
    //    2) setBreakpointEnabled(...) → 修正启用/禁用状态
    //    3) 同步到对应编辑器 → 显示红点
    // ============================================================
    const QJsonArray breakpointsArr = session["breakpoints"].toArray();
    if (m_debugManager && !breakpointsArr.isEmpty()) {
        int restored = 0;
        for (const QJsonValue& val : breakpointsArr) {
            const QJsonObject bpObj = val.toObject();
            const QString filePath  = bpObj["file"].toString();
            const int     line      = bpObj["line"].toInt();
            const bool    enabled   = bpObj["enabled"].toBool(true);
            const QString condition = bpObj["condition"].toString();

            if (filePath.isEmpty() || line <= 0) continue;

            // ---- 2.1 写入 DebugManager（带条件） ----
            if (!m_debugManager->hasBreakpoint(filePath, line)) {
                m_debugManager->addBreakpoint(filePath, line, condition);
            }

            // ---- 2.2 恢复启用/禁用状态 ----
            // addBreakpoint 默认是 enabled=true，因此只需在禁用时修正
            if (!enabled) {
                m_debugManager->setBreakpointEnabled(filePath, line, false);
            }

            // ---- 2.3 同步到已打开的编辑器 ----
            TextEditor* ed = findEditorByFilePath(filePath);
            if (ed) {
                ed->setBreakpoint(line, enabled);   // 禁用断点通常渲染成空心红点
                ed->viewport()->update();
            }
            ++restored;
        }
        qDebug() << "[Session] 已恢复断点数:" << restored;
    }

    // ---------- 3. 恢复活动标签 ----------
    const int activeIndex = session["activeIndex"].toInt(0);
    if (activeIndex >= 0 && activeIndex < d->editorTabs->count()) {
        d->editorTabs->setCurrentIndex(activeIndex);
    }

    // ---------- 4. 恢复项目树展开状态 ----------
    const QJsonArray expandedPaths = session["expandedPaths"].toArray();
    if (!expandedPaths.isEmpty()) {
        QTimer::singleShot(500, this, [this, expandedPaths]() {
            restoreExpandedPaths(expandedPaths);
        });
    }
}

void MainWindow::saveExpandedPaths(QJsonArray &paths, const QModelIndex &parent)
{
    if (!parent.isValid()) return;

    QModelIndexList children;
    for (int row = 0; row < d->projectPanel->model()->rowCount(parent); ++row) {
        QModelIndex child = d->projectPanel->model()->index(row, 0, parent);
        if (d->projectPanel->isExpanded(child)) {
            QString path = d->projectPanel->getFilePath(child); // 需要 ProjectPanel 提供此方法
            if (!path.isEmpty()) {
                paths.append(path);
            }
            // 递归展开子目录
            saveExpandedPaths(paths, child);
        }
    }
}

void MainWindow::restoreExpandedPaths(const QJsonArray &paths)
{
    if (!d->projectPanel) return;

    for (const QJsonValue &val : paths) {
        QString path = val.toString();
        if (path.isEmpty()) continue;

        // 在文件系统模型中查找该路径的索引
        QModelIndex index = d->projectPanel->findIndexByPath(path); // 需要 ProjectPanel 提供
        if (index.isValid()) {
            d->projectPanel->expand(index);
            // 递归展开父级（以确保路径可达）
            QModelIndex parent = index.parent();
            while (parent.isValid()) {
                if (!d->projectPanel->isExpanded(parent)) {
                    d->projectPanel->expand(parent);
                }
                parent = parent.parent();
            }
        }
    }
}

bool MainWindow::startDebuggingWithExecutable(const QString& executablePath)
{
    if (executablePath.isEmpty()) {
        d->outputPanel->append("可执行文件路径为空");
        return false;
    }

    if (!QFile::exists(executablePath)) {
        d->outputPanel->append(QString("可执行文件不存在: %1").arg(executablePath));
        return false;
    }

    QFileInfo exeInfo(executablePath);
    if (!exeInfo.isExecutable()) {
        d->outputPanel->append(QString("文件不可执行: %1").arg(executablePath));
        return false;
    }

    // 同步断点
    syncBreakpointsToDebugger();

    // 构建调试配置
    DebugConfig config;
    config.executablePath = executablePath;
    config.workingDirectory = exeInfo.absolutePath();
    config.stopOnEntry = true;

    // 启动调试
    bool success = m_debugManager->startDebugging(config);
    if (success) {
        m_isDebugging = true;
        d->outputPanel->append(QString("调试已启动: %1").arg(executablePath));
        updateDebugActions();
    } else {
        d->outputPanel->append("启动调试失败");
    }

    return success;
}

bool MainWindow::isPythonDebugging() const
{
    return m_pythonDebugger != nullptr && m_isDebugging;
}

bool MainWindow::isCppDebugging() const
{
    return m_cppDebugger != nullptr && m_isDebugging;
}

void MainWindow::runExecutableAsync(const QString& executablePath)
{
    if (executablePath.isEmpty() || !QFile::exists(executablePath)) {
        d->outputPanel->append("可执行文件不存在: " + executablePath);
        return;
    }

    QString timeStr = QTime::currentTime().toString("hh:mm:ss");
    d->outputPanel->append(QString("<span style='color:#33CC33;'>[%1] 正在运行: %2</span>")
                               .arg(timeStr).arg(executablePath));

    QProcess* process = new QProcess(this);
    process->setProgram(executablePath);
    process->setWorkingDirectory(QFileInfo(executablePath).absolutePath());
    process->setProcessChannelMode(QProcess::MergedChannels);

    connect(process, &QProcess::readyReadStandardOutput, this, [this, process]() {
        QByteArray data = process->readAllStandardOutput();
        QString output = QString::fromLocal8Bit(data);
        if (!output.isEmpty()) {
            d->outputPanel->append(output);
        }
    });
    connect(process, &QProcess::readyReadStandardError, this, [this, process]() {
        QByteArray data = process->readAllStandardError();
        QString output = QString::fromLocal8Bit(data);
        if (!output.isEmpty()) {
            d->outputPanel->append("<span style='color:#FF6B6B;'>"
                                   + output.toHtmlEscaped() + "</span>");
        }
    });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process](int exitCode, QProcess::ExitStatus status) {
                QString t = QTime::currentTime().toString("hh:mm:ss");
                if (status == QProcess::NormalExit) {
                    d->outputPanel->append(
                        QString("<span style='color:#33CC33;'>[%1] 进程结束，退出码: %2</span>")
                            .arg(t).arg(exitCode));
                } else {
                    d->outputPanel->append(
                        QString("<span style='color:#FF6B6B;'>[%1] 进程异常崩溃</span>").arg(t));
                }
                process->deleteLater();
                m_statusLabel->setText("运行完成");
            });

    process->start();
    if (!process->waitForStarted(3000)) {
        d->outputPanel->append("启动失败: " + process->errorString());
        process->deleteLater();
        return;
    }
    m_statusLabel->setText("程序运行中...");
}

// mainwindow.cpp - 实现
QString MainWindow::resolveDebuggerPath(const QString& filePath)
{
    if (filePath.isEmpty()) {
        return filePath;
    }

    // 如果是绝对路径，直接返回
    if (QFileInfo(filePath).isAbsolute() && QFile::exists(filePath)) {
        return filePath;
    }

    // 尝试从项目路径解析
    if (!d->currentProjectPath.isEmpty()) {
        QStringList possiblePaths = {
            d->currentProjectPath + "/src/" + filePath,
            d->currentProjectPath + "/" + filePath,
            d->currentProjectPath + "/../" + filePath
        };
        if (filePath.contains("src/") || filePath.startsWith("src/")) {
            possiblePaths.prepend(d->currentProjectPath + "/" + filePath);
        }
        for (const QString& path : possiblePaths) {
            if (QFile::exists(path)) {
                return path;
            }
        }
    }

    // 检查当前编辑器
    TextEditor* editor = currentEditor();
    if (editor && !editor->filePath().isEmpty()) {
        QString currentFile = editor->filePath();
        if (QFileInfo(currentFile).fileName() == QFileInfo(filePath).fileName()) {
            return currentFile;
        }
    }

    // 返回原路径（尝试打开时也会失败，但至少不会崩溃）
    return filePath;
}

// ==================== 菜单槽函数实现 ====================

void MainWindow::onNewFile()
{
    // 1. 获取目标目录（优先使用项目面板中选中的目录）
    QString targetDir;
    if (d->projectPanel) {
        QString selectedPath = d->projectPanel->selectedFilePath();
        if (!selectedPath.isEmpty()) {
            QFileInfo info(selectedPath);
            if (info.isDir()) {
                targetDir = selectedPath;          // 选中文件夹
            } else {
                targetDir = info.absolutePath();   // 选中文件，取其父目录
            }
        }
    }

    // 2. 如果项目面板没有选中项，回退到项目根目录或当前目录
    if (targetDir.isEmpty()) {
        if (!d->currentProjectPath.isEmpty()) {
            targetDir = d->currentProjectPath;
        } else {
            targetDir = QDir::currentPath();
        }
    }

    // 3. 调用现有的创建函数，传入目标目录
    //    createNewFileWithName 会弹出输入框让用户输入文件名
    //    默认文件名 "main.cpp"，标题 "新建文件"
    createNewFileWithName("new_file.cpp", "新建文件", targetDir);
}

void MainWindow::onOpenFile()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "打开文件",
        "",
        "C++ 文件 (*.cpp *.h *.hpp *.c *.cc);;所有文件 (*.*)"
        );

    if (!fileName.isEmpty()) {
        openFile(fileName);
    }
}

void MainWindow::onSaveFile()
{
    TextEditor* editor = currentEditor();
    if (editor) {
        if (editor->saveFile()) {
            // d->outputPanel->append(QString("已保存: %1").arg(editor->filePath()));
        } else {
            QMessageBox::warning(this, "保存错误", "无法保存文件。");
        }
    } else {
        QMessageBox::information(this, "提示", "没有打开的文件可以保存。");
    }
}

void MainWindow::onSaveAs()
{
    TextEditor* editor = currentEditor();
    if (editor) {
        QString fileName = QFileDialog::getSaveFileName(
            this,
            "另存为",
            editor->filePath().isEmpty() ? "" : editor->filePath(),
            "C++ 文件 (*.cpp *.h *.hpp);;所有文件 (*.*)"
            );

        if (!fileName.isEmpty()) {
            if (editor->saveAs(fileName)) {
                d->outputPanel->append(QString("已另存为: %1").arg(fileName));
                updateTabTitle(d->editorTabs->currentIndex(), editor->fileName());
            }
        }
    } else {
        QMessageBox::information(this, "提示", "没有打开的文件可以另存。");
    }
}

void MainWindow::onCloseFile()
{
    int index = d->editorTabs->currentIndex();
    if (index >= 0) {
        closeEditorTab(index);
    }
}

void MainWindow::onExit()
{
    close();
}

void MainWindow::onUndo()
{
    TextEditor* editor = currentEditor();
    if (editor) {
        editor->undo();
    }
}

void MainWindow::onRedo()
{
    TextEditor* editor = currentEditor();
    if (editor) {
        editor->redo();
    }
}

void MainWindow::onCut()
{
    TextEditor* editor = currentEditor();
    if (editor) {
        editor->cut();
    }
}

void MainWindow::onCopy()
{
    TextEditor* editor = currentEditor();
    if (editor) {
        editor->copy();
    }
}

void MainWindow::onPaste()
{
    TextEditor* editor = currentEditor();
    if (editor) {
        editor->paste();
    }
}



// ==================== 查找/替换槽函数 ====================
// 取消使用FindDialog查找/替换，使用TextEdit中的内联查找和替换
// void MainWindow::onFind()
// {
//     TextEditor* editor = currentEditor();
//     if (!editor) {
//         QMessageBox::information(this, "提示", "请先打开一个文件进行查找。");
//         return;
//     }

//     if (!m_findDialog) {
//         m_findDialog = new FindDialog(editor, this);  // editor 是 TextEditor*，可转换为 QPlainTextEdit*
//         connect(m_findDialog, &FindDialog::findNext,
//                 this, &MainWindow::onFindNext);
//         connect(m_findDialog, &FindDialog::findPrevious,
//                 this, &MainWindow::onFindPrevious);
//     }

//     // 如果有选中的文本，自动填入查找框
//     if (editor->textCursor().hasSelection()) {
//         QString selected = editor->textCursor().selectedText();
//         QLineEdit* findEdit = m_findDialog->findChild<QLineEdit*>();
//         if (findEdit) {
//             findEdit->setText(selected);
//         }
//     }

//     m_findDialog->show();
//     m_findDialog->raise();
//     m_findDialog->activateWindow();
// }

// void MainWindow::onReplace()
// {
//     TextEditor* editor = currentEditor();
//     if (!editor) {
//         QMessageBox::information(this, "提示", "请先打开一个文件进行替换。");
//         return;
//     }

//     if (!m_replaceDialog) {
//         m_replaceDialog = new ReplaceDialog(editor, this);
//         connect(m_replaceDialog, &ReplaceDialog::findNext,
//                 this, &MainWindow::onFindNext);
//         connect(m_replaceDialog, &ReplaceDialog::replaceCurrent,
//                 this, &MainWindow::onReplaceCurrent);
//         connect(m_replaceDialog, &ReplaceDialog::replaceAll,
//                 this, &MainWindow::onReplaceAll);
//     }

//     // 如果有选中的文本，自动填入查找框
//     if (editor->textCursor().hasSelection()) {
//         QString selected = editor->textCursor().selectedText();
//         QLineEdit* findEdit = m_replaceDialog->findChild<QLineEdit*>();
//         if (findEdit) {
//             findEdit->setText(selected);
//         }
//     }

//     m_replaceDialog->show();
//     m_replaceDialog->raise();
//     m_replaceDialog->activateWindow();
// }

// ========== 查找 ==========
void MainWindow::onFind()
{
    TextEditor* editor = currentEditor();
    if (!editor) {
        QMessageBox::information(this, "提示", "请先打开一个文件进行查找。");
        return;
    }

    // 使用内联查找栏
    editor->showFindBar();
}

// ========== 替换 ==========
void MainWindow::onReplace()
{
    TextEditor* editor = currentEditor();
    if (!editor) {
        QMessageBox::information(this, "提示", "请先打开一个文件进行替换。");
        return;
    }

    // 使用内联查找栏 + 自动展开替换模式
    editor->showFindBarWithReplace();
}

void MainWindow::onFindNext(const QString& text, bool caseSensitive, bool wholeWord)
{
    TextEditor* editor = currentEditor();
    if (!editor) return;

    QTextDocument* doc = editor->document();
    QTextCursor cursor = editor->textCursor();

    QTextDocument::FindFlags flags;
    if (caseSensitive) flags |= QTextDocument::FindCaseSensitively;
    if (wholeWord) flags |= QTextDocument::FindWholeWords;

    QTextCursor found = doc->find(text, cursor, flags);
    if (found.isNull()) {
        // 从头开始找
        cursor.movePosition(QTextCursor::Start);
        found = doc->find(text, cursor, flags);
        if (found.isNull()) {
            m_statusLabel->setText("未找到 \"" + text + "\"");
            return;
        }
    }

    editor->setTextCursor(found);
    m_lastFindCursor = found;
    m_statusLabel->setText("找到 \"" + text + "\"");
}

void MainWindow::onFindPrevious(const QString& text, bool caseSensitive, bool wholeWord)
{
    TextEditor* editor = currentEditor();
    if (!editor) return;

    QTextDocument* doc = editor->document();
    QTextCursor cursor = editor->textCursor();

    QTextDocument::FindFlags flags;
    flags |= QTextDocument::FindBackward;
    if (caseSensitive) flags |= QTextDocument::FindCaseSensitively;
    if (wholeWord) flags |= QTextDocument::FindWholeWords;

    QTextCursor found = doc->find(text, cursor, flags);
    if (found.isNull()) {
        cursor.movePosition(QTextCursor::End);
        found = doc->find(text, cursor, flags);
        if (found.isNull()) {
            m_statusLabel->setText("未找到 \"" + text + "\"");
            return;
        }
    }

    editor->setTextCursor(found);
    m_lastFindCursor = found;
    m_statusLabel->setText("找到 \"" + text + "\"");
}

void MainWindow::onReplaceCurrent(const QString& findText, const QString& replaceText,
                                  bool caseSensitive, bool wholeWord)
{
    TextEditor* editor = currentEditor();
    if (!editor) return;

    QTextCursor cursor = editor->textCursor();

    if (cursor.hasSelection()) {
        QString selected = cursor.selectedText();
        if (selected == findText ||
            (caseSensitive ? selected == findText : selected.toLower() == findText.toLower())) {
            cursor.insertText(replaceText);
            cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, replaceText.length());
            editor->setTextCursor(cursor);
        }
    }

    // 自动查找下一个
    onFindNext(findText, caseSensitive, wholeWord);
}

void MainWindow::onReplaceAll(const QString& findText, const QString& replaceText,
                              bool caseSensitive, bool wholeWord)
{
    TextEditor* editor = currentEditor();
    if (!editor) return;

    QTextDocument* doc = editor->document();
    QTextCursor cursor(doc);

    QTextDocument::FindFlags flags;
    if (caseSensitive) flags |= QTextDocument::FindCaseSensitively;
    if (wholeWord) flags |= QTextDocument::FindWholeWords;

    int count = 0;
    cursor.movePosition(QTextCursor::Start);

    while (true) {
        QTextCursor found = doc->find(findText, cursor, flags);
        if (found.isNull()) break;
        found.insertText(replaceText);
        count++;
        cursor = found;
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, replaceText.length());
    }

    m_statusLabel->setText(QString("替换完成，共替换 %1 处").arg(count));
    d->outputPanel->append(QString("替换完成，共替换 %1 处").arg(count));
}

void MainWindow::onDebuggerContinued()
{
    // 这个槽函数在调试器继续运行时被调用
    // 主要任务是更新 UI 状态
    d->outputPanel->append("调试器已继续运行");
    // 清除上一处停止的行高亮，避免两个绿色行
    if (TextEditor* editor = currentEditor()) {
        editor->clearCurrentDebugLine();
    }

    updateDebugActions(); // 这个函数会根据 m_isDebugging 状态更新按钮
}

void MainWindow::onComment()
{
    TextEditor* editor = currentEditor();
    if (editor) {
        editor->commentSelection();
    }
}

void MainWindow::onGotoLine()
{
    TextEditor* editor = currentEditor();
    if (editor) {
        bool ok;
        int line = QInputDialog::getInt(
            this,
            "跳转到行",
            "请输入行号:", 1, 1, editor->lineCount(), 1, &ok
            );

        if (ok) {
            editor->gotoLine(line);
        }
    } else {
        QMessageBox::information(this, "提示", "请先打开一个文件进行跳转。");
    }
}

void MainWindow::onFormatCode()
{
    TextEditor* editor = currentEditor();
    if (!editor) {
        QMessageBox::information(this, "提示", "请先打开一个文件进行格式化。");
        return;
    }

    QString sourceCode = editor->toPlainText();
    if (sourceCode.isEmpty()) {
        d->outputPanel->append("代码为空，无需格式化。");
        return;
    }

    QTemporaryFile tempFile;
    tempFile.setAutoRemove(false);
    if (!tempFile.open()) {
        QMessageBox::warning(this, "错误", "无法创建临时文件。");
        return;
    }

    QString filePath = editor->filePath();
    if (filePath.isEmpty()) {
        QMessageBox::information(this, "提示", "请先保存文件。");
        return;
    }

    // ========== Python 格式化 ==========
    if (filePath.endsWith(".py", Qt::CaseInsensitive)) {
        QString error;
        if (PythonFormatter::isAvailable()) {
            if (PythonFormatter::formatFile(filePath, error)) {
                editor->reloadFile();
                d->outputPanel->append("Python 代码格式化完成。");
            } else {
                d->outputPanel->append("Python 格式化失败: " + error);
            }
        } else {
            QMessageBox::warning(this, "提示",
                                 "未找到 Black 格式化工具。\n请安装: pip install black");
        }
        return;
    }

    QString tempFileName = tempFile.fileName();
    QTextStream out(&tempFile);
    out << sourceCode;
    tempFile.close();

    QString clangFormatPath = findClangFormat();
    if (clangFormatPath.isEmpty()) {
        QMessageBox::warning(this, "错误", "未找到Clang Format工具，请确保已安装。");
        QFile::remove(tempFileName);
        return;
    }

    QStringList args;
    args << "-style=Google" << tempFileName;

    QProcess process;
    process.setProgram(clangFormatPath);
    process.setArguments(args);

    process.start();
    if (!process.waitForFinished(5000)) {
        QMessageBox::warning(this, "错误", "Clang Format执行超时。");
        QFile::remove(tempFileName);
        return;
    }

    QString formattedCode = QString::fromLocal8Bit(process.readAllStandardOutput());
    QString errorOutput = QString::fromLocal8Bit(process.readAllStandardError());

    QFile::remove(tempFileName);

    if (!errorOutput.isEmpty()) {
        d->outputPanel->append(QString("Clang Format错误: %1").arg(errorOutput));
        return;
    }

    if (!formattedCode.isEmpty()) {
        editor->setPlainText(formattedCode);
        d->outputPanel->append("代码格式化完成。");
    } else {
        d->outputPanel->append("格式化后代码为空，可能是输入代码有问题。");
    }
}

void MainWindow::loadAllProjectFiles(const QString& projectPath)
{
    QDir projectDir(projectPath);
    if (!projectDir.exists()) {
        QMessageBox::warning(this, "错误", "项目路径不存在");
        return;
    }

    // 定义支持的文件扩展名（可根据需要扩展）
    QStringList filters;
    filters << "*.cpp" << "*.c" << "*.cc" << "*.cxx"
            << "*.h" << "*.hpp" << "*.hh" << "*.hxx"
            << "*.java" << "*.py" << "*.js" << "*.ts"
            << "*.html" << "*.css" << "*.xml" << "*.json"
            << "*.txt" << "*.md" << "*.cmake" << "*.makefile";

    // 收集所有符合条件的文件路径
    QStringList sourceFiles;
    QStack<QString> directories;
    directories.push(projectPath);

    while (!directories.isEmpty()) {
        QString currentDir = directories.pop();
        QDir dir(currentDir);

        // 添加子目录到栈
        QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& subdir : subdirs) {
            directories.push(dir.absoluteFilePath(subdir));
        }

        // 添加当前目录中的文件
        dir.setNameFilters(filters);
        QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
        for (const QString& file : files) {
            sourceFiles.append(dir.absoluteFilePath(file));
        }
    }

    // 逐个打开文件
    // for (const QString& filePath : sourceFiles) {
    //     openFile(filePath);
    // }
}

void MainWindow::onNewProject()
{
    ProjectCreationDialog dialog(this);

    QString defaultPath;
    if (!d->currentProjectPath.isEmpty()) {
        QFileInfo projectInfo(d->currentProjectPath);
        defaultPath = projectInfo.dir().absolutePath();
    } else {
        defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }

    dialog.setDefaultProjectPath(defaultPath);

    if (dialog.exec() == QDialog::Accepted) {
        ProjectCreationResult result = dialog.getResult();

        if (result.success) {
            d->currentProjectPath = result.projectPath;
            openProject(result.projectPath);

            ProjectConfig projectConfig = dialog.getProjectConfig();

            d->outputPanel->append("==========================================");
            d->outputPanel->append("项目创建成功！");
            d->outputPanel->append(QString("项目名称: %1").arg(result.displayName));
            d->outputPanel->append(QString("项目类型: %1").arg(getProjectTypeString(result.projectType)));
            d->outputPanel->append(QString("项目路径: %1").arg(result.projectPath));
            d->outputPanel->append("==========================================");

            m_statusLabel->setText(QString("已创建项目: %1").arg(result.displayName));

            if (!result.createdFiles.isEmpty()) {
                QString firstCppFile;
                for (const QString& file : result.createdFiles) {
                    if (file.endsWith(".cpp", Qt::CaseInsensitive)) {
                        firstCppFile = file;
                        break;
                    }
                }

                if (!firstCppFile.isEmpty()) {
                    openFile(firstCppFile);
                }
            }

            d->projectPanel->openProject(result.projectPath);

            if (m_terminalPanel) {
                m_terminalPanel->setWorkingDirectory(result.projectPath);
            }

        } else {
            QMessageBox::critical(this, "项目创建失败",
                                  QString("无法创建项目:\n%1").arg(result.errorMessage));
        }
    }


}

QString MainWindow::getProjectTypeString(ProjectType type) const
{
    switch (type) {
    case ProjectType::ConsoleApp:
        return "控制台应用程序";
    case ProjectType::SimpleGame:
        return "简单游戏";
    case ProjectType::GuiApp:
        return "图形界面程序";
    case ProjectType::EmptyProject:
        return "空白项目";
    default:
        return "未知类型";
    }
}

void MainWindow::onOpenProject()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "打开项目文件夹",
        "",
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
        );
    if (!dir.isEmpty()) {
        openProject(dir);
    }
}

void MainWindow::onCloseProject()
{
    closeProject();
}

void MainWindow::onBuildProject()
{
    // ============================================================
    // 第一步：智能获取项目路径
    // ============================================================
    QString projectPath = d->currentProjectPath;

    // 如果当前没有通过 "打开项目" 设置路径，尝试从当前编辑器中推导
    if (projectPath.isEmpty()) {
        TextEditor* editor = currentEditor();
        if (editor) {
            QString filePath = editor->filePath();
            if (!filePath.isEmpty()) {
                QDir dir = QFileInfo(filePath).absoluteDir();
                // 向上查找常见的项目标识文件（pom.xml, build.gradle, gradlew, .git, CMakeLists.txt）
                while (!dir.isRoot()) {
                    if (dir.exists("pom.xml") || dir.exists("build.gradle") ||
                        dir.exists("gradlew") || dir.exists(".git") ||
                        dir.exists("CMakeLists.txt") || dir.exists("Makefile")) {
                        projectPath = dir.absolutePath();
                        break;
                    }
                    dir.cdUp();
                }
                // 如果没找到任何标识，就使用文件所在目录（降级方案）
                if (projectPath.isEmpty()) {
                    projectPath = QFileInfo(filePath).absolutePath();
                }
                // 临时设置，以便后续流程使用
                d->currentProjectPath = projectPath;
            }
        }
    }

    d->outputPanel->clear();
    switchToOutputTab();

    // 如果经过以上步骤仍为空，给出明确提示并退出
    if (projectPath.isEmpty()) {
        QMessageBox::information(this, "提示",
                                 "无法确定项目路径。请先打开一个文件，或通过 \"文件 → 打开项目\" 打开项目根目录。");
        return;
    }

    // ============================================================
    // 第二步：检测项目语言
    // ============================================================
    LanguageType lang = detectProjectLanguage(projectPath);
    if (lang == LanguageType::Unknown) {
        QMessageBox::warning(this, "无法识别项目类型",
                             "未能识别项目语言。请确保项目根目录包含对应的构建文件（如 pom.xml, CMakeLists.txt 等）。");
        return;
    }

    // ============================================================
    // 第三步：加载项目配置与构建任务
    // ============================================================
    ProjectConfig projectConfig;
    QString configFile = projectPath + "/.lightcppproj";
    QStringList errors;
    if (!projectConfig.loadFromFile(configFile, errors)) {
        qWarning() << "Failed to load project config:" << errors.join(", ");
    }

    BuildTask task;
    task.taskId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    task.language = lang;
    task.projectPath = projectPath;

    // ★ C++ 项目：不预设 outputName，让 CppBuilder 从 CMakeLists 里取真正的 target 名
    if (lang == LanguageType::Cpp) {
        task.outputName = QString();
        task.outputDir = "build";
    } else {
        task.outputName = projectConfig.projectName().isEmpty() ?
                              QFileInfo(projectPath).fileName() :
                              projectConfig.projectName();
        task.outputDir = projectConfig.outputPath().isEmpty() ? "bin" : projectConfig.outputPath();
    }
    task.enableDebug = projectConfig.enableDebugInfo();
    task.sdkConfig = projectConfig.sdkEnvironment();

    // ============================================================
    // 第四步（前置）：C++ 项目 —— 只做快速探测，避免重复扫描
    //
    //   注意：不要把 CppProjectModel 完整 load 一遍。真正的 CMake 目标
    //         探测在 CppBuilder::buildWithCMake() 内部会做，这里只打印
    //         一条日志告知用户即可。
    // ============================================================
    if (lang == LanguageType::Cpp) {
        QDir projDir(projectPath);
        if (projDir.exists("CMakeLists.txt")) {
            d->outputPanel->append("C++ 项目构建系统: CMake");
            // task.outputName 留空，让 CppBuilder 内部用 model.defaultTarget() 自动选择
        } else if (projDir.exists("Makefile") ||
                   projDir.exists("makefile") ||
                   projDir.exists("GNUmakefile")) {
            d->outputPanel->append("C++ 项目构建系统: Makefile");
        } else {
            d->outputPanel->append("C++ 项目构建系统: SingleFile");
        }
    }

    // ============================================================
    // 第四步：Java 项目特殊处理（强制刷新类路径和 Spring Boot 标志）
    // ============================================================
    if (lang == LanguageType::Java) {
        // 检查是否存在 pom.xml
        QDir dir(projectPath);
        if (dir.exists("pom.xml")) {
            // 利用已有的 isSpringBootProject 静态方法检测 Spring Boot
            bool isSpringBoot = JavaMavenBuilder::isSpringBootProject(projectPath);
            task.isSpringBoot = isSpringBoot;
            qDebug() << "Java project detected, Spring Boot:" << isSpringBoot;

            // 主动解析项目模型，获取类路径（这样即使 ProjectManager 未初始化也能工作）
            MavenProjectModel model(projectPath);
            if (!model.dependencyJars().isEmpty()) {
                task.classpath = model.classpath();
                task.dependencyJars = model.dependencyJars();
                task.aiCustomPayload["mainClass"] = model.defaultMainClass();
                qDebug() << "Resolved classpath entries:" << task.classpath.size();
            } else {
                qWarning() << "Failed to resolve dependencies for Maven project";
            }
        } else {
            // 尝试通过 ProjectManager 获取模型（兜底）
            ProjectManager* pm = ProjectManager::instance();
            ProjectModel* model = pm->javaProjectModel();
            if (model) {
                task.classpath = model->classpath();
                task.dependencyJars = model->dependencyJars();
                task.aiCustomPayload["mainClass"] = model->defaultMainClass();
            }
        }
    }

    // 如果是 Generic_AI，加载通用配置
    if (lang == LanguageType::Generic_AI) {
        task.aiCustomPayload = projectConfig.genericBuildConfig();
    }

    // ============================================================
    // 第五步：调用编译管理器
    // ============================================================
    m_currentCompileTaskId = m_compileManager->compileTask(task);
    if (m_currentCompileTaskId.isEmpty()) {
        QMessageBox::warning(this, "编译错误", "无法启动编译任务。请检查输出面板获取详细错误信息。");
        return;
    }
}

void MainWindow::onCleanBuild()
{
    if (d->currentProjectPath.isEmpty()) {
        QMessageBox::information(this, "提示", "请先打开一个项目。");
        return;
    }

    // 检测项目语言
    LanguageType lang = detectProjectLanguage(d->currentProjectPath);
    if (lang == LanguageType::Unknown) {
        QMessageBox::warning(this, "无法识别项目类型",
                             "无法自动识别项目语言，请确保项目根目录包含相应的构建文件。");
        return;
    }

    int reply = QMessageBox::question(this, "清理构建",
                                      "确定要清理编译产物吗？这将删除所有生成的文件。",
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }

    // 加载项目配置（获取输出目录等）
    ProjectConfig projectConfig;
    QString configFile = d->currentProjectPath + "/.lightcppproj";
    QStringList errors;
    if (!projectConfig.loadFromFile(configFile, errors)) {
        qWarning() << "Failed to load project config:" << errors.join(", ");
        // 仍可继续
    }

    // 构建 BuildTask
    BuildTask task;
    task.language = lang;
    task.projectPath = d->currentProjectPath;
    task.outputName = projectConfig.projectName().isEmpty() ?
                          QFileInfo(d->currentProjectPath).fileName() :
                          projectConfig.projectName();
    if (lang == LanguageType::Cpp) {
        task.outputName.clear();
    }
    task.outputDir = projectConfig.outputPath().isEmpty() ? "bin" : projectConfig.outputPath();

    // 如果是 Java，使用模型中的输出目录
    if (lang == LanguageType::Java) {
        ProjectManager* pm = ProjectManager::instance();
        ProjectModel* model = pm->javaProjectModel();
        if (model) {
            task.outputDir = model->outputDirectory();
        }
    }

    // CMake 项目：不需要手动 load 模型，CppBuilder 内部会判断
    // 只设置 outputDir 为 "build"，让 cleanWithCMake() 走 CMake clean 路径
    if (lang == LanguageType::Cpp) {
        QDir projDir(d->currentProjectPath);
        if (projDir.exists("CMakeLists.txt")) {
            task.outputDir = "build";
        }
    }

    // 执行清理
    BuildResult result = m_compileManager->cleanTask(task);

    if (result.success) {
        d->outputPanel->append("清理构建完成");
        m_statusLabel->setText("清理构建完成");
    } else {
        QString error = result.errorMessage.isEmpty() ? "清理构建失败" : result.errorMessage;
        d->outputPanel->append("清理构建失败: " + error);
        m_statusLabel->setText("清理构建失败");
    }
}

void MainWindow::onCompile() {
    TextEditor* editor = currentEditor();
    if (!editor) {
        QMessageBox::information(this, "提示", "请先打开一个文件进行编译。");
        return;
    }

    QString filePath = editor->filePath();
    if (filePath.isEmpty()) {
        if (editor->isModified()) {
            if (!editor->saveFile()) {
                QMessageBox::warning(this, "文件未保存", "请先保存文件再进行编译。");
                return;
            }
            filePath = editor->filePath();
        }
        if (filePath.isEmpty()) {
            onSaveAs();
            filePath = editor->filePath();
            if (filePath.isEmpty()) return;
        }
    }

    // 保存文件（如果未保存）
    if (editor->isModified()) {
        if (!editor->saveFile()) {
            QMessageBox::warning(this, "保存错误", "无法保存文件，编译中止。");
            return;
        }
        filePath = editor->filePath();
    }

    // 检测语言
    LanguageType lang = detectLanguageTypeFromPath(filePath);
    // 构造 BuildTask
    BuildTask task;
    task.taskId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    task.language = detectLanguageTypeFromPath(filePath);
    // 项目路径优先使用当前打开的项目，否则用文件所在目录
    task.projectPath = d->currentProjectPath.isEmpty() ?
                           QFileInfo(filePath).absolutePath() :
                           d->currentProjectPath;
    task.sourceFiles << filePath;
    task.outputName = QFileInfo(filePath).completeBaseName();
    task.outputDir = "bin"; // 可配置
    task.enableDebug = true;

    // ========== Python 特殊处理 ==========
    if (lang == LanguageType::Python) {
        auto model = ProjectManager::instance()->pythonProjectModel();
        if (model) {
            // 使用虚拟环境中的 Python
            task.sdkConfig["pythonInterpreter"] = model->interpreterPath();
            task.sdkConfig["venv"] = model->venvPath();
            // 支持依赖安装（从设置读取）
            ConfigManager* cfg = Application::instance().configManager();
            if (cfg && cfg->value("Python/AutoInstallDeps", false).toBool()) {
                task.aiCustomPayload["installDeps"] = true;
            }
        }
    }

    // 读取项目配置中的 SDK 环境
    if (!d->currentProjectPath.isEmpty()) {
        ProjectConfig projConfig;
        QString configFile = d->currentProjectPath + "/.lightcppproj";
        QStringList errors;
        if (projConfig.loadFromFile(configFile, errors)) {
            task.sdkConfig = projConfig.sdkEnvironment();
            // 如果是 Generic_AI，加载通用配置
            if (task.language == LanguageType::Generic_AI) {
                task.aiCustomPayload = projConfig.genericBuildConfig();
            }
        }
    }

    // 调用新接口
    m_currentCompileTaskId = m_compileManager->compileTask(task);
    if (m_currentCompileTaskId.isEmpty()) {
        QMessageBox::warning(this, "编译错误", "无法启动编译任务。");
    }
}

void MainWindow::onRun()
{
    TextEditor* editor = currentEditor();
    if (!editor) {
        QMessageBox::information(this, "提示", "请先打开一个文件进行运行。");
        return;
    }

    QString sourceFile = editor->filePath();
    if (sourceFile.isEmpty()) {
        QMessageBox::warning(this, "运行错误", "请先保存文件再进行运行。");
        return;
    }

    LanguageType lang = detectLanguageTypeFromPath(sourceFile);

    // ========== Python 特殊处理（异步执行） ==========
    if (lang == LanguageType::Python) {
        auto model = ProjectManager::instance()->pythonProjectModel();
        QString projectPath = d->currentProjectPath.isEmpty() ?
                                  QFileInfo(sourceFile).absolutePath() :
                                  d->currentProjectPath;
        QString python;

        // 1. 优先使用虚拟环境中的解释器
        if (model && !model->interpreterPath().isEmpty()) {
            python = model->interpreterPath();
        }

        // 2. 如果模型未提供，从 SDK 环境变量 PYTHON_HOME 读取
        if (python.isEmpty() || python == "python3") {
            ConfigManager* cfg = Application::instance().configManager();
            if (cfg) {
                QVariantMap sdkEnv = cfg->value("Sdk/Environment").toMap();
                if (sdkEnv.contains("PYTHON_HOME")) {
                    QString pyHome = sdkEnv["PYTHON_HOME"].toString();
                    if (!pyHome.isEmpty() && QFile::exists(pyHome)) {
                        python = pyHome;
                    }
                }
            }
        }

        // 3. 最终 fallback
        if (python.isEmpty()) {
            python = "python3";
        }

        // 输出诊断信息
        d->outputPanel->append(QString("使用 Python 解释器: %1").arg(python));
        d->outputPanel->append(QString("工作目录: %1").arg(projectPath));
        d->outputPanel->append("正在运行...");

        // 构建命令参数
        QStringList args;
        args << sourceFile;

        // ===== 构造环境变量（保留！后面 QProcess 要用） =====
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        if (model && !model->venvPath().isEmpty()) {
#ifdef Q_OS_WIN
            QString binDir = model->venvPath() + "/Scripts";
#else
            QString binDir = model->venvPath() + "/bin";
#endif
            if (QFile::exists(binDir)) {
                QString path = env.value("PATH");
                env.insert("PATH", binDir + QDir::listSeparator() + path);
                env.insert("VIRTUAL_ENV", model->venvPath());
            }
        }
        // =================================================

        // 使用 QProcess 异步运行（信号槽机制，不阻塞 UI 线程）
        QProcess* process = new QProcess(this);
        process->setProgram(python);
        process->setArguments(args);
        process->setWorkingDirectory(projectPath);
        process->setProcessEnvironment(env);
        process->setProcessChannelMode(QProcess::MergedChannels);

        // 连接输出信号
        connect(process, &QProcess::readyReadStandardOutput, this, [this, process]() {
            QByteArray data = process->readAllStandardOutput();
            QString output = QString::fromLocal8Bit(data);
            if (!output.isEmpty()) {
                d->outputPanel->append(output);
            }
        });
        connect(process, &QProcess::readyReadStandardError, this, [this, process]() {
            QByteArray data = process->readAllStandardError();
            QString output = QString::fromLocal8Bit(data);
            if (!output.isEmpty()) {
                d->outputPanel->append("<span style='color:#FF6B6B;'>"
                                       + output.toHtmlEscaped() + "</span>");
            }
        });

        // 连接完成信号
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, process](int exitCode, QProcess::ExitStatus status) {
                    QString timeStr = QTime::currentTime().toString("hh:mm:ss");
                    if (status == QProcess::NormalExit) {
                        d->outputPanel->append(
                            QString("<span style='color:#33CC33;'>[%1] 进程结束，退出码: %2</span>")
                                .arg(timeStr).arg(exitCode));
                    } else {
                        d->outputPanel->append(
                            QString("<span style='color:#FF6B6B;'>[%1] 进程异常崩溃</span>")
                                .arg(timeStr));
                    }
                    process->deleteLater();
                    m_statusLabel->setText("运行完成");
                });

        // 启动进程（异步）
        process->start();
        if (!process->waitForStarted(3000)) {
            d->outputPanel->append("启动失败: " + process->errorString());
            process->deleteLater();
            return;
        }

        m_statusLabel->setText("程序运行中...");
        return;
    }


    // ========== C++ / 其他可执行语言：异步运行 ==========
    if (lang == LanguageType::Cpp) {
        if (editor->isModified()) {
            if (!editor->saveFile()) {
                QMessageBox::warning(this, "保存错误", "无法保存文件，运行中止。");
                return;
            }
            sourceFile = editor->filePath();
        }

        if (m_isCompiling) {
            QMessageBox::information(this, "提示", "当前正在编译，请等待编译完成后再运行。");
            return;
        }

        // 1. 判断项目路径
        QString projectPath = d->currentProjectPath.isEmpty()
                                  ? QFileInfo(sourceFile).absolutePath()
                                  : d->currentProjectPath;

        // 2. 构造 BuildTask
        BuildTask task;
        task.taskId     = QUuid::createUuid().toString(QUuid::WithoutBraces);
        task.language   = LanguageType::Cpp;
        task.projectPath = projectPath;
        task.sourceFiles << sourceFile;
        task.outputName = QFileInfo(sourceFile).completeBaseName();
        task.outputDir  = "bin";
        task.enableDebug = false;

        // 3. 读取项目配置
        if (!d->currentProjectPath.isEmpty()) {
            ProjectConfig projConfig;
            QString configFile = d->currentProjectPath + "/.lightcppproj";
            QStringList errors;
            if (projConfig.loadFromFile(configFile, errors)) {
                task.sdkConfig = projConfig.sdkEnvironment();
            }
        }

        // 4. 拿可执行文件路径
        IBuilder* builder = BuilderFactory::createBuilder(task.language);
        QString executablePath;
        if (builder) {
            executablePath = builder->getExecutablePath(task);
        }

        bool needBuild = executablePath.isEmpty()
                         || !QFile::exists(executablePath)
                         || QFileInfo(sourceFile).lastModified() > QFileInfo(executablePath).lastModified();

        if (needBuild) {
            int reply = QMessageBox::question(this, "编译并运行",
                                              "文件尚未编译或已过期，是否现在编译并运行?",
                                              QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                m_autoRunAfterCompile = true;
                onCompile();
            }
            return;
        }

        // 5. 异步运行可执行文件
        d->outputPanel->append(QString("使用可执行文件: %1").arg(executablePath));
        d->outputPanel->append("正在运行...");

        QProcess* process = new QProcess(this);
        process->setProgram(executablePath);
        process->setArguments(task.extraArgs);
        process->setWorkingDirectory(QFileInfo(executablePath).absolutePath());
        process->setProcessChannelMode(QProcess::MergedChannels);

        connect(process, &QProcess::readyReadStandardOutput, this, [this, process]() {
            QByteArray data = process->readAllStandardOutput();
            QString output = QString::fromLocal8Bit(data);
            if (!output.isEmpty()) {
                d->outputPanel->append(output);
            }
        });
        connect(process, &QProcess::readyReadStandardError, this, [this, process]() {
            QByteArray data = process->readAllStandardError();
            QString output = QString::fromLocal8Bit(data);
            if (!output.isEmpty()) {
                d->outputPanel->append("<span style='color:#FF6B6B;'>"
                                       + output.toHtmlEscaped() + "</span>");
            }
        });
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, process](int exitCode, QProcess::ExitStatus status) {
                    QString timeStr = QTime::currentTime().toString("hh:mm:ss");
                    if (status == QProcess::NormalExit) {
                        d->outputPanel->append(
                            QString("<span style='color:#33CC33;'>[%1] 进程结束，退出码: %2</span>")
                                .arg(timeStr).arg(exitCode));
                    } else {
                        d->outputPanel->append(
                            QString("<span style='color:#FF6B6B;'>[%1] 进程异常崩溃</span>")
                                .arg(timeStr));
                    }
                    process->deleteLater();
                    m_statusLabel->setText("运行完成");
                });

        process->start();
        if (!process->waitForStarted(3000)) {
            d->outputPanel->append("启动失败: " + process->errorString());
            process->deleteLater();
            return;
        }
        m_statusLabel->setText("程序运行中...");
        return;
    }

    // 如果文件未保存，先保存
    if (editor->isModified()) {
        if (!editor->saveFile()) {
            QMessageBox::warning(this, "保存错误", "无法保存文件，运行中止。");
            return;
        }
        sourceFile = editor->filePath();
    }

    // 检查是否正在编译
    if (m_isCompiling) {
        QMessageBox::information(this, "提示", "当前正在编译，请等待编译完成后再运行。");
        return;
    }

    // ----- 使用 BuildTask 新方式 -----
    QString projectRoot = d->currentProjectPath.isEmpty() ?
                              QFileInfo(sourceFile).absolutePath() :
                              d->currentProjectPath;

    BuildTask task;
    task.taskId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    task.language = detectLanguageTypeFromPath(sourceFile);
    task.projectPath = d->currentProjectPath.isEmpty() ?
                           QFileInfo(sourceFile).absolutePath() :
                           d->currentProjectPath;
    task.sourceFiles << sourceFile;
    task.outputName = QFileInfo(sourceFile).completeBaseName();
    task.outputDir = "bin";
    task.enableDebug = false;   // 运行模式不调试

    // 读取项目配置中的 SDK 环境
    if (!d->currentProjectPath.isEmpty()) {
        ProjectConfig projConfig;
        QString configFile = d->currentProjectPath + "/.lightcppproj";
        QStringList errors;
        if (projConfig.loadFromFile(configFile, errors)) {
            task.sdkConfig = projConfig.sdkEnvironment();
            if (task.language == LanguageType::Generic_AI) {
                task.aiCustomPayload = projConfig.genericBuildConfig();
            }
        }
    }

    // ----- 针对 Java 项目填充额外信息 -----
    if (task.language == LanguageType::Java) {
        ProjectManager* pm = ProjectManager::instance();
        ProjectModel* model = pm->javaProjectModel();
        if (model) {
            task.classpath = model->classpath();
            task.dependencyJars = model->dependencyJars();
            // 将主类存入 aiCustomPayload 供 JavaBuilder 使用
            task.aiCustomPayload["mainClass"] = model->defaultMainClass();
            task.aiCustomPayload["isFatJar"] = model->isFatJar();
            task.aiCustomPayload["fatJarPath"] = model->fatJarPath();
            // 设置输出目录为 Maven/Gradle 标准
            task.outputDir = model->outputDirectory();
            // 项目路径
            task.projectPath = model->projectPath();
        }
    }

    // 获取 Builder，尝试查找可执行文件
    IBuilder* builder = BuilderFactory::createBuilder(task.language);
    QString executablePath;
    if (builder) {
        executablePath = builder->getExecutablePath(task);
    }

    if (executablePath.isEmpty() || !QFile::exists(executablePath) ||
        QFileInfo(sourceFile).lastModified() > QFileInfo(executablePath).lastModified()) {
        // 需要先编译
        int reply = QMessageBox::question(this, "编译并运行",
                                          "文件尚未编译或已过期，是否现在编译并运行?",
                                          QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            m_autoRunAfterCompile = true;
            onCompile();  // onCompile 内部也会构建 BuildTask
        }
        return;
    }

    // 直接运行
    BuildResult dummyResult;
    dummyResult.executablePath = executablePath;
    QString output;
    if (m_compileManager->runTask(task, dummyResult, output)) {
        d->outputPanel->append(output);
    }
}



QString MainWindow::findExecutableForSource(const QString& sourceFile)
{
    QFileInfo sourceInfo(sourceFile);
    QString baseName = sourceInfo.completeBaseName();
    QString sourceDir = sourceInfo.absolutePath();

    QStringList possibleDirs = {
        sourceDir + "/bin",
        sourceDir + "/build",
        sourceDir,
        QDir::currentPath() + "/bin",
        QDir::currentPath()
    };

    QStringList possibleNames;
#ifdef Q_OS_WIN
    possibleNames << baseName + ".exe" << "a.exe";
#else
    possibleNames << baseName << "a.out" << "a";
#endif

    for (const QString& dir : possibleDirs) {
        QDir searchDir(dir);
        if (searchDir.exists()) {
            for (const QString& name : possibleNames) {
                QString filePath = searchDir.absoluteFilePath(name);
                if (QFile::exists(filePath)) {
                    QFileInfo exeInfo(filePath);
                    if (exeInfo.isExecutable() ||
                        (name.endsWith(".exe") && exeInfo.isFile())) {
                        return filePath;
                    }
                }
            }
        }
    }

    return QString();
}

QString MainWindow::findClangFormat()
{
    QStringList possibleNames = {"clang-format", "clang-format.exe", "clang-format-17", "clang-format-16", "clang-format-15", "clang-format-14", "clang-format-13", "clang-format-12", "clang-format-11", "clang-format-10"};

    QStringList possiblePaths = {
        "",
        "/usr/bin",
        "/usr/local/bin",
        "/opt/homebrew/bin",
        "C:/Program Files/LLVM/bin",
        "C:/Program Files (x86)/LLVM/bin",
        "C:/MinGW/bin",
        "C:/MinGW64/bin",
        "C:/cygwin64/bin"
    };

    for (const QString& path : possiblePaths) {
        for (const QString& name : possibleNames) {
            QString fullPath;
            if (path.isEmpty()) {
                fullPath = name;
            } else {
                fullPath = path + "/" + name;
            }

            if (QFile::exists(fullPath) && QFileInfo(fullPath).isExecutable()) {
                return fullPath;
            }
        }
    }

    QString pathEnv = qgetenv("PATH");
    QStringList pathEntries = pathEnv.split(QDir::listSeparator());
    for (const QString& path : pathEntries) {
        for (const QString& name : possibleNames) {
            QString fullPath = path + "/" + name;
            if (QFile::exists(fullPath) && QFileInfo(fullPath).isExecutable()) {
                return fullPath;
            }
        }
    }

    return QString();
}

QString MainWindow::findDebugger()
{
    QStringList possibleNames = {"gdb", "lldb", "gdb.exe", "lldb.exe"};

    QStringList possiblePaths = {
        "",
        "/usr/bin",
        "/usr/local/bin",
        "/opt/homebrew/bin",
        "C:/MinGW/bin",
        "C:/Program Files/LLVM/bin"
    };

    for (const QString& path : possiblePaths) {
        for (const QString& name : possibleNames) {
            QString fullPath;
            if (path.isEmpty()) {
                fullPath = name;
            } else {
                fullPath = path + "/" + name;
            }

            if (QFile::exists(fullPath) && QFileInfo(fullPath).isExecutable()) {
                return fullPath;
            }
        }
    }

    return QString();
}

void MainWindow::onCompileSettings()
{
    SettingsDialog dialog(this);
    dialog.exec();
}

void MainWindow::onZoomIn()
{
    TextEditor* editor = currentEditor();
    if (editor) {
        QFont font = editor->font();
        int newSize = font.pointSize() + 1;
        if (newSize <= 24) {
            font.setPointSize(newSize);
            editor->setEditorFont(font);
        }
    }
}

void MainWindow::onZoomOut()
{
    TextEditor* editor = currentEditor();
    if (editor) {
        QFont font = editor->font();
        int newSize = font.pointSize() - 1;
        if (newSize >= 8) {
            font.setPointSize(newSize);
            editor->setEditorFont(font);
        }
    }
}

void MainWindow::onZoomReset()
{
    TextEditor* editor = currentEditor();
    if (editor) {
        QFont font("Consolas", 12);
        editor->setEditorFont(font);
    }
}

void MainWindow::onSettings()
{
    d->outputPanel->append("打开设置对话框...");
}

void MainWindow::onQuickStart()
{
    QMessageBox::information(this, "快速开始指南",
                             "SprintIDE 快速开始：\n"
                             "1. 新建或打开C++文件\n"
                             "2. 编写代码\n"
                             "3. 保存文件\n"
                             "4. 编译运行");
}

void MainWindow::onDocumentation()
{
    d->outputPanel->append("打开使用文档...");
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "关于 SprintIDE",
                       "SprintIDE v1.0.0\n"
                       "轻量级 C++ 集成开发环境\n"
                       "刘腾骏胡乱做的");
}

void MainWindow::onThemeDark()
{
    ThemeManager::instance().setTheme(ThemeManager::ThemeType::Dark);
    applyCurrentTheme();

    d->outputPanel->append("已切换到深色主题");
    m_statusLabel->setText("主题已切换为深色");
}

void MainWindow::onThemeLight()
{
    ThemeManager::instance().setTheme(ThemeManager::ThemeType::Light);
    applyCurrentTheme();

    d->outputPanel->append("已切换到浅色主题");
    m_statusLabel->setText("主题已切换为浅色");
}

void MainWindow::onCustomTheme()
{
    d->outputPanel->append("打开自定义主题...");
}

void MainWindow::createNewFileWithName(const QString &defaultName,
                                       const QString &title,
                                       const QString &customDir)
{
    // ============================================================
    // 1. 弹出输入对话框，让用户输入文件名
    // ============================================================
    bool ok;
    QString fileName = QInputDialog::getText(
        this,
        title,
        "请输入文件名（含扩展名）:\n"
        "例如: main.cpp, test.py, app.js, index.html",
        QLineEdit::Normal,
        defaultName,
        &ok
        );

    if (!ok || fileName.isEmpty()) {
        return;  // 用户取消或未输入
    }

    // ============================================================
    // 2. 确定保存路径
    // ============================================================
    QString savePath;

    if (!customDir.isEmpty()) {
        // 优先使用自定义目录
        savePath = customDir;
    } else if (!d->currentProjectPath.isEmpty()) {
        // 如果有打开的项目，优先保存到项目 src 目录
        QDir projectDir(d->currentProjectPath);
        if (projectDir.exists("src")) {
            savePath = projectDir.absoluteFilePath("src");
        } else {
            savePath = d->currentProjectPath;
        }
    } else {
        // 没有项目，保存在当前工作目录
        savePath = QDir::currentPath();
    }

    // 确保目录存在
    QDir dir(savePath);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            QMessageBox::warning(this, "创建失败",
                                 QString("无法创建目录:\n%1").arg(savePath));
            return;
        }
    }

    QString fullPath = QDir(savePath).absoluteFilePath(fileName);

    // ============================================================
    // 3. 检查文件是否已存在
    // ============================================================
    if (QFile::exists(fullPath)) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "文件已存在",
            QString("文件 '%1' 已存在，是否打开它？").arg(fileName),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes
            );

        if (reply == QMessageBox::Yes) {
            openFile(fullPath);
        }
        return;
    }

    // ============================================================
    // 4. 创建空文件
    // ============================================================
    QFile file(fullPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "创建失败",
                             QString("无法创建文件:\n%1\n错误: %2")
                                 .arg(fullPath).arg(file.errorString()));
        return;
    }
    file.close();

    // ============================================================
    // 5. 打开文件（自动应用语言高亮）
    // ============================================================
    openFile(fullPath);

    // ============================================================
    // 6. 更新界面状态
    // ============================================================
    d->outputPanel->append(QString("已创建文件: %1").arg(fullPath));
    m_statusLabel->setText(QString("已创建: %1").arg(fileName));

    // 获取当前编辑器，更新状态栏语言信息
    TextEditor* editor = currentEditor();
    if (editor) {
        LanguageInfo langInfo = LanguageDetector::instance().detectLanguage(fullPath);
        if (m_languageLabel) {
            m_languageLabel->setText(langInfo.name);
        }
    }

    // 刷新项目面板（如果有）
    if (d->projectPanel && !d->currentProjectPath.isEmpty()) {
        // 简单刷新：重新加载项目树
        d->projectPanel->openProject(d->currentProjectPath);
    }
}

// ==================== 状态查询方法 ====================

bool MainWindow::hasUnsavedChanges() const
{
    return d->hasUnsavedChanges;
}

QString MainWindow::currentFilePath() const
{
    return d->currentFilePath;
}

QString MainWindow::projectPath() const
{
    return d->currentProjectPath;
}

bool MainWindow::isCompiling() const
{
    return m_isCompiling;
}

void MainWindow::stopCompile()
{
    if (!m_currentCompileTaskId.isEmpty()) {
        bool stopped = m_compileManager->cancelCompile(m_currentCompileTaskId);

        QString timeStr = QTime::currentTime().toString("hh:mm:ss");
        if (stopped) {
            d->outputPanel->append(QString("<span style='color:#FF9900;'>[%1] 编译已取消</span>").arg(timeStr));
            m_statusLabel->setText("编译已取消");
        }

        m_isCompiling = false;
        m_currentCompileTaskId.clear();
    }
}

bool MainWindow::compileProject(const QString& projectPath, const QString& projectName)
{
    Q_UNUSED(projectPath);
    Q_UNUSED(projectName);
    return false;
}

bool MainWindow::runProject(const QString& projectPath, const QString& projectName)
{
    Q_UNUSED(projectPath);
    Q_UNUSED(projectName);
    return false;
}

} // namespace SprintIDE
