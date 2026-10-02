#ifndef AI_INTEGRATION_H
#define AI_INTEGRATION_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QSettings>
#include <QMenu>

#include "ai/ai_action_executor.h"
#include "ai/chat_panel.h"


namespace SprintIDE {
// 前向声明（避免循环依赖）
class MainWindow;

// 前向声明 AI 模块的类
class AIServer;
class AIActionExecutor;
class ChatPanel;
class AiConnectionManager;

struct CompileResult;

/**
 * ============================================================
 * AI 模块集成接口
 * ============================================================
 *
 * 这个头文件提供了将 AI 模块集成到 MainWindow 的所有函数。
 * 典型的集成流程：
 *
 * 1. 在 MainWindow 构造函数中调用 initializeAIIntegration()
 * 2. 在 MainWindow 构造函数中调用 setupAIMenu()
 * 3. 在 MainWindow 的 closeEvent 中调用 saveChatPanelState()
 * 4. 在 MainWindow 构造函数或初始化函数中调用 restoreChatPanelState()
 *
 * 所有函数都是线程安全的，因为 Qt 的 GUI 操作都在主线程执行。
 */

/**
 * @brief 初始化 AI 模块并集成到主窗口
 *
 * 此函数会：
 * 1. 创建 AI 服务器（监听 127.0.0.1:9876）
 * 2. 创建动作执行器
 * 3. 创建聊天面板并停靠在右侧
 * 4. 连接各组件之间的信号槽
 * 5. 连接编译/调试错误自动上报
 *
 * @param mainWindow 主窗口指针（不能为空）
 * @param outServer 输出参数，返回 AI 服务器指针
 * @param outExecutor 输出参数，返回动作执行器指针
 * @param outChatPanel 输出参数，返回聊天面板指针
 * @return 初始化成功返回 true，失败返回 false
 *
 * 使用示例：
 * @code
 * AIServer *server = nullptr;
 * AIActionExecutor *executor = nullptr;
 * ChatPanel *chatPanel = nullptr;
 * if (initializeAIIntegration(this, server, executor, chatPanel)) {
 *     // AI 模块已就绪
 * }
 * @endcode
 */
bool initializeAIIntegration(MainWindow *mainWindow,
                             AiConnectionManager *&outConnMgr,
                             AIActionExecutor *&outExecutor,
                             ChatPanel *&outChatPanel);

/**
 * @brief 处理用户发送的消息
 *
 * 当用户在聊天面板中输入消息并点击发送时，
 * 此函数会被调用来处理消息并转发给 AI Agent。
 *
 * @param mainWindow 主窗口指针（用于获取上下文）
 * @param server AI 服务器指针（用于发送请求）
 * @param chatPanel 聊天面板指针（用于获取历史记录）
 * @param message 用户输入的消息内容
 * @param includeContext 是否包含当前 IDE 上下文（文件内容、项目信息等）
 *
 * 内部流程：
 * 1. 构建包含对话历史和上下文的 JSON 请求
 * 2. 通过 AIServer 发送 "agent.chat" 请求
 * 3. Agent 的流式响应会通过信号更新聊天面板
 *
 * @note 此函数通常在 ChatPanel::userMessageSent 信号中调用
 */
void handleUserMessage(MainWindow *mainWindow,
                       AiConnectionManager *connMgr,
                       ChatPanel *chatPanel,
                       const QString &message,
                       bool includeContext);

/**
 * @brief 处理来自 AI Agent 的 IDE 操作请求
 *
 * 当 Agent 调用 ide.* 方法时（如 ide.openFile、ide.replaceText），
 * 此函数会被调用来执行对应的 IDE 操作。
 *
 * @param mainWindow 主窗口指针（用于执行操作）
 * @param server AI 服务器指针（用于发送响应）
 * @param executor 动作执行器指针（用于执行操作）
 * @param requestId 请求 ID（用于匹配响应）
 * @param method 方法名（如 "ide.openFile"）
 * @param params 参数对象
 *
 * 内部流程：
 * 1. 解析 method，提取操作类型
 * 2. 调用 AIActionExecutor::executeAction() 执行操作
 * 3. 根据执行结果发送成功或错误响应给 Agent
 *
 * @note 此函数通常在 AIServer::ideRequestReceived 信号中调用
 */
void handleIdeRequest(MainWindow *mainWindow,
                      AIServer *server,
                      AIActionExecutor *executor,
                      int requestId,
                      const QString &method,
                      const QJsonObject &params);

/**
 * @brief 连接编译/调试错误的自动上报
 *
 * 此函数会连接以下信号到自动上报函数：
 * 1. CompileManager::compileFinished → 编译错误上报
 * 2. DebugManager::programOutput → 运行时错误上报
 *
 * @param mainWindow 主窗口指针
 * @param chatPanel 聊天面板指针（用于显示状态消息和发送修复请求）
 *
 * 上报流程：
 * 1. 检测到错误后，构建包含上下文信息的 JSON 请求
 * 2. 发送 "agent.autoFix" 请求给 Agent
 * 3. Agent 返回修复建议，通过动作执行器应用修复
 *
 * @note 此函数在 initializeAIIntegration 中自动调用
 */
void connectAutoErrorReporting(MainWindow *mainWindow,
                               ChatPanel *chatPanel);

/**
 * @brief 上报编译错误并请求自动修复
 *
 * 当编译失败时，此函数会自动调用，
 * 将错误信息发送给 AI Agent 请求修复方案。
 *
 * @param mainWindow 主窗口指针
 * @param chatPanel 聊天面板指针
 * @param result 编译结果（包含错误和警告信息）
 *
 * 上报内容：
 * - 错误列表（文件、行号、消息）
 * - 当前文件内容
 * - 项目信息
 *
 * @note 此函数由 connectAutoErrorReporting 中的信号触发
 */
void reportCompileErrors(MainWindow *mainWindow,
                         ChatPanel *chatPanel,
                         const CompileResult &result);

/**
 * @brief 上报运行时错误并请求分析
 *
 * 当程序运行时发生错误（崩溃、异常等），
 * 此函数会自动调用，将错误信息发送给 AI Agent。
 *
 * @param mainWindow 主窗口指针
 * @param chatPanel 聊天面板指针
 * @param errorOutput 错误输出（stderr 内容）
 *
 * 上报内容：
 * - 错误输出文本
 * - 当前文件内容
 * - 调用栈（如果正在调试）
 *
 * @note 此函数由 connectAutoErrorReporting 中的信号触发
 */
void reportRuntimeError(MainWindow *mainWindow,
                        ChatPanel *chatPanel,
                        const QString &errorOutput);

/**
 * @brief 在菜单栏中添加 AI 相关菜单项
 *
 * 创建 "AI" 菜单，包含以下子菜单：
 * - 显示对话面板（切换显示/隐藏）
 * - 清空对话
 * - 自动修复错误（开启/关闭）
 * - 连接状态（只读显示）
 *
 * @param mainWindow 主窗口指针
 * @param chatPanel 聊天面板指针
 *
 * @note 通常在 MainWindow 构造函数中调用
 */
void setupAIMenu(MainWindow *mainWindow,
                 ChatPanel *chatPanel);

/**
 * @brief 保存聊天面板的状态到设置文件
 *
 * 保存的内容包括：
 * - 面板是否可见
 * - 面板的几何位置
 *
 * @param chatPanel 聊天面板指针
 *
 * @note 通常在 MainWindow 的 closeEvent 中调用
 */
void saveChatPanelState(ChatPanel *chatPanel);

/**
 * @brief 从设置文件恢复聊天面板的状态
 *
 * 恢复的内容包括：
 * - 面板是否可见
 * - 面板的几何位置
 *
 * @param chatPanel 聊天面板指针
 *
 * @note 通常在 MainWindow 构造函数或初始化函数中调用
 */
void restoreChatPanelState(ChatPanel *chatPanel);

} // namespace SprintIDE

#endif // AI_INTEGRATION_H
