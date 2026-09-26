#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDialog>
#include <QString>
#include <QStandardItemModel>

#include <QPlainTextEdit>
#include <QWidget>
#include <QFile>
#include <QTextStream>
#include <QProgressBar>
#include <QToolButton>

#include "text_editor.h"
#include "tab_widget.h"
#include "theme_manager.h"
#include "project_creation_dialog.h"
#include "project_config.h"
#include "compile_manager.h"
#include "debug_manager.h"
#include "debug_panel.h"
#include "find_dialog.h"
#include "replace_dialog.h"
#include "lsp/lsp_manager.h"
#include "terminal/terminal_panel.h"

#include "ai/ai_action_executor.h"
#include "ai/ai_integration.h"
#include "ai/ai_server.h"
#include "ai/chat_panel.h"
#include "history/local_history.h"
#include "python/python_debugger.h"

#include "cpp/cpp_debugger.h"
#include "core/log_buffer.h"
#include "performance/performance_monitor.h"

class TextEditor;

namespace SprintIDE {

class MainWindowPrivate;
class CompileManager;
class FindDialog;
class ReplaceDialog;
struct CompileResult;
class PythonDebugger;
class BreakpointListPanel;
class SearchInFilesDialog;
class OutputPanel;

// Git 模块前向声明
namespace Git { class GitPanel; }

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool compileProject(const QString& projectPath, const QString& projectName);
    bool runProject(const QString& projectPath, const QString& projectName);
    void onNewProject();

    void openProject(const QString &path);
    void closeProject();

    void openFile(const QString &path);
    void openFiles(const QStringList &paths);
    void closeFile(const QString &path);

    bool startDebugging();
    void stopDebugging();

    TextEditor* currentEditor() const;

    bool hasUnsavedChanges() const;
    QString currentFilePath() const;
    QString projectPath() const;
    bool isCompiling() const;

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

public slots:
    // 调试菜单槽函数
    void onStartDebug();
    void onStopDebug();
    void onStepOver();
    void onStepInto();
    void onStepOut();
    void onContinueDebug();
    void onAddBreakpoint();

    void clearOutputPanel();

    // 调试信号处理
    void onDebugStateChanged(DebugState state);
    void onBreakpointHit(const QString& filePath, int lineNumber);
    void onProgramStopped(const QString& filePath, int lineNumber, const QString& reason);
    void onProgramExited(int exitCode);
    void onDebugError(const QString& error);
    void onBreakpointToggled(const QString& filePath, int line);
    void onPythonDebugFinished();

    // 格式化代码
    void onFormatCode();

    // 标签页管理
    void createNewEditorTab(const QString& title);
    bool closeEditorTab(int index);
    bool closeEditorForFile(const QString& filePath);
    void updateTabTitle(int index, const QString& title);

    // 标签页信号处理
    void onTabCloseRequested(int index);
    void onTabCurrentChanged(int index);
    void onTabContextMenuRequested(int index, const QPoint &pos);

    // 编辑器信号处理
    void onEditorModificationChanged(bool modified);
    void onEditorCursorPositionChanged(int line, int column);
    void onFileLoaded(const QString &filePath);
    void onFileSaved(const QString &filePath);
    void onFileRenamed(const QString& oldPath, const QString& newPath);
    void onEditorTextChanged(TextEditor* editor);

    // 项目面板信号处理
    void onProjectItemDoubleClicked(const QModelIndex &index);

    // 编译相关槽函数
    void onCompileStarted(const QString& taskId, const QString& sourceFile);
    void onCompileProgress(const QString& taskId, int progress, const QString& currentFile);
    void onCompileOutput(const QString& taskId, const QString& output, bool isError);
    void onCompileFinished(const QString& taskId, const CompileResult& result);
    void onCompileError(const QString& taskId, const QString& errorMessage);
    void onCompilersDetected(const QStringList& compilers);

    void stopCompile();
    void askRunProgram(const QString& executablePath);
    void runProgram(const QString& executablePath, bool inTerminal = false);
    void syncBreakpointsToDebugger();
    void onLSPDiagnostics(const QString& filePath, const QList<LSPDiagnostic>& diagnostics);

    // LSP 跳转到定义
    void onDefinitionReceived(const QString& file, int line, int column);

    void onQuickOpen();
    // void generateCompileCommands(const QString& projectPath);   // 生成 compile_commands.json

    // 菜单槽函数
    void onNewFile();
    void onOpenFile();
    void onSaveFile();
    void onSaveAs();
    void onCloseFile();
    void onExit();

    void onUndo();
    void onRedo();
    void onCut();
    void onCopy();
    void onPaste();
    void onFind();
    void onReplace();
    void onSearchInFiles();
    void onComment();
    void onGotoLine();

    void onOpenProject();
    void onCloseProject();
    void onBuildProject();
    void onCleanBuild();

    void onCompile();
    void onRun();
    QString findDebugger();
    void onCompileSettings();

    void onZoomIn();
    void onZoomOut();
    void onZoomReset();

    void onSettings();
    void onQuickStart();
    void onDocumentation();
    void onAbout();

    void onThemeChanged(ThemeManager::ThemeType theme);
    void onThemeLight();
    void onThemeDark();
    void onCustomTheme();

    /**
     * @brief 创建一个新文件，并让用户输入文件名
     * @param defaultName 默认文件名（如 "main.cpp"）
     * @param title 对话框标题（如 "新建 C++ 文件"）
     * @param customDir 自定义目录（为空则使用项目 src 目录或当前目录）
     */
    void createNewFileWithName(const QString &defaultName,
                               const QString &title = "新建文件",
                               const QString &customDir = QString());

    //查找相关函数
    void onFindNext(const QString& text, bool caseSensitive, bool wholeWord);
    void onFindPrevious(const QString& text, bool caseSensitive, bool wholeWord);
    void onReplaceCurrent(const QString& findText, const QString& replaceText,
                          bool caseSensitive, bool wholeWord);
    void onReplaceAll(const QString& findText, const QString& replaceText,
                      bool caseSensitive, bool wholeWord);
    void onDebuggerContinued();

    void onAIRestartRequested(const QString& host, quint16 port);

    // 性能监控：一次采样回调
    void onPerformanceSnapshot(const SprintIDE::PerformanceSnapshot& snapshot);

    // ========== Git 槽函数 ==========
    void onGitInit();
    void onGitCommit();
    void onGitPush();
    void onGitPull();
    void onGitFetch();
    void onGitRefresh();
    void onGitShowHistory();
    void onGitFileDoubleClicked(const QString& absolutePath);
    void updateGitActions();
    void updateGitStatusIndicator();   // 刷新状态栏的 Git 指示器

    void showGitDialog();          // 打开 Git 窗口
    void toggleGitDialog();        // 菜单/快捷键：切换显隐
    void onGitStatusLabelClicked();// 状态栏指示器点击
    void onOpenGitSettingsFromPanel();  // GitPanel 请求打开设置页

    // ProjectPanel 右键 Git 子菜单分发
    void onProjectPanelGitAction(int action, const QStringList& filePaths);

    /// 断点变更后刷新列表面板
    void refreshBreakpointListPanel();

    /// 窄条按钮：查看断点（toggle 断点页签）
    void onToggleBreakpointTab();

    /// 窄条按钮：禁用 / 恢复所有断点
    void onToggleMuteBreakpoints();

    void logInfo (const QString& msg,
                 LogKind kind = LogKind::System,
                 const QString& source = QStringLiteral("IDE"));
    void logWarn (const QString& msg,
                 LogKind kind = LogKind::System,
                 const QString& source = QStringLiteral("IDE"));
    void logError(const QString& msg,
                  LogKind kind = LogKind::System,
                  const QString& source = QStringLiteral("IDE"));

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupDockWidgets();
    void setupConnections();
    void setupDebugPanel(); 
    void setupTerminalPanel();
    void setupGitPanel();
    void setupBreakpointListPanel();
    void loadAllProjectFiles(const QString& projectPath);
    void reloadLspFilterSettings();

    void applyTheme();

    bool confirmUnsavedChanges();
    void updateStatusBarPosition(TextEditor* editor);
    void updateWindowTitle();
    void saveWindowState();

    void applyCurrentTheme();
    void applyThemeToDockWidget(QDockWidget* dockWidget, bool isDark);
    void applyThemeToAll();
    void applyThemeToWidgetRecursive(QWidget* widget, bool isDark);
    void applyThemeToProjectTree(bool isDark);
    void applyThemeToTextEditors(bool isDark);
    void applyThemeToTabWidget(bool isDark);
    QString getProjectTypeString(ProjectType type) const;
    // void updateProjectTree(const QString& projectPath);
    // void loadDirectoryToTree(const QString& directoryPath, QStandardItem* parentItem);

    void connectCompileManagerSignals();
    QString findExecutableForSource(const QString& sourceFile);
    QString findClangFormat();

    // 编译相关成员
    CompileManager* m_compileManager = nullptr;
    QString m_currentCompileTaskId;
    bool m_isCompiling = false;
    QElapsedTimer m_compileTimer;
    QMap<QString, QString> m_compilerOutputCache;

    // 调试菜单动作指针
    QAction* m_startDebugAction = nullptr;
    QAction* m_stopDebugAction = nullptr;
    QAction* m_stepOverAction = nullptr;
    QAction* m_stepIntoAction = nullptr;
    QAction* m_stepOutAction = nullptr;
    QAction* m_continueAction = nullptr;
    QAction* m_addBreakpointAction = nullptr;

    bool isSourceNewerThanExecutable(const QString& sourceFile, const QString& executablePath);
    bool needsRecompile(const QString& sourceFile, const QString& executablePath);

    // 调试相关
    DebugManager* m_debugManager = nullptr;
    bool m_isDebugging = false;

    // 调试面板（独立类）
    DebugPanel* m_debugPanel = nullptr;

    // 调试状态缓存
    QList<StackFrame> m_callStack;
    QList<DebugVariable> m_localVariables;

    void updateDebugActions();
    void updateDebugPanels();
    QString resolveDebuggerPath(const QString& filePath);

    // 状态栏控件
    QLabel* m_statusLabel = nullptr;          // 主状态信息
    QLabel* m_cursorPositionLabel = nullptr;  // 光标位置 (行:列)
    QLabel* m_encodingLabel = nullptr;        // 编码 (UTF-8)
    QLabel* m_languageLabel = nullptr;        // 语言 (C++)
    QLabel* m_indentLabel = nullptr;          // 缩进 (空格:4)
    QProgressBar* m_buildProgressBar = nullptr; // 编译进度条

    // ========== 性能监控 ==========
    QLabel*             m_performanceLabel   = nullptr;
    PerformanceMonitor* m_performanceMonitor = nullptr;


    // 面板切换方法
    void switchToDebugTab();
    void switchToOutputTab();
    void switchToTerminalTab();

    // 集中管理调试 Tab 的可见性
    /// @param visible true=显示（并切过去），false=隐藏（若当前在它上面则切到输出）
    void setDebugTabVisible(bool visible);

    void showFileHistoryDialog(const QString &filePath);

    /**
     * @brief 根据文件路径检测语言类型
     */
    // LanguageType detectLanguageTypeFromPath(const QString& filePath);
    /**
     * @brief 检查并生成 Maven 项目的 Eclipse 元数据文件
     * @param projectPath 项目根目录
     * @return true 表示成功或已存在，false 表示失败或用户取消
     */
    // bool ensureEclipseProjectFiles(const QString& projectPath);

    void saveSession();
    void restoreSession();
    void saveExpandedPaths(QJsonArray &paths, const QModelIndex &parent);
    void restoreExpandedPaths(const QJsonArray &paths);
    bool startDebuggingWithExecutable(const QString& executablePath);

    bool isPythonDebugging() const;

    void runExecutableAsync(const QString& executablePath);

    bool isCppDebugging() const;
    // entry 必填（结构化）；html 为空时退化为纯文本渲染
    void appendLog(const LogEntry& entry, const QString& html = QString());

    QStringList collectProjectFiles(bool forceRescan = false);
private:
    MainWindowPrivate *d;
    // 底部面板相关
    QDockWidget* m_bottomDockWidget = nullptr;
    QTabWidget* m_bottomTabWidget = nullptr;
    OutputPanel* m_outputPanel = nullptr;   // 结构化输出面板

    FindDialog* m_findDialog = nullptr;
    ReplaceDialog* m_replaceDialog = nullptr;
    SearchInFilesDialog* m_searchInFilesDialog = nullptr;
    QTextCursor m_lastFindCursor;
    bool m_compileForDebug = false;
    bool m_autoRunAfterCompile = false;
    int m_dirtyTabCount = 0;      // 当前有未保存修改的 tab 数量

    // LSPClient* m_lspClient = nullptr;
    LSPManager *m_lspManager;
    TextEditor* findEditorByFilePath(const QString& filePath);
    TerminalPanel* m_terminalPanel = nullptr;  // 终端面板
    // AI 模块成员
    AIServer *m_aiServer = nullptr;
    AIActionExecutor *m_aiExecutor = nullptr;
    ChatPanel *m_aiChatPanel = nullptr;
    QAction *m_aiAutoFixAction = nullptr;  // 用于菜单状态
    // ========== 本地历史管理器指针 ==========
    LocalHistory *m_localHistory = nullptr;
    // 存储每个 LSPClient 对应的 definition 信号连接
    QMap<LSPClient*, QMetaObject::Connection> m_definitionConnections;

    // 用于等待 LSP 就绪的延迟连接管理（防止重复创建）
    QMap<LSPClient*, QMetaObject::Connection> m_pendingDefinitionConnections;

    // 用于等待 LSP 就绪的定时器（可选，但建议保留）
    QTimer* m_lspWaitTimer = nullptr;

    PythonDebugger* m_pythonDebugger = nullptr;

    CppDebugger* m_cppDebugger = nullptr;

    QStringList m_cachedProjectFiles;
    QString     m_cachedProjectRoot;
    QDateTime   m_cachedScanTime;

    // ========== Git 集成 ==========
    // Git 面板改为独立窗口，不再占用右侧停靠区
    Git::GitPanel* m_gitPanel   = nullptr;
    QDialog*       m_gitDialog  = nullptr;

    QAction* m_gitInitAction        = nullptr;
    QAction* m_gitCommitAction      = nullptr;
    QAction* m_gitPushAction        = nullptr;
    QAction* m_gitPullAction        = nullptr;
    QAction* m_gitFetchAction       = nullptr;
    QAction* m_gitRefreshAction     = nullptr;
    QAction* m_gitShowHistoryAction = nullptr;
    QAction* m_gitToggleAction      = nullptr;   // 打开/关闭 Git 窗口

    // 状态栏 Git 指示器
    QLabel*  m_gitStatusLabel = nullptr;

    QAction* m_toggleBottomPanelAction = nullptr;
    QAction* m_toggleProjectPanelAction = nullptr;

    // ========== 断点窄条 + 列表页签 ==========
    BreakpointListPanel* m_breakpointListPanel     = nullptr;
    QToolButton*         m_viewBreakpointsButton   = nullptr;
    QToolButton*         m_muteBreakpointsButton   = nullptr;
    bool                 m_breakpointsMuted        = false;
};

} // namespace SprintIDE

#endif // MAINWINDOW_H
