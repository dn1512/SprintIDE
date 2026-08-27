#ifndef AI_ACTION_EXECUTOR_H
#define AI_ACTION_EXECUTOR_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QMap>
#include <QList>

#include "debug_manager.h"
#include "compile_manager.h"

#include "compile_manager.h"   // 包含 CompileResult 定义
#include "debug_manager.h"     // 包含 DebugState 定义


namespace SprintIDE {

// 前向声明（避免循环依赖）
class MainWindow;
class TextEditor;
class CompileManager;
class DebugManager;

/**
 * @brief AI 动作执行结果
 *
 * 用于返回单个操作或批量操作的执行结果
 */
struct AIActionResult {
    bool success;           ///< 是否执行成功
    QString actionType;     ///< 操作类型（如 "openFile", "replaceText"）
    QString message;        ///< 执行结果描述（成功或错误信息）
    QJsonObject data;       ///< 附加数据（如执行后的文件内容、编译结果等）

    AIActionResult() : success(false) {}
    AIActionResult(bool ok, const QString &msg, const QJsonObject &extra = QJsonObject())
        : success(ok), message(msg), data(extra) {}
};

/**
 * @brief AI 动作执行器
 *
 * 负责执行 AI Agent 下发的各种 IDE 操作指令。
 * 所有操作都在主线程中执行（通过信号槽或 QMetaObject::invokeMethod），
 * 确保与 Qt GUI 的线程安全。
 *
 * 支持的操作类型：
 * - openFile: 打开文件
 * - closeFile: 关闭文件
 * - saveFile: 保存文件
 * - saveAllFiles: 保存所有文件
 * - replaceText: 替换文本
 * - insertText: 插入文本
 * - deleteText: 删除文本
 * - formatFile: 格式化文件
 * - compile: 编译当前文件
 * - compileProject: 编译项目
 * - run: 运行程序
 * - startDebug: 开始调试
 * - stopDebug: 停止调试
 * - stepOver: 步过
 * - stepInto: 步入
 * - stepOut: 步出
 * - continueDebug: 继续调试
 * - addBreakpoint: 添加断点
 * - removeBreakpoint: 移除断点
 * - executeCommand: 执行终端命令
 * - getContext: 获取上下文信息
 * - getFileContent: 获取文件内容
 * - createFile: 创建文件
 * - deleteFile: 删除文件
 */
class AIActionExecutor : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param mainWindow 主窗口指针（用于访问所有管理器）
     * @param parent 父对象
     */
    explicit AIActionExecutor(MainWindow *mainWindow, QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~AIActionExecutor();

    /**
     * @brief 执行单个操作
     * @param action 操作 JSON 对象
     * @return 执行结果
     *
     * action 格式示例：
     * {
     *   "type": "replaceText",
     *   "filePath": "/project/src/main.cpp",
     *   "startLine": 10,
     *   "startCol": 5,
     *   "endLine": 10,
     *   "endCol": 20,
     *   "newText": "int result = a + b;"
     * }
     */
    AIActionResult executeAction(const QJsonObject &action);

    /**
     * @brief 批量执行多个操作
     * @param actions 操作 JSON 数组
     * @return 执行结果列表
     *
     * 按顺序执行，如果某个操作失败，默认停止后续执行
     * （可通过 stopOnError 参数控制）
     */
    QList<AIActionResult> executeActions(const QJsonArray &actions,
                                         bool stopOnError = true);

    /**
     * @brief 执行操作并返回结果（异步，通过信号返回）
     * @param action 操作 JSON 对象
     *
     * 适用于耗时操作（如编译），结果通过 actionFinished 信号返回
     */
    void executeActionAsync(const QJsonObject &action);

    /**
     * @brief 获取当前 IDE 上下文（用于发送给 Agent）
     * @return 上下文 JSON 对象
     *
     * 包含当前打开的文件、项目信息、编译状态、调试状态等
     * 格式与 ai_context.h 中的 AIContext 对应
     */
    QJsonObject getContext() const;

    /**
     * @brief 获取指定文件的内容
     * @param filePath 文件路径
     * @return 文件内容（如果文件未打开或不存在，返回空字符串）
     */
    QString getFileContent(const QString &filePath) const;

    /**
     * @brief 检查文件是否已打开
     */
    bool isFileOpen(const QString &filePath) const;

    /**
     * @brief 获取当前活动编辑器的文件路径
     */
    QString getActiveFilePath() const;

signals:
    /**
     * @brief 操作执行完成信号（异步操作使用）
     * @param actionId 操作 ID（由调用方生成）
     * @param result 执行结果
     */
    void actionFinished(const QString &actionId, const AIActionResult &result);

    /**
     * @brief 批量操作执行完成信号
     * @param results 所有操作的结果列表
     */
    void actionsFinished(const QList<AIActionResult> &results);

    /**
     * @brief 操作执行过程中产生输出（用于日志或 UI 显示）
     * @param message 输出消息
     * @param isError 是否为错误信息
     */
    void outputProduced(const QString &message, bool isError);

private slots:
    // 编译/调试完成信号的处理（用于异步操作）
    void onCompileFinished(const QString &taskId, const CompileResult &result);
    void onDebugStateChanged(DebugState state);

private:
    // ========== 具体操作执行方法 ==========

    // 文件操作
    AIActionResult executeOpenFile(const QJsonObject &params);
    AIActionResult executeCloseFile(const QJsonObject &params);
    AIActionResult executeSaveFile(const QJsonObject &params);
    AIActionResult executeSaveAllFiles(const QJsonObject &params);
    AIActionResult executeCreateFile(const QJsonObject &params);
    AIActionResult executeDeleteFile(const QJsonObject &params);
    AIActionResult executeGetFileContent(const QJsonObject &params);

    // 文本编辑操作
    AIActionResult executeReplaceText(const QJsonObject &params);
    AIActionResult executeInsertText(const QJsonObject &params);
    AIActionResult executeDeleteText(const QJsonObject &params);
    AIActionResult executeFormatFile(const QJsonObject &params);

    // 编译/运行操作
    AIActionResult executeCompile(const QJsonObject &params);
    AIActionResult executeCompileProject(const QJsonObject &params);
    AIActionResult executeRun(const QJsonObject &params);

    // 调试操作
    AIActionResult executeStartDebug(const QJsonObject &params);
    AIActionResult executeStopDebug(const QJsonObject &params);
    AIActionResult executeStepOver(const QJsonObject &params);
    AIActionResult executeStepInto(const QJsonObject &params);
    AIActionResult executeStepOut(const QJsonObject &params);
    AIActionResult executeContinueDebug(const QJsonObject &params);
    AIActionResult executeAddBreakpoint(const QJsonObject &params);
    AIActionResult executeRemoveBreakpoint(const QJsonObject &params);

    // 终端操作
    AIActionResult executeExecuteCommand(const QJsonObject &params);

    // 上下文操作
    AIActionResult executeGetContext(const QJsonObject &params);

    // ========== 辅助方法 ==========

    /**
     * @brief 查找指定文件的编辑器
     */
    TextEditor* findEditorForFile(const QString &filePath) const;

    /**
     * @brief 获取当前活动的编辑器
     */
    TextEditor* getCurrentEditor() const;

    /**
     * @brief 确保文件已打开，如果未打开则打开它
     */
    bool ensureFileOpen(const QString &filePath, QString *errorMessage = nullptr);

    /**
     * @brief 验证参数中是否包含必需的字段
     */
    bool validateParams(const QJsonObject &params, const QStringList &requiredKeys,
                        QString &errorMessage) const;

    /**
     * @brief 将文件路径转换为绝对路径（基于项目根目录）
     */
    QString resolvePath(const QString &path) const;

    // ========== 成员变量 ==========

    MainWindow *m_mainWindow = nullptr;          ///< 主窗口指针（弱引用，防止悬空）
    CompileManager *m_compileManager = nullptr; ///< 编译管理器指针（缓存，提高性能）
    DebugManager *m_debugManager = nullptr;     ///< 调试管理器指针

    // 异步操作状态
    QMap<QString, QJsonObject> m_pendingActions;  ///< 等待完成的异步操作
    int m_nextActionId = 1;                       ///< 下一个操作 ID

    // 项目路径缓存
    QString m_currentProjectPath;
};

} // namespace SprintIDE

#endif // AI_ACTION_EXECUTOR_H
