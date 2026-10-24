#include "ai_integration.h"

#include "application.h"
#include "mainwindow.h"
#include "text_editor.h"
#include "compile_manager.h"
#include "debug_manager.h"
#include "project_manager.h"
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
#include "ai/executor/system_executor.h"

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
#include <QCryptographicHash>
#include <QFileInfo>

namespace SprintIDE {

using namespace SprintIDE::Protocol;

// ============================================================
// AI 集成初始化函数
// ============================================================
bool initializeAIIntegration(MainWindow *mainWindow,
                             AiConnectionManager *&outConnMgr,
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

    // ---- 4. 创建聊天面板 ----
    ChatPanel *chatPanel = new ChatPanel(mainWindow);
    chatPanel->setObjectName("AIChatPanel");
    chatPanel->setWindowTitle("AI Agent");
    chatPanel->setFeatures(QDockWidget::DockWidgetClosable |
                           QDockWidget::DockWidgetMovable |
                           QDockWidget::DockWidgetFloatable);
    mainWindow->addDockWidget(Qt::RightDockWidgetArea, chatPanel);
    outChatPanel = chatPanel;

    // ---- 5. 组件互连 ----
    chatPanel->setConnectionManager(connMgr);

    // 用户发消息 → handleUserMessage
    QObject::connect(chatPanel, &ChatPanel::userMessageSent,
                     mainWindow,
                     [mainWindow, connMgr, chatPanel](const QString &message, bool includeContext) {
                         handleUserMessage(mainWindow, connMgr, chatPanel, message, includeContext);
                     });

    // ---- 6 创建 OperationBus 并注册 Executor（阶段 1） ----
    OperationBus* operationBus = new OperationBus(connMgr, mainWindow);

    // 协议层日志 → OutputPanel
    QObject::connect(operationBus, &OperationBus::logInfoMessage,
                     mainWindow,
                     [mainWindow](const QString& message) {
                         mainWindow->logInfo(message, LogKind::System,
                                             QStringLiteral("AI"));
                     });
    QObject::connect(operationBus, &OperationBus::logWarnMessage,
                     mainWindow,
                     [mainWindow](const QString& message) {
                         mainWindow->logWarn(message, LogKind::System,
                                             QStringLiteral("AI"));
                     });
    QObject::connect(operationBus, &OperationBus::logErrorMessage,
                     mainWindow,
                     [mainWindow](const QString& message) {
                         mainWindow->logError(message, LogKind::System,
                                              QStringLiteral("AI"));
                     });

    ChatExecutor*    chatExec    = new ChatExecutor(mainWindow);
    MetaExecutor*    metaExec    = new MetaExecutor(operationBus, mainWindow);
    FileExecutor*    fileExec    = new FileExecutor(mainWindow, mainWindow);
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

    // ProjectExecutor 也支持异步，需要连接 deferredResultReady
    QObject::connect(projExec, &ProjectExecutor::deferredResultReady,
                     operationBus, &OperationBus::onDeferredResultReady);

    operationBus->registerExecutor(envExec);

    // SystemExecutor 处理 system.* 域
    SystemExecutor* sysExec = new SystemExecutor(mainWindow);
    operationBus->registerExecutor(sysExec);

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

    // ---- chat.ask 提问：OperationBus ↔ ChatPanel ----
    QObject::connect(operationBus, &OperationBus::askRequired,
                     chatPanel, &ChatPanel::onAskRequired);
    QObject::connect(chatPanel, &ChatPanel::askAnswered,
                     operationBus, &OperationBus::resumeAfterAsk);

    // Agent 消息 → OperationBus 路由
    QObject::connect(connMgr, &AiConnectionManager::agentMessageReceived,
                     operationBus, &OperationBus::handleAgentMessage);

    // ---- chat.confirm 门禁：OperationBus ↔ ChatPanel ----
    QObject::connect(operationBus, &OperationBus::confirmRequired,
                     chatPanel, &ChatPanel::onConfirmRequired);
    QObject::connect(chatPanel, &ChatPanel::confirmAnswered,
                     operationBus, &OperationBus::resumeAfterConfirm);

    qDebug() << "[AI Integration] OperationBus 已创建并注册 7 个 Executor";

    // ---- 7. 自动连接 ----
    if (autoConnect) {
        // 上报项目信息给 Agent
        if (mainWindow) {
            const QString projPath = mainWindow->projectPath();
            if (!projPath.isEmpty()) {
                // projectId 用路径的 SHA1 前 16 位（与 ProjectExecutor 保持一致）
                const QByteArray hash = QCryptographicHash::hash(
                    projPath.toUtf8(), QCryptographicHash::Sha1);
                const QString projectId =
                QString::fromLatin1(hash.toHex().left(16));

                const QString projectName =
                QFileInfo(projPath).fileName();

                connMgr->setProjectInfo(projectId, projectName, projPath);
                qDebug() << "[AI Integration] handshake 将上报项目:"
                << projectName << "(" << projPath << ")";
            }
        }
        connMgr->start(host, port, token, clientId);
        QObject::connect(connMgr, &AiConnectionManager::connected,
            mainWindow, [mainWindow, connMgr]() {
                if (!mainWindow) return;
                const QString projPath = mainWindow->projectPath();
                if (projPath.isEmpty()) return;

                const QByteArray hash = QCryptographicHash::hash(
                    projPath.toUtf8(), QCryptographicHash::Sha1);
                const QString projectId =
                QString::fromLatin1(hash.toHex().left(16));
                const QString projectName = QFileInfo(projPath).fileName();

                connMgr->setProjectInfo(projectId, projectName, projPath);

                QJsonObject payload;
                payload["projectId"]   = projectId;
                payload["projectName"] = projectName;
                payload["projectRoot"] = projPath;

                QJsonObject msg;
                msg["method"]          = "event";
                msg["sessionId"]       = connMgr->sessionId();
                msg["requestId"]       = QString();
                msg["protocolVersion"] = QStringLiteral("1.1");
                msg["eventType"]       = QStringLiteral("projectChanged");
                msg["payload"]         = payload;

                connMgr->sendRaw(msg);
                qDebug() << "[AI Integration] connected → 已推送 projectChanged:"
                << projectName;
            });
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
        chatPanel->showSystemMessage("AI Agent 未连接，请先启动 Agent", true);
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
// 创建 AI 菜单项（添加到 MainWindow 的菜单栏）
// ============================================================
void setupAIMenu(MainWindow *mainWindow, ChatPanel *chatPanel)
{
    if (!mainWindow || !chatPanel) {
        return;
    }

    QMenuBar *menuBar = mainWindow->menuBar();
    if (!menuBar) {
        return;
    }

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

    QAction *showChatAction = new QAction("显示对话面板", mainWindow);
    showChatAction->setCheckable(true);
    showChatAction->setChecked(chatPanel->isVisible());
    QObject::connect(showChatAction, &QAction::toggled, chatPanel, &QDockWidget::setVisible);
    QObject::connect(chatPanel, &QDockWidget::visibilityChanged, showChatAction, &QAction::setChecked);
    aiMenu->addAction(showChatAction);

    aiMenu->addSeparator();

    QAction *clearAction = new QAction("清空对话", mainWindow);
    QObject::connect(clearAction, &QAction::triggered, chatPanel, &ChatPanel::clearMessages);
    aiMenu->addAction(clearAction);

    QAction *autoFixAction = new QAction("自动修复错误", mainWindow);
    autoFixAction->setCheckable(true);
    autoFixAction->setChecked(true);
    aiMenu->addAction(autoFixAction);

    aiMenu->addSeparator();

    QAction *statusAction = new QAction("状态: 未连接", mainWindow);
    statusAction->setEnabled(false);
    aiMenu->addAction(statusAction);

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