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
#include <QMouseEvent>
#include <QDialog>
#include <QSet>
#include <QLocale>
#include <QEvent>
#include <QKeySequence>

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
#include "lsp/diagnostic_filter.h"

//ai
#include "ai/ai_integration.h"
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

// Git 模块
#include "git/git_manager.h"
#include "git/git_panel.h"
#include "git/git_diff_dialog.h"
#include "git/git_file_history_dialog.h"
#include "git/git_settings_page.h"

#include "quick_open_dialog.h"
#include "search_in_files_dialog.h"
#include "log/output_panel.h"
#include "log/log_list_model.h"     // 只需要 LogKind / LogLevel 常量时
#include "breakpoint_list_panel.h"


using namespace SprintIDE;

namespace SprintIDE {

// ============================================================================
//  从文件路径向上查找真正的项目根
//
//  优先级（第一个命中的目录就是项目根）：
//    1. compile_commands.json 所在的目录（CMake / clangd 项目的标准位置）
//    2. CMakeLists.txt 所在的目录
//    3. .git 所在的目录
//    4. 其他语言的标识文件（pom.xml / package.json / pyproject.toml / Cargo.toml / go.mod）
//    5. 都找不到 → 返回文件所在目录
//
//  为什么不能用 currentProjectPath：
//    用户可能在项目 A 里打开项目 B 的文件（临时查看、跳转定义），
//    LSP 必须按文件归属的项目解析，否则编译参数全错。
// ============================================================================
static QString findProjectRootForFile(const QString& filePath)
{
    if (filePath.isEmpty()) return QString();

    QDir dir = QFileInfo(filePath).absoluteDir();

    while (!dir.isRoot()) {
        // 1. CMake 项目：优先 compile_commands.json
        if (dir.exists("compile_commands.json") ||
            dir.exists("CMakeLists.txt")         ||
            dir.exists(".git")                    ||
            dir.exists("pom.xml")                 ||
            dir.exists("build.gradle")            ||
            dir.exists("package.json")            ||
            dir.exists("pyproject.toml")          ||
            dir.exists("Cargo.toml")              ||
            dir.exists("go.mod")) {
            return dir.absolutePath();
        }
        if (!dir.cdUp()) break;
    }

    // 向上到根还没找到 → 返回文件所在目录
    return QFileInfo(filePath).absolutePath();
}

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

    // Qt（必须在 C++ 之前判定）
    {
        SprintIDE::CppProjectModel qtProbe(projectPath);
        qtProbe.load();
        if (qtProbe.isQtProject()) {
            return SprintIDE::LanguageType::Qt;
        }
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

// ====================================================================
// 快速打开：扫描项目目录下的文件（不含构建产物 / 二进制）
// ====================================================================
static QStringList scanFilesForQuickOpen(const QString& root, int maxFiles = 20000)
{
    QStringList out;
    if (root.isEmpty() || !QDir(root).exists()) return out;

    // 与 loadAllProjectFiles 保持一致的排除名单
    static const QStringList kExcludeDirs = {
        ".git", ".svn", ".hg",
        ".cache", ".SprintIDE",
        "build", "bin", "obj", "out", "dist", "target",
        "Debug", "Release", "CMakeFiles",
        "node_modules", "__pycache__", ".venv", "venv",
        ".gradle", ".mvn", ".cargo", ".rustup",
        ".vs", ".idea", ".vscode",
        ".dSYM"
    };

    // 明显是二进制/构建产物 → 不列入快速打开
    static const QSet<QString> kSkipExt = {
        "o", "a", "so", "dylib", "dll", "exe", "d", "obj", "lib",
        "class", "jar", "war", "pyc", "pyo", "pdb", "ilk", "exp",
        "png", "jpg", "jpeg", "gif", "ico", "bmp", "webp",
        "ttf", "otf", "woff", "woff2",
        "zip", "tar", "gz", "bz2", "xz", "7z", "rar",
        "mp3", "mp4", "mov", "avi", "mkv", "wav",
        "pdf"
    };

    constexpr int kMaxDepth = 16;
    QStack<QPair<QString, int>> pending;      // (dirPath, depth)
    pending.push({QDir(root).absolutePath(), 0});

    while (!pending.isEmpty()) {
        if (out.size() >= maxFiles) break;

        const auto item = pending.pop();
        const QString dirPath = item.first;
        const int depth = item.second;
        if (depth > kMaxDepth) continue;

        QDir dir(dirPath);
        if (!dir.exists()) continue;

        // ---- 子目录 ----
        const QStringList subdirs = dir.entryList(
            QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
        for (const QString& sub : subdirs) {
            bool skip = false;
            for (const QString& ex : kExcludeDirs) {
                if (sub.compare(ex, Qt::CaseInsensitive) == 0) { skip = true; break; }
            }
            if (!skip && (sub.endsWith(".dSYM", Qt::CaseInsensitive) ||
                          sub.endsWith(".framework", Qt::CaseInsensitive) ||
                          sub.endsWith(".app", Qt::CaseInsensitive))) {
                skip = true;
            }
            if (skip) continue;
            pending.push({dir.absoluteFilePath(sub), depth + 1});
        }

        // ---- 当前目录文件 ----
        const QStringList files = dir.entryList(
            QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);
        for (const QString& f : files) {
            if (f.startsWith('.'))  continue;   // 隐藏文件
            if (f.startsWith("._")) continue;   // macOS 元数据

            const QString ext = QFileInfo(f).suffix().toLower();
            if (!ext.isEmpty() && kSkipExt.contains(ext)) continue;

            out.append(dir.absoluteFilePath(f));
            if (out.size() >= maxFiles) break;
        }
    }

    qDebug() << "[QuickOpen] 收集文件数:" << out.size() << "root:" << root;
    return out;
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

    // 从配置读取上次的主题
    // ThemeManager& tm = ThemeManager::instance();
    // {
    //     ConfigManager* cfg = Application::instance().configManager();
    //     QString savedTheme = cfg ? cfg->value("General/Theme", "Dark").toString()
    //                              : QStringLiteral("Dark");

    //     if (savedTheme == QLatin1String("Light")) {
    //         tm.setTheme(ThemeManager::ThemeType::Light);
    //         qDebug() << "[MW] 从配置恢复浅色主题";
    //     } else {
    //         tm.setTheme(ThemeManager::ThemeType::Dark);
    //         qDebug() << "[MW] 使用深色主题（默认或配置）";
    //     }
    // }

    // 设置深色主题
    ThemeManager& tm = ThemeManager::instance();
    // tm.setTheme(ThemeManager::ThemeType::Dark);

    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupDockWidgets();
    setupConnections();

    // 应用用户自定义快捷键（必须在所有 action 创建完之后）
    applyShortcutsFromConfig();
    setupDebugPanel();
    setupTerminalPanel();
    setupGitPanel();

    connect(&tm, &ThemeManager::themeChanged,
            this, &MainWindow::onThemeChanged);
    connect(&tm, &ThemeManager::fontChanged, this, [this]() {
        // 字体或字号变化 → 遍历所有编辑器重新应用
        applyCurrentTheme();
    });

    // 恢复窗口状态（必须在所有 dock 创建之后）
    restoreWindowState();
    applyCurrentTheme();

    // 初始化 LSP 管理器
    m_lspManager = new LSPManager(this);
    m_lspManager->initialize();
    qDebug() << "[MW-13] LSP ready";

    connect(m_lspManager, &LSPManager::clientCreated,
            this, [this](const QString &languageId, const QString &projectRoot) {
                Q_UNUSED(languageId);
                Q_UNUSED(projectRoot);
            });
    connect(m_lspManager, &LSPManager::clientError,
            this, [this](const QString &languageId, const QString &error) {
                logError(QString("LSP (%1) 错误: %2")
                                           .arg(languageId).arg(error));
            });

    connect(d->projectPanel, &ProjectPanel::fileOpenRequested, this, &MainWindow::openFile);
    connect(d->projectPanel, &ProjectPanel::fileCreated,       this, &MainWindow::openFile);

    // 初始化调试管理器
    m_debugManager = &DebugManager::instance();
    m_debugManager->initialize();
    if (d->projectPanel) {
        connect(d->projectPanel, &ProjectPanel::fileRenamed,
                this, &MainWindow::onFileRenamed);
    }

    // 断点列表面板：必须在 m_debugManager 初始化之后
    setupBreakpointListPanel();
    // 连接调试管理器信号
    connect(m_debugManager, &DebugManager::stateChanged,
            this, &MainWindow::onDebugStateChanged);
    // ... 其余调试信号连接（保持不变） ...

    // 初始化 AI 模块
    if (initializeAIIntegration(this, m_aiConnMgr, m_aiChatPanel)) {
        setupAIMenu(this, m_aiChatPanel);
        restoreChatPanelState(m_aiChatPanel);
        if (m_aiChatPanel) {
            m_aiChatPanel->setVisible(true);
            m_aiChatPanel->raise();
        }
        qDebug() << "AI 模块已集成到 MainWindow";

        // AI 状态变化 → 同步到所有编辑器和 OutputPanel
        if (m_aiConnMgr) {
            connect(m_aiConnMgr, &AiConnectionManager::stateChanged,
                    this, [this](AiConnectionManager::State) {
                        syncAIAvailabilityToEditors();
                    });
            syncAIAvailabilityToEditors();
        }
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

    // 加载 LSP 过滤配置（模式 + 自定义规则）
    reloadLspFilterSettings();

    qDebug() << "[MW] LogBuffer ready:"
         << (Application::instance().logBuffer() != nullptr);
}

MainWindow::~MainWindow()
{
    qDebug() << "[MainWindow] 析构开始";
    // 防止 LSP shutdown 期间嵌套事件循环把队列里的旧事件重新投递
    this->blockSignals(true);

    // 把当前线程队列里给 this 的事件全部取消（安全，不会漏掉正在派发的）
    QCoreApplication::removePostedEvents(this);

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
    qDebug() << "[MainWindow] 断开可能产生回调的信号";
    // ========== 2. Python 调试器停止（不 delete，交给 Qt parent）==========
    if (m_pythonDebugger) {
        m_pythonDebugger->stopDebugging();
    }
    qDebug() << "[MainWindow] Python 调试器停止";
    // ========== 2.5 C++ 调试器停止 ==========
    if (m_cppDebugger) {
        m_cppDebugger->stopDebugging();
        delete m_cppDebugger;
        m_cppDebugger = nullptr;
    }
    qDebug() << "[MainWindow] 调试器停止";
    // ========== 3. 显式关闭 LSP（它有 parent = this，会随后被 Qt 删除）==========
    if (m_lspManager) {
        m_lspManager->shutdownAll();
    }
    qDebug() << "[MainWindow] 显式关闭 LSP";
    // ========== 4. 清理编辑器标签 ==========
    if (d && d->editorTabs) {
        while (d->editorTabs->count() > 0) {
            QWidget* w = d->editorTabs->widget(0);
            d->editorTabs->removeTab(0);
            if (w) {
                // 先解除它对外部对象（LSPClient 等）的监听，避免析构期间被调用
                disconnect(w, nullptr, nullptr, nullptr);
                // 也断开外部对象对它的信号
                //   （对 QObject 的接收者版本：断开所有 source→w 的连接）
                if (auto* te = qobject_cast<TextEditor*>(w)) {
                    if (te->document()) {
                        disconnect(te->document(), nullptr, nullptr, nullptr);
                    }
                }
                delete w;
            }
        }
    }
    qDebug() << "[MainWindow] 清理编辑器标签";
    // ========== 5. 断开 definition 连接 ==========
    // for (auto it = m_definitionConnections.begin(); it != m_definitionConnections.end(); ++it) {
    //     disconnect(it.value());
    // }
    // m_definitionConnections.clear();

    // for (auto it = m_pendingDefinitionConnections.begin(); it != m_pendingDefinitionConnections.end(); ++it) {
    //     disconnect(it.value());
    // }
    // m_pendingDefinitionConnections.clear();

    // ========== 6. 删除私有实现 ==========
    delete d;
    d = nullptr;
    qDebug() << "[MainWindow] 删除私有实现";
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

    // 全局 QSS 已经由 ThemeManager::applyThemeToApplication() 推送
    // 这里只需要处理 QSS 覆盖不到的自绘组件

    // 1. 编辑器主题（TextEditor 内部用令牌，不用硬编码）
    for (int i = 0; i < d->editorTabs->count(); ++i) {
        if (auto* editor = qobject_cast<TextEditor*>(d->editorTabs->widget(i))) {
            editor->applyTheme(tm.isDarkTheme() ? "dark" : "light");
        }
    }

    // 2. 标签页自绘
    if (auto* tabWidget = qobject_cast<TabWidget*>(d->editorTabs)) {
        tabWidget->updateTheme();
    }

    // 3. 项目树（自绘 branch 图标时需要）
    if (d->projectPanel) {
        tm.applyThemeToTreeView(d->projectPanel->getFileTreeView());
    }

    // 4 输出面板：三档过滤按钮 / 工具条 / 摘要条都是自绘/自定义 QSS
    if (m_outputPanel) {
        m_outputPanel->applyTheme();
    }

    // 性能标签用内联 HTML 颜色，主题切换需要主动重刷
    if (m_performanceMonitor && m_performanceLabel) {
        onPerformanceSnapshot(m_performanceMonitor->lastSnapshot());
    }

    // 窄条按钮样式跟随主题
    if (m_viewBreakpointsButton) {
        QWidget* strip = m_viewBreakpointsButton->parentWidget();
        if (strip) {
            ThemeManager& tm2 = ThemeManager::instance();
            strip->setStyleSheet(QString(
                "QWidget#BottomStrip {"
                "  background-color: %1;"
                "  border-right: 1px solid %2;"
                "}"
                "QWidget#BottomStrip QToolButton {"
                "  background: transparent; border: none; border-radius: 3px;"
                "  color: %3; font-size: 14px; font-weight: 700;"
                "}"
                "QWidget#BottomStrip QToolButton:hover { background-color: %4; }"
                "QWidget#BottomStrip QToolButton:checked {"
                "  background-color: %5; color: %6;"
                "}"
            ).arg(
                tm2.colorString(ThemeManager::UIElement::MainWindowBackground),
                tm2.colorString(ThemeManager::UIElement::BorderColor),
                tm2.colorString(ThemeManager::UIElement::SecondaryTextColor),
                tm2.colorString(ThemeManager::UIElement::HoverBackground),
                tm2.colorString(ThemeManager::UIElement::ActiveTabBackground),
                tm2.colorString(ThemeManager::UIElement::PrimaryColor)
            ));
        }
    }

    update();
    repaint();
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
    QMenu *fileMenu = menuBar->addMenu(tr("文件(&F)"));

    QMenu *newMenu = fileMenu->addMenu(QIcon(":/icons/icons/new_file.svg"), tr("新建(&N)"));

    // 通用新建文件（弹出对话框）
    QAction *newFileAction = newMenu->addAction(QIcon(":/icons/icons/text.svg"), tr("文件..."));
    newFileAction->setShortcut(QKeySequence::New);
    connect(newFileAction, &QAction::triggered, this, &MainWindow::onNewFile);
    registerShortcut(QStringLiteral("file.new"), tr("新建文件"), newFileAction);

    newMenu->addSeparator();

    // 快速创建 C++ 文件
    QAction *newCppAction = newMenu->addAction(QIcon(":/icons/icons/cpp.svg"), tr("C++ 源文件"));
    connect(newCppAction, &QAction::triggered, this, [this]() {
        createNewFileWithName("main.cpp", tr("新建 C++ 文件"), QString());
    });

    // 快速创建 C++ 头文件
    QAction *newHppAction = newMenu->addAction(QIcon(":/icons/icons/header.svg"), tr("C++ 头文件"));
    connect(newHppAction, &QAction::triggered, this, [this]() {
        createNewFileWithName("main.h", tr("新建 C++ 头文件"), QString());
    });

    newMenu->addSeparator();

    // 快速创建 Python 文件
    QAction *newPythonAction = newMenu->addAction(tr("Python 文件"));
    connect(newPythonAction, &QAction::triggered, this, [this]() {
        createNewFileWithName("main.py", tr("新建 Python 文件"), QString());
    });

    // 快速创建 JavaScript 文件
    QAction *newJsAction = newMenu->addAction(tr("JavaScript 文件"));
    connect(newJsAction, &QAction::triggered, this, [this]() {
        createNewFileWithName("main.js", tr("新建 JavaScript 文件"), QString());
    });

    // 快速创建 HTML 文件
    QAction *newHtmlAction = newMenu->addAction(tr("HTML 文件"));
    connect(newHtmlAction, &QAction::triggered, this, [this]() {
        createNewFileWithName("index.html", tr("新建 HTML 文件"), QString());
    });

    // 快速创建纯文本文件
    QAction *newTxtAction = newMenu->addAction(tr("纯文本文件"));
    connect(newTxtAction, &QAction::triggered, this, [this]() {
        createNewFileWithName("untitled.txt", tr("新建文本文件"), QString());
    });

    QAction *newProjectAction = newMenu->addAction(QIcon(":/icons/icons/new_project.svg"), tr("项目"));
    connect(newProjectAction, &QAction::triggered, this, &MainWindow::onNewProject);

    // 新窗口（启动新实例，用于多项目并行开发）
    QAction* newWindowAction = fileMenu->addAction(tr("新窗口(&W)"));
    newWindowAction->setShortcut(QKeySequence("Ctrl+Shift+N"));   // Cmd+Shift+N
    newWindowAction->setStatusTip(tr("打开一个新的 SprintIDE 窗口（多实例）"));
    connect(newWindowAction, &QAction::triggered, this, &MainWindow::onNewWindow);
    registerShortcut(QStringLiteral("file.newWindow"), tr("新窗口"), newWindowAction);

    QMenu *openMenu = fileMenu->addMenu(QIcon(":/icons/icons/open_file.svg"), tr("打开(&O)"));

    // 快速打开（Ctrl+O）—— 搜索文件名快速打开
    QAction *quickOpenAction = openMenu->addAction(tr("快速打开文件..."));
    quickOpenAction->setShortcut(QKeySequence::Open);          // Ctrl+O / Cmd+O
    quickOpenAction->setStatusTip(tr("输入文件名快速打开 (Ctrl+O)"));
    connect(quickOpenAction, &QAction::triggered, this, &MainWindow::onQuickOpen);
    registerShortcut(QStringLiteral("file.quickOpen"), tr("快速打开"), quickOpenAction);

    // 传统打开对话框（改为 Ctrl+Alt+O，避免与快速打开冲突）
    // QAction *openFileAction = openMenu->addAction(
        // QIcon(":/icons/icons/open_file.svg"), tr("打开文件..."));
    // openFileAction->setShortcut(QKeySequence("Ctrl+Alt+O"));
    // connect(openFileAction, &QAction::triggered, this, &MainWindow::onOpenFile);
    // registerShortcut(QStringLiteral("file.open"), tr("打开文件"), openFileAction);

    // QAction *openProjectAction = openMenu->addAction(
        // QIcon(":/icons/icons/open_project.svg"), tr("项目"));
    // openProjectAction->setShortcut(QKeySequence("Ctrl+Shift+O"));
    // connect(openProjectAction, &QAction::triggered, this, &MainWindow::onOpenProject);
    // registerShortcut(QStringLiteral("file.openProject"), tr("打开项目"), openProjectAction);

    fileMenu->addSeparator();

    QAction *saveFileAction = fileMenu->addAction(QIcon(":/icons/icons/save_file.svg"), tr("保存(&S)"));
    saveFileAction->setShortcut(QKeySequence::Save);
    connect(saveFileAction, &QAction::triggered, this, &MainWindow::onSaveFile);
    registerShortcut(QStringLiteral("file.save"), tr("保存"), saveFileAction);

    QAction *saveAllAction = fileMenu->addAction(QIcon(":/icons/icons/save_all.svg"), tr("保存全部(&A)"));
    saveAllAction->setShortcut(QKeySequence("Ctrl+Shift+S"));
    connect(saveAllAction, &QAction::triggered, this, [this]() {
        for (int i = 0; i < d->editorTabs->count(); ++i) {
            TextEditor* editor = qobject_cast<TextEditor*>(d->editorTabs->widget(i));
            if (editor && editor->isModified()) {
                editor->saveFile();
            }
        }
        logInfo("已保存所有文件");
    });
    registerShortcut(QStringLiteral("file.saveAll"), tr("保存全部"), saveAllAction);

    QAction *saveAsAction = fileMenu->addAction(tr("另存为(&A)"));
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::onSaveAs);

    fileMenu->addSeparator();

    QAction *closeFileAction = fileMenu->addAction(QIcon(":/icons/icons/close_tab.svg"), tr("关闭(&C)"));
    closeFileAction->setShortcut(QKeySequence::Close);
    connect(closeFileAction, &QAction::triggered, this, &MainWindow::onCloseFile);
    registerShortcut(QStringLiteral("file.close"), tr("关闭文件"), closeFileAction);

    QAction *exitAction = fileMenu->addAction(tr("退出(&X)"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &MainWindow::onExit);
    registerShortcut(QStringLiteral("file.exit"), tr("退出"), exitAction);

    // ==================== 编辑菜单 ====================
    QMenu *editMenu = menuBar->addMenu(tr("编辑(&E)"));

    QAction *undoAction = editMenu->addAction(QIcon(":/icons/icons/undo.svg"), tr("撤销(&U)"));
    undoAction->setShortcut(QKeySequence::Undo);
    connect(undoAction, &QAction::triggered, this, &MainWindow::onUndo);
    registerShortcut(QStringLiteral("edit.undo"), tr("撤销"), undoAction);

    QAction *redoAction = editMenu->addAction(QIcon(":/icons/icons/redo.svg"), tr("重做(&R)"));
    redoAction->setShortcut(QKeySequence::Redo);
    connect(redoAction, &QAction::triggered, this, &MainWindow::onRedo);
    registerShortcut(QStringLiteral("edit.redo"), tr("重做"), redoAction);

    editMenu->addSeparator();

    QAction *cutAction = editMenu->addAction(QIcon(":/icons/icons/cut.svg"), tr("剪切(&T)"));
    cutAction->setShortcut(QKeySequence::Cut);
    connect(cutAction, &QAction::triggered, this, &MainWindow::onCut);
    registerShortcut(QStringLiteral("edit.cut"), tr("剪切"), cutAction);

    QAction *copyAction = editMenu->addAction(QIcon(":/icons/icons/copy.svg"), tr("复制(&C)"));
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, this, &MainWindow::onCopy);
    registerShortcut(QStringLiteral("edit.copy"), tr("复制"), copyAction);

    QAction *pasteAction = editMenu->addAction(QIcon(":/icons/icons/paste.svg"), tr("粘贴(&P)"));
    pasteAction->setShortcut(QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, this, &MainWindow::onPaste);
    registerShortcut(QStringLiteral("edit.paste"), tr("粘贴"), pasteAction);

    editMenu->addSeparator();

    QAction *copyLineAction = editMenu->addAction(tr("复制行(&D)"));
    copyLineAction->setShortcut(QKeySequence("Ctrl+D"));
    connect(copyLineAction, &QAction::triggered, this, [this]() {
        TextEditor* editor = currentEditor();
        if (editor) {
            editor->duplicateLine();
        }
    });
    registerShortcut(QStringLiteral("edit.copyLine"), tr("复制行"), copyLineAction);

    QAction *deleteLineAction = editMenu->addAction(tr("删除行(&Y)"));
    deleteLineAction->setShortcut(QKeySequence("Ctrl+Y"));
    connect(deleteLineAction, &QAction::triggered, this, [this]() {
        TextEditor* editor = currentEditor();
        if (editor) {
            editor->deleteLine();
        }
    });
    registerShortcut(QStringLiteral("edit.deleteLine"), tr("删除行"), deleteLineAction);

    // ========== Python 菜单 ==========
    QMenu* pythonMenu = menuBar->addMenu("Python(&P)");

    QAction* installDepsAction = pythonMenu->addAction(tr("安装依赖"));
    connect(installDepsAction, &QAction::triggered, this, [this]() {
        auto model = ProjectManager::instance()->pythonProjectModel();
        if (!model) {
            QMessageBox::information(this, tr("提示"), tr("未打开 Python 项目"));
            return;
        }
        QString python = model->interpreterPath();
        if (python.isEmpty()) python = "python3";
        logInfo("正在安装依赖...");
        PythonDependencyManager::installDependencies(
            model->projectPath(), python,
            [this](const QString& out) { logInfo(out); }
            );
        logInfo("依赖安装完成");
    });

    QAction* createVenvAction = pythonMenu->addAction(tr("创建虚拟环境"));
    connect(createVenvAction, &QAction::triggered, this, [this]() {
        auto model = ProjectManager::instance()->pythonProjectModel();
        if (!model) {
            QMessageBox::information(this, tr("提示"), tr("未打开 Python 项目"));
            return;
        }
        if (PythonVirtualEnv::createVenv(model->projectPath())) {
            logInfo("虚拟环境创建成功");
            model->load(); // 重新加载
        } else {
            logError("虚拟环境创建失败");
        }
    });

    QAction* formatPythonAction = pythonMenu->addAction(tr("格式化代码"));
    connect(formatPythonAction, &QAction::triggered, this, [this]() {
        TextEditor* editor = currentEditor();
        if (!editor) return;
        QString filePath = editor->filePath();
        if (!filePath.endsWith(".py")) {
            QMessageBox::information(this, tr("提示"), tr("当前文件不是 Python 文件"));
            return;
        }
        QString error;
        if (PythonFormatter::formatFile(filePath, error)) {
            editor->reloadFile();
            logInfo("代码格式化完成");
        } else {
            logError("格式化失败: " + error);
        }
    });

    // 设置动作（原编译配置）
    editMenu->addSeparator();

    QAction *settingsAction = editMenu->addAction(QIcon(":/icons/icons/setting.svg"), tr("设置..."));
    settingsAction->setShortcut(QKeySequence("Ctrl+,"));
    settingsAction->setMenuRole(QAction::PreferencesRole);  // 关键：让其在 macOS 下进入应用程序菜单
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onCompileSettings);
    registerShortcut(QStringLiteral("edit.settings"), tr("设置"), settingsAction);

    // ---------- 跳转到行 ----------
    QAction *gotoLineAction = editMenu->addAction(tr("跳转到行(&L)..."));
    gotoLineAction->setShortcut(QKeySequence("Ctrl+G"));   // macOS 上 = Cmd+G
    connect(gotoLineAction, &QAction::triggered, this, &MainWindow::onGotoLine);
    registerShortcut(QStringLiteral("edit.gotoLine"), tr("跳转到行"), gotoLineAction);

    editMenu->addSeparator();

    QAction *findAction = editMenu->addAction(tr("查找(&F)"));
    findAction->setShortcut(QKeySequence::Find);
    connect(findAction, &QAction::triggered, this, &MainWindow::onFind);
    registerShortcut(QStringLiteral("edit.find"), tr("查找"), findAction);

    QAction *replaceAction = editMenu->addAction(tr("替换(&R)"));
    replaceAction->setShortcut(QKeySequence("Ctrl+Shift+H"));
    connect(replaceAction, &QAction::triggered, this, &MainWindow::onReplace);
    registerShortcut(QStringLiteral("edit.replace"), tr("替换"), replaceAction);

    QAction *searchInFilesAction = editMenu->addAction(tr("在文件中查找(&F)..."));
    searchInFilesAction->setShortcut(QKeySequence("Ctrl+Shift+F"));
    searchInFilesAction->setStatusTip(tr("在整个项目中搜索文本 (Ctrl+Shift+F)"));
    connect(searchInFilesAction, &QAction::triggered,
            this, &MainWindow::onSearchInFiles);
    registerShortcut(QStringLiteral("edit.searchInFiles"), tr("在文件中查找"), searchInFilesAction);

    QAction *formatCodeAction = editMenu->addAction(QIcon(":/icons/icons/format_code.svg"), tr("格式化代码(&O)"));
    formatCodeAction->setShortcut(QKeySequence("Ctrl+Alt+L"));
    connect(formatCodeAction, &QAction::triggered, this, &MainWindow::onFormatCode);
    registerShortcut(QStringLiteral("edit.formatCode"), tr("格式化代码"), formatCodeAction);

    QAction *commentAction = editMenu->addAction(QIcon(":/icons/icons/comment_code.svg"), tr("注释(&C)"));
    commentAction->setShortcut(QKeySequence("Ctrl+/"));
    connect(commentAction, &QAction::triggered, this, &MainWindow::onComment);
    registerShortcut(QStringLiteral("edit.comment"), tr("注释"), commentAction);

    // ==================== 构建菜单 ====================
    QMenu *buildMenu = menuBar->addMenu(tr("构建(&B)"));

    QAction *compileFileAction = buildMenu->addAction(QIcon(":/icons/icons/build.svg"), tr("编译当前文件(&C)"));
    compileFileAction->setShortcut(QKeySequence("Ctrl+Shift+B"));
    connect(compileFileAction, &QAction::triggered, this, &MainWindow::onCompile);
    registerShortcut(QStringLiteral("build.compileFile"), tr("编译当前文件"), compileFileAction);

    QAction *compileProjectAction = buildMenu->addAction(QIcon(":/icons/icons/build.svg"), tr("编译项目(&P)"));
    compileProjectAction->setShortcut(QKeySequence("Ctrl+B"));
    connect(compileProjectAction, &QAction::triggered, this, &MainWindow::onBuildProject);
    registerShortcut(QStringLiteral("build.compileProject"), tr("编译项目"), compileProjectAction);

    QAction *cleanBuildAction = buildMenu->addAction(tr("清理构建(&O)"));
    connect(cleanBuildAction, &QAction::triggered, this, &MainWindow::onCleanBuild);

    // QAction *compileSettingsAction = buildMenu->addAction(QIcon(":/icons/icons/setting.svg"), "编译配置(&C)");
    // connect(compileSettingsAction, &QAction::triggered, this, &MainWindow::onCompileSettings);

    // ==================== 调试菜单 ====================
    QMenu *debugMenu = menuBar->addMenu(tr("调试(&D)"));

    m_startDebugAction = debugMenu->addAction(QIcon(":/icons/icons/debug.svg"), tr("启动调试(&S)"));
    m_startDebugAction->setShortcut(QKeySequence("Ctrl+Shift+D"));
    connect(m_startDebugAction, &QAction::triggered, this, &MainWindow::onStartDebug);
    registerShortcut(QStringLiteral("debug.start"), tr("启动调试"), m_startDebugAction);

    m_stopDebugAction = debugMenu->addAction(QIcon(":/icons/icons/stop.svg"),tr("停止调试(&T)"));
    m_stopDebugAction->setShortcut(QKeySequence("Ctrl+."));
    m_stopDebugAction->setEnabled(false);
    connect(m_stopDebugAction, &QAction::triggered, this, &MainWindow::onStopDebug);
    registerShortcut(QStringLiteral("debug.stop"), tr("停止调试"), m_stopDebugAction);

    debugMenu->addSeparator();

    m_stepOverAction = debugMenu->addAction(QIcon(":/icons/icons/stop_over.svg"), tr("步过(&N)"));
    m_stepOverAction->setShortcut(QKeySequence("F10"));
    m_stepOverAction->setEnabled(false);
    connect(m_stepOverAction, &QAction::triggered, this, &MainWindow::onStepOver);
    registerShortcut(QStringLiteral("debug.stepOver"), tr("步过"), m_stepOverAction);

    m_stepIntoAction = debugMenu->addAction(QIcon(":/icons/icons/step_into.svg"), tr("步入(&S)"));
    m_stepIntoAction->setShortcut(QKeySequence("F11"));
    m_stepIntoAction->setEnabled(false);
    connect(m_stepIntoAction, &QAction::triggered, this, &MainWindow::onStepInto);
    registerShortcut(QStringLiteral("debug.stepInto"), tr("步入"), m_stepIntoAction);

    m_stepOutAction = debugMenu->addAction(QIcon(":/icons/icons/step_out.svg"), tr("步出(&O)"));
    m_stepOutAction->setShortcut(QKeySequence("Shift+F11"));
    m_stepOutAction->setEnabled(false);
    connect(m_stepOutAction, &QAction::triggered, this, &MainWindow::onStepOut);
    registerShortcut(QStringLiteral("debug.stepOut"), tr("步出"), m_stepOutAction);

    debugMenu->addSeparator();

    m_continueAction = debugMenu->addAction(QIcon(":/icons/icons/run.svg"), tr("继续(&C)"));
    m_continueAction->setShortcut(QKeySequence("F5"));
    m_continueAction->setEnabled(false);
    connect(m_continueAction, &QAction::triggered, this, &MainWindow::onContinueDebug);
    registerShortcut(QStringLiteral("debug.continue"), tr("继续"), m_continueAction);

    debugMenu->addSeparator();

    m_addBreakpointAction = debugMenu->addAction(QIcon(":/icons/icons/toggle_break_point.svg"), tr("添加断点(&B)"));
    m_addBreakpointAction->setShortcut(QKeySequence("Ctrl+F8"));
    connect(m_addBreakpointAction, &QAction::triggered, this, &MainWindow::onAddBreakpoint);
    registerShortcut(QStringLiteral("debug.addBreakpoint"), tr("添加断点"), m_addBreakpointAction);

    // ==================== Git 菜单 ====================
    QMenu *gitMenu = menuBar->addMenu("Git(&G)");

    m_gitToggleAction = gitMenu->addAction(tr("打开 Git 窗口(&O)"));
    m_gitToggleAction->setShortcut(QKeySequence("Ctrl+Shift+G"));
    m_gitToggleAction->setStatusTip(tr("显示 / 隐藏 Git 窗口 (Ctrl+Shift+G)"));
    connect(m_gitToggleAction, &QAction::triggered, this, &MainWindow::toggleGitDialog);
    registerShortcut(QStringLiteral("git.toggle"), tr("打开 Git 窗口"), m_gitToggleAction);

    gitMenu->addSeparator();

    m_gitInitAction = gitMenu->addAction(tr("初始化仓库(&I)"));
    connect(m_gitInitAction, &QAction::triggered, this, &MainWindow::onGitInit);

    m_gitCommitAction = gitMenu->addAction(tr("提交(&C)..."));
    m_gitCommitAction->setShortcut(QKeySequence("Ctrl+K"));
    connect(m_gitCommitAction, &QAction::triggered, this, &MainWindow::onGitCommit);
    registerShortcut(QStringLiteral("git.commit"), tr("Git 提交"), m_gitCommitAction);

    gitMenu->addSeparator();

    m_gitFetchAction = gitMenu->addAction(tr("获取(&F)"));
    connect(m_gitFetchAction, &QAction::triggered, this, &MainWindow::onGitFetch);

    m_gitPullAction = gitMenu->addAction(tr("拉取(&P)"));
    connect(m_gitPullAction, &QAction::triggered, this, &MainWindow::onGitPull);

    m_gitPushAction = gitMenu->addAction(tr("推送(&U)"));
    connect(m_gitPushAction, &QAction::triggered, this, &MainWindow::onGitPush);

    gitMenu->addSeparator();

    m_gitRefreshAction = gitMenu->addAction(tr("刷新状态(&R)"));
    m_gitRefreshAction->setShortcut(QKeySequence("F5"));
    connect(m_gitRefreshAction, &QAction::triggered, this, &MainWindow::onGitRefresh);
    registerShortcut(QStringLiteral("git.refresh"), tr("Git 刷新状态"), m_gitRefreshAction);

    m_gitShowHistoryAction = gitMenu->addAction(tr("查看历史(&H)"));
    connect(m_gitShowHistoryAction, &QAction::triggered, this, &MainWindow::onGitShowHistory);

    // ==================== 工具菜单 ====================
    QMenu *toolsMenu = menuBar->addMenu(tr("工具(&T)"));

    QMenu *themeMenu = toolsMenu->addMenu(QIcon(":/icons/icons/setting.svg"), tr("主题设置(&H)"));

    QAction *lightThemeAction = themeMenu->addAction(tr("浅色主题(&L)"));
    connect(lightThemeAction, &QAction::triggered, this, &MainWindow::onThemeLight);

    QAction *darkThemeAction = themeMenu->addAction(tr("深色主题(&D)"));
    connect(darkThemeAction, &QAction::triggered, this, &MainWindow::onThemeDark);

    QAction *pluginManagerAction = toolsMenu->addAction(QIcon(":/icons/icons/services.svg"), tr("插件管理(&P)"));
    connect(pluginManagerAction, &QAction::triggered, this, [this]() {
        logInfo("插件管理功能开发中...");
    });

    QAction *performanceMonitorAction = toolsMenu->addAction(tr("性能监控(&M)"));
    performanceMonitorAction->setCheckable(true);
    performanceMonitorAction->setChecked(true);
    performanceMonitorAction->setStatusTip(tr("显示 / 隐藏状态栏性能监控指示器"));
    connect(performanceMonitorAction, &QAction::toggled, this, [this](bool checked) {
        if (m_performanceLabel) {
            m_performanceLabel->setVisible(checked);
        }
    });

    QAction *terminalAction = toolsMenu->addAction(QIcon(":/icons/icons/terminal.svg"), tr("终端(&T)"));
    terminalAction->setShortcut(QKeySequence("Ctrl+`"));  // Cmd+`
    connect(terminalAction, &QAction::triggered, this, [this]() {
        if (m_terminalPanel) {
            m_terminalPanel->setVisible(!m_terminalPanel->isVisible());
        }
    });
    registerShortcut(QStringLiteral("tools.terminal"), tr("终端"), terminalAction);

    // ==================== 视图菜单 ====================
    QMenu *viewMenu = menuBar->addMenu(tr("视图(&V)"));

    // 底部面板 toggle
    QAction* toggleBottomPanelAction = viewMenu->addAction(tr("底部面板(&B)"));
    toggleBottomPanelAction->setShortcut(QKeySequence("Ctrl+J"));
    toggleBottomPanelAction->setCheckable(true);
    toggleBottomPanelAction->setChecked(true);
    toggleBottomPanelAction->setStatusTip(tr("显示 / 隐藏底部面板 (Ctrl+J)"));

    connect(toggleBottomPanelAction, &QAction::toggled, this, [this](bool checked) {
        if (m_bottomDockWidget) {
            m_bottomDockWidget->setVisible(checked);
        }
    });
    registerShortcut(QStringLiteral("view.bottomPanel"), tr("底部面板"), toggleBottomPanelAction);
    m_toggleBottomPanelAction = toggleBottomPanelAction;   // 存成员，供后面同步状态

    viewMenu->addSeparator();

    // 项目面板 toggle（顺手做）
    QAction* toggleProjectPanelAction = viewMenu->addAction(tr("项目面板(&P)"));
    toggleProjectPanelAction->setShortcut(QKeySequence("Ctrl+Shift+E"));
    toggleProjectPanelAction->setCheckable(true);
    toggleProjectPanelAction->setChecked(true);
    toggleProjectPanelAction->setStatusTip(tr("显示 / 隐藏项目面板 (Ctrl+Shift+E)"));

    connect(toggleProjectPanelAction, &QAction::toggled, this, [this](bool checked) {
        if (d->projectDock) {
            d->projectDock->setVisible(checked);
        }
    });
    registerShortcut(QStringLiteral("view.projectPanel"), tr("项目面板"), toggleProjectPanelAction);
    m_toggleProjectPanelAction = toggleProjectPanelAction;

    QAction* resetLayoutAction = viewMenu->addAction(tr("重置窗口布局"));
    resetLayoutAction->setStatusTip(tr("恢复窗口到默认大小、位置和面板布局"));
    connect(resetLayoutAction, &QAction::triggered, this, [this]() {
        const auto reply = QMessageBox::question(
            this, tr("重置窗口布局"),
            tr("将窗口大小、位置和面板布局恢复为默认值。\n\n继续？"));
        if (reply != QMessageBox::Yes) return;

        // 清除保存的状态
        ConfigManager* cfg = Application::instance().configManager();
        if (cfg) {
            cfg->remove("Window/Geometry");
            cfg->remove("Window/State");
            cfg->remove("Window/Maximized");
        }

        // 恢复默认（可能需要重启才完全生效）
        resize(1600, 1000);
        // 或者更简单地提示用户"下次启动生效"
    });

    // ==================== 帮助菜单 ====================
    QMenu *helpMenu = menuBar->addMenu(tr("帮助(&H)"));

    QAction *quickStartAction = helpMenu->addAction(QIcon(":/icons/icons/run.svg"), tr("快速指南(&G)"));
    connect(quickStartAction, &QAction::triggered, this, &MainWindow::onQuickStart);

    QAction *documentationAction = helpMenu->addAction(QIcon(":/icons/icons/text.svg"), tr("文档中心(&D)"));
    connect(documentationAction, &QAction::triggered, this, &MainWindow::onDocumentation);

    QAction *aboutAction = helpMenu->addAction(QIcon(":/icons/icons/app_icon.svg"), tr("关于(&A)"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::setupToolBar()
{
    QToolBar *toolBar = addToolBar(tr("主工具栏"));
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(20, 20));

    QAction *newAction = toolBar->addAction(QIcon(":/icons/icons/new_file.svg"), tr("新建"));
    connect(newAction, &QAction::triggered, this, &MainWindow::onNewFile);

    QAction *openAction = toolBar->addAction(QIcon(":/icons/icons/open_file.svg"), tr("打开"));
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenFile);

    QAction *saveAction = toolBar->addAction(QIcon(":/icons/icons/save_file.svg"), tr("保存"));
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveFile);

    toolBar->addSeparator();

    QAction *buildAction = toolBar->addAction(QIcon(":/icons/icons/build.svg"),tr("编译"));
    connect(buildAction, &QAction::triggered, this, &MainWindow::onBuildProject);

    m_runAction = toolBar->addAction(QIcon(":/icons/icons/run.svg"), tr("运行"));
    m_runAction->setShortcut(QKeySequence("Ctrl+R"));
    m_runAction->setToolTip(tr("运行 (Ctrl+R)"));
    connect(m_runAction, &QAction::triggered, this, &MainWindow::onRun);
    registerShortcut(QStringLiteral("build.run"), tr("运行"), m_runAction);

    QAction *debugAction = toolBar->addAction(QIcon(":/icons/icons/debug.svg"),tr("调试"));
    connect(debugAction, &QAction::triggered, this, &MainWindow::onStartDebug);
}

void MainWindow::setupStatusBar()
{
    QStatusBar *statusBar = this->statusBar();

    // 主状态标签
    m_statusLabel = new QLabel(tr("就绪"), statusBar);
    statusBar->addWidget(m_statusLabel, 1);  // stretch = 1

    // 分隔线
    statusBar->addPermanentWidget(new QLabel(" | "));

    // 行号/列号
    m_cursorPositionLabel = new QLabel(tr("行: 1  列: 1"), statusBar);
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
    m_indentLabel = new QLabel(tr("空格: 4"), statusBar);
    statusBar->addPermanentWidget(m_indentLabel);

    // 编译进度条（默认隐藏）
    m_buildProgressBar = new QProgressBar(statusBar);
    m_buildProgressBar->setMaximumSize(150, 20);
    m_buildProgressBar->setMinimum(0);
    m_buildProgressBar->setMaximum(100);
    m_buildProgressBar->setValue(0);
    m_buildProgressBar->setVisible(false);
    statusBar->addPermanentWidget(m_buildProgressBar);

    // 性能监控指示器
    m_performanceLabel = new QLabel(statusBar);
    m_performanceLabel->setObjectName("PerformanceIndicator");
    m_performanceLabel->setTextInteractionFlags(Qt::NoTextInteraction);
    m_performanceLabel->setToolTip(tr("性能监控"));
    // 固定一个最小宽度，避免数值变化时状态栏右侧抖动
    m_performanceLabel->setMinimumWidth(170);
    statusBar->addPermanentWidget(m_performanceLabel);

    qRegisterMetaType<SprintIDE::PerformanceSnapshot>("SprintIDE::PerformanceSnapshot");

    m_performanceMonitor = new PerformanceMonitor(this);
    connect(m_performanceMonitor, &PerformanceMonitor::snapshotUpdated,
            this, &MainWindow::onPerformanceSnapshot);
    m_performanceMonitor->start(1000);

    // 更新光标位置（初始）
    onTabCurrentChanged(0);
}

void MainWindow::setupDockWidgets()
{
    ThemeManager& tm = ThemeManager::instance();

    // ============================== 项目面板（左侧） ==============================
    d->projectDock = new QDockWidget(tr("项目面板"), this);
    d->projectDock->setObjectName("ProjectDock");
    d->projectDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    connect(d->projectDock, &QDockWidget::visibilityChanged,
            this, [this](bool visible) {
                if (m_toggleProjectPanelAction) {
                    QSignalBlocker blocker(m_toggleProjectPanelAction);
                    m_toggleProjectPanelAction->setChecked(visible);
                }
            });

    d->projectPanel = new ProjectPanel(d->projectDock);
    d->projectDock->setWidget(d->projectPanel);
    addDockWidget(Qt::LeftDockWidgetArea, d->projectDock);

    d->projectPanel->setShowHiddenFiles(false);
    d->projectPanel->setAlternatingRowColors(false);

    //    只需调用 ThemeManager，不再手动拼 QSS
    tm.applyThemeToTreeView(d->projectPanel->getFileTreeView());

    // =============================== 底部面板 =============================
    m_bottomDockWidget = new QDockWidget(tr("面板"), this);
    m_bottomDockWidget->setObjectName("BottomPanel");
    m_bottomDockWidget->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    m_bottomDockWidget->setFeatures(QDockWidget::DockWidgetClosable |
                                    QDockWidget::DockWidgetMovable);
    m_bottomDockWidget->setMinimumHeight(0);
    m_bottomDockWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    connect(m_bottomDockWidget, &QDockWidget::visibilityChanged,
                this, [this](bool visible) {
                    if (m_toggleBottomPanelAction) {
                        QSignalBlocker blocker(m_toggleBottomPanelAction);
                        m_toggleBottomPanelAction->setChecked(visible);
                    }
                });

    m_bottomTabWidget = new QTabWidget(m_bottomDockWidget);
    m_bottomTabWidget->setObjectName("BottomTabWidget");
    m_bottomTabWidget->tabBar()->setObjectName("BottomTabBar");
    m_bottomTabWidget->setTabPosition(QTabWidget::South);
    m_bottomTabWidget->setDocumentMode(true);
    m_bottomTabWidget->setTabsClosable(false);
    m_bottomTabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // =============================== 输出面板 =============================
    // 结构化日志面板：LogListModel + LogListView，支持多选复制、过滤、双击跳转
    m_outputPanel = new OutputPanel(m_bottomTabWidget);
    m_outputPanel->setObjectName("OutputPanel");
    connect(m_outputPanel, &OutputPanel::aiExplainErrorRequested,
            this, &MainWindow::onAIExplainError);

    // 双击跳转：错误日志带 file:line 时打开文件定位
    connect(m_outputPanel, &OutputPanel::jumpRequested,
            this, [this](const QString& file, int line, int column) {
                if (file.isEmpty()) return;
                openFile(file);
                if (TextEditor* ed = findEditorByFilePath(file)) {
                    d->editorTabs->setCurrentWidget(ed);
                    ed->gotoLine(line);

                    // 若已知列号 → 精确定位到列
                    if (column > 0) {
                        QTextCursor c = ed->textCursor();
                        c.movePosition(QTextCursor::Right,
                                       QTextCursor::MoveAnchor,
                                       column - 1);
                        ed->setTextCursor(c);
                    }
                    ed->setFocus();
                }
            });

    m_bottomTabWidget->addTab(m_outputPanel,
                              QIcon(":/icons/icons/run_view.svg"),
                              tr("输出"));

    // ============================================================
    //  底部容器：左侧窄条 + 右侧 Tab
    // ============================================================
    QWidget* bottomContainer = new QWidget(m_bottomDockWidget);
    QHBoxLayout* bottomLayout = new QHBoxLayout(bottomContainer);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(0);

    // ---------- 左侧窄条 ----------
    QWidget* strip = new QWidget(bottomContainer);
    strip->setObjectName("BottomStrip");
    strip->setFixedWidth(32);                                        // 28 → 32
    strip->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    QVBoxLayout* stripLayout = new QVBoxLayout(strip);
    stripLayout->setContentsMargins(3, 6, 3, 6);                     // 内边距加大
    stripLayout->setSpacing(4);                                      // 间距加大

    // ---------- 通用工厂：带图标 + 可选角色属性 ----------
    auto makeStripButton = [&](const QString& iconPath,
                                const QString& tip,
                                const QString& role) -> QToolButton* {
        auto* b = new QToolButton(strip);
        b->setIcon(QIcon(iconPath));
        b->setIconSize(QSize(14, 14));                               // 14×14
        b->setToolTip(tip);
        b->setFixedSize(26, 26);                                     // 24 → 26
        b->setCheckable(false);
        b->setFocusPolicy(Qt::NoFocus);
        b->setCursor(Qt::PointingHandCursor);
        b->setProperty("role", role);                                // 用于 QSS 区分
        return b;
    };

    // ---- 查看断点 ----
    m_viewBreakpointsButton = makeStripButton(
        ":/icons/icons/toggle_break_point.svg",
        tr("查看断点"),
        "primary");
    m_viewBreakpointsButton->setCheckable(true);
    m_viewBreakpointsButton->setChecked(false);
    connect(m_viewBreakpointsButton, &QToolButton::clicked,
            this, &MainWindow::onToggleBreakpointTab);

    // ---- 禁用全部断点 ----
    m_muteBreakpointsButton = makeStripButton(
        ":/icons/icons/breakpoints_mute.svg",   // 新增图标
        tr("禁用所有断点"),
        "warning");
    m_muteBreakpointsButton->setCheckable(true);
    m_muteBreakpointsButton->setChecked(false);
    connect(m_muteBreakpointsButton, &QToolButton::clicked,
            this, &MainWindow::onToggleMuteBreakpoints);

    stripLayout->addWidget(m_viewBreakpointsButton);
    stripLayout->addWidget(m_muteBreakpointsButton);
    stripLayout->addStretch(1);

    // ---------- 组装 ----------
    bottomLayout->addWidget(strip);
    bottomLayout->addWidget(m_bottomTabWidget, 1);

    m_bottomDockWidget->setWidget(bottomContainer);
    addDockWidget(Qt::BottomDockWidgetArea, m_bottomDockWidget);

    // 不再 setGeometry，改用 resizeDocks 设定初始高度
    resizeDocks({m_bottomDockWidget}, {200}, Qt::Vertical);
    m_bottomDockWidget->setVisible(true);
    m_bottomDockWidget->setFloating(false);

    connect(m_bottomTabWidget, &QTabWidget::currentChanged,
                this, [this](int index) {
                    QWidget* w = m_bottomTabWidget->widget(index);
                    if (!w) return;

                    int targetHeight = 300;      // 默认

                    if (w == m_debugPanel) {
                        targetHeight = 420;
                    } else if (w == m_terminalPanel) {
                        targetHeight = 300;
                    } else if (w == m_outputPanel) {
                        targetHeight = 300;
                    }

                    // 避免小于最小值
                    targetHeight = qMax(targetHeight, 100);

                    resizeDocks({m_bottomDockWidget}, {targetHeight}, Qt::Vertical);
                });

    // ============================================================
    // 四、删除请求信号
    // ============================================================
    connect(d->projectPanel, &ProjectPanel::deleteRequested,
            this, [this](const QStringList& filePaths) {
                bool canDelete = true;
                for (const QString& path : filePaths) {
                    if (!closeEditorForFile(path)) {
                        canDelete = false;
                        break;
                    }
                }
                if (!canDelete) {
                    logInfo("删除操作已取消");
                    return;
                }
                for (const QString& path : filePaths) {
                    d->projectPanel->deleteFile(path, true);
                    updateWindowTitle();
                }
            });

    qDebug() << "[SDW] setupDockWidgets 完成";
}

void MainWindow::setupConnections()
{
    connect(d->editorTabs, &TabWidget::tabCloseRequested,
            this, &MainWindow::onTabCloseRequested);
    connect(d->editorTabs, &TabWidget::currentChanged,
            this, &MainWindow::onTabCurrentChanged);
    // 编辑器标签栏右键菜单
    //   TabWidget 内部 CustomTabBar 已经在 contextMenuEvent 里发这个信号了，
    //   这里只需要接住即可。pos 是全局坐标
    connect(d->editorTabs, &TabWidget::contextMenuRequested,
                this, &MainWindow::onTabContextMenuRequested);
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
                                              logInfo(output);
                                          });
            });
    // ========== 连接测试运行请求 ==========
    connect(d->projectPanel, &ProjectPanel::runTestRequested,
            this, [this](const QString& filePath, const QString& testFullName) {
                if (filePath.isEmpty() || testFullName.isEmpty()) {
                    logInfo("测试信息不完整");
                    return;
                }

                QString projectPath = d->projectPanel->currentProjectPath();
                if (projectPath.isEmpty()) {
                    logInfo("未打开项目，无法运行测试");
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

                logInfo(QString("▶ 运行测试: %1").arg(testFullName));

                // 执行测试（异步，输出实时显示）
                PythonTestRunner::runTest(projectPath, test,
                                          [this](const QString& output) {
                                              // 将输出显示到输出面板
                                              if (!output.isEmpty()) {
                                                  logInfo(output);
                                              }
                                          });
            });
    // ============================================================
    //  项目面板：定位 / 刷新
    // ============================================================
    // "定位"按钮 → 拿当前编辑器文件 → 调 locateFile
    connect(d->projectPanel, &ProjectPanel::locateCurrentFileRequested,
            this, [this]() {
                TextEditor* editor = currentEditor();
                if (!editor) return;
                const QString filePath = editor->filePath();
                if (filePath.isEmpty()) return;

                // 文件不在当前项目根目录下 → 提示并跳过
                if (!d->currentProjectPath.isEmpty()
                    && !filePath.startsWith(d->currentProjectPath)) {
                    logInfo(QString("文件 %1 不在当前项目内，无法定位")
                                .arg(QFileInfo(filePath).fileName()));
                    return;
                }

                d->projectPanel->locateFile(filePath);
            });

    // ② 对外信号 → 项目面板刷新槽
    connect(this, &MainWindow::projectPanelRefreshRequested,
            d->projectPanel, &ProjectPanel::onRefreshRequested,
            Qt::UniqueConnection);

    // ③ 窗口激活防抖定时器
    m_projectPanelRefreshDebounce = new QTimer(this);
    m_projectPanelRefreshDebounce->setSingleShot(true);
    m_projectPanelRefreshDebounce->setInterval(300);   // 300ms 防抖
    connect(m_projectPanelRefreshDebounce, &QTimer::timeout,
            this, [this]() {
                emit projectPanelRefreshRequested();
            });
}

// ========== 调试面板设置 ==========

void MainWindow::setupDebugPanel()
{
    m_debugPanel = new DebugPanel(this);
    int index = m_bottomTabWidget->addTab(m_debugPanel, QIcon(":/icons/icons/debug.svg"), tr("调试"));
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
    m_bottomTabWidget->addTab(m_terminalPanel, QIcon(":/icons/icons/terminal.svg"), tr("终端"));

    if (!d->currentProjectPath.isEmpty()) {
        m_terminalPanel->setWorkingDirectory(d->currentProjectPath);
    }
}

// ========== Git 面板设置 ==========

void MainWindow::setupGitPanel()
{
    // ============================================================
    // 1. 创建非模态独立窗口（GitPanel 不占用右侧停靠区）
    //     - Qt::Window：作为独立顶层窗口，可单独最小化
    //     - setModal(false)：不阻塞主窗口编辑
    //     - WA_DeleteOnClose=false：关闭只是 hide，保留状态
    // ============================================================
    m_gitDialog = new QDialog(this);
    m_gitDialog->setWindowTitle("Git");
    m_gitDialog->setModal(false);
    m_gitDialog->setWindowFlag(Qt::Window, true);
    m_gitDialog->setAttribute(Qt::WA_DeleteOnClose, false);
    m_gitDialog->resize(760, 820);

    // 2. Panel 装进 dialog
    m_gitPanel = new Git::GitPanel(m_gitDialog);

    auto* layout = new QVBoxLayout(m_gitDialog);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_gitPanel);

    // 3. 默认隐藏（打开项目且是 git 仓库时才提示存在）
    m_gitDialog->hide();

    // 4. 双击文件 → 打开编辑器
    connect(m_gitPanel, &Git::GitPanel::fileDoubleClicked,
            this, &MainWindow::onGitFileDoubleClicked);

    // 5 提交前身份检查失败 → 打开设置（定位到 Git 页）
    connect(m_gitPanel, &Git::GitPanel::openSettingsRequested,
            this, &MainWindow::onOpenGitSettingsFromPanel);

    // 6. 仓库/状态变化 → 更新菜单可用性 + 状态栏指示器
    connect(&Git::GitManager::instance(), &Git::GitManager::repositoryChanged,
            this, [this](const Git::GitRepositoryInfo&) {
                updateGitActions();
                updateGitStatusIndicator();
            });
    connect(&Git::GitManager::instance(), &Git::GitManager::statusUpdated,
            this, [this](const QList<Git::GitFileStatus>&) {
                updateGitActions();
                updateGitStatusIndicator();
            });

    // 7. 命令失败 → 输出面板
    if (d->projectPanel) {
        connect(&Git::GitManager::instance(), &Git::GitManager::statusUpdated,
                d->projectPanel, &ProjectPanel::onGitStatusUpdated);

        // 7.5 ProjectPanel 右键 Git 子菜单动作 → MainWindow 分发
        //     用 lambda 中转，避免 signal 参数类型跨头文件的 moc 问题
        connect(d->projectPanel, &ProjectPanel::gitActionRequested,
                this, [this](SprintIDE::GitContextAction action,
                             const QStringList& paths) {
            onProjectPanelGitAction(static_cast<int>(action), paths);
        });
    }

    // 8. Git 状态更新 → 转发给项目面板（文件颜色标记）
    if (d->projectPanel) {
        connect(&Git::GitManager::instance(), &Git::GitManager::statusUpdated,
                d->projectPanel, &ProjectPanel::onGitStatusUpdated);
    }

    // 9. 状态栏 Git 指示器
    m_gitStatusLabel = new QLabel(this);
    m_gitStatusLabel->setObjectName("GitStatusIndicator");
    m_gitStatusLabel->setCursor(Qt::PointingHandCursor);
    m_gitStatusLabel->setToolTip(QStringLiteral("点击打开 Git 窗口"));
    m_gitStatusLabel->setTextInteractionFlags(Qt::NoTextInteraction);
    m_gitStatusLabel->installEventFilter(this);   // 拦截鼠标点击（见下方 eventFilter）
    m_gitStatusLabel->setVisible(false);
    statusBar()->addPermanentWidget(m_gitStatusLabel);

    updateGitActions();
    qDebug() << "[MW] setupGitPanel 完成 (dialog mode)";
}

// ============================================================================
//  断点列表面板：延迟到 m_debugManager 初始化之后再创建
// ============================================================================
void MainWindow::setupBreakpointListPanel()
{
    if (!m_debugManager) {
        qWarning() << "[MW] setupBreakpointListPanel: m_debugManager 为空，跳过";
        return;
    }
    if (m_breakpointListPanel) {
        return;   // 已创建，幂等
    }

    m_breakpointListPanel = new BreakpointListPanel(m_debugManager, m_bottomTabWidget);

    int idx = m_bottomTabWidget->addTab(
        m_breakpointListPanel,
        QIcon(":/icons/icons/toggle_break_point.svg"),
        tr("断点"));
    m_bottomTabWidget->setTabVisible(idx, false);

    // 双击 → 打开文件 + 跳转
    connect(m_breakpointListPanel, &BreakpointListPanel::breakpointActivated,
            this, [this](const QString& file, int line) {
                openFile(file);
                if (TextEditor* ed = findEditorByFilePath(file)) {
                    d->editorTabs->setCurrentWidget(ed);
                    ed->gotoLine(line);
                    ed->setFocus();
                }
            });

    // 删除断点
    connect(m_breakpointListPanel, &BreakpointListPanel::breakpointRemoveRequested,
            this, [this](const QString& file, int line) {
                if (!m_debugManager) return;
                if (m_debugManager->hasBreakpoint(file, line)) {
                    m_debugManager->toggleBreakpoint(file, line);
                }
                if (TextEditor* ed = findEditorByFilePath(file)) {
                    ed->setBreakpoint(line, false);
                    ed->viewport()->update();
                }
                if (m_cppDebugger && m_isDebugging) {
                    m_cppDebugger->setBreakpoints(m_debugManager->getBreakpoints());
                }
                if (m_pythonDebugger && m_isDebugging) {
                    m_pythonDebugger->setBreakpoints(m_debugManager->getBreakpoints());
                }
                refreshBreakpointListPanel();
            });

    // 启用 / 禁用
    connect(m_breakpointListPanel, &BreakpointListPanel::breakpointToggleEnabledRequested,
            this, [this](const QString& file, int line, bool enabled) {
                if (!m_debugManager) return;
                m_debugManager->setBreakpointEnabled(file, line, enabled);
                if (TextEditor* ed = findEditorByFilePath(file)) {
                    bool hasCond = false;
                    for (const auto& bp : m_debugManager->getBreakpoints()) {
                        if (bp.filePath == file && bp.lineNumber == line) {
                            hasCond = !bp.condition.trimmed().isEmpty();
                            break;
                        }
                    }
                    ed->setBreakpointWithCondition(line, enabled, hasCond);
                    ed->viewport()->update();
                }
                if (m_cppDebugger && m_isDebugging) {
                    m_cppDebugger->setBreakpoints(m_debugManager->getBreakpoints());
                }
                if (m_pythonDebugger && m_isDebugging) {
                    m_pythonDebugger->setBreakpoints(m_debugManager->getBreakpoints());
                }
                refreshBreakpointListPanel();
            });

    // 编辑条件
    connect(m_breakpointListPanel, &BreakpointListPanel::breakpointEditConditionRequested,
            this, [this](const QString& file, int line) {
                if (!m_debugManager) return;

                QString currentCond;
                for (const auto& bp : m_debugManager->getBreakpoints()) {
                    if (bp.filePath == file && bp.lineNumber == line) {
                        currentCond = bp.condition;
                        break;
                    }
                }

                bool ok = false;
                const QString cond = QInputDialog::getText(
                    this, tr("编辑断点条件"),
                    tr("第 %1 行的断点条件（留空 = 无条件）：").arg(line),
                    QLineEdit::Normal, currentCond, &ok);
                if (!ok) return;

                m_debugManager->setBreakpointCondition(file, line, cond);

                bool enabled = true;
                for (const auto& bp : m_debugManager->getBreakpoints()) {
                    if (bp.filePath == file && bp.lineNumber == line) {
                        enabled = bp.enabled;
                        break;
                    }
                }
                if (TextEditor* ed = findEditorByFilePath(file)) {
                    ed->setBreakpointWithCondition(
                        line, enabled, !cond.trimmed().isEmpty());
                    ed->viewport()->update();
                }
                refreshBreakpointListPanel();
            });

    qDebug() << "[MW] setupBreakpointListPanel 完成";
}

// ============================================================================
//  断点列表：刷新
// ============================================================================
void MainWindow::refreshBreakpointListPanel()
{
    if (m_breakpointListPanel) {
        m_breakpointListPanel->refresh();
    }
}

// ============================================================================
//  窄条按钮：查看断点（toggle）
// ============================================================================
void MainWindow::onToggleBreakpointTab()
{
    // 若 m_debugManager 已就绪但面板还没建 → 现场创建
    if (!m_breakpointListPanel && m_debugManager) {
        setupBreakpointListPanel();
    }

    if (!m_bottomTabWidget || !m_breakpointListPanel) return;

    const int idx = m_bottomTabWidget->indexOf(m_breakpointListPanel);
    if (idx < 0) return;

    const bool wantShow = !m_bottomTabWidget->isTabVisible(idx);

    if (wantShow) {
        m_bottomTabWidget->setTabVisible(idx, true);
        m_bottomTabWidget->setCurrentIndex(idx);
        refreshBreakpointListPanel();
    } else {
        // 若当前正在断点页签上，先切到输出再隐藏
        if (m_bottomTabWidget->currentIndex() == idx) {
            switchToOutputTab();
        }
        m_bottomTabWidget->setTabVisible(idx, false);
    }

    if (m_viewBreakpointsButton) {
        QSignalBlocker b(m_viewBreakpointsButton);
        m_viewBreakpointsButton->setChecked(wantShow);
    }
}

// ============================================================================
//  窄条按钮：禁用 / 恢复所有断点
// ============================================================================
void MainWindow::onToggleMuteBreakpoints()
{
    if (!m_debugManager) return;

    const QList<Breakpoint> bps = m_debugManager->getBreakpoints();
    if (bps.isEmpty()) {
        if (m_statusLabel) m_statusLabel->setText(tr("没有断点可禁用"));
        if (m_muteBreakpointsButton) {
            QSignalBlocker b(m_muteBreakpointsButton);
            m_muteBreakpointsButton->setChecked(false);
        }
        m_breakpointsMuted = false;
        return;
    }

    const bool target = !m_breakpointsMuted;   // 目标状态

    for (const Breakpoint& bp : bps) {
        m_debugManager->setBreakpointEnabled(bp.filePath, bp.lineNumber, !target);
    }
    m_breakpointsMuted = target;

    // 同步所有打开编辑器的断点视觉
    for (int i = 0; i < d->editorTabs->count(); ++i) {
        TextEditor* ed = qobject_cast<TextEditor*>(d->editorTabs->widget(i));
        if (!ed) continue;
        const QString file = ed->filePath();
        if (file.isEmpty()) continue;

        // 该文件的所有断点，重新按当前状态画
        for (const Breakpoint& bp : bps) {
            if (bp.filePath != file) continue;
            bool hasCond = !bp.condition.trimmed().isEmpty();
            ed->setBreakpointWithCondition(bp.lineNumber, !target, hasCond);
        }
        ed->viewport()->update();
    }

    // 调试器同步
    const QList<Breakpoint> after = m_debugManager->getBreakpoints();
    if (m_cppDebugger && m_isDebugging) {
        m_cppDebugger->setBreakpoints(after);
    }
    if (m_pythonDebugger && m_isDebugging) {
        m_pythonDebugger->setBreakpoints(after);
    }

    // 更新按钮状态与提示
    if (m_muteBreakpointsButton) {
        QSignalBlocker b(m_muteBreakpointsButton);
        m_muteBreakpointsButton->setChecked(target);
        m_muteBreakpointsButton->setToolTip(
            target ? tr("恢复所有断点") : tr("禁用所有断点"));
    }

    if (m_statusLabel) {
        m_statusLabel->setText(target ? tr("已禁用所有断点") : tr("已恢复所有断点"));
    }

    refreshBreakpointListPanel();
}

void MainWindow::logInfo(const QString& msg, LogKind kind, const QString& source)
{
    appendLog(LogEntry::make(kind, LogLevel::Info, source, msg));
}
void MainWindow::logWarn(const QString& msg, LogKind kind, const QString& source)
{
    appendLog(LogEntry::make(kind, LogLevel::Warning, source, msg));
}
void MainWindow::logError(const QString& msg, LogKind kind, const QString& source)
{
    appendLog(LogEntry::make(kind, LogLevel::Error, source, msg));
}

void MainWindow::applyTheme()
{
    // 主题应用留空，后续实现
}

// ==================== 窗口事件处理 ====================

void MainWindow::closeEvent(QCloseEvent *event)
{
    // 退出前先停掉正在运行的程序
    if (isRunningProgram()) {
        stopRunningProcess();
    }

    if (!confirmUnsavedChanges()) {
        event->ignore();
        return;
    }

    if (m_isCompiling) {
        int reply = QMessageBox::question(this, tr("正在编译"),
                                          tr("当前正在编译，是否停止编译并退出？"),
                                          QMessageBox::Yes | QMessageBox::No,
                                          QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            stopCompile();
        } else {
            event->ignore();
            return;
        }
    }

    if (m_lspManager) {
        m_lspManager->shutdownAll();
    }

    saveSession();          // 项目级 session
    saveWindowState();      // 全局窗口状态

    // 保存 AI 面板状态
    if (m_aiChatPanel) {
        saveChatPanelState(m_aiChatPanel);
    }

    event->accept();
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    // 状态栏 Git 指示器：左键点击 → 打开 Git 窗口
    if (watched == m_gitStatusLabel && event->type() == QEvent::MouseButtonRelease) {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            onGitStatusLabelClicked();
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

// ============================================================
//  窗口激活 → 请求刷新项目面板
//
//  使用 300ms 防抖，避免：
//    - 从子对话框返回时立刻触发（此时不需要刷）
//    - 短时间多次激活叠加刷新
// ============================================================
void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowActivate) {
        if (m_projectPanelRefreshDebounce) {
            m_projectPanelRefreshDebounce->start();
        }
    }
    QMainWindow::changeEvent(event);
}

bool MainWindow::confirmUnsavedChanges()
{
    qDebug() << "[MW::confirmUnsavedChanges] 遍历编辑器检查未保存";

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
        msgBox.setWindowTitle(tr("未保存的更改"));
        msgBox.setText(tr("有未保存的更改，是否保存？"));
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
        QString(tr("行: %1  列: %2"))
            .arg(editor->currentLine())
            .arg(editor->currentColumn())
        );
}

void MainWindow::updateWindowTitle()
{
    QString title = "SprintIDE";

    TextEditor* editor = currentEditor();
    if (editor) {
        const QString fileName = editor->fileName();
        if (!fileName.isEmpty()) {
            title += " - " + fileName;
        }
    }

    // 直接使用计数器，避免每次按键遍历所有 tab
    if (m_dirtyTabCount > 0) {
        title += " *";
    }

    setWindowTitle(title);
}

void MainWindow::restoreWindowState()
{
    ConfigManager* cfg = Application::instance().configManager();
    if (!cfg) return;

    // ---- 窗口几何 ----
    const QByteArray geometry = cfg->value("Window/Geometry").toByteArray();
    if (!geometry.isEmpty()) {
        // restoreGeometry 会做屏幕有效性检查，失效则返回 false
        const bool ok = restoreGeometry(geometry);
        if (!ok) {
            qDebug() << "[Window] 恢复窗口几何失败（屏幕配置可能已变），使用默认布局";
            resize(1600, 1000);   // 你的默认尺寸
        }
    } else {
        // 首次启动：用默认尺寸
        resize(1600, 1000);
    }

    // ---- Dock 布局 ----
    const QByteArray state = cfg->value("Window/State").toByteArray();
    if (!state.isEmpty()) {
        const bool ok = restoreState(state);
        if (!ok) {
            qDebug() << "[Window] 恢复 Dock 布局失败，使用默认布局";
        }
    }

    qDebug() << "[Window] 窗口状态已恢复";
}

void MainWindow::saveWindowState()
{
    ConfigManager* cfg = Application::instance().configManager();
    if (!cfg) return;

    // ---- 窗口几何 + 最大化状态 ----
    cfg->setValue("Window/Geometry", saveGeometry());

    // ---- Dock 布局（项目面板宽度、底部面板高度、可见性等） ----
    cfg->setValue("Window/State", saveState());

    // ---- 显式记录"上次是否最大化"（saveGeometry 已含，但独立存更直观） ----
    cfg->setValue("Window/Maximized", isMaximized());

    qDebug() << "[Window] 窗口状态已保存";
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
        QMessageBox::warning(this, tr("打开项目失败"), tr("无法打开项目：") + path);
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

    // 加载项目文件列表
    // 注意：loadAllProjectFiles 会深度遍历目录，遇到符号链接可能无限递归。
    // 已经改为带过滤 + 深度限制的版本，见下方函数实现。
    loadAllProjectFiles(actualRoot);
    // 顺手把扫描结果缓存起来，供快速打开复用
    m_cachedProjectFiles = scanFilesForQuickOpen(actualRoot);
    m_cachedProjectRoot  = actualRoot;
    m_cachedScanTime     = QDateTime::currentDateTime();

    ProjectModel* model = pm->javaProjectModel();
    if (model) {
        QString typeStr;
        switch (model->type()) {
        case JavaProjectType::Maven: typeStr = "Maven"; break;
        case JavaProjectType::Gradle: typeStr = "Gradle"; break;
        default: typeStr = "Plain Java"; break;
        }
        logInfo(QString("检测到 Java 项目类型: %1").arg(typeStr));
        logInfo(QString("  主类: %1").arg(model->defaultMainClass()));
        logInfo(QString("  依赖数量: %1").arg(model->dependencyJars().size()));
        logInfo(QString("  类路径条目: %1").arg(model->classpath().size()));
    }

    logInfo(QString("项目已打开: %1").arg(d->currentProjectPath));

    // ========== Git 绑定 ==========
    if (m_gitPanel) {
        m_gitPanel->setProjectRoot(d->currentProjectPath);
        updateGitActions();
        updateGitStatusIndicator();
    }

    // 延迟恢复上次会话
    // 用 singleShot 确保当前 openProject 流程走完、UI 已就绪后再恢复，
    // 避免在 MainWindow 构造期间被 main.cpp 提前调用导致 tab 错乱。
    QTimer::singleShot(150, this, &MainWindow::restoreSession);
}


void MainWindow::closeProject()
{
    d->currentProjectPath.clear();

    // Git 解绑
    Git::GitManager::instance().setProjectRoot(QString());
    if (m_gitDialog) m_gitDialog->hide();

    // 清掉项目树上的 git 状态标记
    if (d->projectPanel) {
        d->projectPanel->onGitStatusUpdated(QList<Git::GitFileStatus>{});
    }

    updateGitActions();
    updateGitStatusIndicator();

    // updateWindowTitle();
}

void MainWindow::openFile(const QString &path)
{
    //    空路径 / 不存在的路径：直接返回，不创建编辑器，不弹窗
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

    // 2. 创建编辑器并加载文件
    TextEditor* editor = new TextEditor(this);

    QPointer<TextEditor> editorPtr = editor;
    connect(editor, &TextEditor::completionRequested,
            editor,
            [this, editorPtr](const QString& filePath, int line, int column) {
                if (!editorPtr || !m_lspManager) return;
                const QString projectRoot = d->currentProjectPath.isEmpty()
                    ? QFileInfo(filePath).absolutePath()
                    : d->currentProjectPath;
                LSPClient* client = m_lspManager->getClientForFile(filePath, projectRoot);
                if (!client) return;

                // 用 unique connection 保证同一个 editor + client 只连一次
                connect(client, &LSPClient::completionReceived,
                        editorPtr.data(),                          // 接收者 = 编辑器
                        &TextEditor::onLSPCompletionReceived,      // 成员函数指针
                        Qt::UniqueConnection);                     // 现在合法了

                client->requestCompletion(filePath, line, column);
            });
    // 跳转定义：编辑器的 definitionRequested → 找 client → 发请求
    // client 的 definitionReceived → MainWindow::onDefinitionReceived
    connect(editor, &TextEditor::definitionRequested,
            this, [this](const QString& filePath, int line, int column) {

        if (!m_lspManager) return;

        const QString projectRoot = d->currentProjectPath.isEmpty()
                                        ? QFileInfo(filePath).absolutePath()
                                        : d->currentProjectPath;

        LSPClient* client = m_lspManager->getClientForFile(filePath, projectRoot);
        if (!client) {
            qWarning() << "[Def] 无 LSP client:" << filePath;
            return;
        }

        // 用成员函数槽 + UniqueConnection：
        //    - 满足 Qt6 UniqueConnection 约束（不能是 lambda）
        //    - 同一 client 只连接一次（四元组去重）
        //    - MainWindow 析构时 Qt 自动断开，无需手动管理
        connect(client, &LSPClient::definitionReceived,
                this, &MainWindow::onDefinitionReceived,
                Qt::UniqueConnection);

        // ---- 客户端未就绪：等 serverReady 后再发请求 ----
        if (!client->isReady()) {
            QPointer<LSPClient> clientPtr = client;
            auto connPtr = std::make_shared<QMetaObject::Connection>();
            *connPtr = connect(client, &LSPClient::serverReady, this,
                               [clientPtr, filePath, line, column, connPtr]() {
                                   if (clientPtr && clientPtr->isReady()) {
                                       clientPtr->requestDefinition(filePath, line, column);
                                   }
                                   QObject::disconnect(*connPtr);
                               });
            return;
        }

        // ---- 客户端就绪：直接发请求 ----
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
        QMessageBox::warning(this, tr("打开错误"),
                             QString(tr("无法打开文件:\n%1")).arg(path));
        return;
    }

    int index = d->editorTabs->addTab(editor, editor->fileName());
    d->editorTabs->setCurrentIndex(index);
    logInfo(QString("已打开: %1").arg(path));
    // AI 辅助：连接信号 + 同步可用状态
    connect(editor, &TextEditor::aiAssistRequested,
            this, &MainWindow::onAIAssistRequested);
    editor->setAIAvailable(m_aiConnMgr && m_aiConnMgr->isReady());

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
    connect(editor, &TextEditor::documentChangedForLsp,
            this, [this](const QString& filePath, const QString& content) {
                if (!m_lspManager || filePath.isEmpty()) return;
                const QString projectPath = d->currentProjectPath.isEmpty()
                                                ? QFileInfo(filePath).absolutePath()
                                                : d->currentProjectPath;
                LSPClient* client = m_lspManager->getClientForFile(filePath, projectPath);
                if (client && client->isReady()) {
                    client->changeDocument(filePath, content);
                }
            });
    connect(editor, &TextEditor::breakpointToggled,
            this, &MainWindow::onBreakpointToggled);

    connect(editor, &TextEditor::breakpointConditionChanged,
            this, [this](const QString& file, int line, const QString& cond) {
                if (!m_debugManager) return;

                // 1. 写入 DebugManager（断点的权威数据源）
                m_debugManager->setBreakpointCondition(file, line, cond);

                // 2. 更新编辑器的视觉标记
                TextEditor* ed = findEditorByFilePath(file);
                if (ed) {
                    bool hasCond = !cond.trimmed().isEmpty();
                    bool enabled = m_debugManager->hasBreakpoint(file, line);
                    ed->setBreakpointWithCondition(line, enabled, hasCond);
                }

                // 3. 如果 C++ 调试正在进行，重新同步全部断点到 adapter
                if (m_cppDebugger && m_isDebugging) {
                    m_cppDebugger->setBreakpoints(m_debugManager->getBreakpoints());
                }
            });
    connect(editor, &TextEditor::quickFixRequested,
                this, &MainWindow::onQuickFixRequested);
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

    // ---- LSP 打开文档 ----
    if (m_lspManager) {
        QString projectPath = findProjectRootForFile(path);

        LSPClient *client = m_lspManager->getClientForFile(path, projectPath);
        if (client) {
            // 用 Qt::UniqueConnection 替代函数静态 Set
            connect(client, &LSPClient::diagnosticsReceived,
                    this, &MainWindow::onLSPDiagnostics,
                    Qt::UniqueConnection);

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
    connect(editor, &TextEditor::documentChangedForLsp,
            this, [this](const QString& filePath, const QString& content) {
                if (!m_lspManager || filePath.isEmpty()) return;
                const QString projectPath = d->currentProjectPath.isEmpty()
                                                ? QFileInfo(filePath).absolutePath()
                                                : d->currentProjectPath;
                LSPClient* client = m_lspManager->getClientForFile(filePath, projectPath);
                if (client && client->isReady()) {
                    client->changeDocument(filePath, content);
                }
            });
    connect(editor, &TextEditor::breakpointToggled, this, &MainWindow::onBreakpointToggled);
    connect(editor, &TextEditor::quickFixRequested,
                this, &MainWindow::onQuickFixRequested);
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
        msgBox.setWindowTitle(tr("未保存的更改"));
        msgBox.setText(QString(tr("文件 '%1' 有未保存的更改。\n是否保存更改？"))
                           .arg(editor->fileName()));
        msgBox.setIcon(QMessageBox::Question);
        QPushButton *saveButton = msgBox.addButton(tr("保存(&S)"), QMessageBox::AcceptRole);
        QPushButton *discardButton = msgBox.addButton(tr("不保存(&D)"), QMessageBox::RejectRole);
        QPushButton *cancelButton = msgBox.addButton(tr("取消(&C)"), QMessageBox::NoRole);
        msgBox.setDefaultButton(saveButton);
        msgBox.exec();

        QAbstractButton* clicked = msgBox.clickedButton();
        if (clicked == saveButton) {
            if (!editor->saveFile()) {
                return false; // 保存失败，视为取消
            }
            d->editorTabs->removeTab(index);
            onEditorModificationChanged(false);
            return true;
        } else if (clicked == discardButton) {
            d->editorTabs->removeTab(index);
            onEditorModificationChanged(false);
            return true;
        } else { // 取消
            return false;
        }
    } else {
        d->editorTabs->removeTab(index);
        onEditorModificationChanged(false);
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
                m_statusLabel->setText(QString(tr("正在编辑: %1")).arg(editor->fileName()));
            }
        }
    } else {
        m_cursorPositionLabel->setText(tr("行: -  列: -"));
        m_statusLabel->setText(tr("就绪"));
    }
}

// ============================================================================
//  编辑器标签栏：右键菜单
//
//  信号来自 TabWidget::contextMenuRequested（内部由 CustomTabBar 发出）
//    - index : 被右键的 tab 索引
//    - pos   : 全局坐标（CustomTabBar 用的是 event->globalPos()）
//
//  关闭批量操作时索引会漂移，所以：
//    1. 用 QPointer<QWidget> 锁定目标 tab 本身，而不是只记 index
//    2. 倒序关闭，避免 removeTab 造成的索引错位
//    3. 用户取消保存时中止整个批处理（与 PyCharm 行为一致）
// ============================================================================
void MainWindow::onTabContextMenuRequested(int index, const QPoint &pos)
{
    if (!d->editorTabs) return;
    if (index < 0 || index >= d->editorTabs->count()) return;

    // 用 QPointer 锁定目标 tab，防止菜单 exec() 阻塞期间 tab 列表被其他事件改动
    QPointer<QWidget> target = d->editorTabs->widget(index);
    if (!target) return;

    const int total = d->editorTabs->count();

    QMenu menu(this);
    QAction* actClose       = menu.addAction(tr("关闭"));
    QAction* actCloseLeft   = menu.addAction(tr("关闭左侧文件"));
    QAction* actCloseRight  = menu.addAction(tr("关闭右侧文件"));
    QAction* actCloseOthers = menu.addAction(tr("关闭其他文件"));

    // 边界禁用，语义更清晰
    actCloseLeft  ->setEnabled(index > 0);
    actCloseRight ->setEnabled(index < total - 1);
    actCloseOthers->setEnabled(total > 1);

    QAction* chosen = menu.exec(pos);   // pos 已是全局坐标
    if (!chosen) return;

    // exec() 期间 tab 顺序可能变化 → 用 target 重新定位当前索引
    const int cur = d->editorTabs->indexOf(target);
    if (cur < 0) return;                // target 已被别的路径关掉，安全退出

    // ---------- 关闭 ----------
    if (chosen == actClose) {
        closeEditorTab(cur);
        return;
    }

    // ---------- 关闭左侧 ----------
    // 倒序：[cur-1, cur-2, ..., 0]
    if (chosen == actCloseLeft) {
        for (int i = cur - 1; i >= 0; --i) {
            if (!closeEditorTab(i)) break;   // 用户取消 → 中止
        }
        return;
    }

    // ---------- 关闭右侧 ----------
    // 倒序：[count-1, ..., cur+1]
    if (chosen == actCloseRight) {
        for (int i = d->editorTabs->count() - 1; i > cur; --i) {
            if (!closeEditorTab(i)) break;
        }
        return;
    }

    // ---------- 关闭其他 ----------
    // 先右后左：右侧关闭不影响左侧和 cur 的绝对位置
    if (chosen == actCloseOthers) {
        for (int i = d->editorTabs->count() - 1; i > cur; --i) {
            if (!closeEditorTab(i)) return;
        }
        for (int i = cur - 1; i >= 0; --i) {
            if (!closeEditorTab(i)) return;
        }
        // 关完后把 target 切到前台
        const int finalIdx = d->editorTabs->indexOf(target);
        if (finalIdx >= 0) {
            d->editorTabs->setCurrentIndex(finalIdx);
        }
        return;
    }
}

// ==================== 编辑器信号处理 ====================

void MainWindow::onEditorModificationChanged(bool modified)
{
    Q_UNUSED(modified);

    // 重新统计（信号粒度粗但触发频率低，遍历成本可忽略）
    int cnt = 0;
    for (int i = 0; i < d->editorTabs->count(); ++i) {
        TextEditor* ed = qobject_cast<TextEditor*>(d->editorTabs->widget(i));
        if (ed && ed->isModified()) ++cnt;
    }
    m_dirtyTabCount = cnt;

    updateWindowTitle();
}

void MainWindow::onEditorCursorPositionChanged(int line, int column)
{
    if (m_cursorPositionLabel) {
        m_cursorPositionLabel->setText(QString(tr("行: %1  列: %2")).arg(line).arg(column));
    }
}

void MainWindow::onFileLoaded(const QString &filePath)
{
    logInfo(QString("已加载文件: %1").arg(filePath));
    m_statusLabel->setText(QString(tr("已加载: %1")).arg(QFileInfo(filePath).fileName()));
}

void MainWindow::onFileSaved(const QString &filePath)
{
    // qDebug() << "onFileSaved called, filePath:" << filePath;
    // qDebug() << "m_statusLabel:" << m_statusLabel;

    // logInfo(QString("已保存文件: %1").arg(filePath));

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
        m_statusLabel->setText(tr("文件已保存"));
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

    // Git 状态刷新（文件保存后对应的 git status 可能变化）
    // 防抖：快速连按 Ctrl+S 时只触发一次刷新，避免反复
    //   runAsync → cancelAsync → deleteLater 撞上生命周期问题
    if (Git::GitManager::instance().isActive()) {
        if (!m_gitRefreshDebounce) {
            m_gitRefreshDebounce = new QTimer(this);
            m_gitRefreshDebounce->setSingleShot(true);
            m_gitRefreshDebounce->setInterval(300);
            connect(m_gitRefreshDebounce, &QTimer::timeout, this, []() {
                if (Git::GitManager::instance().isActive()) {
                    Git::GitManager::instance().refreshStatus();
                }
            });
        }
        m_gitRefreshDebounce->start();
    }
}

void MainWindow::onFileRenamed(const QString& oldPath, const QString& newPath)
{
    if (oldPath.isEmpty() || newPath.isEmpty() || oldPath == newPath) return;

    qDebug() << "[MW] 文件重命名:" << oldPath << "→" << newPath;

    // ============================================================
    // 1. 同步所有打开该文件的编辑器
    // ============================================================
    int syncedCount = 0;
    for (int i = 0; i < d->editorTabs->count(); ++i) {
        TextEditor* editor = qobject_cast<TextEditor*>(d->editorTabs->widget(i));
        if (!editor) continue;
        if (editor->filePath() != oldPath) continue;

        // ---- 通知 LSP：关闭旧文档 ----
        if (m_lspManager) {
            const QString projectPath = d->currentProjectPath.isEmpty()
                                            ? QFileInfo(oldPath).absolutePath()
                                            : d->currentProjectPath;
            LSPClient* client = m_lspManager->getClientForFile(oldPath, projectPath);
            if (client && client->isReady()) {
                client->closeDocument(oldPath);
            }
        }

        // ---- 更新编辑器路径 ----
        editor->setFilePath(newPath);

        // ---- 更新标签页标题 ----
        d->editorTabs->setTabText(i, editor->fileName());

        // ---- 通知 LSP：打开新文档 ----
        if (m_lspManager) {
            const QString projectPath = d->currentProjectPath.isEmpty()
                                            ? QFileInfo(newPath).absolutePath()
                                            : d->currentProjectPath;
            LSPClient* client = m_lspManager->getClientForFile(newPath, projectPath);
            if (client && client->isReady()) {
                client->openDocument(newPath, editor->toPlainText());
            }
        }

        ++syncedCount;
    }

    // ============================================================
    // 2. 同步断点（DebugManager 里按 filePath 存断点）
    // ============================================================
    if (m_debugManager) {
        QList<Breakpoint> all = m_debugManager->getBreakpoints();
        bool changed = false;
        for (Breakpoint& bp : all) {
            if (bp.filePath == oldPath) {
                bp.filePath = newPath;
                changed = true;
            }
        }
        if (changed) {
            m_debugManager->clearAllBreakpoints();
            for (const Breakpoint& bp : all) {
                m_debugManager->addBreakpoint(bp.filePath, bp.lineNumber, bp.condition);
                if (!bp.enabled) {
                    m_debugManager->setBreakpointEnabled(bp.filePath, bp.lineNumber, false);
                }
            }
        }
    }

    // ============================================================
    // 3. 更新窗口标题（当前编辑文件若被重命名）
    // ============================================================
    updateWindowTitle();

    // ============================================================
    // 4. 状态栏 + 输出面板反馈
    // ============================================================
    if (m_statusLabel) {
        m_statusLabel->setText(QString(tr("已重命名: %1 → %2"))
                                   .arg(QFileInfo(oldPath).fileName(),
                                        QFileInfo(newPath).fileName()));
    }
    logInfo(QString("已重命名: %1 → %2").arg(oldPath, newPath));

    qDebug() << "[MW] 已同步" << syncedCount << "个打开的编辑器";
}

void MainWindow::onEditorTextChanged(TextEditor* editor)
{
    if (!editor) return;
    QTextDocument* doc = qobject_cast<QTextDocument*>(sender());
    if (!doc) return;

    d->hasUnsavedChanges = true;
    updateWindowTitle();
    // LSP 的 changeDocument 已移到 TextEditor 侧防抖（见 documentChangedForLsp）
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

    refreshBreakpointListPanel();
}

void MainWindow::onPythonDebugFinished()
{
    // 幂等：避免 exited + terminated 双触发时重复清理
    if (!m_isDebugging && !m_debugPanel)
        return;

    m_isDebugging = false;

    // 清除当前调试行高亮
    if (TextEditor* editor = currentEditor()) {
        editor->clearCurrentDebugLine();
    }

    // 更新调试动作的启用/禁用
    updateDebugActions();

    // 清空调试面板
    if (m_debugPanel) {
        m_debugPanel->setDebuggingState(false);
        m_debugPanel->updateVariables(QList<DebugVariable>());
        m_debugPanel->updateCallStack(QList<StackFrame>());
    }

    // 状态栏提示
    if (m_statusLabel) {
        m_statusLabel->setText(tr("调试已结束"));
    }

    // 切回输出面板
    switchToOutputTab();

    setDebugTabVisible(false);

    logInfo(QString("调试会话已结束"));
    // 调试面板控制台也打一条结束线
    if (m_debugPanel) {
        m_debugPanel->appendConsoleOutput(tr("=== 调试会话结束 ==="), false);
    }
}

// ==================== 项目面板信号处理 ====================

void MainWindow::onProjectItemDoubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index);
    logInfo("项目面板开发中...");
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

    m_outputPanel->clear();

    appendLog(LogEntry::make(
        LogKind::Compile, LogLevel::Info, "Compile",
        QStringLiteral("开始编译: %1").arg(sourceFile),
        sourceFile));

    m_statusLabel->setText(QString(tr("正在编译 %1...")).arg(QFileInfo(sourceFile).fileName()));

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
        progressStr = QString(tr("正在编译... %1% - %2")).arg(progress).arg(currentFile);
    } else {
        progressStr = QString(tr("正在编译... %1%")).arg(progress);
    }
    m_statusLabel->setText(progressStr);

    if (progress % 25 == 0) {
        QString timeStr = QTime::currentTime().toString("hh:mm:ss");
        logError(QString(tr("[%1] 编译进度: %2%"))
                     .arg(timeStr)
                     .arg(progress));
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

        LogLevel level = LogLevel::Info;
        if (errorRegex.match(trimmed).hasMatch()) {
            level = LogLevel::Error;
        } else if (warningRegex.match(trimmed).hasMatch()) {
            level = LogLevel::Warning;
        } else if (noteRegex.match(trimmed).hasMatch()) {
            // note 级别 → Info（颜色已交给 LogLevel，不再需要单独颜色）
        } else if (progressRegex.match(trimmed).hasMatch()) {
            // 进度 → Info
        } else if (isError) {
            level = LogLevel::Error;
        }

        SprintIDE::ErrorMarker marker = parseErrorLine(trimmed);
        if (marker.line > 0 && !marker.filePath.isEmpty()) {
            // 相对路径 → 依次尝试相对项目根 / <项目根>/build 解析成绝对路径
            QString absPath = marker.filePath;
            if (!QFileInfo(absPath).isAbsolute()) {
                const QString projRoot = d->currentProjectPath;
                if (!projRoot.isEmpty()) {
                    const QString cand1 = QDir(projRoot).absoluteFilePath(marker.filePath);
                    const QString cand2 = QDir(projRoot + "/build").absoluteFilePath(marker.filePath);
                    if (QFileInfo::exists(cand1))      absPath = cand1;
                    else if (QFileInfo::exists(cand2)) absPath = cand2;
                }
            }

            appendLog(LogEntry::make(LogKind::Compile, level, "CompileOutput",
                                     line,                                   // message
                                     absPath,                                // filePath
                                     marker.line,                            // lineNumber
                                     marker.column > 0 ? marker.column : 1));
        } else {
            appendLog(LogEntry::make(LogKind::Compile, level, "CompileOutput", line));
        }
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
        appendLog(LogEntry::make(LogKind::Compile, LogLevel::Info, "Compile",
                         QStringLiteral("编译成功，耗时 %1ms").arg(elapsedMs)));

        if (!result.executablePath.isEmpty()) {
            logInfo(QString("可执行文件: %1")
                                       .arg(result.executablePath));
        }

        // 编译成功的细节统计
        QString statsLine = QString(tr(" 错误: %1, 警告: %2"))
                                .arg(result.errors.size())
                                .arg(result.warnings.size());
        logInfo(statsLine);

        m_statusLabel->setText(tr("编译成功"));

        // ========== 编译后自动运行（新增） ==========
        if (m_autoRunAfterCompile) {
            m_autoRunAfterCompile = false;
            if (!result.executablePath.isEmpty()) {
                logInfo("编译成功，正在运行...");
                // runProgram(result.executablePath, false);
                runExecutableAsync(result.executablePath);
            } else {
                logWarn("编译成功，但未找到可执行文件，无法自动运行。");
            }
        }

        // ========== 编译后自动启动调试（原有） ==========
        if (m_compileForDebug) {
            m_compileForDebug = false;

            if (result.executablePath.isEmpty()) {
                logWarn("编译成功，但未找到可执行文件，无法启动调试。");
            } else {
                // 判断当前编辑器语言
                TextEditor* editor = currentEditor();
                LanguageType lang = editor ? detectLanguageTypeFromPath(editor->filePath())
                                           : LanguageType::Unknown;

                if (lang == LanguageType::Cpp) {
                    //    C++：用 DAP 路径。重新进 onStartDebug()，这次 exePath 已存在
                    logInfo("编译成功，正在启动 C++ 调试...");
                    QTimer::singleShot(100, this, &MainWindow::onStartDebug);
                } else {
                    // 其他语言：保留旧逻辑
                    logInfo("编译成功，正在启动调试...");
                    syncBreakpointsToDebugger();
                    startDebuggingWithExecutable(result.executablePath);
                }
            }
        }

    } else {
        appendLog(LogEntry::make(
              LogKind::Compile, LogLevel::Error, "Compile",
              QStringLiteral("编译失败，耗时 %1ms，错误 %2，警告 %3")
                  .arg(elapsedMs).arg(result.errors.size()).arg(result.warnings.size())));

        for (int i = 0; i < result.errors.size(); ++i) {
            const QString& errLine = result.errors[i];
            SprintIDE::ErrorMarker marker = parseErrorLine(errLine);

            if (marker.line > 0 && !marker.filePath.isEmpty()) {
                QString absPath = marker.filePath;
                if (!QFileInfo(absPath).isAbsolute()) {
                    const QString projRoot = d->currentProjectPath;
                    if (!projRoot.isEmpty()) {
                        const QString cand1 = QDir(projRoot).absoluteFilePath(marker.filePath);
                        const QString cand2 = QDir(projRoot + "/build").absoluteFilePath(marker.filePath);
                        if (QFileInfo::exists(cand1))      absPath = cand1;
                        else if (QFileInfo::exists(cand2)) absPath = cand2;
                    }
                }

                appendLog(LogEntry::make(LogKind::Compile, LogLevel::Error, "Compile",
                                         errLine,
                                         absPath,
                                         marker.line,
                                         marker.column > 0 ? marker.column : 1));
            } else {
                appendLog(LogEntry::make(LogKind::Compile, LogLevel::Error, "Compile",
                                         errLine));
            }
        }
        logError(QString(tr("[%1] 编译失败! 耗时: %2ms"))
                     .arg(timeStr)
                     .arg(elapsedMs));

        m_statusLabel->setText(tr("编译失败"));

        int errorCount = result.errors.size();
        int warningCount = result.warnings.size();

        if (errorCount > 0 || warningCount > 0) {
            QString statsMessage = QString(tr("错误: %1, 警告: %2"))
                                       .arg(errorCount)
                                       .arg(warningCount);
            logError(statsMessage);
        }

        if (!result.errors.isEmpty()) {
            logError("错误列表:");
            for (int i = 0; i < qMin(5, result.errors.size()); i++) {
                const QString& errLine = result.errors[i];
                SprintIDE::ErrorMarker marker = parseErrorLine(errLine);
                const QString prefix = QString(" %1. ").arg(i + 1);

                if (marker.line > 0 && !marker.filePath.isEmpty()) {
                    // 相对路径 → 尝试相对项目根 / <项目根>/build 解析成绝对路径
                    QString absPath = marker.filePath;
                    if (!QFileInfo(absPath).isAbsolute()) {
                        const QString projRoot = d->currentProjectPath;
                        if (!projRoot.isEmpty()) {
                            const QString cand1 = QDir(projRoot).absoluteFilePath(marker.filePath);
                            const QString cand2 = QDir(projRoot + "/build").absoluteFilePath(marker.filePath);
                            if (QFileInfo::exists(cand1))      absPath = cand1;
                            else if (QFileInfo::exists(cand2)) absPath = cand2;
                        }
                    }

                    appendLog(LogEntry::make(
                        LogKind::Compile, LogLevel::Error, "Compile",
                        prefix + errLine.toHtmlEscaped(),
                        absPath,
                        marker.line,
                        marker.column > 0 ? marker.column : 1));
                } else {
                    logError(prefix + errLine.toHtmlEscaped());
                }
            }
            if (result.errors.size() > 5) {
                logError(QString("... 还有 %1 个错误未显示")
                                           .arg(result.errors.size() - 5));
            }
        }

        // 失败时清除自动运行/调试标志
        if (m_compileForDebug) {
            m_compileForDebug = false;
            logInfo("编译失败，无法启动调试。请修复错误后重试。");
        }
        if (m_autoRunAfterCompile) {
            m_autoRunAfterCompile = false;
            logInfo("编译失败，无法自动运行。");
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
        logInfo("=== 编译输出（完整） ===");
        logInfo(result.output);
        logInfo("=== 编译输出结束 ===");
    }

    // ========== 清理工作 ==========
    m_currentCompileTaskId.clear();
    m_buildProgressBar->setVisible(false);
    m_buildProgressBar->setValue(0);

    // ---- AI 触发的构建：发信号给 IdeExecutor ----
    if (m_aiBuildInProgress) {
        m_aiBuildInProgress = false;

        // 日志截断到 64KB（避免超大输出爆内存/协议消息）
        QString logText = result.output;
        constexpr int kMaxLog = 64 * 1024;
        if (logText.size() > kMaxLog) {
            logText = logText.left(kMaxLog)
                      + QStringLiteral("\n...[truncated]");
        }

        emit aiBuildFinished(result.success,
                             result.exitCode,
                             logText,
                             static_cast<int>(result.compileTime));
    }
}

void MainWindow::onCompileError(const QString& taskId, const QString& errorMessage)
{
    if (taskId != m_currentCompileTaskId) {
        return;
    }

    m_isCompiling = false;

    appendLog(LogEntry::make(LogKind::Compile, LogLevel::Error, "Compile", errorMessage));

    m_statusLabel->setText(tr("编译错误"));

    m_currentCompileTaskId.clear();
}

void MainWindow::onCompilersDetected(const QStringList& compilers)
{
    if (compilers.isEmpty()) {
        return;
    }

    QString timeStr = QTime::currentTime().toString("hh:mm:ss");
    logInfo(QString("[%1] 检测到的编译器:").arg(timeStr));

    for (const QString& compiler : compilers) {
        QString info = m_compileManager->getCompilerInfo(compiler);
        logInfo(QString("  - %1: %2")
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
        QMessageBox::warning(this, tr("运行错误"),
                             QString(tr("可执行文件不存在:\n%1")).arg(executablePath));
        return;
    }

    QString timeStr = QTime::currentTime().toString("hh:mm:ss");
    logInfo(QString("[%1] 正在运行程序...").arg(timeStr));

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
        arguments << "-e" << executablePath + tr("; echo '程序执行完毕，按任意键退出...'; read -n1");
#endif

        QProcess::startDetached(program, arguments);

    } else {
        QProcess* process = new QProcess(this);
        process->setProgram(executablePath);
        process->setWorkingDirectory(QFileInfo(executablePath).path());

        connect(process, &QProcess::readyReadStandardOutput, [=]() {
            QString output = QString::fromLocal8Bit(process->readAllStandardOutput());
            QString timeStr = QTime::currentTime().toString("hh:mm:ss");
            logInfo(QString("[%1] 程序输出: %2")
                                       .arg(timeStr)
                                       .arg(output.toHtmlEscaped()));
        });

        connect(process, &QProcess::readyReadStandardError, [=]() {
            QString error = QString::fromLocal8Bit(process->readAllStandardError());
            QString timeStr = QTime::currentTime().toString("hh:mm:ss");
            logError(QString("[%1] 程序错误: %2")
                                       .arg(timeStr)
                                       .arg(error.toHtmlEscaped()));
        });

        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                [=](int exitCode, QProcess::ExitStatus exitStatus) {
                    QString timeStr = QTime::currentTime().toString("hh:mm:ss");
                    QString message;

                    if (exitStatus == QProcess::NormalExit) {
                        message = QString(tr("[%1] 程序执行完毕，退出码: %2"))
                                      .arg(timeStr)
                                      .arg(exitCode);
                    } else {
                        message = QString(tr("[%1] 程序异常退出")).arg(timeStr);
                    }

                    logInfo(message);
                    m_statusLabel->setText(tr("程序执行完成"));

                    process->deleteLater();
                });

        process->start();

        if (!process->waitForStarted(3000)) {
            QMessageBox::warning(this, tr("运行错误"), tr("无法启动程序"));
            process->deleteLater();
        } else {
            m_statusLabel->setText(tr("程序运行中..."));
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


void MainWindow::onLSPDiagnostics(const QString& filePath,
                                  const QList<LSPDiagnostic>& diagnostics)
{
    TextEditor* editor = findEditorByFilePath(filePath);
    if (!editor) return;

    // 缓存本次诊断的完整信息（含 source / code），供后续 codeAction 请求原样回传
    m_lastDiagnosticsByFile[filePath] = diagnostics;

    editor->clearAllMarkers();

    DiagnosticFilter& filter = DiagnosticFilter::instance();

    int keptErrors = 0;
    int keptWarnings = 0;
    int filtered = 0;

    for (const LSPDiagnostic& diag : diagnostics) {
        if (!filter.shouldKeep(diag)) {
            ++filtered;
            continue;
        }
        if (diag.severity == "error") {
            editor->setErrorMarker(filePath, diag.line, diag.column, diag.message);
            ++keptErrors;
        } else if (diag.severity == "warning") {
            editor->setWarningMarker(filePath, diag.line, diag.column, diag.message);
            ++keptWarnings;
        }
    }

    if (filtered > 0 || keptWarnings > 0) {
        qDebug() << "[LSP]" << filePath
                 << "error:" << keptErrors
                 << "warning:" << keptWarnings
                 << "filtered:" << filtered;
    }
}

void MainWindow::onDefinitionReceived(const QString& file, int line, int column)
{
    if (file.isEmpty() || line <= 0) {
        qDebug() << "[Def] 无效跳转目标:" << file << line << column;
        return;
    }

    qDebug() << "[Def] 跳转:" << file << ":" << line;

    openFile(file);

    TextEditor* te = currentEditor();
    if (!te) return;

    te->gotoLine(line);
    te->setFocus();
    // te->highlightLine(line);
}

void MainWindow::onQuickFixRequested(const QString& filePath,
                                     int startLine, int startColumn,
                                     int endLine,   int endColumn)
{
    if (filePath.isEmpty() || !m_lspManager) return;

    TextEditor* editor = findEditorByFilePath(filePath);
    if (!editor) return;

    const QString projectRoot = d->currentProjectPath.isEmpty()
                                    ? QFileInfo(filePath).absolutePath()
                                    : d->currentProjectPath;

    LSPClient* client = m_lspManager->getClientForFile(filePath, projectRoot);
    if (!client) return;

    // 从缓存里取该行的原始诊断（保留 source / code）
    // 不再从 ErrorMarker 现拼，避免丢失 clangd 需要的字段
    QList<LSPDiagnostic> diags;
    const QList<LSPDiagnostic> allDiags =
        m_lastDiagnosticsByFile.value(filePath);
    for (const LSPDiagnostic& d : allDiags) {
        // 诊断 range 与请求 range 有重叠 → 纳入
        // 支持跨多行的诊断（Python 缩进错误 / Java 泛型错误等）
        const int dStartLine = d.line - 1;                        // 0-based
        const int dEndLine   = (d.endLine > 0 ? d.endLine : d.line) - 1;

        // 无重叠 → 跳过
        if (dEndLine < startLine || dStartLine > endLine) continue;

        // 起始行在同一行时，检查列范围（粗略）
        diags.append(d);
    }

    // ---- codeActionReceived 一次性 connect（保留原有逻辑） ----
    connect(client, &LSPClient::codeActionReceived,
            editor, &TextEditor::onCodeActionReceived,
            Qt::UniqueConnection);

    // ---- 客户端未就绪 → 等 serverReady ----
    if (!client->isReady()) {
        QPointer<LSPClient> clientPtr = client;
        auto connPtr = std::make_shared<QMetaObject::Connection>();
        *connPtr = connect(client, &LSPClient::serverReady, this,
                           [clientPtr, filePath, startLine, startColumn,
                            endLine, endColumn, diags, connPtr]() {
                               if (clientPtr && clientPtr->isReady()) {
                                   clientPtr->requestCodeAction(
                                       filePath,
                                       startLine, startColumn,
                                       endLine, endColumn,
                                       diags);
                               }
                               QObject::disconnect(*connPtr);
                           });
        return;
    }

    client->requestCodeAction(filePath,
                              startLine, startColumn,
                              endLine, endColumn,
                              diags);
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
        QMessageBox::information(this, tr("提示"), tr("请先打开一个文件再开始调试。"));
        return;
    }

    QString sourceFile = editor->filePath();
    if (sourceFile.isEmpty()) {
        QMessageBox::warning(this, tr("错误"), tr("请先保存文件。"));
        return;
    }

    if (editor->isModified() && !editor->saveFile()) {
        QMessageBox::warning(this, tr("保存错误"), tr("无法保存文件，调试中止。"));
        return;
    }

    if (m_isCompiling) {
        QMessageBox::information(this, tr("提示"), tr("当前正在编译，请等待编译完成后再调试。"));
        return;
    }

    if (m_isDebugging) {
        QMessageBox::information(this, tr("提示"), tr("当前正在调试中，请先停止调试。"));
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

        logInfo("调试目标文件: " + sourceFile);

        // --- 关键修改：复用调试器实例 ---
        if (!m_pythonDebugger) {
            m_pythonDebugger = new PythonDebugger(this);

            // 连接信号
            connect(m_pythonDebugger, &PythonDebugger::stopped, this,
                    [this](const QString& file, int line, const QString& reason) {

                        logInfo(QString("调试暂停: %1:%2 (%3)")
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
            //     logInfo("继续执行");
            //     TextEditor* ed = currentEditor();
            //     if (ed) { ed->clearCurrentDebugLine(); }
            // });

            connect(m_pythonDebugger, &PythonDebugger::exited, this, [this](int code) {
                logInfo(QString("程序退出，退出码: %1").arg(code));
                onPythonDebugFinished();
            });
            connect(m_pythonDebugger, &PythonDebugger::terminated,
                    this, &MainWindow::onPythonDebugFinished);


            connect(m_pythonDebugger, &PythonDebugger::continued, this, &MainWindow::onDebuggerContinued);

            connect(m_pythonDebugger, &PythonDebugger::outputReceived, this, [this](const QString& out, bool err) {
                // 输出面板
                if (err) {
                    logError(out.toHtmlEscaped());
                } else {
                    logInfo(out);
                }
                // 调试面板控制台
                if (m_debugPanel) {
                    m_debugPanel->appendConsoleOutput(out, err);
                }
            });

            connect(m_pythonDebugger, &PythonDebugger::errorOccurred, this, [this](const QString& error) {
                logError("调试错误: " + error);
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
            logInfo("Python 调试已启动");
            // 清空调试面板控制台并打分隔线
                        if (m_debugPanel) {
                            m_debugPanel->clearConsole();
                            m_debugPanel->appendConsoleOutput(
                                QString(tr("=== 调试会话开始: %1 ==="))
                                    .arg(QFileInfo(sourceFile).fileName()),
                                false);
                        }
            m_isDebugging = true;

            QList<Breakpoint> allBreakpoints = m_debugManager->getBreakpoints();
            m_pythonDebugger->setBreakpoints(allBreakpoints);
            logInfo(QString("已同步 %1 个断点").arg(allBreakpoints.size()));

            // 切换到调试面板
            switchToDebugTab();
            // 更新调试动作状态
            updateDebugActions();

            // --- 关键修改：不再手动发送断点 ---
            // 断点将在 debugpy 发送 "initialized" 事件后，
            // 由 PythonDebugger 内部的 applyBreakpoints() 自动同步
            logInfo("断点将在调试器初始化后自动同步");

            // 初始化调试面板
            if (m_debugPanel) {
                m_debugPanel->setDebuggingState(true);
                m_debugPanel->updateVariables(QList<DebugVariable>());
                m_debugPanel->updateCallStack(QList<StackFrame>());
            }
        } else {
            logError("Python 调试启动失败");
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
            int reply = QMessageBox::question(this, tr("编译并调试"),
                                              tr("文件尚未编译或已过期，是否现在编译并调试?"),
                                              QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                m_compileForDebug = true;
                onBuildProject();      //    用 onBuildProject，不是 onCompile
            }
            return;
        }

        // 2. 解析 DAP adapter
        //    平台策略：
        //     - Linux 上优先 GDB DAP（GDB 14+ 支持 --interpreter=dap）
        //     - macOS / Windows 上仍用 lldb-dap / codelldb
        //    若首选不可用，回退到另一种。
        ToolchainId preferred = ToolchainId::LldbDap;
#ifdef Q_OS_LINUX
        if (ToolchainManager::instance().resolve(ToolchainId::GdbDap).found) {
            preferred = ToolchainId::GdbDap;
            qDebug() << "[onStartDebug] Linux 平台，优先使用 GDB DAP";
        }
#endif

        auto adapterRes = ToolchainManager::instance().resolve(preferred);

        // 首选失败 → 尝试另一种 adapter（跨平台兜底）
        if (!adapterRes.found) {
            ToolchainId fallback =
                (preferred == ToolchainId::LldbDap)
                    ? ToolchainId::GdbDap
                    : ToolchainId::LldbDap;

            qDebug() << "[onStartDebug] 首选 adapter 不可用，尝试回退到"
                     << toolchainIdName(fallback);

            auto fallbackRes = ToolchainManager::instance().resolve(fallback);
            if (fallbackRes.found) {
                adapterRes = fallbackRes;
                preferred  = fallback;
            }
        }

        // 两者都没有 → 弹安装引导（对首选弹，用户可换）
        if (!adapterRes.found) {
            ToolchainManager::instance().resolveOrPrompt(preferred, this, &adapterRes);
            if (!adapterRes.found) {
                logError(
                    QString("未找到 %1 调试适配器，调试中止。")
                        .arg(toolchainIdName(preferred)));
                return;
            }
        }

        logInfo(QString("使用调试适配器: %1").arg(adapterRes.executablePath));

        // 3. 构建配置
        CppDebugConfig cfg;
#ifdef Q_OS_MAC
    // Qt.app 程序需要 DYLD_FRAMEWORK_PATH 才能加载 Qt Framework
    QString qtDir = task.sdkConfig.value("QT_DIR").toString();
    if (qtDir.isEmpty()) {
        ConfigManager* cfgMgr = Application::instance().configManager();
        if (cfgMgr) {
            qtDir = cfgMgr->value("Sdk/Environment").toMap()
                        .value("QT_DIR").toString();
        }
    }
    if (!qtDir.isEmpty()) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        #ifdef Q_OS_MAC
                const QString libDir = qtDir + "/lib";
                env.insert("DYLD_FRAMEWORK_PATH", libDir);
                env.insert("DYLD_LIBRARY_PATH", libDir);
        #elif defined(Q_OS_LINUX)
                const QString libDir = qtDir + "/lib";
                env.insert("LD_LIBRARY_PATH",
                           libDir + ":" + env.value("LD_LIBRARY_PATH"));
        #elif defined(Q_OS_WIN)
                env.insert("PATH",
                           qtDir + "/bin;" + env.value("PATH"));
        #endif
        cfg.environment = env;
    }
#endif
        cfg.adapterPath = adapterRes.executablePath;
        cfg.program = exePath;
        cfg.workingDirectory = QFileInfo(exePath).absolutePath();
        cfg.stopOnEntry = false;

        // 4. 首次创建 CppDebugger
        if (!m_cppDebugger) {
            m_cppDebugger = new CppDebugger(this);

            connect(m_cppDebugger, &CppDebugger::stopped, this,
                    [this](const QString& file, int line, const QString& reason) {
                        logInfo(
                            QString("调试暂停: %1:%2 (%3)").arg(file).arg(line).arg(reason));

                        // 空路径防护：DAP 有时不返回 source.path
                        if (file.isEmpty()) {
                            logError(
                                "警告：调试器未返回源文件路径，无法跳转高亮");
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
                        logInfo(QString("程序退出，退出码: %1").arg(code));
                        onPythonDebugFinished();
                    });

            connect(m_cppDebugger, &CppDebugger::terminated,
                    this, &MainWindow::onPythonDebugFinished);

            connect(m_cppDebugger, &CppDebugger::outputReceived, this,
                    [this](const QString& out, bool err) {
                        // 输出面板
                        if (err) {
                            logError(out.toHtmlEscaped());
                        } else {
                            logInfo(out);
                        }
                        // 调试面板控制台
                        if (m_debugPanel) {
                            m_debugPanel->appendConsoleOutput(out, err);
                        }
                    });

            connect(m_cppDebugger, &CppDebugger::errorOccurred, this,
                    [this](const QString& e) {
                        logError("调试错误: " + e);
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

            // 内联变量显示：变量更新时把关键值贴到当前停止行行尾
            connect(m_cppDebugger, &CppDebugger::variablesUpdated,
                    this, [this](const QList<DebugVariable>& vars) {
                        TextEditor* ed = currentEditor();
                        if (!ed) return;

                        // 只取前 5 个简单变量，跳过复杂类型（结构体/容器）避免刷屏
                        QStringList parts;
                        for (const DebugVariable& v : vars) {
                            if (parts.size() >= 5) break;

                            // 跳过空名或空值
                            if (v.name.isEmpty() || v.value.isEmpty()) continue;

                            // 跳过可展开的复杂对象（它们在变量面板里更适合查看）
                            // 但如果用户想要显示指针/引用的地址，可以放开这条限制
                            // if (v.variablesReference > 0) continue;

                            QString val = v.value;
                            // 长值截断（例如 std::string 全文、vector 的完整内容）
                            if (val.length() > 40) {
                                val = val.left(37) + QStringLiteral("...");
                            }

                            parts << QStringLiteral("%1 = %2").arg(v.name, val);
                        }

                        if (parts.isEmpty()) {
                            ed->clearInlineValues();
                        } else {
                            ed->setInlineValues(
                                ed->currentLine(),
                                QStringLiteral("  ") + parts.join(QStringLiteral("   ")));
                        }
                    });
            // ========== 变量懒加载 + 表达式求值 ==========
            connect(m_debugPanel, &DebugPanel::variablesExpandRequested,
                    m_cppDebugger, &CppDebugger::requestChildVariables);

            connect(m_debugPanel, &DebugPanel::evaluateRequested,
                    m_cppDebugger, &CppDebugger::evaluate);

            connect(m_cppDebugger, &CppDebugger::childVariablesUpdated,
                    m_debugPanel, &DebugPanel::onChildVariablesUpdated);

            connect(m_cppDebugger, &CppDebugger::evaluateResultReady,
                    m_debugPanel, &DebugPanel::onEvaluateResult);
        }

        // 5. 启动
        if (m_cppDebugger->startDebugging(cfg)) {
            logInfo("C++ 调试已启动");
            // 清空调试面板控制台并打分隔线
            if (m_debugPanel) {
                m_debugPanel->clearConsole();
                m_debugPanel->appendConsoleOutput(
                    QString(tr("=== C++ 调试会话开始: %1 ==="))
                        .arg(QFileInfo(sourceFile).fileName()),
                    false);
            }
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
            logInfo(QString("已同步 %1 个断点").arg(allBps.size()));
        } else {
            logError("C++ 调试启动失败");
            m_isDebugging = false;
            updateDebugActions();
            setDebugTabVisible(false);
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
        int reply = QMessageBox::question(this, tr("编译并调试"), tr("文件尚未编译或已过期，是否现在编译并调试?"), QMessageBox::Yes | QMessageBox::No);
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
        logInfo("调试已启动，等待连接...");
        m_isDebugging = true;
        updateDebugActions();
    } else {
        logError("启动调试失败: " + output);
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
            setDebugTabVisible(false);
            // switchToOutputTab();
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
    // switchToOutputTab();
    setDebugTabVisible(false);
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

void MainWindow::clearOutputPanel()
{
    m_outputPanel->clear();
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
        logInfo("请先保存文件后再添加断点");
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
            m_statusLabel->setText(tr("调试已停止"));
        }
        if (m_bottomTabWidget) {
            int debugIndex = m_bottomTabWidget->indexOf(m_debugPanel);
            if (debugIndex >= 0) {
                m_bottomTabWidget->setTabVisible(debugIndex, false);
            }
        }
        // switchToOutputTab();
        setDebugTabVisible(false);
        break;
    case DebugState::Starting:
        m_isDebugging = true;
        if (m_statusLabel) {
            m_statusLabel->setText(tr("正在启动调试..."));
        }
        setDebugTabVisible(true);
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
            m_statusLabel->setText(tr("调试运行中"));
        }
        break;
    case DebugState::Paused:
        m_isDebugging = true;
        if (m_statusLabel) {
            m_statusLabel->setText(tr("调试暂停（断点命中）"));
        }
        switchToDebugTab();
        break;
    case DebugState::Stepping:
        if (m_statusLabel) {
            m_statusLabel->setText(tr("单步执行中..."));
        }
        break;
    case DebugState::Finished:
        m_isDebugging = false;
        if (m_statusLabel) {
            m_statusLabel->setText(tr("程序执行完成"));
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
            m_statusLabel->setText(tr("调试错误"));
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

    // logInfo(QString("断点命中: %1:%2")
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

    // logInfo(QString("程序停止: %1 (%2:%3)")
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
    logInfo(QString("程序退出，退出码: %1").arg(exitCode));
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
    logError(QString("调试错误: %1").arg(error));
    m_isDebugging = false;
    updateDebugActions();
}

// ==================== 调试控制方法 ====================

bool MainWindow::startDebugging()
{
    // qDebug() << "=== startDebugging called ===";

    TextEditor* editor = currentEditor();
    if (!editor) {
        logInfo("请先打开一个文件再开始调试");
        return false;
    }

    QString sourceFile = editor->filePath();
    if (sourceFile.isEmpty()) {
        logInfo("请先保存文件再开始调试");
        return false;
    }
    // qDebug() << "Source file:" << sourceFile;

    if (editor->isModified()) {
        logInfo("文件已修改，正在编译...");
        onCompile();
    }

    QString executablePath = findExecutableForSource(sourceFile);
    // qDebug() << "Executable path:" << executablePath;

    if (executablePath.isEmpty()) {
        logInfo("未找到可执行文件，请等待编译");
        onCompile();
        // return false;
    }

    QFileInfo exeInfo(executablePath);
    if (!exeInfo.exists()) {
        logError("可执行文件不存在: " + executablePath);
        return false;
    }
    if (!exeInfo.isExecutable()) {
        logError("文件不可执行: " + executablePath);
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
        logInfo("调试已启动");
        updateDebugActions();
    } else {
        logError("启动调试失败");
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
        logInfo("=== 调用栈 ===");
        for (const StackFrame& frame : m_callStack) {
            logInfo(QString("  #%1 %2 at %3:%4")
                                       .arg(frame.level)
                                       .arg(frame.functionName)
                                       .arg(frame.filePath)
                                       .arg(frame.lineNumber));
        }
    }

    if (!m_localVariables.isEmpty()) {
        logInfo("=== 局部变量 ===");
        for (const DebugVariable& var : m_localVariables) {
            logInfo(QString("  %1 (%2) = %3")
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
    setDebugTabVisible(true);
}

void MainWindow::setDebugTabVisible(bool visible)
{
    if (!m_bottomTabWidget || !m_debugPanel) return;

    const int idx = m_bottomTabWidget->indexOf(m_debugPanel);
    if (idx < 0) return;

    if (visible) {
        m_bottomTabWidget->setTabVisible(idx, true);
        m_bottomTabWidget->setCurrentIndex(idx);
    } else {
        // 如果当前 Tab 就是调试面板，先切到输出面板，再隐藏
        // 否则 Qt 在隐藏当前 Tab 时会自己找另一个 Tab 顶上，可能不是我们想要的
        if (m_bottomTabWidget->currentIndex() == idx) {
            switchToOutputTab();
        }
        m_bottomTabWidget->setTabVisible(idx, false);
    }
}

void MainWindow::switchToOutputTab()
{
    if (m_bottomTabWidget && m_outputPanel) {
        int index = m_bottomTabWidget->indexOf(m_outputPanel);
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
        QString displayText = QString("v%1  %2  %3  %4  %5")
                                  .arg(v.version)
                                  .arg(v.displayTime())
                                  .arg(v.displaySize())
                                  .arg(v.displaySource())
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
    // 2.3    保存断点
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

    // ---------- 1. 恢复打开的文件（分批异步，避免一次性卡住 UI） ----------
    const QJsonArray openFiles = session["openFiles"].toArray();

    struct RestoreEntry { QString path; int line; };
    QList<RestoreEntry> entries;
    entries.reserve(openFiles.size());
    for (const QJsonValue &val : openFiles) {
        const QJsonObject fileObj = val.toObject();
        const QString path = fileObj["path"].toString();
        if (path.isEmpty() || !QFile::exists(path)) continue;
        entries.append({ path, fileObj["line"].toInt(1) });
    }

    // 使用 shared_ptr 让 lambda 自递归（捕获自己）
    auto queue = std::make_shared<std::function<void(int)>>();
    *queue = [this, entries, queue](int i) {
        if (i >= entries.size()) return;
        openFile(entries[i].path);
        if (TextEditor* ed = currentEditor()) {
            ed->gotoLine(entries[i].line);
        }
        // 0ms 让事件循环跑一轮重绘 / 布局
        QTimer::singleShot(0, this, [queue, i]() { (*queue)(i + 1); });
    };
    QTimer::singleShot(0, this, [queue]() { (*queue)(0); });

    // 2.    恢复断点
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
                bool hasCond = !condition.trimmed().isEmpty();   // condition 是 JSON 里读出来的
                ed->setBreakpointWithCondition(line, enabled, hasCond);   // 禁用断点通常渲染成空心红点
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
        logInfo("可执行文件路径为空");
        return false;
    }

    if (!QFile::exists(executablePath)) {
        logInfo(QString("可执行文件不存在: %1").arg(executablePath));
        return false;
    }

    QFileInfo exeInfo(executablePath);
    if (!exeInfo.isExecutable()) {
        logInfo(QString("文件不可执行: %1").arg(executablePath));
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
        logInfo(QString("调试已启动: %1").arg(executablePath));
        updateDebugActions();
    } else {
        logError("启动调试失败");
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

QStringList MainWindow::collectProjectFiles(bool forceRescan)
{
    const QString root = d->currentProjectPath;
    if (root.isEmpty()) return {};

    constexpr int kCacheSecs = 30;
    if (!forceRescan
        && m_cachedProjectRoot == root
        && m_cachedScanTime.isValid()
        && m_cachedScanTime.secsTo(QDateTime::currentDateTime()) < kCacheSecs) {
        return m_cachedProjectFiles;
    }

    m_cachedProjectFiles = scanFilesForQuickOpen(root);
    m_cachedProjectRoot  = root;
    m_cachedScanTime     = QDateTime::currentDateTime();
    return m_cachedProjectFiles;
}

void MainWindow::appendLog(const LogEntry& entry)
{
    // 1. 结构化写入 LogBuffer（AI 用）
    if (auto* buf = Application::instance().logBuffer()) {
        buf->append(entry);
    }

    // 2. 渲染到 outputPanel（人看）
    if (m_outputPanel) {
        m_outputPanel->appendEntry(entry);
    }
}

void MainWindow::runExecutableAsync(const QString& executablePath)
{
    if (executablePath.isEmpty() || !QFile::exists(executablePath)) {
        logError("可执行文件不存在: " + executablePath);
        return;
    }

    QString timeStr = QTime::currentTime().toString("hh:mm:ss");
    logInfo(QString("[%1] 正在运行: %2")
                               .arg(timeStr).arg(executablePath));

    QProcess* process = createRunProcess(
        executablePath,
        QStringList(),
        QFileInfo(executablePath).absolutePath(),
        QProcessEnvironment());

    process->start();
    if (!process->waitForStarted(3000)) {
        logError("启动失败: " + process->errorString());
        process->deleteLater();
        return;
    }
    markProcessRunning(process, executablePath);
}

// ============================================================================
//  运行进程管理
// ============================================================================
QProcess* MainWindow::createRunProcess(const QString& program,
                                       const QStringList& arguments,
                                       const QString& workDir,
                                       const QProcessEnvironment& env)
{
    QProcess* p = new QProcess(this);
    p->setProgram(program);
    p->setArguments(arguments);
    if (!workDir.isEmpty()) p->setWorkingDirectory(workDir);
    if (!env.isEmpty())     p->setProcessEnvironment(env);
    p->setProcessChannelMode(QProcess::MergedChannels);

    auto lineBuf = std::make_shared<QByteArray>();

    // 处理一个 chunk：拆出所有完整行，逐行写入日志
    auto handleChunk = [this, lineBuf](const QByteArray& chunk, bool isError) {
        if (chunk.isEmpty()) return;
        lineBuf->append(chunk);

        int pos;
        while ((pos = lineBuf->indexOf('\n')) >= 0) {
            QByteArray rawLine = lineBuf->left(pos);
            lineBuf->remove(0, pos + 1);

            // Windows 的 CRLF：把尾部 '\r' 去掉
            if (rawLine.endsWith('\r')) rawLine.chop(1);

            const QString line = QString::fromLocal8Bit(rawLine);
            if (line.isEmpty()) continue;   // 过滤空行，避免 UI 上出现"空白 entry"

            if (isError) logError(line);
            else         logInfo(line);
        }
    };

    connect(p, &QProcess::readyReadStandardOutput, this, [this, p, handleChunk]() {
            const QByteArray chunk = p->readAllStandardOutput();
            if (chunk.isEmpty()) return;
            if (m_aiRunInProgress) {
                m_aiRunOutputBuffer += QString::fromLocal8Bit(chunk);
            }
            handleChunk(chunk, /*isError=*/false);
        });

        connect(p, &QProcess::readyReadStandardError, this, [this, p, handleChunk]() {
            const QByteArray chunk = p->readAllStandardError();
            if (chunk.isEmpty()) return;
            if (m_aiRunInProgress) {
                m_aiRunOutputBuffer += QString::fromLocal8Bit(chunk);
            }
            handleChunk(chunk, /*isError=*/true);
        });

        connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, p, lineBuf, handleChunk](int code, QProcess::ExitStatus status) {
            if (m_runningProcess == p) {
                // ---- 补读 socket 缓冲里可能残留的内容 ----
                const QByteArray remainingOut = p->readAllStandardOutput();
                const QByteArray remainingErr = p->readAllStandardError();
                if (m_aiRunInProgress) {
                    m_aiRunOutputBuffer += QString::fromLocal8Bit(remainingOut);
                    m_aiRunOutputBuffer += QString::fromLocal8Bit(remainingErr);
                }
                handleChunk(remainingOut, /*isError=*/false);
                handleChunk(remainingErr, /*isError=*/true);

                // ---- v8.4 关键：flush 行缓冲里的最后半行 ----
                // 场景：进程最后一条输出没有 '\n'（比如 C 的 printf 不带 \n 就退出）
                if (!lineBuf->isEmpty()) {
                    QByteArray tail = *lineBuf;
                    lineBuf->clear();
                    if (tail.endsWith('\r')) tail.chop(1);
                    const QString line = QString::fromLocal8Bit(tail);
                    if (!line.isEmpty()) logInfo(line);
                }

                // ---- 以下为原有 finished 逻辑，保持不变 ----
                if (m_stoppingProcess) {
                    logInfo("程序已停止");
                } else if (status == QProcess::NormalExit) {
                    logInfo(QString("[%1] 进程结束，退出码: %2")
                                .arg(QTime::currentTime().toString("hh:mm:ss"))
                                .arg(code));
                } else {
                    logError(QString("[%1] 进程异常崩溃（退出码: %2）")
                                 .arg(QTime::currentTime().toString("hh:mm:ss"))
                                 .arg(code));
                }

                if (m_aiRunStopInProgress) {
                    m_aiRunStopInProgress = false;
                    m_aiRunInProgress = false;
                    QString logText = m_aiRunOutputBuffer;
                    constexpr int kMaxLog = 64 * 1024;
                    if (logText.size() > kMaxLog) {
                        logText = logText.left(kMaxLog) + QStringLiteral("\n...[truncated]");
                    }
                    m_aiRunOutputBuffer.clear();
                    emit aiRunStopped(code, logText);
                } else if (m_aiRunInProgress) {
                    m_aiRunInProgress = false;
                    QString logText = m_aiRunOutputBuffer;
                    constexpr int kMaxLog = 64 * 1024;
                    if (logText.size() > kMaxLog) {
                        logText = logText.left(kMaxLog) + QStringLiteral("\n...[truncated]");
                    }
                    m_aiRunOutputBuffer.clear();
                    emit aiRunFinished(code, logText);
                }

                markProcessFinished();
            }
            p->deleteLater();
        });
    connect(p, &QProcess::errorOccurred, this, [this, p](QProcess::ProcessError err) {
        if (m_runningProcess != p) return;

        // 主动停止时，QProcess 会发 Crashed 错误，忽略
        if (m_stoppingProcess) return;

        // FailedToStart 是真正的错误（程序根本起不来）
        if (err == QProcess::FailedToStart) {
            logError("进程启动失败: " + p->errorString());
            return;
        }

        // 其他错误：Crashed / Timedout / ReadError / WriteError
        // 如果是 Timedout，不算崩溃，只是超时
        if (err == QProcess::Timedout) {
            logWarn("进程超时: " + p->errorString());
            return;
        }

        logError(QString("进程错误: %1 (%2)")
                     .arg(p->errorString())
                     .arg(static_cast<int>(err)));
    });

    return p;
}

void MainWindow::markProcessRunning(QProcess* proc, const QString& sourcePath)
{
    m_runningProcess   = proc;
    m_runningFilePath  = sourcePath;

    if (m_runAction) {
        m_runAction->setIcon(QIcon(":/icons/icons/stop.svg"));
        m_runAction->setText(tr("停止"));
        m_runAction->setToolTip(tr("停止运行 (Ctrl+R)"));
        // 断开旧连接 → 连到停止
        disconnect(m_runAction, nullptr, this, nullptr);
        connect(m_runAction, &QAction::triggered, this, &MainWindow::stopRunningProcess);
    }
    if (m_statusLabel) {
        m_statusLabel->setText(tr("程序运行中..."));
    }
}

void MainWindow::markProcessFinished()
{
    m_stoppingProcess = false;
    m_runningProcess.clear();
    m_runningFilePath.clear();

    if (m_runAction) {
        m_runAction->setIcon(QIcon(":/icons/icons/run.svg"));
        m_runAction->setText(tr("运行"));
        m_runAction->setToolTip(tr("运行 (Ctrl+R)"));
        disconnect(m_runAction, nullptr, this, nullptr);
        connect(m_runAction, &QAction::triggered, this, &MainWindow::onRun);
    }
    if (m_statusLabel) {
        m_statusLabel->setText(tr("运行完成"));
    }
}

void MainWindow::stopRunningProcess()
{
    if (m_runningProcess.isNull()) return;

    logInfo("正在停止程序...");
    QProcess* p = m_runningProcess.data();

    // 防止 finished 信号重复触发 markProcessFinished（幂等，但保险）
    if (p->state() == QProcess::NotRunning) {
        markProcessFinished();
        return;
    }

    logInfo("正在停止程序...");
    m_stoppingProcess = true;

    p->terminate();
    if (!p->waitForFinished(3000)) {
        if (m_runningProcess.isNull()) {
            return;   // 已经被 finished 处理清空 → 结束
        }

        logWarn("程序未响应，强制结束");
        p->kill();
        p->waitForFinished(1000);
    }
    // finished 信号会触发 markProcessFinished()，这里不用手动调
}

// ============================================================================
//  AI 集成入口
// ============================================================================
void MainWindow::aiRequestBuild()
{
    m_aiBuildInProgress = true;
    onBuildProject();
}

void MainWindow::aiRequestRun()
{
    m_aiRunInProgress = true;
    m_aiRunOutputBuffer.clear();    // 每次新 run 从空开始
    onRun();
}

void MainWindow::aiRequestStopRun()
{
    if (!isRunningProgram()) {
        // 没在跑：延后一个事件循环再发信号。
        // 原因：handleStopRun 调用本函数时还没返回，OperationBus 还没设置
        // m_pending.itemId；如果同步 emit，onDeferredResultReady 会拒绝。
        QTimer::singleShot(0, this, [this]() {
            emit aiRunStopped(-1, QString());
        });
        return;
    }

    m_aiRunStopInProgress = true;
    stopRunningProcess();
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
        "所有文件 (*.*)"
        );

    if (!fileName.isEmpty()) {
        openFile(fileName);
    }
}

// ==================== 快速打开（Ctrl+O） ====================

void MainWindow::onQuickOpen()
{
    // -------- 1. 确定项目根目录 --------
    // 没有显式打开项目时，尝试从当前编辑器向上推导
    if (d->currentProjectPath.isEmpty()) {
        TextEditor* editor = currentEditor();
        if (editor && !editor->filePath().isEmpty()) {
            QDir dir = QFileInfo(editor->filePath()).absoluteDir();
            while (!dir.isRoot()) {
                if (dir.exists(".git")           ||
                    dir.exists("CMakeLists.txt") ||
                    dir.exists("pom.xml")        ||
                    dir.exists("package.json")   ||
                    dir.exists("pyproject.toml") ||
                    dir.exists("Cargo.toml")     ||
                    dir.exists("go.mod")) {
                    d->currentProjectPath = dir.absolutePath();
                    break;
                }
                dir.cdUp();
            }
        }
    }

    // -------- 2. 完全没有项目上下文 → 退化为系统文件对话框 --------
    if (d->currentProjectPath.isEmpty()) {
        qDebug() << "[QuickOpen] 无项目上下文，回退到系统文件对话框";
        onOpenFile();
        return;
    }

    // -------- 3. 扫描项目文件 --------
    const QStringList files = collectProjectFiles();
    if (files.isEmpty()) {
        QMessageBox::information(this, "快速打开",
                                 "项目中没有可打开的文件。");
        return;
    }

    // -------- 4. 弹出搜索对话框 --------
    QuickOpenDialog dlg(this);
    dlg.setFileList(files, d->currentProjectPath);

    connect(&dlg, &QuickOpenDialog::fileChosen,
            this, [this](const QString& path) {
                if (path.isEmpty()) return;
                if (!QFile::exists(path)) {
                    logError(
                        QString("文件不存在: %1")
                            .arg(path));
                    return;
                }
                openFile(path);
            });

    dlg.exec();
}

void MainWindow::onSaveFile()
{
    TextEditor* editor = currentEditor();
    if (editor) {
        if (editor->saveFile()) {
            // logInfo(QString("已保存: %1").arg(editor->filePath()));
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
                logInfo(QString("已另存为: %1").arg(fileName));
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

void MainWindow::onSearchInFiles()
{
    // ---------- 1. 决定搜索根目录 ----------
    QString root = d->currentProjectPath;

    if (root.isEmpty()) {
        if (TextEditor* ed = currentEditor()) {
            if (!ed->filePath().isEmpty()) {
                QDir dir = QFileInfo(ed->filePath()).absoluteDir();
                while (!dir.isRoot()) {
                    if (dir.exists(".git")            ||
                        dir.exists("CMakeLists.txt")  ||
                        dir.exists("pom.xml")         ||
                        dir.exists("package.json")    ||
                        dir.exists("pyproject.toml")  ||
                        dir.exists("Cargo.toml")      ||
                        dir.exists("go.mod")) {
                        root = dir.absolutePath();
                        break;
                    }
                    dir.cdUp();
                }
                if (root.isEmpty()) {
                    root = QFileInfo(ed->filePath()).absolutePath();
                }
            }
        }
    }

    if (root.isEmpty()) {
        QMessageBox::information(this, "在文件中查找",
                                 "请先打开一个项目或文件。");
        return;
    }

    // ---------- 2. 懒创建对话框 ----------
    if (!m_searchInFilesDialog) {
        m_searchInFilesDialog = new SearchInFilesDialog(root, this);
        m_searchInFilesDialog->setAttribute(Qt::WA_DeleteOnClose, false);

        connect(m_searchInFilesDialog, &SearchInFilesDialog::openFileAt,
                this, [this](const QString& file, int line, int col) {
                    Q_UNUSED(col);
                    openFile(file);
                    if (TextEditor* ed = findEditorByFilePath(file)) {
                        d->editorTabs->setCurrentWidget(ed);
                        ed->gotoLine(line);
                        ed->setFocus();
                    }
                });
    } else {
        m_searchInFilesDialog->setRootPath(root);
    }

    // ---------- 3. 复用项目扫描缓存，注入文件列表 ----------
    const QStringList files = collectProjectFiles();   // 上次回答里 MainWindow 新增的那个
    m_searchInFilesDialog->setFileList(files);

    // ---------- 4. 若当前有单行选中文本，预填 ----------
    if (TextEditor* ed = currentEditor()) {
        const QTextCursor cur = ed->textCursor();
        if (cur.hasSelection()) {
            const QString sel = cur.selectedText();
            // 只处理单行选区（多行含段落分隔符不填）
            if (!sel.contains(QChar::ParagraphSeparator) && !sel.contains('\n')) {
                m_searchInFilesDialog->setSearchText(sel);
            }
        }
    }

    m_searchInFilesDialog->show();
    m_searchInFilesDialog->raise();
    m_searchInFilesDialog->activateWindow();
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
    logInfo(QString("替换完成，共替换 %1 处").arg(count));
}

void MainWindow::onDebuggerContinued()
{
    // 这个槽函数在调试器继续运行时被调用
    // 主要任务是更新 UI 状态
    logInfo("调试器已继续运行");
    // 清除上一处停止的行高亮，避免两个绿色行
    if (TextEditor* editor = currentEditor()) {
        editor->clearCurrentDebugLine();
    }

    updateDebugActions(); // 这个函数会根据 m_isDebugging 状态更新按钮
}

// ============================================================================
//  性能监控
// ============================================================================
void MainWindow::onPerformanceSnapshot(const SprintIDE::PerformanceSnapshot& s)
{
    if (!m_performanceLabel) return;

    ThemeManager& tm = ThemeManager::instance();

    // ---- CPU 颜色分级 ----
    QString cpuColor;
    if (s.processCpuPercent >= 80.0) {
        cpuColor = tm.colorString(ThemeManager::UIElement::ErrorColor);
    } else if (s.processCpuPercent >= 40.0) {
        cpuColor = tm.colorString(ThemeManager::UIElement::WarningColor);
    } else {
        cpuColor = tm.colorString(ThemeManager::UIElement::SecondaryTextColor);
    }

    // ---- 内存颜色分级：按系统内存压力着色 ----
    QString memColor;
    if (s.systemMemoryPercent >= 90.0) {
        memColor = tm.colorString(ThemeManager::UIElement::ErrorColor);
    } else if (s.systemMemoryPercent >= 75.0) {
        memColor = tm.colorString(ThemeManager::UIElement::WarningColor);
    } else {
        memColor = tm.colorString(ThemeManager::UIElement::SecondaryTextColor);
    }

    const QString cpuText = QStringLiteral("CPU %1%")
                                .arg(s.processCpuPercent, 0, 'f', 1);
    const QString memText = QLocale::system().formattedDataSize(
        s.processMemoryBytes, 1, QLocale::DataSizeTraditionalFormat);

    m_performanceLabel->setText(QStringLiteral(
        "<span style=\"color:%1;\">%2</span>"
        "  MEM <span style=\"color:%3;\">%4</span>")
        .arg(cpuColor, cpuText.toHtmlEscaped(),
             memColor, memText.toHtmlEscaped()));

    // ---- Tooltip：完整信息 ----
    const QString sysMemUsed = QLocale::system().formattedDataSize(
        s.systemUsedMemoryBytes, 1, QLocale::DataSizeTraditionalFormat);
    const QString sysMemTotal = QLocale::system().formattedDataSize(
        s.systemTotalMemoryBytes, 1, QLocale::DataSizeTraditionalFormat);

    m_performanceLabel->setToolTip(QStringLiteral(
        "IDE CPU：%1%\n"
        "IDE 内存：%2\n"
        "系统 CPU：%3%\n"
        "系统内存：%4 / %5（%6%）")
        .arg(s.processCpuPercent, 0, 'f', 1)
        .arg(memText)
        .arg(s.systemCpuPercent, 0, 'f', 1)
        .arg(sysMemUsed)
        .arg(sysMemTotal)
        .arg(s.systemMemoryPercent, 0, 'f', 1));
}

// ==================== Git 槽函数实现 ====================

void MainWindow::onGitInit()
{
    if (d->currentProjectPath.isEmpty()) {
        QMessageBox::information(this, "提示", "请先打开一个项目。");
        return;
    }
    if (!m_gitPanel) return;

    m_gitPanel->setProjectRoot(d->currentProjectPath);

    // 若已是仓库，直接打开窗口
    if (Git::GitManager::instance().isActive()) {
        logInfo("当前目录已是 Git 仓库");
        showGitDialog();
        return;
    }

    QString err;
    if (!Git::GitManager::instance().initRepository(d->currentProjectPath, &err)) {
        QMessageBox::warning(this, "初始化失败",
                             QString("无法初始化 Git 仓库：\n\n%1").arg(err));
        return;
    }
    logInfo(
        QString("Git 仓库已初始化: %1")
            .arg(d->currentProjectPath));
    updateGitActions();
    showGitDialog();
}

void MainWindow::onGitCommit()
{
    if (!Git::GitManager::instance().isActive()) return;
    showGitDialog();
    // 让面板聚焦到提交消息框
    if (m_gitPanel) {
        // GitPanel 内部可加 focusCommitInput()，这里暂不实现
    }
}

void MainWindow::onGitPush()
{
    auto& gm = Git::GitManager::instance();
    if (!gm.isActive()) return;

    if (!gm.hasRemote()) {
        logError(
            "没有可用的远程仓库，无法推送。");
        return;
    }

    // ============================================================
    // Push 前状态检查
    //   1. 没有配置上游 → 引导用户先 push -u
    //   2. behind > 0 → 落后远程，必须先 pull
    //   3. 分叉（ahead>0 && behind>0）→ 需要 pull 后 merge/rebase
    //   4. ahead == 0 → 没有新提交可推
    //   5. 一切正常 → 放行
    // ============================================================
    const auto st = gm.currentUpstreamStatus();

    // 分支未设置上游
    if (!st.hasUpstream) {
        QMessageBox box(this);
        box.setWindowTitle(QStringLiteral("推送"));
        box.setIcon(QMessageBox::Information);
        box.setTextFormat(Qt::RichText);
        box.setText(QStringLiteral(
            "当前分支 <b>%1</b> 没有设置上游分支。<br><br>"
            "先在终端运行：<br>"
            "<code>git push -u origin %1</code><br><br>"
            "之后就可以用本窗口推送了。")
            .arg(st.branch.toHtmlEscaped()));
        box.exec();
        return;
    }

    // 分叉（同时 ahead 和 behind）
    if (st.isDiverge()) {
        const auto reply = QMessageBox::warning(
            this, QStringLiteral("推送警告"),
            QStringLiteral(
                "本地与远程已分叉：<br><br>"
                "&nbsp;&nbsp;本地领先：<b>%1</b> 个提交<br>"
                "&nbsp;&nbsp;本地落后：<b>%2</b> 个提交<br><br>"
                "直接推送会被拒绝。<br>"
                "建议先「拉取」并解决冲突，再推送。")
                .arg(st.ahead).arg(st.behind),
            QMessageBox::Cancel | QMessageBox::Yes);
        if (reply != QMessageBox::Yes) return;
        // 用户坚持要 push，交给 git 处理
    }
    // 纯落后
    else if (st.behind > 0) {
        QMessageBox::warning(
            this, QStringLiteral("推送警告"),
            QStringLiteral(
                "本地落后远程 <b>%1</b> 个提交。<br><br>"
                "请先「拉取」更新，再推送。").arg(st.behind));
        return;
    }
    // 没有新提交
    else if (st.ahead == 0) {
        QMessageBox::information(
            this, QStringLiteral("推送"),
            QStringLiteral("本地没有需要推送的提交（已与 %1 同步）。")
                .arg(st.upstream.toHtmlEscaped()));
        return;
    }

    logInfo(
        QStringLiteral("正在推送 %1 个提交到 %2 ...")
            .arg(st.ahead).arg(st.upstream));
    gm.push();
}

void MainWindow::onGitPull()
{
    auto& gm = Git::GitManager::instance();
    if (!gm.isActive()) return;

    if (!gm.hasRemote()) {
        logError("没有可用的远程仓库，无法拉取。");
        return;
    }

    const auto st = gm.currentUpstreamStatus();

    // 没有上游
    if (!st.hasUpstream) {
        QMessageBox::information(
            this, QStringLiteral("拉取"),
            QStringLiteral(
                "当前分支 <b>%1</b> 没有设置上游分支，无法拉取。<br><br>"
                "先在终端运行：<br>"
                "<code>git branch --set-upstream-to=origin/%1</code>")
                .arg(st.branch.toHtmlEscaped()));
        return;
    }

    // 已经在最新状态
    if (st.behind == 0) {
        // 但 working tree 可能脏 → pull 会失败
        int dirtyCount = 0;
        for (const auto& s : gm.currentStatus()) {
            if (s.isStaged() || s.hasUnstagedChanges()) ++dirtyCount;
        }
        if (dirtyCount > 0) {
            QMessageBox::information(
                this, QStringLiteral("拉取"),
                QStringLiteral(
                    "已与 %1 同步，无需拉取。<br><br>"
                    "（注意：当前有 %2 个未提交的更改，即使拉取也会失败，请先处理）")
                    .arg(st.upstream.toHtmlEscaped()).arg(dirtyCount));
        } else {
            QMessageBox::information(
                this, QStringLiteral("拉取"),
                QStringLiteral("已与 %1 同步，无需拉取。")
                    .arg(st.upstream.toHtmlEscaped()));
        }
        return;
    }

    logInfo(
        QStringLiteral("正在拉取 %1 个提交（%2）...")
            .arg(st.behind).arg(st.upstream));
    gm.pull();
}

void MainWindow::onGitFetch()
{
    if (!Git::GitManager::instance().isActive()) return;
    if (!Git::GitManager::instance().hasRemote()) {
        logError("没有可用的远程仓库，无法获取。");
        return;
    }
    logInfo("正在获取远程更新...");
    Git::GitManager::instance().fetch();
}

void MainWindow::onGitRefresh()
{
    if (!Git::GitManager::instance().isActive()) return;
    Git::GitManager::instance().refreshStatus();
}

void MainWindow::onGitShowHistory()
{
    if (!Git::GitManager::instance().isActive()) return;
    showGitDialog();
}

void MainWindow::onGitFileDoubleClicked(const QString& absolutePath)
{
    if (absolutePath.isEmpty()) return;
    if (!QFile::exists(absolutePath)) {
        logError(
            QString("文件已被删除或移动: %1")
                .arg(absolutePath));
        return;
    }
    openFile(absolutePath);
}

void MainWindow::updateGitActions()
{
    const bool active = Git::GitManager::instance().isActive();
    const bool hasRemote = Git::GitManager::instance().hasRemote();
    const bool hasProject = !d->currentProjectPath.isEmpty();

    if (m_gitInitAction) {
        m_gitInitAction->setEnabled(hasProject && !active);
    }
    if (m_gitCommitAction) {
        m_gitCommitAction->setEnabled(active);
    }
    if (m_gitPushAction) {
        m_gitPushAction->setEnabled(active && hasRemote);
    }
    if (m_gitPullAction) {
        m_gitPullAction->setEnabled(active && hasRemote);
    }
    if (m_gitFetchAction) {
        m_gitFetchAction->setEnabled(active && hasRemote);
    }
    if (m_gitRefreshAction) {
        m_gitRefreshAction->setEnabled(active);
    }
    if (m_gitShowHistoryAction) {
        m_gitShowHistoryAction->setEnabled(active);
    }
    if (m_gitToggleAction) {
        m_gitToggleAction->setEnabled(active);
    }
}

// ==================== Git 窗口控制 ====================

void MainWindow::showGitDialog()
{
    if (!m_gitDialog) return;

    // 打开时完整刷新（分支、历史、stash、状态）
    if (Git::GitManager::instance().isActive() && m_gitPanel) {
        m_gitPanel->refreshAll();
    }

    if (m_gitDialog->isMinimized()) {
        m_gitDialog->showNormal();
    } else {
        m_gitDialog->show();
    }
    m_gitDialog->raise();
    m_gitDialog->activateWindow();
}

void MainWindow::toggleGitDialog()
{
    if (!m_gitDialog) return;
    if (m_gitDialog->isVisible() && !m_gitDialog->isMinimized()) {
        m_gitDialog->hide();
    } else {
        showGitDialog();
    }
}

void MainWindow::onGitStatusLabelClicked()
{
    showGitDialog();
}

void MainWindow::onOpenGitSettingsFromPanel()
{
    SettingsDialog dlg(this, this);
    connect(&dlg, &SettingsDialog::aiServerRestartRequested,
            this, &MainWindow::onAIRestartRequested);
    connect(&dlg, &SettingsDialog::lspSettingsChanged,
            this, &MainWindow::reloadLspFilterSettings);
    dlg.showCategory(QStringLiteral("Git"));
    dlg.exec();
}

// ==================== ProjectPanel 右键 Git 动作分发 ====================

void MainWindow::onProjectPanelGitAction(int action, const QStringList& filePaths)
{
    using GA = SprintIDE::GitContextAction;
    const GA act = static_cast<GA>(action);

    auto& gm = Git::GitManager::instance();
    if (!gm.isActive()) return;

    const QString repoRoot = gm.repositoryInfo().rootPath;

    // 把绝对路径转成相对仓库根的路径
    auto toRel = [&](const QString& abs) -> QString {
        if (abs.isEmpty() || repoRoot.isEmpty()) return QString();
        return QDir(repoRoot).relativeFilePath(abs);
    };

    const QString firstAbs = filePaths.isEmpty() ? QString() : filePaths.first();
    const QString firstRel = toRel(firstAbs);

    switch (act) {

    // ============================================================
    // 文件级
    // ============================================================

    case GA::Add: {
        if (filePaths.isEmpty()) return;
        for (const QString& abs : filePaths) {
            const QString rel = toRel(abs);
            if (!rel.isEmpty()) gm.stageFile(rel);
        }
        logInfo(
            QStringLiteral("已暂存 %1 个文件").arg(filePaths.size()));
        break;
    }

    case GA::ShowDiff: {
        if (firstRel.isEmpty()) return;
        // 判断是已暂存还是工作区
        bool staged = false;
        for (const auto& s : gm.currentStatus()) {
            if (s.absolutePath == firstAbs) {
                // 有工作区改动 → 显示 worktree diff；否则显示 staged diff
                staged = !s.hasUnstagedChanges() && s.isStaged();
                break;
            }
        }
        const QString diffText = gm.rawDiff(firstRel, staged);
        auto* dlg = new Git::GitDiffDialog(this);
        dlg->setAttribute(Qt::WA_DeleteOnClose, true);
        connect(dlg, &Git::GitDiffDialog::openFileRequested,
                this, &MainWindow::openFile);
        dlg->showFileDiff(
            QStringLiteral("%1: %2")
                .arg(staged ? QStringLiteral("已暂存") : QStringLiteral("工作区"),
                     firstRel),
            diffText,
            firstAbs);
        break;
    }

    case GA::ShowHistory: {
        if (firstRel.isEmpty()) return;
        auto* dlg = new Git::GitFileHistoryDialog(firstAbs, firstRel, this);
        dlg->setAttribute(Qt::WA_DeleteOnClose, true);
        dlg->show();
        break;
    }

    case GA::ShowCurrentRevision: {
        const Git::GitCommit c = gm.fileRevision(firstRel);
        if (c.hash.isEmpty()) {
            QMessageBox::information(
                this, QStringLiteral("当前修订"),
                QStringLiteral("该文件尚未有任何提交。"));
            return;
        }
        QMessageBox box(this);
        box.setWindowTitle(QStringLiteral("当前修订"));
        box.setTextFormat(Qt::RichText);
        box.setIcon(QMessageBox::Information);
        box.setText(QStringLiteral(
            "<b>文件：</b> %1<br><br>"
            "<b>Commit：</b> <code>%2</code><br>"
            "<b>作者：</b> %3 &lt;%4&gt;<br>"
            "<b>时间：</b> %5<br><br>"
            "<b>消息：</b><br>%6")
            .arg(QFileInfo(firstAbs).fileName().toHtmlEscaped(),
                 c.hash,
                 c.author.toHtmlEscaped(),
                 c.email.toHtmlEscaped(),
                 c.timestamp.toString(Qt::ISODate),
                 c.message.toHtmlEscaped().replace('\n', "<br>")));
        box.exec();
        break;
    }

    case GA::Rollback: {
        if (firstRel.isEmpty()) return;

        // 判断是未跟踪还是已跟踪
        bool isUntracked = false;
        for (const auto& s : gm.currentStatus()) {
            if (s.absolutePath == firstAbs) {
                isUntracked = s.isUntracked() && !s.isStaged();
                break;
            }
        }

        const QString title = isUntracked
                                  ? QStringLiteral("删除文件")
                                  : QStringLiteral("回滚更改");
        const QString question = isUntracked
            ? QStringLiteral("确定要删除以下未跟踪文件吗？<br><br>"
                             "<code>%1</code><br><br>此操作不可恢复。")
                  .arg(firstRel.toHtmlEscaped())
            : QStringLiteral("确定要回滚对该文件的所有未提交更改吗？<br><br>"
                             "<code>%1</code><br><br>此操作不可恢复。")
                  .arg(firstRel.toHtmlEscaped());

        QMessageBox box(this);
        box.setWindowTitle(title);
        box.setIcon(QMessageBox::Warning);
        box.setTextFormat(Qt::RichText);
        box.setText(question);
        auto* okBtn     = box.addButton(
            isUntracked ? QStringLiteral("删除") : QStringLiteral("回滚"),
            QMessageBox::AcceptRole);
        auto* cancelBtn = box.addButton(QStringLiteral("取消"),
                                        QMessageBox::RejectRole);
        box.setDefaultButton(cancelBtn);
        box.exec();

        if (box.clickedButton() == okBtn) {
            gm.discardFileChanges(firstRel);
            logInfo(
                QStringLiteral("已回滚: %1")
                    .arg(firstRel));
        }
        break;
    }

    // ============================================================
    // 仓库级
    // ============================================================

    case GA::Commit:
        // 打开 Git 窗口，用户可以在里面暂存 / 提交
        showGitDialog();
        break;

    case GA::Push:
        onGitPush();
        break;

    case GA::Pull:
        onGitPull();
        break;

    case GA::Fetch:
        onGitFetch();
        break;

    case GA::NewBranch: {
        // 简单版：直接输入名字，从当前 HEAD 创建
        bool ok = false;
        const QString name = QInputDialog::getText(
            this, QStringLiteral("新建分支"),
            QStringLiteral("分支名（将从当前 HEAD 创建）："),
            QLineEdit::Normal, QString(), &ok);
        if (!ok || name.trimmed().isEmpty()) return;

        const QString branchName = name.trimmed();

        // 简单校验
        static const QRegularExpression invalid(
            "[\\s~^:?*\\[\\\\]|^\\.|^-|/\\.|\\.lock$|\\.$");
        if (invalid.match(branchName).hasMatch()) {
            QMessageBox::warning(
                this, QStringLiteral("新建分支"),
                QStringLiteral("分支名包含非法字符或格式。"));
            return;
        }

        // 是否已存在
        for (const auto& b : gm.branches()) {
            if (!b.isRemote && b.name == branchName) {
                QMessageBox::warning(
                    this, QStringLiteral("新建分支"),
                    QStringLiteral("分支 <b>%1</b> 已存在。")
                        .arg(branchName.toHtmlEscaped()));
                return;
            }
        }

        // 是否立即切换
        const auto reply = QMessageBox::question(
            this, QStringLiteral("新建分支"),
            QStringLiteral("从当前 HEAD 创建分支 <b>%1</b>，<br>"
                           "是否立即切换过去？")
                .arg(branchName.toHtmlEscaped()),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

        if (reply == QMessageBox::Yes) {
            gm.createBranchAndCheckout(branchName);
        } else {
            gm.createBranch(branchName);
        }
        logInfo(
            QStringLiteral("分支已创建: %1")
                .arg(branchName));
        break;
    }

    case GA::StashChanges: {
        // 简单版：输入消息，默认包含未跟踪文件
        bool ok = false;
        const QString msg = QInputDialog::getText(
            this, QStringLiteral("存储变更"),
            QStringLiteral("Stash 消息（可留空）："),
            QLineEdit::Normal, QString(), &ok);
        if (!ok) return;

        gm.stashPush(msg.isEmpty() ? QStringLiteral("WIP") : msg,
                     /*includeUntracked=*/true);
        logInfo(QStringLiteral("已创建 stash"));
        break;
    }

    // ============================================================
    // 尚未实现（占位）
    // ============================================================
    case GA::Annotate:
    case GA::CompareWithRevision:
    case GA::CompareWithBranch:
    case GA::Merge:
    case GA::Rebase:
    case GA::Branches:
    case GA::NewTag:
    case GA::ResetHead:
    case GA::UnstashChanges:
    case GA::ManageRemotes:
    default:
        QMessageBox::information(
            this, QStringLiteral("尚未实现"),
            QStringLiteral("该功能将在后续版本提供，<br>"
                           "请使用「Git → 打开 Git 窗口」进行相关操作。"));
        break;
    }
}

void MainWindow::updateGitStatusIndicator()
{
    if (!m_gitStatusLabel) return;

    auto& gm = Git::GitManager::instance();

    if (!gm.isActive()) {
        m_gitStatusLabel->setVisible(false);
        return;
    }

    const auto info = gm.repositoryInfo();
    const auto status = gm.currentStatus();

    int modified = 0, untracked = 0, staged = 0, conflicted = 0;
    for (const auto& s : status) {
        if (s.hasConflict())                    ++conflicted;
        if (s.isStaged())                       ++staged;
        if (s.hasUnstagedChanges())             ++modified;
        if (s.isUntracked())                    ++untracked;
    }

    QString branch = info.currentBranch;
    if (info.isDetachedHead) {
        branch = QStringLiteral("(detached)");
    }
    if (info.isEmpty) {
        branch = QStringLiteral("(空)");
    }

    QStringList parts;
    if (modified   > 0) parts << QStringLiteral("M:%1").arg(modified);
    if (staged     > 0) parts << QStringLiteral("S:%1").arg(staged);
    if (untracked  > 0) parts << QStringLiteral("U:%1").arg(untracked);
    if (conflicted > 0) parts << QStringLiteral("C:%1").arg(conflicted);

    QString text = QStringLiteral(
            "<img src=':/icons/icons/git_branch.svg' width='12' height='12' "
            "style='vertical-align:middle'> ");
        text += QStringLiteral("Git: <b>%1</b>").arg(branch.toHtmlEscaped());
    if (!parts.isEmpty()) {
        text += QStringLiteral("  ") + parts.join(QStringLiteral("  "));
    }

    m_gitStatusLabel->setText(text);
    m_gitStatusLabel->setVisible(true);
}

void MainWindow::onAIRestartRequested(const QString& host, quint16 port)
{
    if (!m_aiConnMgr) {
        logError("AI 连接管理器未初始化");
        return;
    }

    ConfigManager* cfg = Application::instance().configManager();
    const QString token = cfg ? cfg->value("AI/Token", "").toString() : QString();

    QString clientId = cfg ? cfg->value("AI/ClientId", "").toString() : QString();
    if (clientId.isEmpty()) {
        clientId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        if (cfg) cfg->setValue("AI/ClientId", clientId);
    }

    m_aiConnMgr->stop();
    m_aiConnMgr->start(host, port, token, clientId);

    logInfo(QString("AI 连接已重启：%1:%2").arg(host).arg(port));
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
        logInfo("代码为空，无需格式化。");
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
                logInfo("Python 代码格式化完成。");
            } else {
                logError("Python 格式化失败: " + error);
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
        logError(QString("Clang Format错误: %1").arg(errorOutput));
        return;
    }

    if (!formattedCode.isEmpty()) {
        editor->setPlainText(formattedCode);
        logInfo("代码格式化完成。");
    } else {
        logError("格式化后代码为空，可能是输入代码有问题。");
    }
}

void MainWindow::loadAllProjectFiles(const QString& projectPath)
{
    QDir projectDir(projectPath);
    if (!projectDir.exists()) {
        QMessageBox::warning(this, "错误", "项目路径不存在");
        return;
    }

    // ============================================================
    // 关键修复（v8.1+Git）：
    //   1. 排除构建产物 / 元数据目录（build、.git、node_modules …）
    //   2. QDir::NoSymLinks —— 不跟随符号链接
    //      原因：macOS .framework 里存在 QtCore ↔ Versions/5/QtCore
    //            的双向链接，跟随会导致无限递归 → 崩溃
    //   3. 深度硬限制，防止路径过深的仓库耗尽栈/内存
    // ============================================================
    static const QStringList kExcludeDirs = {
        // 版本控制
        ".git", ".svn", ".hg",
        // 构建产物
        "build", "bin", "obj", "out", "dist", "target",
        "Debug", "Release", "CMakeFiles",
        // 依赖 / 缓存
        "node_modules", "__pycache__", ".venv", "venv",
        ".gradle", ".mvn", ".cargo", ".rustup",
        // IDE 元数据
        ".vs", ".idea", ".vscode",
        // 打包
        "dSYM", ".dSYM"
    };

    constexpr int kMaxDepth = 16;
    constexpr int kMaxFiles = 20000;   // 硬上限，防止极端仓库拖死 UI

    QStringList filters;
    filters << "*.cpp" << "*.c" << "*.cc" << "*.cxx"
            << "*.h" << "*.hpp" << "*.hh" << "*.hxx"
            << "*.java" << "*.py" << "*.js" << "*.ts"
            << "*.html" << "*.css" << "*.xml" << "*.json"
            << "*.txt" << "*.md" << "*.cmake" << "*.makefile";

    QStringList sourceFiles;
    QStack<QPair<QString, int>> pending;    // (path, depth)
    pending.push({QDir(projectPath).absolutePath(), 0});

    while (!pending.isEmpty()) {
        if (sourceFiles.size() >= kMaxFiles) {
            qWarning() << "[loadAllProjectFiles] 达到文件数上限，提前结束";
            break;
        }

        const auto item  = pending.pop();
        const QString dirPath = item.first;
        const int depth  = item.second;

        if (depth > kMaxDepth) {
            continue;
        }

        QDir dir(dirPath);
        if (!dir.exists()) continue;

        // ---- 子目录：NoSymLinks + 排除名单 ----
        const QStringList subdirs = dir.entryList(
            QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);

        for (const QString& sub : subdirs) {
            // 大小写不敏感地检查排除目录
            bool skip = false;
            for (const QString& ex : kExcludeDirs) {
                if (sub.compare(ex, Qt::CaseInsensitive) == 0) {
                    skip = true;
                    break;
                }
            }
            // 排除 *.dSYM / *.framework 等
            if (!skip &&
                (sub.endsWith(QLatin1String(".dSYM"), Qt::CaseInsensitive) ||
                 sub.endsWith(QLatin1String(".framework"), Qt::CaseInsensitive) ||
                 sub.endsWith(QLatin1String(".app"), Qt::CaseInsensitive))) {
                skip = true;
            }
            if (skip) continue;

            pending.push({dir.absoluteFilePath(sub), depth + 1});
        }

        // ---- 当前目录文件：同样不跟随符号链接 ----
        dir.setNameFilters(filters);
        const QStringList files = dir.entryList(
            QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);
        for (const QString& f : files) {
            sourceFiles.append(dir.absoluteFilePath(f));
            if (sourceFiles.size() >= kMaxFiles) break;
        }
    }

    qDebug() << "[loadAllProjectFiles] 扫描完成，共"
             << sourceFiles.size() << "个文件";

    // ---- 顺手缓存给快速打开用 ----
    m_cachedProjectFiles = sourceFiles;
    m_cachedProjectRoot  = QDir(projectPath).absolutePath();
    m_cachedScanTime     = QDateTime::currentDateTime();
}

void MainWindow::reloadLspFilterSettings()
{
    ConfigManager* cfg = Application::instance().configManager();
    if (!cfg) return;

    const QString modeStr = cfg->value("Lsp/WarningMode", "errors-only").toString();

    DiagnosticFilter::Mode mode = DiagnosticFilter::Mode::ErrorsOnly;
    if (modeStr == "show-all") {
        mode = DiagnosticFilter::Mode::ShowAll;
    } else if (modeStr == "custom") {
        mode = DiagnosticFilter::Mode::Custom;
    }
    DiagnosticFilter::instance().setMode(mode);

    const QStringList rules = cfg->value("Lsp/IgnoreWarnings").toStringList();
    DiagnosticFilter::instance().loadUserRules(rules);

    qDebug() << "[LSP] 过滤模式:" << modeStr << "自定义规则数:" << rules.size();
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

            logInfo("==========================================");
            logInfo("项目创建成功！");
            logInfo(QString("项目名称: %1").arg(result.displayName));
            logInfo(QString("项目类型: %1").arg(getProjectTypeString(result.projectType)));
            logInfo(QString("项目路径: %1").arg(result.projectPath));
            logInfo("==========================================");

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

    m_outputPanel->clear();
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

    //    C++ 项目：不预设 outputName，让 CppBuilder 从 CMakeLists 里取真正的 target 名
    if (lang == LanguageType::Cpp || lang == LanguageType::Qt) {
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

    // ========== Qt 特有：把 QT_DIR 注入到 cmake 命令行的 CMAKE_PREFIX_PATH ==========
    if (lang == LanguageType::Qt) {
        QString qtDir = task.sdkConfig.value("QT_DIR").toString();

        // 项目配置没写就回退到全局设置
        if (qtDir.isEmpty()) {
            ConfigManager* cfg = Application::instance().configManager();
            if (cfg) {
                QVariantMap globalSdk = cfg->value("Sdk/Environment").toMap();
                qtDir = globalSdk.value("QT_DIR").toString();
            }
        }

        if (qtDir.isEmpty()) {
            // 给一个提示但不阻塞：让 CMake 靠系统自动查找，很多机器装在标准路径
            logError("警告：未在设置里配置 QT_DIR，"
                "CMake 将依赖系统自动探测 Qt。");
        } else {
            task.extraArgs << QString("-DCMAKE_PREFIX_PATH=%1").arg(qtDir);
            logInfo(QString("Qt SDK: %1").arg(qtDir));
        }
    }

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
            logInfo("C++ 项目构建系统: CMake");
            // task.outputName 留空，让 CppBuilder 内部用 model.defaultTarget() 自动选择
        } else if (projDir.exists("Makefile") ||
                   projDir.exists("makefile") ||
                   projDir.exists("GNUmakefile")) {
            logInfo("C++ 项目构建系统: Makefile");
        } else {
            logInfo("C++ 项目构建系统: SingleFile");
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
        logInfo("清理构建完成");
        m_statusLabel->setText("清理构建完成");
    } else {
        QString error = result.errorMessage.isEmpty() ? "清理构建失败" : result.errorMessage;
        logError("清理构建失败: " + error);
        m_statusLabel->setText("清理构建失败");
    }
}

void MainWindow::onCompile() {
    TextEditor* editor = currentEditor();
    // Qt 文件必须走项目构建（AUTOMOC/AUTOUIC 只能由 CMake 处理）
    if (!d->currentProjectPath.isEmpty()) {
        SprintIDE::CppProjectModel probe(d->currentProjectPath);
        probe.load();
        if (probe.isQtProject()) {
            logInfo("Qt 项目，切换到项目构建模式");
            onBuildProject();
            return;
        }
    }

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
        logInfo(QString("使用 Python 解释器: %1").arg(python));
        logInfo(QString("工作目录: %1").arg(projectPath));
        logInfo("正在运行...");

        // 构建命令参数
        QStringList args;
        args << "-u";
        args << sourceFile;

        // ===== 构造环境变量（保留！后面 QProcess 要用） =====
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("PYTHONUNBUFFERED", "1");
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
        QProcess* process = createRunProcess(python, args, projectPath, env);
        process->start();
        if (!process->waitForStarted(3000)) {
            logError("启动失败: " + process->errorString());
            process->deleteLater();
            return;
        }
        markProcessRunning(process, sourceFile);
        return;
    }


    // ========== C++ / 其他可执行语言：异步运行 ==========
    if (lang == LanguageType::Cpp || lang == LanguageType::Qt) {
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
        logInfo(QString("使用可执行文件: %1").arg(executablePath));
        logInfo("正在运行...");

        QProcess* process = createRunProcess(
            executablePath,
            task.extraArgs,
            QFileInfo(executablePath).absolutePath(),
            QProcessEnvironment());   // 空 env，用系统默认

        process->start();
        if (!process->waitForStarted(3000)) {
            logError("启动失败: " + process->errorString());
            process->deleteLater();
            return;
        }
        markProcessRunning(process, sourceFile);
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
        logInfo(output);
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
    SettingsDialog dialog(this, this);
    connect(&dialog, &SettingsDialog::aiServerRestartRequested,
            this,   &MainWindow::onAIRestartRequested);
    connect(&dialog, &SettingsDialog::lspSettingsChanged,
            this,   &MainWindow::reloadLspFilterSettings);   // 设置变化后立即生效
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
    logInfo("打开设置对话框...");
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
    logInfo("打开使用文档...");
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

    if (ConfigManager* cfg = Application::instance().configManager()) {
        cfg->setValue("General/Theme", "Dark");
    }

    logInfo("已切换到深色主题");
    m_statusLabel->setText("主题已切换为深色");
}

void MainWindow::onThemeLight()
{
    ThemeManager::instance().setTheme(ThemeManager::ThemeType::Light);
    applyCurrentTheme();

    if (ConfigManager* cfg = Application::instance().configManager()) {
            cfg->setValue("General/Theme", "Light");
    }

    logInfo("已切换到浅色主题");
    m_statusLabel->setText("主题已切换为浅色");
}

void MainWindow::onCustomTheme()
{
    logInfo("打开自定义主题...");
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
    logInfo(QString("已创建文件: %1").arg(fullPath));
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
            logError(QString("[%1] 编译已取消").arg(timeStr));
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

// ============================================================================
//  快捷键配置
// ============================================================================

void MainWindow::registerShortcut(const QString& id,
                                  const QString& displayName,
                                  QAction* action)
{
    if (!action || id.isEmpty()) return;

    ShortcutEntry entry;
    entry.id              = id;
    entry.displayName     = displayName;
    entry.action          = action;
    entry.defaultShortcut = action->shortcut();

    m_shortcutRegistry.insert(id, entry);
}

QList<MainWindow::ShortcutEntry> MainWindow::configurableShortcuts() const
{
    return m_shortcutRegistry.values();
}

void MainWindow::applyShortcutsFromConfig()
{
    ConfigManager* cfg = Application::instance().configManager();
    if (!cfg) return;

    for (auto it = m_shortcutRegistry.constBegin();
         it != m_shortcutRegistry.constEnd(); ++it) {
        const ShortcutEntry& entry = it.value();
        if (!entry.action) continue;

        const QString key = QStringLiteral("Shortcuts/") + entry.id;
        if (!cfg->contains(key)) {
            // 用户从未改过 → 保持默认
            continue;
        }

        const QString stored = cfg->value(key).toString();
        if (stored == QLatin1String("__EMPTY__")) {
            // 用户显式清空（禁用快捷键）
            entry.action->setShortcut(QKeySequence());
        } else {
            entry.action->setShortcut(QKeySequence(stored));
        }
    }
}

void MainWindow::setShortcutForAction(const QString& id, const QKeySequence& seq)
{
    auto it = m_shortcutRegistry.find(id);
    if (it == m_shortcutRegistry.end()) return;
    if (!it.value().action) return;
    it.value().action->setShortcut(seq);
}

void MainWindow::resetShortcutToDefault(const QString& id)
{
    auto it = m_shortcutRegistry.find(id);
    if (it == m_shortcutRegistry.end()) return;
    if (!it.value().action) return;
    it.value().action->setShortcut(it.value().defaultShortcut);
}

void MainWindow::resetAllShortcutsToDefault()
{
    for (auto it = m_shortcutRegistry.begin();
         it != m_shortcutRegistry.end(); ++it) {
        if (it.value().action) {
            it.value().action->setShortcut(it.value().defaultShortcut);
        }
    }
}

// ============================================================================
//  AI 辅助：状态同步
// ============================================================================
void MainWindow::syncAIAvailabilityToEditors()
{
    const bool available = (m_aiConnMgr && m_aiConnMgr->isReady());

    for (int i = 0; i < d->editorTabs->count(); ++i) {
        if (auto* ed = qobject_cast<TextEditor*>(d->editorTabs->widget(i))) {
            ed->setAIAvailable(available);
        }
    }
    if (m_outputPanel) {
        m_outputPanel->setAIAvailable(available);
    }
}

// ============================================================================
//  AI 辅助：编辑器右键菜单触发的请求
// ============================================================================
void MainWindow::onAIAssistRequested(const QString& prompt,
                                     const QString& filePath,
                                     int line, int column)
{
    if (!m_aiConnMgr || !m_aiConnMgr->isReady()) {
        if (m_aiChatPanel) {
            m_aiChatPanel->showSystemMessage(
                tr("AI Agent 未连接，请先启动 Agent。"), true);
            m_aiChatPanel->setVisible(true);
            m_aiChatPanel->raise();
            m_aiChatPanel->activateWindow();
        }
        return;
    }

    QJsonObject payload;
    payload["message"] = prompt;

    QJsonObject context;
    context["activeFile"]   = filePath;
    context["cursorLine"]   = line;
    context["cursorColumn"] = column;
    payload["context"] = context;

    const QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_aiConnMgr->sendMethod("chat.send", payload, requestId);

    if (m_aiChatPanel) {
        m_aiChatPanel->addUserMessage(prompt);
        m_aiChatPanel->setVisible(true);
        m_aiChatPanel->raise();
        m_aiChatPanel->activateWindow();
    }

    logInfo(QStringLiteral("AI 助手请求已发送：%1")
                .arg(prompt.left(80).replace(QLatin1Char('\n'), QLatin1Char(' '))));
}

// ============================================================================
//  AI 辅助：OutputPanel 触发"解释错误"
// ============================================================================
void MainWindow::onAIExplainError(const QString& errorText)
{
    if (errorText.trimmed().isEmpty()) return;

    const QString prompt =
        tr("请解释下面这条编译/运行错误，说明原因和修复方法：\n```\n%1\n```")
            .arg(errorText);

    onAIAssistRequested(prompt, QString(), 0, 0);
}

void MainWindow::onNewWindow()
{
    // 启动一个新的 SprintIDE 进程
    //   不传参数 → 新实例会弹出项目选择对话框（或空白主窗口）
    //   QProcess::startDetached 让新进程独立于当前实例，父进程退出不影响它
    const QString appPath = QCoreApplication::applicationFilePath();
    if (appPath.isEmpty()) return;

    if (!QProcess::startDetached(appPath, QStringList())) {
        logError(tr("无法启动新窗口：") + appPath);
    }
}

} // namespace SprintIDE

