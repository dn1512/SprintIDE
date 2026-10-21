/*
* SprintIDE - SprintIDE is a cross-platform IDE built from scratch with Qt 6 and C++20. Its core capability is to provide AI Agents with an executable, verifiable code execution environment.
* Copyright (C) 2026  Liu Tengjun (刘腾骏)
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
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

#include <QPointer>
#include <QProcess>
#include <QProcessEnvironment>
#include <QKeySequence>

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

#include "ai/ai_integration.h"
#include "ai/chat_panel.h"
#include "ai/ai_connection_manager.h"

#include "history/local_history.h"
#include "python/python_debugger.h"

#include "cpp/cpp_debugger.h"
#include "core/log_buffer.h"
#include "performance/performance_monitor.h"
#include "problems/diagnostics_store.h"
#include "problems/problems_panel.h"

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
class PanelFocusManager;

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

    /// 切换到右侧编辑器标签（循环）
    void activateNextEditorTab();

    /// 切换到左侧编辑器标签（循环）
    void activatePreviousEditorTab();

    bool hasUnsavedChanges() const;
    QString currentFilePath() const;
    QString projectPath() const;
    bool isCompiling() const;
    // ========== AI 集成入口（供外部模块调用）==========
    void aiRequestBuild();      ///< 触发项目构建（等价 onBuildProject）
    void aiRequestRun();        ///< 触发运行当前文件（等价 onRun）
    void aiRequestStopRun();    ///< 停止正在运行的程序

    // ========== 快捷键配置（供 ShortcutSettingsPage 使用）==========
    struct ShortcutEntry {
        QString    id;
        QString    displayName;
        QAction*   action          = nullptr;
        QKeySequence defaultShortcut;
    };

    /// 返回所有已注册的可配置快捷键（按注册顺序）
    QList<ShortcutEntry> configurableShortcuts() const;

    /// 从 QSettings 读取用户自定义快捷键并应用到 action
    /// 应在 setupMenuBar / setupToolBar 之后调用
    void applyShortcutsFromConfig();

    /// 更新单个 action 的快捷键（供设置页实时预览用）
    void setShortcutForAction(const QString& id, const QKeySequence& seq);

    /// 重置单个快捷键为默认值
    void resetShortcutToDefault(const QString& id);

    /// 重置全部快捷键为默认值
    void resetAllShortcutsToDefault();
protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void changeEvent(QEvent* event) override;

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

    // 按文件路径找已打开的编辑器（供 AI file.edit 使用）
    TextEditor* findEditorByFilePath(const QString& filePath);

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

    /// 诊断变化时刷新状态栏计数
    void onDiagnosticsChanged();

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

    // Quick Fix 请求中转
    void onQuickFixRequested(const QString& filePath,
                             int startLine, int startColumn,
                             int endLine,   int endColumn);

     // ========== AI 辅助 ==========
     /// TextEditor 右键菜单触发的 AI 请求
     void onAIAssistRequested(const QString& prompt,
                              const QString& filePath,
                              int line, int column);

     /// OutputPanel 触发的"AI 解释此错误"
     void onAIExplainError(const QString& errorText);

     void onNewWindow();   // 启动一个新 SprintIDE 实例

signals:
    // 请求项目面板刷新
    // 任何地方想刷新项目面板，只需要：
    //     emit projectPanelRefreshRequested();
    // 已自动连接到 ProjectPanel::onRefreshRequested()
    void projectPanelRefreshRequested();

    /// AI 触发的构建完成（供 IdeExecutor 使用）
    void aiBuildFinished(bool success, int exitCode,
                         const QString& log, int durationMs);

    /// AI 触发的运行结束（供 IdeExecutor 使用）
    void aiRunFinished(int exitCode, const QString& log);

    /// AI 触发的运行被停止（供 IdeExecutor 使用）
    void aiRunStopped(int exitCode, const QString& log);

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

    //   返回 true 表示"新生成且 clangd 已启动，需要提示用户重启"
    bool ensureQtClangdConfig(const QString& projectPath);

    void applyTheme();

    bool confirmUnsavedChanges();
    void updateStatusBarPosition(TextEditor* editor);
    void updateWindowTitle();
    void autoSaveAllFiles();
    void restoreWindowState();
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
    bool m_aiBuildInProgress = false;   // AI 触发的构建标志
    bool m_aiRunInProgress = false;     // AI 触发的运行标志
    bool m_aiRunStopInProgress = false; // AI 主动停标志
    QString m_aiRunOutputBuffer;        // AI 触发运行期间的 stdout/stderr 累积
    QElapsedTimer m_compileTimer;
    QMap<QString, QString> m_compilerOutputCache;

    // 工具栏运行按钮（运行中会切换成"停止"）
    QAction* m_runAction = nullptr;

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
    QLabel* m_problemsIndicatorLabel = nullptr; // Problems 计数 + 点击跳转

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
    // entry 必填（结构化）；颜色由 LogLevel 决定
    void appendLog(const LogEntry& entry);

    QStringList collectProjectFiles(bool forceRescan = false);
    /// 创建并连接一个"运行用"QProcess（统一输出/结束信号处理）
    QProcess* createRunProcess(const QString& program,
                               const QStringList& arguments,
                               const QString& workDir,
                               const QProcessEnvironment& env);

    /// 标记"有程序正在运行"（更新 UI）
    void markProcessRunning(QProcess* proc, const QString& sourcePath);

    /// 标记"运行结束"（恢复 UI）
    void markProcessFinished();

    /// 停止当前运行的程序（terminate → kill）
    void stopRunningProcess();

    /// 是否正在运行程序
    bool isRunningProgram() const { return !m_runningProcess.isNull(); }

    /// AI 连接状态变化时，同步所有编辑器 + OutputPanel
    void syncAIAvailabilityToEditors();

private:
    MainWindowPrivate *d;
    // 底部面板相关
    QDockWidget* m_bottomDockWidget = nullptr;
    QTabWidget* m_bottomTabWidget = nullptr;
    OutputPanel* m_outputPanel = nullptr;   // 结构化输出面板
    ProblemsPanel* m_problemsPanel = nullptr;   // Problems 面板

    FindDialog* m_findDialog = nullptr;
    ReplaceDialog* m_replaceDialog = nullptr;
    SearchInFilesDialog* m_searchInFilesDialog = nullptr;
    QTextCursor m_lastFindCursor;
    bool m_compileForDebug = false;
    bool m_autoRunAfterCompile = false;
    int m_dirtyTabCount = 0;      // 当前有未保存修改的 tab 数量

    // LSPClient* m_lspClient = nullptr;
    LSPManager *m_lspManager;
    TerminalPanel* m_terminalPanel = nullptr;  // 终端面板
    // AI 模块成员
    ChatPanel *m_aiChatPanel = nullptr;
    AiConnectionManager *m_aiConnMgr = nullptr;   // 阶段 0：连接管理器
    QAction *m_aiAutoFixAction = nullptr;  // 用于菜单状态

    // ========== 快捷键注册表 ==========
    /// id → entry。id 用 "domain.action" 格式（如 "file.save"）
    QMap<QString, ShortcutEntry> m_shortcutRegistry;

    /// 注册一个可配置快捷键（在 action 创建后立即调用）
    void registerShortcut(const QString& id,
                          const QString& displayName,
                          QAction* action);
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
    QTimer* m_gitRefreshDebounce = nullptr;      // git防抖
    // 状态栏 Git 指示器
    QLabel*  m_gitStatusLabel = nullptr;

    QAction* m_toggleBottomPanelAction = nullptr;
    QAction* m_toggleProjectPanelAction = nullptr;
    QAction* m_clearDiagnosticsAction  = nullptr;

    // ========== 断点窄条 + 列表页签 ==========
    BreakpointListPanel* m_breakpointListPanel     = nullptr;
    QToolButton*         m_viewBreakpointsButton   = nullptr;
    QToolButton*         m_muteBreakpointsButton   = nullptr;
    bool                 m_breakpointsMuted        = false;

    // ========== 运行进程管理 ==========
    QPointer<QProcess> m_runningProcess;   ///< 当前正在运行的程序（自动置空）
    QString            m_runningFilePath;  ///< 当前运行源文件（用于状态恢复）
    bool               m_stoppingProcess = false;  ///< 是否正在主动停止进程
    // 窗口激活防抖定时器
    QTimer* m_projectPanelRefreshDebounce = nullptr;
    // 按文件缓存最近一次 LSP 诊断的完整信息（保留 source / code）供 onQuickFixRequested 组装 codeAction 的 context.diagnostics 使用
    QHash<QString, QList<LSPDiagnostic>> m_lastDiagnosticsByFile;

    // ========== Problems ==========
    DiagnosticsStore* m_diagnosticsStore = nullptr;

    /// 把 DiagnosticsStore 里某文件的诊断同步到编辑器的波浪线
    void syncDiagnosticsToEditor(const QString& filePath);

    PanelFocusManager* m_panelFocusManager = nullptr;
};

} // namespace SprintIDE

#endif // MAINWINDOW_H
