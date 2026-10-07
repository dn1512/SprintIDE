#include "chat_panel.h"
#include "ai_connection_manager.h"
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
    : QDockWidget(tr("AI agent"), parent)
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
    m_toolbar->setFixedHeight(34);

    // ---------- 左：状态点 + 文字 ----------
    m_statusDot = new QLabel(this);
    m_statusDot->setObjectName("ChatStatusDot");
    m_statusDot->setFixedSize(8, 8);
    m_statusDot->setProperty("connected", false);   // QSS 用它判断颜色
    m_toolbar->addWidget(m_statusDot);

    m_statusLabel = new QLabel(tr("未连接"), this);
    m_statusLabel->setObjectName("statusLabel");
    m_statusLabel->setProperty("class", "disconnected");
    m_toolbar->addWidget(m_statusLabel);

    // ---------- 重连按钮（默认隐藏） ----------
    m_reconnectButton = new QPushButton(tr("连接"), this);
    m_reconnectButton->setObjectName("ChatReconnectButton");
    m_reconnectButton->setToolTip(tr("立即尝试连接 Agent"));
    m_reconnectButton->setFixedHeight(22);
    m_reconnectButton->setVisible(true);
    connect(m_reconnectButton, &QPushButton::clicked, this, [this]() {
        if (m_connMgr) m_connMgr->reconnectNow();
    });
    m_toolbar->addWidget(m_reconnectButton);

    // ---------- 中：弹性空白 ----------
    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_toolbar->addWidget(spacer);

    // ---------- 右：清空 + 复制 ----------
    m_clearButton = new QPushButton(tr("清空"), this);
    m_clearButton->setObjectName("ChatClearButton");
    m_clearButton->setToolTip(tr("清空所有对话消息"));
    m_clearButton->setFixedHeight(24);
    connect(m_clearButton, &QPushButton::clicked, this, &ChatPanel::onClearClicked);
    m_toolbar->addWidget(m_clearButton);

    QPushButton *copyButton = new QPushButton(tr("复制"), this);
    copyButton->setObjectName("ChatCopyButton");
    copyButton->setToolTip(tr("复制整个对话内容"));
    copyButton->setFixedHeight(24);
    connect(copyButton, &QPushButton::clicked, this, [this]() {
        if (m_messageDisplay) {
            QApplication::clipboard()->setText(m_messageDisplay->toPlainText());
            showSystemMessage(tr("对话已复制到剪贴板"), false);
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
    m_inputEdit->setPlaceholderText(tr("输入消息，按 Enter 发送，Ctrl+Enter 换行..."));
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

    m_sendButton = new QPushButton(tr("发送"), this);
    m_sendButton->setObjectName("sendButton");
    m_sendButton->setEnabled(false);
    m_sendButton->setToolTip(tr("发送消息 (Enter)"));
    m_sendButton->setFixedWidth(64);
    m_sendButton->setMinimumHeight(36);
    m_sendButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    connect(m_sendButton, &QPushButton::clicked,
            this, &ChatPanel::onSendClicked);

    // ===== chat.ask 提问提示条 =====
    m_askBar = new QWidget(this);
    m_askBar->setObjectName("askBar");
    m_askBar->setVisible(false);

    QHBoxLayout* askLayout = new QHBoxLayout(m_askBar);
    askLayout->setContentsMargins(12, 6, 12, 6);
    askLayout->setSpacing(6);

    QLabel* askIcon = new QLabel(QStringLiteral("❓"), m_askBar);
    askIcon->setObjectName("askIcon");
    askLayout->addWidget(askIcon);

    m_askLabel = new QLabel(m_askBar);
    m_askLabel->setObjectName("askLabel");
    m_askLabel->setWordWrap(true);
    askLayout->addWidget(m_askLabel, 1);

    {
        ThemeManager& tm = ThemeManager::instance();
        m_askBar->setStyleSheet(QString(
                                    "QWidget#askBar {"
                                    "  background-color: %1;"
                                    "  border-top: 1px solid %2;"
                                    "  border-bottom: 1px solid %2;"
                                    "}"
                                    "QLabel#askIcon {"
                                    "  color: %2;"
                                    "  font-size: 14px;"
                                    "  font-weight: 700;"
                                    "}"
                                    "QLabel#askLabel {"
                                    "  color: %3;"
                                    "  font-size: 12px;"
                                    "  font-weight: 600;"
                                    "}"
                                    ).arg(tm.rgba(ThemeManager::UIElement::WarningColor, 40),
                                         tm.colorString(ThemeManager::UIElement::WarningColor),
                                         tm.colorString(ThemeManager::UIElement::TextColor)));
    }

    // ===== 输入行 =====
    QHBoxLayout* inputRow = new QHBoxLayout();
    inputRow->setContentsMargins(12, 8, 12, 12);
    inputRow->setSpacing(8);
    inputRow->addWidget(m_inputEdit, 1);
    inputRow->addWidget(m_sendButton);

    // ===== 组装：askBar 在上，输入行在下 =====
    QVBoxLayout* vbox = new QVBoxLayout();
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(0);
    vbox->addWidget(m_askBar);
    vbox->addLayout(inputRow);

    QWidget* inputContainer = new QWidget(this);
    inputContainer->setObjectName("inputFrame");
    inputContainer->setLayout(vbox);

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
    m_conversationHistory.append(tr("用户: ") + message);

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

    // 从 ThemeManager 取色（原来是硬编码 #E0E0E0）
    ThemeManager& tm = ThemeManager::instance();
    QColor textColor = tm.color(ThemeManager::UIElement::TextColor);

    cursor.beginEditBlock();
    QTextCharFormat format;
    format.setForeground(textColor);
    cursor.setCharFormat(format);
    cursor.insertText(chunk);
    cursor.endEditBlock();

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
            cursor.insertText(tr("\n\n--- 完整内容 ---\n"));
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
    showSystemMessage(tr("对话已清空"), false);
}

// ============================================================
// 设置连接状态
// ============================================================
void ChatPanel::setConnectionStatus(bool connected)
{
    m_isConnected = connected;

    // ---- 状态点 + 文字 ----
    if (m_statusDot) {
        m_statusDot->setProperty("connected", connected);
        m_statusDot->style()->unpolish(m_statusDot);
        m_statusDot->style()->polish(m_statusDot);
    }
    if (m_statusLabel) {
        m_statusLabel->setText(connected ? tr("已连接") : tr("未连接"));
        m_statusLabel->setProperty("class", connected ? "connected" : "disconnected");
        m_statusLabel->style()->unpolish(m_statusLabel);
        m_statusLabel->style()->polish(m_statusLabel);
    }

    if (m_inputEdit) {
        m_inputEdit->setEnabled(connected);
    }
    if (m_sendButton) {
        m_sendButton->setEnabled(connected);
    }

    setWindowTitle(connected ? tr("AI agent (已连接)") : tr("AI agent (未连接)"));

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
        if (entry.startsWith(tr("用户: "))) {
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

void ChatPanel::setConnectionManager(AiConnectionManager* mgr) {
    m_connMgr = mgr;
    qDebug() << "[ChatPanel] setConnectionManager called, mgr =" << mgr;
    if (!mgr) return;

    connect(mgr, &AiConnectionManager::stateChanged,
            this, &ChatPanel::onConnectionStateChanged);
    connect(mgr, &AiConnectionManager::agentMessageReceived,
            this, &ChatPanel::onAgentMessageReceived);
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

    m_inputEdit->clear();

    // ===== 提问模式：把答案发给 Agent =====
    if (m_askMode) {
        m_askMode = false;
        m_pendingAskItemId.clear();

        if (m_askBar)      m_askBar->setVisible(false);
        if (m_sendButton) {
            m_sendButton->setText(tr("发送"));
            m_sendButton->setToolTip(tr("发送消息 (Enter)"));
        }
        if (m_inputEdit) {
            m_inputEdit->setPlaceholderText(
                tr("输入消息，按 Enter 发送，Ctrl+Enter 换行..."));
        }

        // 让用户看到自己答了什么
        addUserMessage(text);

        emit askAnswered(text);
        return;
    }

    // ===== 普通模式：原逻辑 =====
    emit userMessageSent(text, m_autoSendContext);
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
void ChatPanel::onConnectionStateChanged(AiConnectionManager::State state) {
    qDebug() << "[ChatPanel] stateChanged: " << static_cast<int>(state);
    using S = AiConnectionManager::State;

    switch (state) {
    case S::Ready:
        setConnectionStatus(true);
        showSystemMessage(tr("AI agent已连接"), false);
        break;
    case S::Disconnected:
    case S::Failed:
        setConnectionStatus(false);
        showSystemMessage(tr("AI agent已断开连接"), true);
        break;
    case S::Reconnecting:
        setConnectionStatus(false);
        showSystemMessage(tr("正在重连..."), false);
        break;
    default:
        break;
    }

    // 根据状态切换重连按钮可见性
    if (m_reconnectButton) {
        using S = AiConnectionManager::State;

        // 按钮常驻；只在"已连接"时禁用，其余状态都可点
        m_reconnectButton->setVisible(true);
        m_reconnectButton->setEnabled(state != S::Ready);

        // 文字随状态变化，给用户明确反馈
        switch (state) {
        case S::Ready:         m_reconnectButton->setText(tr("连接"));   break;
        case S::Failed:        m_reconnectButton->setText(tr("重试"));   break;
        case S::Reconnecting:  m_reconnectButton->setText(tr("重连中")); break;
        case S::Connecting:    m_reconnectButton->setText(tr("连接中")); break;
        case S::Disconnected:
        default:               m_reconnectButton->setText(tr("连接"));   break;
        }
    }
}

void ChatPanel::onAgentMessageReceived(const QJsonObject& msg) {
    // 1. reply → ChatPanel 气泡
    const QString reply = msg.value("reply").toString();
    if (!reply.isEmpty()) {
        addAIMessage(reply);
    }

    // 2. items → 阶段 0 暂不处理，阶段 1 交给 OperationBus
    //    这里只记日志
    const QJsonArray items = msg.value("items").toArray();
    if (!items.isEmpty()) {
        qDebug() << "[ChatPanel] 收到" << items.size()
            << "个 items（已交给 OperationBus 处理）";
    }

    // 3. error → OutputPanel（不进 ChatPanel）
    if (msg.contains("error")) {
        const QJsonObject err = msg.value("error").toObject();
        qWarning() << "[AI] Agent 错误:" << err.value("code").toString()
                   << err.value("message").toString();
        // 阶段 1 接入 OutputPanel 后改这里
    }
}

void ChatPanel::onAskRequired(const QString& itemId,
                              const QString& question,
                              const QJsonArray& choices,
                              const QString& defaultAnswer)
{
    // ---- 如果 ChatPanel 被隐藏，先弹出来 ----
    if (!isVisible()) {
        setVisible(true);
    }
    raise();
    activateWindow();

    m_askMode = true;
    m_pendingAskItemId = itemId;

    // ---- 组装显示文本：问题 + 选项 ----
    QString display = question;
    if (!choices.isEmpty()) {
        QStringList ops;
        for (const QJsonValue& v : choices) {
            const QString s = v.toString();
            if (!s.isEmpty()) ops.append(s);
        }
        if (!ops.isEmpty()) {
            display += QStringLiteral("\n选项：") + ops.join(QStringLiteral("  /  "));
        }
    }

    if (m_askLabel) {
        m_askLabel->setText(tr("AI 提问：%1").arg(display));
    }
    if (m_askBar) {
        m_askBar->setVisible(true);
    }

    // ---- 输入区切换到"回答模式" ----
    if (m_inputEdit) {
        m_inputEdit->setPlaceholderText(
            choices.isEmpty()
                ? tr("输入你的回答，按 Enter 提交...")
                : tr("输入答案，或从上面的选项中选择..."));
        if (!defaultAnswer.isEmpty()) {
            m_inputEdit->setPlainText(defaultAnswer);
        }
        m_inputEdit->setFocus();
    }
    if (m_sendButton) {
        m_sendButton->setText(tr("回答"));
        m_sendButton->setToolTip(tr("提交回答 (Enter)"));
    }

    // ---- 在消息流里也留一条系统消息，方便回看 ----
    showSystemMessage(tr("AI 提问：%1").arg(display), false);
}

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

void ChatPanel::onConfirmRequired(const QString& itemId,
                                  const QString& targetId,
                                  const QString& prompt,
                                  const QString& defaultApprove)
{
    Q_UNUSED(itemId);
    Q_UNUSED(targetId);

    const QMessageBox::StandardButton defaultBtn =
        (defaultApprove == QStringLiteral("approve"))
            ? QMessageBox::Yes : QMessageBox::No;

    const auto reply = QMessageBox::question(
        this, tr("AI 请求确认"),
        prompt.isEmpty() ? tr("AI 请求执行一个操作，是否继续？") : prompt,
        QMessageBox::Yes | QMessageBox::No,
        defaultBtn);

    emit confirmAnswered(reply == QMessageBox::Yes);
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

    const QString displayName = getSenderDisplayName(sender);
    const QString time        = QDateTime::currentDateTime().toString("hh:mm");

    // 内容转义 + 换行
    QString escaped = content.toHtmlEscaped();
    escaped.replace("\n", "<br/>");

    // ============================================================
    //  配色
    // ============================================================
    const QString textColor      = tm.colorString(ThemeManager::UIElement::TextColor);
    const QString secondaryColor = tm.colorString(ThemeManager::UIElement::SecondaryTextColor);

    // 用户：PrimaryColor 填充 + 白字（与 AI 的淡底形成强对比）
    const QString userBubbleBg  = tm.colorString(ThemeManager::UIElement::PrimaryColor);
    const QString userBubbleFg  = "#FFFFFF";
    const QString userMetaColor = "rgba(255,255,255,0.75)";

    // AI：淡青底 + 正文色 + 极光青左边条
    const QString aiBubbleBg = tm.colorString(ThemeManager::UIElement::AIChatAIBubble);
    const QString aiBubbleFg = textColor;
    const QString aiBorder   = tm.colorString(ThemeManager::UIElement::AIChatAIBorder);

    // ============================================================
    //  气泡外壳样式
    //  - 不对称圆角（指向感）
    //  - 用户气泡：PrimaryColor 填充，白字
    //  - AI 气泡：淡青底，左侧 3px 极光青边
    // ============================================================
    QString bubbleStyle;
    if (isUser) {
        bubbleStyle = QString(
                          "background-color: %1;"
                          "color: %2;"
                          "border-radius: 12px 4px 12px 12px;"   // 右上角小
                          "padding: 8px 12px;"
                          ).arg(userBubbleBg, userBubbleFg);
    } else {
        bubbleStyle = QString(
                          "background-color: %1;"
                          "color: %2;"
                          "border-radius: 4px 12px 12px 12px;"   // 左上角小
                          "border-left: 3px solid %3;"
                          "padding: 8px 12px;"
                          ).arg(aiBubbleBg, aiBubbleFg, aiBorder);
    }

    const QString metaColor = isUser ? userMetaColor : secondaryColor;
    const QString metaText  = QString("%1 · %2").arg(displayName, time);

    // ============================================================
    //  气泡 HTML
    // ============================================================
    const QString bubbleHtml = QString(
                                   "<div style='%1'>"
                                   "  <div style='font-size: 10px; color: %2; margin-bottom: 3px;'>%3</div>"
                                   "  <div style='font-size: 13px; line-height: 1.55;'>%4</div>"
                                   "</div>"
                                   ).arg(bubbleStyle, metaColor, metaText, escaped);

    // ============================================================
    //  用 HTML 表格保证左右分列（这是 Qt 里最可靠的方式）
    //   - AI   → 左 78%，右 22% 空
    //   - 用户 → 左 22% 空，右 78%
    // ============================================================
    if (isUser) {
        return QString(
                   "<table width='100%' cellspacing='0' cellpadding='0' style='margin-top: 2px; margin-bottom: 8px;'>"
                   "  <tr>"
                   "    <td width='22%'></td>"
                   "    <td width='78%' align='right'>%1</td>"
                   "  </tr>"
                   "</table>"
                   ).arg(bubbleHtml);
    } else {
        return QString(
                   "<table width='100%' cellspacing='0' cellpadding='0' style='margin-top: 2px; margin-bottom: 8px;'>"
                   "  <tr>"
                   "    <td width='78%' align='left'>%1</td>"
                   "    <td width='22%'></td>"
                   "  </tr>"
                   "</table>"
                   ).arg(bubbleHtml);
    }
}

// ============================================================
// 格式化系统消息（居中，灰色斜体）
// ============================================================
QString ChatPanel::formatSystemMessage(const QString &content, bool isError) const
{
    ThemeManager& tm = ThemeManager::instance();

    // 错误用 ErrorColor，正常用 SecondaryTextColor
    const QString color = isError
                              ? tm.colorString(ThemeManager::UIElement::ErrorColor)
                              : tm.colorString(ThemeManager::UIElement::SecondaryTextColor);

    const QString bg = tm.colorString(ThemeManager::UIElement::HoverBackground);

    const QString escaped = content.toHtmlEscaped();

    // 用表格保证水平居中（text-align: center 对块级元素在 Qt 里不可靠）
    return QString(
               "<table width='100%' cellspacing='0' cellpadding='0' style='margin-top: 8px; margin-bottom: 8px;'>"
               "  <tr>"
               "    <td align='center'>"
               "      <span style='display: inline; "
               "                    background-color: %1; "
               "                    color: %2; "
               "                    border-radius: 10px; "
               "                    padding: 4px 12px; "
               "                    font-size: 11px;'>%3</span>"
               "    </td>"
               "  </tr>"
               "</table>"
               ).arg(bg, color, escaped);
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
        return tr("系统");
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
    context["history"] = getConversationHistory();
    return context;
}

QJsonArray ChatPanel::historyToJson() const
{
    return getConversationHistory();
}

} // namespace SprintIDE
