#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QStandardItemModel>

#include <QPlainTextEdit>
#include <QWidget>
#include <QFile>
#include <QTextStream>
#include <QProgressBar>

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

class TextEditor;

namespace SprintIDE {

class MainWindowPrivate;
class CompileManager;
class FindDialog;
class ReplaceDialog;
struct CompileResult;

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

public slots:
    // 调试菜单槽函数
    void onStartDebug();
    void onStopDebug();
    void onStepOver();
    void onStepInto();
    void onStepOut();
    void onContinueDebug();
    void onAddBreakpoint();

    void onOutputPanelContextMenu(const QPoint& pos);
    void clearOutputPanel();

    // 调试信号处理
    void onDebugStateChanged(DebugState state);
    void onBreakpointHit(const QString& filePath, int lineNumber);
    void onProgramStopped(const QString& filePath, int lineNumber, const QString& reason);
    void onProgramExited(int exitCode);
    void onDebugError(const QString& error);
    void onBreakpointToggled(const QString& filePath, int line);

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
    void generateCompileCommands(const QString& projectPath);   // 生成 compile_commands.json

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

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupDockWidgets();
    void setupConnections();
    void setupDebugPanel(); 
    void setupTerminalPanel();
    void loadAllProjectFiles(const QString& projectPath);

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


    // 面板切换方法
    void switchToDebugTab();
    void switchToOutputTab();
    void switchToTerminalTab();

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
    bool ensureEclipseProjectFiles(const QString& projectPath);

    void saveSession();
    void restoreSession();
    void saveExpandedPaths(QJsonArray &paths, const QModelIndex &parent);
    void restoreExpandedPaths(const QJsonArray &paths);

private:
    MainWindowPrivate *d;
    // 底部面板相关
    QDockWidget* m_bottomDockWidget = nullptr;
    QTabWidget* m_bottomTabWidget = nullptr;

    FindDialog* m_findDialog = nullptr;
    ReplaceDialog* m_replaceDialog = nullptr;
    QTextCursor m_lastFindCursor;
    bool m_compileForDebug = false;

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
};

} // namespace SprintIDE

#endif // MAINWINDOW_H
