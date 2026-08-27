#include "chat_panel.h"
#include "ai_server.h"
#include "ai_action_executor.h"

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
    setWidget(container);

    QVBoxLayout *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    createToolbar();
    createMessageArea();
    createInputArea();

    mainLayout->addWidget(m_toolbar);
    mainLayout->addWidget(m_messageDisplay, 1);
    mainLayout->addWidget(m_inputEdit);

    // 输入框获得焦点
    m_inputEdit->setFocus();

    // 应用样式表（深色主题）
    setStyleSheet(
        "QTextEdit#messageDisplay {"
        "  background-color: #1E1E1E;"
        "  color: #E0E0E0;"
        "  border: none;"
        "  font-size: 13px;"
        "  padding: 8px;"
        "}"
        "QTextEdit#inputEdit {"
        "  background-color: #2D2D2D;"
        "  color: #E0E0E0;"
        "  border: 1px solid #555555;"
        "  border-radius: 4px;"
        "  padding: 6px 10px;"
        "  font-size: 13px;"
        "  margin: 4px 8px;"
        "  max-height: 120px;"   /* 限制高度为6行左右 */
        "}"
        "QTextEdit#inputEdit:focus {"
        "  border: 1px solid #4A90E2;"
        "}"
        "QPushButton {"
        "  background-color: #3C3C3C;"
        "  color: #E0E0E0;"
        "  border: 1px solid #555555;"
        "  border-radius: 3px;"
        "  padding: 4px 12px;"
        "  margin: 4px 2px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #4A4A4A;"
        "}"
        "QPushButton:disabled {"
        "  color: #666666;"
        "}"
        "QPushButton#sendButton {"
        "  background-color: #4A90E2;"
        "  border-color: #4A90E2;"
        "  font-weight: bold;"
        "}"
        "QPushButton#sendButton:hover {"
        "  background-color: #5A9EF2;"
        "}"
        "QPushButton#sendButton:disabled {"
        "  background-color: #555555;"
        "  border-color: #555555;"
        "}"
        "QLabel#statusLabel {"
        "  color: #888888;"
        "  font-size: 11px;"
        "  padding: 4px 8px;"
        "}"
        "QLabel#statusLabel.connected {"
        "  color: #4CAF50;"
        "}"
        "QLabel#statusLabel.disconnected {"
        "  color: #F44336;"
        "}"
        );
}

// ============================================================
// 创建工具栏
// ============================================================
void ChatPanel::createToolbar()
{
    m_toolbar = new QToolBar(this);
    m_toolbar->setMovable(false);
    m_toolbar->setFloatable(false);
    m_toolbar->setIconSize(QSize(16, 16));
    m_toolbar->setStyleSheet("QToolBar { background-color: #2D2D2D; border: none; padding: 2px; }");

    m_statusLabel = new QLabel("⚪ 未连接", this);
    m_statusLabel->setObjectName("statusLabel");
    m_statusLabel->setProperty("class", "disconnected");
    m_toolbar->addWidget(m_statusLabel);

    m_toolbar->addSeparator();

    m_clearButton = new QPushButton("清空对话", this);
    m_clearButton->setToolTip("清空所有对话消息");
    connect(m_clearButton, &QPushButton::clicked, this, &ChatPanel::onClearClicked);
    m_toolbar->addWidget(m_clearButton);

    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_toolbar->addWidget(spacer);

    QPushButton *copyButton = new QPushButton("复制", this);
    copyButton->setToolTip("复制整个对话内容");
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
void ChatPanel::createInputArea()
{
    // 使用 QTextEdit 作为输入框（支持多行）
    m_inputEdit = new QTextEdit(this);
    m_inputEdit->setObjectName("inputEdit");
    m_inputEdit->setPlaceholderText("输入消息，按 Enter 发送，Ctrl+Enter 换行...");
    m_inputEdit->setEnabled(false);
    m_inputEdit->setAcceptRichText(false);
    m_inputEdit->setTabChangesFocus(false);

    // 连接文本变化信号以动态调整高度（但 Qt 的 sizeHint 已自动调整）
    connect(m_inputEdit, &QTextEdit::textChanged, this, &ChatPanel::onInputTextChanged);

    // 捕获按键事件（在事件过滤器中处理更简单，但这里用 keyPressEvent 重写？）
    // 由于 ChatPanel 不是输入框父类，我们用 installEventFilter，但更简单：连接信号
    // 但 QTextEdit 没有 returnPressed 信号，我们需要用事件过滤器或重写 keyPressEvent。
    // 方法：安装事件过滤器到 m_inputEdit
    m_inputEdit->installEventFilter(this);

    // 发送按钮
    m_sendButton = new QPushButton("发送", this);
    m_sendButton->setObjectName("sendButton");
    m_sendButton->setEnabled(false);
    m_sendButton->setToolTip("发送消息 (Enter)");
    m_sendButton->setFixedHeight(30);

    connect(m_sendButton, &QPushButton::clicked, this, &ChatPanel::onSendClicked);

    // 布局
    QHBoxLayout *containerLayout = new QHBoxLayout();
    containerLayout->setContentsMargins(8, 4, 8, 8);
    containerLayout->setSpacing(8);
    containerLayout->addWidget(m_inputEdit, 1);
    containerLayout->addWidget(m_sendButton);

    QWidget *inputContainer = new QWidget(this);
    inputContainer->setLayout(containerLayout);

    QWidget *container = widget();
    if (container) {
        QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(container->layout());
        if (mainLayout) {
            mainLayout->addWidget(inputContainer);
        }
    }
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
    QString displayName = getSenderDisplayName(sender);
    QString color = getSenderColor(sender);
    QString time = QDateTime::currentDateTime().toString("hh:mm");

    // 转义 HTML
    QString escapedContent = content.toHtmlEscaped();
    escapedContent.replace("\n", "<br/>");

    // 对齐方式：用户消息右对齐，AI消息左对齐
    QString alignment = (sender == "user") ? "right" : "left";
    QString bgColor = (sender == "user") ? "#4A90E2" : "#3C3C3C";
    QString textColor = (sender == "user") ? "#FFFFFF" : "#E0E0E0";
    QString maxWidth = "70%";

    // 构建气泡样式
    QString html = QString(
                       "<div style='text-align: %1; margin-bottom: 8px;'>"
                       "  <div style='display: inline-block; max-width: %2; "
                       "              background-color: %3; color: %4; "
                       "              border-radius: 12px; padding: 8px 14px; "
                       "              text-align: left; word-wrap: break-word;'>"
                       "    <div style='font-size: 12px; color: #AAAAAA; margin-bottom: 2px;'>%5</div>"
                       "    <div>%6</div>"
                       "  </div>"
                       "</div>"
                       ).arg(alignment, maxWidth, bgColor, textColor, displayName + " " + time, escapedContent);

    return html;
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
