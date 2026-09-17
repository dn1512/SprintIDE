// debug_manager.h
#ifndef DEBUG_MANAGER_H
#define DEBUG_MANAGER_H

#include <QObject>
#include <QProcess>
#include <QMap>
#include <QList>
#include <QString>
#include <QDateTime>
#include <QScopedPointer>

namespace SprintIDE {

// 前向声明
class TextEditor;

/**
 * @brief 断点信息
 */
struct Breakpoint {
    QString filePath;       // 文件路径
    int lineNumber;         // 行号
    QString condition;      // 条件表达式（空表示无条件）
    bool enabled;           // 是否启用
    int hitCount;           // 命中次数

    Breakpoint()
        : lineNumber(0)
        , enabled(true)
        , hitCount(0) {}
};

/**
 * @brief 变量信息
 */
struct DebugVariable {
    QString name;           // 变量名
    QString type;           // 类型
    QString value;          // 当前值
    bool isComplex;         // 是否复杂类型（结构体/类）
    QList<DebugVariable> children;  // 子变量（成员）

    int variablesReference = 0;     // 来自 DAP；>0 表示可展开
    QString evaluateName;           // DAP 的变量全路径名（用于表达式求值）

    DebugVariable() : isComplex(false) {}
};

/**
 * @brief 栈帧信息
 */
struct StackFrame {
    int level;              // 帧级别
    QString functionName;   // 函数名
    QString filePath;       // 文件路径
    int lineNumber;         // 行号
    QString address;        // 地址
    QList<DebugVariable> locals;  // 局部变量
};

/**
 * @brief 调试状态
 */
enum class DebugState {
    Stopped,        // 已停止（未启动）
    Starting,       // 启动中
    Running,        // 运行中
    Paused,         // 暂停（断点命中）
    Stepping,       // 单步执行中
    Finished,       // 程序结束
    Error           // 错误状态
};

/**
 * @brief 调试配置
 */
struct DebugConfig {
    QString debuggerPath;       // 调试器路径（为空则自动查找）
    QString executablePath;     // 可执行文件路径
    QStringList arguments;      // 程序参数
    QString workingDirectory;   // 工作目录
    QStringList breakpoints;    // 初始断点（文件:行号）
    bool stopOnEntry;           // 是否在入口处停止
    bool enablePrettyPrinting;  // 启用美化输出

    DebugConfig()
        : stopOnEntry(false)
        , enablePrettyPrinting(true) {}
};

/**
 * @class DebugManager
 * @brief 调试管理器 - 管理程序的调试会话
 */
class DebugManager : public QObject
{
    Q_OBJECT

public:
    static DebugManager& instance();

    bool initialize();
    void shutdown();

    // ========== 调试会话控制 ==========
    bool startDebugging(const DebugConfig& config);
    void stopDebugging();
    bool isDebugging() const;
    DebugState currentState() const;
    DebugConfig currentConfig() const;

    // ========== 执行控制 ==========
    void continueExecution();
    void stepOver();
    void stepInto();
    void stepOut();
    void runToCursor(const QString& filePath, int lineNumber);

    // ========== 断点管理 ==========
    bool addBreakpoint(const QString& filePath, int lineNumber, const QString& condition = QString());
    bool removeBreakpoint(const QString& filePath, int lineNumber);
    void toggleBreakpoint(const QString& filePath, int lineNumber);
    bool setBreakpointEnabled(const QString& filePath, int lineNumber, bool enabled);
    bool setBreakpointCondition(const QString& filePath, int lineNumber, const QString& condition);
    QList<Breakpoint> getBreakpoints() const;
    void clearAllBreakpoints();
    bool hasBreakpoint(const QString& filePath, int lineNumber) const;

    // ========== 状态查询 ==========
    QList<StackFrame> getCallStack() const;
    QList<DebugVariable> getLocalVariables(int frameIndex = 0) const;
    QPair<QString, int> getCurrentPosition() const;
    QString evaluateExpression(const QString& expression);

    // ========== 编辑器集成 ==========
    void registerEditor(TextEditor* editor);
    void unregisterEditor(TextEditor* editor);
    void updateAllEditors();

    /**
     * @brief 启动 Java 调试（简化版，仅启动 JVM 并开启 JDWP）
     * @param config 调试配置
     * @return 成功返回 true
     */
    bool startJavaDebugging(const DebugConfig& config);
signals:
    void stateChanged(DebugState state);
    void debuggingStarted();
    void debuggingStopped();
    void breakpointHit(const QString& filePath, int lineNumber);
    void programStopped(const QString& filePath, int lineNumber, const QString& reason);
    void programContinued();
    void programExited(int exitCode);
    void programOutput(const QString& output, bool isError);
    void variablesUpdated(const QList<DebugVariable>& variables);
    void callStackUpdated(const QList<StackFrame>& stack);
    void errorOccurred(const QString& error);

private slots:
    void onDebuggerReadyRead();
    void onDebuggerReadyReadError();
    void onDebuggerFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onDebuggerErrorOccurred(QProcess::ProcessError error);

private:
    explicit DebugManager(QObject* parent = nullptr);
    ~DebugManager();

    // 禁用拷贝
    DebugManager(const DebugManager&) = delete;
    DebugManager& operator=(const DebugManager&) = delete;

    // 内部辅助方法
    QString findDebugger() const;
    QString detectDebuggerType(const QString& debuggerPath) const;
    void sendCommand(const QString& command);
    void syncBreakpoints();
    QString breakpointKey(const QString& filePath, int lineNumber) const;

    // GDB输出解析方法
    void parseGdbOutput(const QString& output);
    void parseLldbOutput(const QString& output);
    void parseGdbStoppedEvent(const QString& event);
    void parseGdbBreakpointCreated(const QString& event);
    void parseGdbDoneResponse(const QString& response);
    void parseGdbErrorResponse(const QString& response);

    class Private;
    QScopedPointer<Private> d;

private:
    bool m_waitingForStackInfo = false;
    DebugManager* q_ptr;
    bool m_firstStop = true;
};


} // namespace SprintIDE

#endif // DEBUG_MANAGER_H
