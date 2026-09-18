#include "chat_panel.h"
#include "ai_server.h"
#include "ai_action_executor.h"
#include "theme_manager.h"

#include <QScrollBar>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QStyle>
#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QAction>
#include <QKeyEvent>

namespace SprintIDE {

// ============================================================
// 构造函数
// ============================================================
ChatPanel::ChatPanel(QWidget *parent)
    : QDockWidget("AI 助手", parent)
    , m_maxHistorySize(50)
{
    setObjectName("ChatPanel");
    setMinimumWidth(380);
    setMaximumWidth(550);

    setupUI();
    setConnectionStatus(false);

    qDebug() << "[ChatPanel] 已创建";
}

// ============================================================
// 析构函数
// ============================================================
ChatPanel::~ChatPanel()
{
    qDebug() << "[ChatPanel] 已销毁";
}

// ============================================================
// 设置 AI 服务器指针
// ============================================================
void ChatPanel::setAIServer(AIServer *server)
{
    m_aiServer = server;

    if (server) {
        connect(server, &AIServer::agentConnected,
                this, [this]() {
                    setConnectionStatus(true);
                    showSystemMessage("AI 助手已连接", false);
                });

        connect(server, &AIServer::agentDisconnected,
                this, [this]() {
                    setConnectionStatus(false);
                    showSystemMessage("AI 助手已断开连接", true);
                });

        connect(server, &AIServer::streamDataReceived,
                this, &ChatPanel::onStreamDataReceived);

        connect(server, &AIServer::responseReceived,
                this, [this](int requestId, const QJsonObject &result) {
                    Q_UNUSED(requestId);
                    QString content = result.value("content").toString();
                    if (!content.isEmpty()) {
                        addAIMessage(content);
                    }
                });

        connect(server, &AIServer::errorResponseReceived,
                this, [this](int requestId, int code, const QString &message) {
                    Q_UNUSED(requestId);
                    showSystemMessage(QString("AI 错误 (%1): %2").arg(code).arg(message), true);
                });

        qDebug() << "[ChatPanel] AI 服务器已设置";
    }
}

// ============================================================
// 设置动作执行器指针
// ============================================================
void ChatPanel::setActionExecutor(AIActionExecutor *executor)
{
    m_actionExecutor = executor;

    if (executor) {
        connect(executor, &AIActionExecutor::outputProduced,
                this, &ChatPanel::onActionOutput);
    }

    qDebug() << "[ChatPanel] 动作执行器已设置";
}

// ============================================================
// 初始化 UI
// ============================================================
void ChatPanel::setupUI()
{
    QWidget *container = new QWidget(this);
    container->setObjectName("ChatPanelContainer");
    setWidget(container);

    QVBoxLayout *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 按顺序创建
    createToolbar();
    createMessageArea();
    QWidget* inputContainer = createInputArea();   // 拿到返回的容器

    // 按顺序加入主布局 —— 只加这三个，不再单独加 m_inputEdit
    mainLayout->addWidget(m_toolbar);
    mainLayout->addWidget(m_messageDisplay, 1);    // stretch = 1，占满中间
    mainLayout->addWidget(inputContainer);         // 底部

    m_inputEdit->setFocus();
}

// ============================================================
// 创建工具栏
// ============================================================
void ChatPanel::createToolbar()
{
    m_toolbar = new QToolBar(this);
    m_toolbar->setObjectName("ChatToolbar");
    m_toolbar->setMovable(false);
    m_toolbar->setFloatable(false);
    m_toolbar->setIconSize(QSize(14, 14));
    m_toolbar->setFixedHeight(32);

    // 左：状态
    m_statusLabel = new QLabel("● 未连接", this);
    m_statusLabel->setObjectName("statusLabel");
    m_statusLabel->setProperty("class", "disconnected");
    m_toolbar->addWidget(m_statusLabel);

    // 中：弹性空白
    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_toolbar->addWidget(spacer);

    // 右：清空 + 复制
    m_clearButton = new QPushButton("清空", this);
    m_clearButton->setObjectName("ChatClearButton");
    m_clearButton->setToolTip("清空所有对话消息");
    m_clearButton->setFixedHeight(24);
    connect(m_clearButton, &QPushButton::clicked, this, &ChatPanel::onClearClicked);
    m_toolbar->addWidget(m_clearButton);

    QPushButton *copyButton = new QPushButton("复制", this);
    copyButton->setObjectName("ChatCopyButton");
    copyButton->setToolTip("复制整个对话内容");
    copyButton->setFixedHeight(24);
    connect(copyButton, &QPushButton::clicked, this, [this]() {
        if (m_messageDisplay) {
            QApplication::clipboard()->setText(m_messageDisplay->toPlainText());
            showSystemMessage("对话已复制到剪贴板", false);
        }
    });
    m_toolbar->addWidget(copyButton);
}

// ============================================================
// 创建消息显示区域
// ============================================================
void ChatPanel::createMessageArea()
{
    m_messageDisplay = new QTextEdit(this);
    m_messageDisplay->setObjectName("messageDisplay");
    m_messageDisplay->setReadOnly(true);
    m_messageDisplay->setAcceptRichText(true);
    m_messageDisplay->setLineWrapMode(QTextEdit::WidgetWidth);
    m_messageDisplay->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_messageDisplay->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_messageDisplay->setContextMenuPolicy(Qt::CustomContextMenu);
    m_messageDisplay->setFrameShape(QFrame::NoFrame);        // 无边框
    m_messageDisplay->document()->setDocumentMargin(12);     // 文本边距 12px

    connect(m_messageDisplay, &QTextEdit::customContextMenuRequested,
            this, [this](const QPoint &pos) {
                QMenu *menu = m_messageDisplay->createStandardContextMenu();
                if (menu) {
                    menu->exec(m_messageDisplay->viewport()->mapToGlobal(pos));
                    delete menu;
                }
            });
}

// ============================================================
// 创建输入区域
// ============================================================
QWidget* ChatPanel::createInputArea()
{
    m_inputEdit = new QTextEdit(this);
    m_inputEdit->setObjectName("inputEdit");
    m_inputEdit->setPlaceholderText("输入消息，按 Enter 发送，Ctrl+Enter 换行...");
    m_inputEdit->setEnabled(false);
    m_inputEdit->setAcceptRichText(false);
    m_inputEdit->setTabChangesFocus(false);
    m_inputEdit->setFrameShape(QFrame::NoFrame);
    m_inputEdit->document()->setDocumentMargin(6);
    m_inputEdit->setMinimumHeight(36);
    m_inputEdit->setMaximumHeight(120);
    m_inputEdit->installEventFilter(this);
    connect(m_inputEdit, &QTextEdit::textChanged,
            this, &ChatPanel::onInputTextChanged);

    m_sendButton = new QPushButton("发送", this);
    m_sendButton->setObjectName("sendButton");
    m_sendButton->setEnabled(false);
    m_sendButton->setToolTip("发送消息 (Enter)");

    // 宽度固定 64，高度不设 —— 交给布局拉伸
    m_sendButton->setFixedWidth(64);
    m_sendButton->setMinimumHeight(36);           // 最小与输入框一致
    m_sendButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);   // ← 高度可拉伸

    connect(m_sendButton, &QPushButton::clicked,
            this, &ChatPanel::onSendClicked);

    QHBoxLayout* inputRow = new QHBoxLayout();
    inputRow->setContentsMargins(12, 8, 12, 12);
    inputRow->setSpacing(8);
    inputRow->addWidget(m_inputEdit, 1);
    inputRow->addWidget(m_sendButton);   // 不加 Qt::AlignBottom —— 让按钮跟输入框等高

    QWidget* inputContainer = new QWidget(this);
    inputContainer->setObjectName("inputFrame");
    inputContainer->setLayout(inputRow);

    return inputContainer;
}

// ============================================================
// 事件过滤器（处理 Enter 键发送）
// ============================================================
bool ChatPanel::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_inputEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (keyEvent->modifiers() & Qt::ControlModifier) {
                // Ctrl+Enter 换行（默认行为）
                return false;
            } else {
                // Enter 发送
                onSendClicked();
                return true; // 事件已处理
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

// ============================================================
// 输入框文本变化（用于动态调整高度）
// ============================================================
void ChatPanel::onInputTextChanged()
{
    if (!m_inputEdit) return;
    // 自动调整高度由样式表的 max-height 控制，不需要额外代码
}

// ============================================================
// 添加用户消息
// ============================================================
void ChatPanel::addUserMessage(const QString &message)
{
    if (message.trimmed().isEmpty()) {
        return;
    }

    addMessage("user", message);
    m_conversationHistory.append("用户: " + message);

    while (m_conversationHistory.size() > m_maxHistorySize) {
        m_conversationHistory.removeFirst();
    }

    emit historyChanged();
}

// ============================================================
// 添加 AI 消息（非流式）
// ============================================================
void ChatPanel::addAIMessage(const QString &message)
{
    if (message.trimmed().isEmpty()) {
        return;
    }

    addMessage("ai", message);
    m_conversationHistory.append("AI: " + message);

    while (m_conversationHistory.size() > m_maxHistorySize) {
        m_conversationHistory.removeFirst();
    }

    emit historyChanged();
}

// ============================================================
// 显示系统消息（带换行）
// ============================================================
void ChatPanel::showSystemMessage(const QString &message, bool isError)
{
    // 系统消息与普通消息格式不同，直接插入 HTML
    QString html = formatSystemMessage(message, isError);
    QTextCursor cursor = m_messageDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertHtml(html);
    // 确保换行
    cursor.insertHtml("<br/>");
    updateScrollPosition();
}

// ============================================================
// 开始流式消息
// ============================================================
bool ChatPanel::startStreamMessage(const QString &streamId)
{
    if (m_streamCursors.contains(streamId)) {
        finishStreamMessage(streamId);
    }

    QTextCursor cursor = addMessage("ai", "", true, streamId);
    m_streamCursors[streamId] = cursor;
    m_streamContents[streamId] = "";

    qDebug() << "[ChatPanel] 流式消息开始:" << streamId;
    return true;
}

// ============================================================
// 追加流式数据块
// ============================================================
bool ChatPanel::appendStreamChunk(const QString &streamId, const QString &chunk)
{
    if (!m_streamCursors.contains(streamId)) {
        qWarning() << "[ChatPanel] 流式消息不存在:" << streamId;
        return false;
    }

    if (chunk.isEmpty()) {
        return true;
    }

    QTextCursor cursor = m_streamCursors[streamId];
    m_streamContents[streamId] += chunk;

    cursor.beginEditBlock();
    QTextCharFormat format;
    format.setForeground(QColor("#E0E0E0"));
    cursor.setCharFormat(format);
    cursor.insertText(chunk);
    cursor.endEditBlock();

    // 更新光标位置（保持在末尾）
    m_streamCursors[streamId] = cursor;
    updateScrollPosition();

    return true;
}

// ============================================================
// 结束流式消息
// ============================================================
bool ChatPanel::finishStreamMessage(const QString &streamId, const QString &fullContent)
{
    if (!m_streamCursors.contains(streamId)) {
        qWarning() << "[ChatPanel] 流式消息不存在:" << streamId;
        return false;
    }

    if (!fullContent.isEmpty()) {
        QString currentContent = m_streamContents[streamId];
        if (currentContent != fullContent) {
            QTextCursor cursor = m_streamCursors[streamId];
            cursor.beginEditBlock();
            cursor.insertText("\n\n--- 完整内容 ---\n");
            cursor.insertText(fullContent);
            cursor.endEditBlock();
            m_streamContents[streamId] = fullContent;
        }
    }

    m_streamCursors.remove(streamId);
    QString content = m_streamContents.value(streamId);
    m_streamContents.remove(streamId);

    if (!content.isEmpty()) {
        m_conversationHistory.append("AI: " + content);
        while (m_conversationHistory.size() > m_maxHistorySize) {
            m_conversationHistory.removeFirst();
        }
        emit historyChanged();
    }

    qDebug() << "[ChatPanel] 流式消息结束:" << streamId;
    return true;
}

// ============================================================
// 清空所有消息
// ============================================================
void ChatPanel::clearMessages()
{
    if (m_messageDisplay) {
        m_messageDisplay->clear();
    }

    m_streamCursors.clear();
    m_streamContents.clear();
    m_conversationHistory.clear();

    emit historyChanged();
    showSystemMessage("对话已清空", false);
}

// ============================================================
// 设置连接状态
// ============================================================
void ChatPanel::setConnectionStatus(bool connected)
{
    m_isConnected = connected;

    if (m_statusLabel) {
        if (connected) {
            m_statusLabel->setText("🟢 已连接");
            m_statusLabel->setProperty("class", "connected");
        } else {
            m_statusLabel->setText("🔴 未连接");
            m_statusLabel->setProperty("class", "disconnected");
        }
        m_statusLabel->style()->unpolish(m_statusLabel);
        m_statusLabel->style()->polish(m_statusLabel);
    }

    if (m_inputEdit) {
        m_inputEdit->setEnabled(connected);
    }
    if (m_sendButton) {
        m_sendButton->setEnabled(connected);
    }

    if (connected) {
        setWindowTitle("AI 助手 (已连接)");
    } else {
        setWindowTitle("AI 助手 (未连接)");
    }

    emit connectionStatusChanged(connected);
}

// ============================================================
// 获取对话历史（JSON 格式）
// ============================================================
QJsonArray ChatPanel::getConversationHistory() const
{
    QJsonArray history;

    for (const QString &entry : m_conversationHistory) {
        QJsonObject msg;
        if (entry.startsWith("用户: ")) {
            msg["role"] = "user";
            msg["content"] = entry.mid(3);
        } else if (entry.startsWith("AI: ")) {
            msg["role"] = "assistant";
            msg["content"] = entry.mid(3);
        } else {
            continue;
        }
        history.append(msg);
    }

    return history;
}

// ============================================================
// 设置是否自动发送上下文
// ============================================================
void ChatPanel::setAutoSendContext(bool enabled)
{
    m_autoSendContext = enabled;
}

// ============================================================
// 私有槽：发送按钮点击
// ============================================================
void ChatPanel::onSendClicked()
{
    if (!m_inputEdit) return;
    QString text = m_inputEdit->toPlainText().trimmed();
    if (text.isEmpty()) {
        return;
    }

    // 清空输入框
    m_inputEdit->clear();

    // 发送消息
    emit userMessageSent(text, m_autoSendContext);

    // 添加用户消息到显示
    addUserMessage(text);
}

// ============================================================
// 私有槽：清空对话
// ============================================================
void ChatPanel::onClearClicked()
{
    clearMessages();
}

// ============================================================
// 私有槽：流式数据接收
// ============================================================
void ChatPanel::onStreamDataReceived(int requestId, const QString &streamId,
                                     const QString &chunk, bool isEnd,
                                     const QString &fullContent)
{
    Q_UNUSED(requestId);

    if (isEnd) {
        finishStreamMessage(streamId, fullContent);
    } else if (!chunk.isEmpty()) {
        if (!m_streamCursors.contains(streamId)) {
            startStreamMessage(streamId);
        }
        appendStreamChunk(streamId, chunk);
    }
}

// ============================================================
// 私有槽：动作执行输出
// ============================================================
void ChatPanel::onActionOutput(const QString &message, bool isError)
{
    QString prefix = isError ? "⚠️ " : "🔧 ";
    showSystemMessage(prefix + message, isError);
}

// ============================================================
// 私有槽：更新滚动位置
// ============================================================
void ChatPanel::updateScrollPosition()
{
    if (!m_messageDisplay) {
        return;
    }

    QScrollBar *scrollBar = m_messageDisplay->verticalScrollBar();
    if (scrollBar) {
        scrollBar->setValue(scrollBar->maximum());
    }
}

// ============================================================
// ==================== 私有辅助方法 ====================
// ============================================================

QTextCursor ChatPanel::addMessage(const QString &sender, const QString &content,
                                  bool isStreaming, const QString &streamId)
{
    if (!m_messageDisplay) {
        return QTextCursor();
    }

    QTextCursor cursor = m_messageDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);

    QString html = formatMessage(sender, content);

    cursor.beginEditBlock();
    cursor.insertHtml(html);
    // 每条消息后加一个换行，防止粘连
    cursor.insertHtml("<br/>");
    cursor.endEditBlock();

    updateScrollPosition();
    return cursor;
}

// ============================================================
// 格式化普通消息（气泡样式）
// ============================================================
QString ChatPanel::formatMessage(const QString &sender, const QString &content) const
{
    ThemeManager& tm = ThemeManager::instance();
    const bool isUser = (sender == "user");
    const bool isAI   = (sender == "ai");

    QString displayName = getSenderDisplayName(sender);
    QString time = QDateTime::currentDateTime().toString("hh:mm");

    QString escapedContent = content.toHtmlEscaped();
    escapedContent.replace("\n", "<br/>");

    QString alignment = isUser ? "right" : "left";
    QString bgColor   = isUser
                          ? tm.colorString(ThemeManager::UIElement::AIChatUserBubble)
                          : tm.colorString(ThemeManager::UIElement::AIChatAIBubble);
    QString textColor = tm.colorString(ThemeManager::UIElement::TextColor);
    QString aiBorder  = tm.colorString(ThemeManager::UIElement::AIChatAIBorder);

    // AI 气泡：左侧 3px 极光青竖条
    QString aiBar = isAI
                        ? QString("border-left: 3px solid %1; padding-left: 10px;").arg(aiBorder)
                        : QString();

    return QString(
               "<div style='text-align: %1; margin-bottom: 10px;'>"
               "  <div style='display: inline-block; max-width: 78%; "
               "              background-color: %2; color: %3; "
               "              border-radius: 8px; padding: 8px 12px; "
               "              text-align: left; word-wrap: break-word; %4'>"
               "    <div style='font-size: 11px; color: %5; margin-bottom: 3px;'>%6 · %7</div>"
               "    <div style='font-size: 13px; line-height: 1.5;'>%8</div>"
               "  </div>"
               "</div>"
               ).arg(alignment)
        .arg(bgColor)
        .arg(textColor)
        .arg(aiBar)
        .arg(tm.colorString(ThemeManager::UIElement::SecondaryTextColor))
        .arg(displayName, time, escapedContent);
}

// ============================================================
// 格式化系统消息（居中，灰色斜体）
// ============================================================
QString ChatPanel::formatSystemMessage(const QString &content, bool isError) const
{
    QString color = isError ? "#F44336" : "#888888";
    QString icon = isError ? "⚠️ " : "ℹ️ ";
    QString escaped = content.toHtmlEscaped();

    return QString(
               "<div style='text-align: center; margin: 6px 0;'>"
               "  <span style='color: %1; font-style: italic; font-size: 12px;'>%2%3</span>"
               "</div>"
               ).arg(color, icon, escaped);
}

// ============================================================
// 获取发送者显示名称
// ============================================================
QString ChatPanel::getSenderDisplayName(const QString &sender) const
{
    if (sender == "user") {
        return m_userName;
    } else if (sender == "ai") {
        return m_aiName;
    } else if (sender == "system") {
        return "系统";
    }
    return sender;
}

// ============================================================
// 获取发送者颜色
// ============================================================
QString ChatPanel::getSenderColor(const QString &sender) const
{
    if (sender == "user") {
        return "#4A90E2";
    } else if (sender == "ai") {
        return "#4CAF50";
    } else if (sender == "system") {
        return "#FF9800";
    }
    return "#888888";
}

QJsonObject ChatPanel::buildContext() const
{
    QJsonObject context;
    if (m_actionExecutor) {
        context = m_actionExecutor->getContext();
    }
    context["history"] = getConversationHistory();
    return context;
}

QJsonArray ChatPanel::historyToJson() const
{
    return getConversationHistory();
}

} // namespace SprintIDE
