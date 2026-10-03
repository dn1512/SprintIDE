#include "ai_integration.h"

#include "application.h"
#include "mainwindow.h"
#include "text_editor.h"
#include "compile_manager.h"
#include "debug_manager.h"
#include "project_manager.h"
#include "ai_server.h"
#include "ai_action_executor.h"
#include "chat_panel.h"
#include "config_manager.h"
#include "ai/ai_connection_manager.h"
#include "ai/operation_bus.h"
#include "ai/executor/chat_executor.h"
#include "ai/executor/meta_executor.h"
#include "ai/executor/file_executor.h"
#include "ai/executor/command_executor.h"
#include "ai/executor/ide_executor.h"
#include "ai/executor/project_executor.h"
#include "ai/executor/env_executor.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMetaObject>
#include <QTimer>
#include <QUuid>

namespace SprintIDE {

using namespace SprintIDE::Protocol;

// ============================================================
// AI 集成初始化函数
// ============================================================
bool initializeAIIntegration(MainWindow *mainWindow,
                             AiConnectionManager *&outConnMgr,
                             AIActionExecutor *&outExecutor,
                             ChatPanel *&outChatPanel)
{
    if (!mainWindow) {
        qCritical() << "[AI Integration] MainWindow 为空，无法初始化";
        return false;
    }

    qDebug() << "[AI Integration] 开始初始化 AI 模块...";

    // ---- 1. 读配置 ----
    ConfigManager* cfg = Application::instance().configManager();
    const QString host = cfg
        ? cfg->value("AI/Host", "127.0.0.1").toString()
        : QStringLiteral("127.0.0.1");
    const quint16 port = cfg
        ? static_cast<quint16>(cfg->value("AI/Port", 9876).toInt())
        : 9876;
    const QString token = cfg
        ? cfg->value("AI/Token", "").toString()
        : QString();
    const bool autoConnect = cfg
        ? cfg->value("AI/AutoConnect", true).toBool()
        : true;

    // ---- 2. 生成持久化 clientId（首次运行） ----
    QString clientId = cfg ? cfg->value("AI/ClientId", "").toString() : QString();
    if (clientId.isEmpty()) {
        clientId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        if (cfg) {
            cfg->setValue("AI/ClientId", clientId);
        }
    }

    // ---- 3. 创建连接管理器 ----
    AiConnectionManager *connMgr = new AiConnectionManager(mainWindow);
    outConnMgr = connMgr;

    // ---- 4. 创建动作执行器（阶段 0 保留） ----
    AIActionExecutor *executor = new AIActionExecutor(mainWindow, mainWindow);
    outExecutor = executor;

    // ---- 5. 创建聊天面板 ----
    ChatPanel *chatPanel = new ChatPanel(mainWindow);
    chatPanel->setObjectName("AIChatPanel");
    chatPanel->setWindowTitle("AI Agent");
    chatPanel->setFeatures(QDockWidget::DockWidgetClosable |
                           QDockWidget::DockWidgetMovable |
                           QDockWidget::DockWidgetFloatable);
    mainWindow->addDockWidget(Qt::RightDockWidgetArea, chatPanel);
    outChatPanel = chatPanel;

    // ---- 6. 组件互连 ----
    chatPanel->setConnectionManager(connMgr);
    chatPanel->setActionExecutor(executor);

    // 用户发消息 → handleUserMessage
    QObject::connect(chatPanel, &ChatPanel::userMessageSent,
                     mainWindow,
                     [mainWindow, connMgr, chatPanel](const QString &message, bool includeContext) {
                         handleUserMessage(mainWindow, connMgr, chatPanel, message, includeContext);
                     });

    // 动作执行器的输出 → ChatPanel 系统消息
    QObject::connect(executor, &AIActionExecutor::outputProduced,
                     chatPanel, &ChatPanel::onActionOutput);

    // ---- 6.5 创建 OperationBus 并注册 Executor（阶段 1） ----
    OperationBus* operationBus = new OperationBus(connMgr, mainWindow);

    ChatExecutor*    chatExec    = new ChatExecutor(mainWindow);
    MetaExecutor*    metaExec    = new MetaExecutor(operationBus, mainWindow);
    FileExecutor*    fileExec    = new FileExecutor(mainWindow);
    CommandExecutor* cmdExec     = new CommandExecutor(mainWindow);
    IdeExecutor*     ideExec     = new IdeExecutor(mainWindow, mainWindow);
    ProjectExecutor* projExec    = new ProjectExecutor(mainWindow);
    EnvExecutor*     envExec     = new EnvExecutor(mainWindow);

    operationBus->registerExecutor(chatExec);
    operationBus->registerExecutor(metaExec);
    operationBus->registerExecutor(fileExec);
    operationBus->registerExecutor(cmdExec);
    operationBus->registerExecutor(ideExec);
    QObject::connect(ideExec, &IdeExecutor::deferredResultReady,
                     operationBus, &OperationBus::onDeferredResultReady);
    operationBus->registerExecutor(projExec);
    operationBus->registerExecutor(envExec);

    // ChatExecutor 的 notify → ChatPanel 系统消息
    QObject::connect(chatExec, &ChatExecutor::notifyRequested,
                     chatPanel,
                     [chatPanel](const QString& level,
                                 const QString& title,
                                 const QString& message) {
                         const bool isError = (level == QStringLiteral("error"));
                         const bool isWarn  = (level == QStringLiteral("warning"));
                         QString text = message;
                         if (!title.isEmpty()) {
                             text = QStringLiteral("[%1] %2").arg(title, message);
                         }
                         chatPanel->showSystemMessage(text, isError || isWarn);
                     });

    // Agent 消息 → OperationBus 路由
    QObject::connect(connMgr, &AiConnectionManager::agentMessageReceived,
                     operationBus, &OperationBus::handleAgentMessage);

    qDebug() << "[AI Integration] OperationBus 已创建并注册 7 个 Executor";

    // ---- 7. 自动连接 ----
    if (autoConnect) {
        connMgr->start(host, port, token, clientId);
    } else {
        qDebug() << "[AI Integration] AutoConnect=false，跳过连接";
    }

    // ---- 8. 启动提示 ----
    chatPanel->showSystemMessage("AI 模块已初始化，等待 Agent 连接...", false);

    qDebug() << "[AI Integration] AI 模块初始化完成";
    return true;
}

// ============================================================
// 处理用户消息
// ============================================================
void handleUserMessage(MainWindow *mainWindow,
                       AiConnectionManager *connMgr,
                       ChatPanel *chatPanel,
                       const QString &message,
                       bool includeContext)
{
    if (!connMgr || !connMgr->isReady()) {
        chatPanel->showSystemMessage("AI Agent 手未连接，请先启动 Agent", true);
        return;
    }

    QJsonObject payload;
    payload["message"] = message;

    if (includeContext && mainWindow) {
        QJsonObject context;
        if (TextEditor* editor = mainWindow->currentEditor()) {
            context["activeFile"]   = editor->filePath();
            context["cursorLine"]   = editor->currentLine();
            context["cursorColumn"] = editor->currentColumn();
            if (editor->textCursor().hasSelection()) {
                context["selection"] = editor->textCursor().selectedText();
            }
        }
        payload["context"] = context;
    }

    const QString requestId =
        QUuid::createUuid().toString(QUuid::WithoutBraces);

    connMgr->sendMethod("chat.send", payload, requestId);
    qDebug() << "[AI Integration] chat.send 已发送，requestId:" << requestId;
}

// ============================================================
// 处理 IDE 请求（来自 Agent）
// ============================================================
void handleIdeRequest(MainWindow *mainWindow, AIServer *server,
                      AIActionExecutor *executor, int requestId,
                      const QString &method, const QJsonObject &params)
{
    qDebug() << "[AI Integration] 收到 IDE 请求:" << method << "ID:" << requestId;

    // 如果方法以 "ide." 开头，去掉前缀
    QString action = method;
    if (action.startsWith("ide.")) {
        action = action.mid(4);
    }

    // 构建操作 JSON
    QJsonObject actionObj;
    actionObj["type"] = action;
    actionObj["params"] = params;

    // 执行操作
    AIActionResult result = executor->executeAction(actionObj);

    // 发送响应
    if (result.success) {
        QJsonObject response;
        response["success"] = true;
        response["message"] = result.message;
        if (!result.data.isEmpty()) {
            response["data"] = result.data;
        }
        server->sendResultResponse(requestId, response);
    } else {
        server->sendErrorResponse(requestId, -32000, result.message);
    }

    // 显示操作日志
    if (mainWindow) {
        // 在输出面板显示（如果有）
        // 也可以通过聊天面板显示
    }
}

// ============================================================
// 连接错误自动上报
// ============================================================
void connectAutoErrorReporting(MainWindow *mainWindow, ChatPanel *chatPanel)
{
    if (!mainWindow || !chatPanel) {
        return;
    }

    // ---- 1. 编译错误自动上报 ----
    CompileManager *compileMgr = &CompileManager::instance();
    if (compileMgr) {
        QObject::connect(compileMgr, &CompileManager::compileFinished,
                         mainWindow, [mainWindow, chatPanel](const QString &taskId, const CompileResult &result) {
                             // 只在编译失败且用户开启了自动修复时上报
                             if (!result.success && !result.errors.isEmpty()) {
                                 // 检查是否启用自动修复（通过配置，默认为 true）
                                 // 这里我们简化：始终上报
                                 QTimer::singleShot(500, [mainWindow, chatPanel, result]() {
                                     reportCompileErrors(mainWindow, chatPanel, result);
                                 });
                             }
                         });
        qDebug() << "[AI Integration] 编译错误自动上报已连接";
    }

    // ---- 2. 调试错误自动上报 ----
    DebugManager *debugMgr = &DebugManager::instance();
    if (debugMgr) {
        QObject::connect(debugMgr, &DebugManager::programOutput,
                         mainWindow, [mainWindow, chatPanel](const QString &output, bool isError) {
                             // 只处理错误输出
                             if (isError && !output.isEmpty()) {
                                 // 检查是否包含崩溃或错误信息
                                 if (output.contains("error") || output.contains("crash") ||
                                     output.contains("Segmentation fault") || output.contains("异常")) {
                                     // 延迟一点，确保程序完全停止
                                     QTimer::singleShot(1000, [mainWindow, chatPanel, output]() {
                                         reportRuntimeError(mainWindow, chatPanel, output);
                                     });
                                 }
                             }
                         });
        qDebug() << "[AI Integration] 调试错误自动上报已连接";
    }

    // ---- 3. LSP 诊断错误自动上报（可选） ----
    // 这个需要 LSPClient 的支持，暂时跳过
}

// ============================================================
// 上报编译错误
// ============================================================
void reportCompileErrors(MainWindow *mainWindow, ChatPanel *chatPanel,
                         const CompileResult &result)
{
    if (!mainWindow || !chatPanel) {
        return;
    }

    // 检查是否已经上报过相同的错误（防止重复上报）
    static QString lastErrorHash;
    QString currentHash = QString::number(qHash(result.errors.join("")));
    if (currentHash == lastErrorHash) {
        // 相同错误，不上报
        return;
    }
    lastErrorHash = currentHash;

    // 构建错误报告
    QString errorSummary;
    if (!result.errors.isEmpty()) {
        errorSummary = result.errors.first();
    }

    // 构建上下文
    QJsonObject context;

    // 获取当前文件内容
    TextEditor *editor = mainWindow->currentEditor();
    if (editor) {
        context["fileContent"] = editor->toPlainText();
        context["filePath"] = editor->filePath();
    }

    // 添加错误列表
    QJsonArray errorArray;
    for (const QString &err : result.errors) {
        errorArray.append(err);
    }
    context["errors"] = errorArray;

    // 添加警告列表
    QJsonArray warningArray;
    for (const QString &warn : result.warnings) {
        warningArray.append(warn);
    }
    context["warnings"] = warningArray;

    // 构建修复请求
    QJsonObject params;
    params["type"] = "compile_error";
    params["summary"] = errorSummary;
    params["context"] = context;

    // 发送自动修复请求
    if (chatPanel && chatPanel->isVisible()) {
        chatPanel->showSystemMessage("🔧 检测到编译错误，正在请求 AI 修复...", false);

        // 获取 AI 服务器
        AIServer *server = chatPanel->findChild<AIServer*>();
        if (server && server->isAgentConnected()) {
            static int fixRequestId = 2000;
            int requestId = ++fixRequestId;

            // 发送 agent.autoFix 请求
            QJsonObject request;
            request["method"] = "agent.autoFix";
            request["params"] = params;
            server->sendRequest("agent.autoFix", params, requestId);

            qDebug() << "[AI Integration] 自动修复请求已发送，ID:" << requestId;
        } else {
            chatPanel->showSystemMessage("❌ AI agent 未连接，无法自动修复", true);
        }
    }
}

// ============================================================
// 上报运行时错误
// ============================================================
void reportRuntimeError(MainWindow *mainWindow, ChatPanel *chatPanel,
                        const QString &errorOutput)
{
    if (!mainWindow || !chatPanel) {
        return;
    }

    // 检查是否已经上报过相同的错误（简化去重）
    static QString lastRuntimeErrorHash;
    QString currentHash = QString::number(qHash(errorOutput));
    if (currentHash == lastRuntimeErrorHash) {
        return;
    }
    lastRuntimeErrorHash = currentHash;

    // 构建错误报告
    QString errorSummary = errorOutput.left(200);
    if (errorOutput.length() > 200) {
        errorSummary += "...";
    }

    // 构建上下文
    QJsonObject context;

    // 获取当前文件内容
    TextEditor *editor = mainWindow->currentEditor();
    if (editor) {
        context["fileContent"] = editor->toPlainText();
        context["filePath"] = editor->filePath();
    }

    // 添加错误输出
    context["errorOutput"] = errorOutput;

    // 获取调用栈（如果有）
    DebugManager *debugMgr = &DebugManager::instance();
    if (debugMgr && debugMgr->isDebugging()) {
        QList<StackFrame> stack = debugMgr->getCallStack();
        QJsonArray stackArray;
        for (const StackFrame &frame : stack) {
            QJsonObject f;
            f["function"] = frame.functionName;
            f["file"] = frame.filePath;
            f["line"] = frame.lineNumber;
            stackArray.append(f);
        }
        context["callStack"] = stackArray;
    }

    // 构建请求
    QJsonObject params;
    params["type"] = "runtime_error";
    params["summary"] = errorSummary;
    params["context"] = context;

    // 发送自动修复请求
    if (chatPanel && chatPanel->isVisible()) {
        chatPanel->showSystemMessage("🐛 检测到运行时错误，正在请求 AI 分析...", false);

        AIServer *server = chatPanel->findChild<AIServer*>();
        if (server && server->isAgentConnected()) {
            static int fixRequestId = 3000;
            int requestId = ++fixRequestId;

            server->sendRequest("agent.autoFix", params, requestId);
            qDebug() << "[AI Integration] 运行时错误修复请求已发送，ID:" << requestId;
        } else {
            chatPanel->showSystemMessage("❌ AI agent 未连接，无法分析错误", true);
        }
    }
}

// ============================================================
// 创建 AI 菜单项（添加到 MainWindow 的菜单栏）
// ============================================================
void setupAIMenu(MainWindow *mainWindow, ChatPanel *chatPanel)
{
    if (!mainWindow || !chatPanel) {
        return;
    }

    // 获取菜单栏
    QMenuBar *menuBar = mainWindow->menuBar();
    if (!menuBar) {
        return;
    }

    // 查找或创建 "AI" 菜单
    QMenu *aiMenu = nullptr;
    for (QAction *action : menuBar->actions()) {
        if (action->text() == "AI(&A)") {
            aiMenu = action->menu();
            break;
        }
    }

    if (!aiMenu) {
        aiMenu = menuBar->addMenu("AI(&A)");
    }

    // 添加 "显示对话面板" 菜单项
    QAction *showChatAction = new QAction("显示对话面板", mainWindow);
    showChatAction->setCheckable(true);
    showChatAction->setChecked(chatPanel->isVisible());
    QObject::connect(showChatAction, &QAction::toggled, chatPanel, &QDockWidget::setVisible);
    QObject::connect(chatPanel, &QDockWidget::visibilityChanged, showChatAction, &QAction::setChecked);
    aiMenu->addAction(showChatAction);

    aiMenu->addSeparator();

    // 添加 "清空对话" 菜单项
    QAction *clearAction = new QAction("清空对话", mainWindow);
    QObject::connect(clearAction, &QAction::triggered, chatPanel, &ChatPanel::clearMessages);
    aiMenu->addAction(clearAction);

    // 添加 "自动修复" 切换菜单项
    QAction *autoFixAction = new QAction("自动修复错误", mainWindow);
    autoFixAction->setCheckable(true);
    autoFixAction->setChecked(true);
    // 保存到 MainWindow 的某个地方，这里简化
    aiMenu->addAction(autoFixAction);

    aiMenu->addSeparator();

    // 添加 "连接状态" 显示（只读）
    QAction *statusAction = new QAction("状态: 未连接", mainWindow);
    statusAction->setEnabled(false);
    aiMenu->addAction(statusAction);

    // 更新状态显示
    QObject::connect(chatPanel, &ChatPanel::connectionStatusChanged,
                     statusAction, [statusAction](bool connected) {
                         if (connected) {
                             statusAction->setText("状态: 已连接");
                         } else {
                             statusAction->setText("状态: 未连接");
                         }
                     });

    qDebug() << "[AI Integration] AI 菜单已添加";
}

// ============================================================
// 保存和恢复聊天面板状态
// ============================================================
void saveChatPanelState(ChatPanel *chatPanel)
{
    if (!chatPanel) return;

    ConfigManager *config = Application::instance().configManager();
    if (!config) return;

    config->beginGroup("AIChatPanel");
    config->setValue("visible", chatPanel->isVisible());
    config->setValue("geometry", chatPanel->saveGeometry());
    config->endGroup();
}

void restoreChatPanelState(ChatPanel *chatPanel)
{
    if (!chatPanel) return;

    ConfigManager *config = Application::instance().configManager();
    if (!config) return;

    config->beginGroup("AIChatPanel");
    bool visible = config->value("visible", true).toBool();
    chatPanel->setVisible(visible);
    if (config->contains("geometry")) {
        chatPanel->restoreGeometry(config->value("geometry").toByteArray());
    }
    config->endGroup();
}

} // namespace SprintIDE
