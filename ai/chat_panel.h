#ifndef CHAT_PANEL_H
#define CHAT_PANEL_H

#include <QDockWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolBar>
#include <QTextCursor>
#include <QPointer>
#include <QMap>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>

#include "ai_connection_manager.h"

// 前向声明（避免循环包含）
// 注意：这些类在 SprintIDE 命名空间中，但为了能用于 QPointer，我们只需前向声明
namespace SprintIDE {
class AiConnectionManager;
}

namespace SprintIDE {

/**
 * @class ChatPanel
 * @brief AI 对话面板（停靠在 IDE 右侧）
 *
 * 主要功能：
 * - 显示用户与 AI 的对话消息（气泡样式）
 * - 支持流式输出（AI 回复逐字显示，打字机效果）
 * - 多行输入框，Enter 发送，Ctrl+Enter 换行
 * - 显示连接状态（已连接/未连接）
 * - 自动滚动到最新消息
 */
class ChatPanel : public QDockWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    explicit ChatPanel(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ChatPanel() override;

    // ============================================================
    // 设置依赖对象
    // ============================================================

    /**
     * @brief 设置 AI 连接管理
     */
    void setConnectionManager(AiConnectionManager* mgr);

    // ============================================================
    // 消息添加方法（供外部调用）
    // ============================================================

    /**
     * @brief 添加一条用户消息
     * @param message 消息内容
     */
    void addUserMessage(const QString &message);

    /**
     * @brief 添加一条 AI 消息（非流式，直接显示完整内容）
     */
    void addAIMessage(const QString &message);

    /**
     * @brief 显示系统消息（如错误提示、连接状态等）
     * @param message 消息内容
     * @param isError 是否为错误消息（红色显示）
     */
    void showSystemMessage(const QString &message, bool isError = false);

    // ============================================================
    // 流式消息支持（供 AI 服务器调用）
    // ============================================================

    /**
     * @brief 开始一个流式消息（创建消息占位）
     * @param streamId 流标识（用于后续匹配数据块）
     * @return 是否成功创建
     */
    bool startStreamMessage(const QString &streamId);

    /**
     * @brief 追加流式数据块到当前 AI 消息
     * @param streamId 流标识
     * @param chunk 文本块
     * @return 是否成功追加
     */
    bool appendStreamChunk(const QString &streamId, const QString &chunk);

    /**
     * @brief 结束流式消息
     * @param streamId 流标识
     * @param fullContent 完整内容（可选，如果提供则替换整个消息）
     * @return 是否成功结束
     */
    bool finishStreamMessage(const QString &streamId, const QString &fullContent = QString());

    // ============================================================
    // 管理方法
    // ============================================================

    /**
     * @brief 清空所有对话消息
     */
    void clearMessages();

    /**
     * @brief 设置连接状态（更新标题栏和状态标签）
     */
    void setConnectionStatus(bool connected);

    /**
     * @brief 获取对话历史（用于发送给 AI）
     * @return JSON 数组，每条消息包含 role 和 content
     */
    QJsonArray getConversationHistory() const;

    /**
     * @brief 设置是否自动包含上下文信息（文件内容、项目信息等）
     */
    void setAutoSendContext(bool enabled);

    void focusPanel();

signals:
    /**
     * @brief 用户发送消息信号（由 ChatPanel 发出，外部监听并转发给 AI）
     * @param message 消息内容
     * @param includeContext 是否包含上下文
     */
    void userMessageSent(const QString &message, bool includeContext);

    /**
     * @brief 对话历史变化信号（可用于持久化）
     */
    void historyChanged();

    /**
     * @brief 连接状态变化信号
     */
    void connectionStatusChanged(bool connected);

    void askAnswered(const QString& userAnswer);

    void confirmAnswered(bool approved);

public slots:
    // ============================================================
    // 公共槽函数（供外部连接，如 AI 服务器）
    // ============================================================

    /**
     * @brief 处理 AI 服务器发来的流式数据
     */
    void onStreamDataReceived(int requestId, const QString &streamId,
                              const QString &chunk, bool isEnd,
                              const QString &fullContent);

    /// 状态变化
    void onConnectionStateChanged(AiConnectionManager::State state);

    /// 接收 Agent 消息
    void onAgentMessageReceived(const QJsonObject& message);

    void onAskRequired(const QString& itemId,
                       const QString& question,
                       const QJsonArray& choices,
                       const QString& defaultAnswer);

    /**
     * @brief 处理动作执行器产生的输出（显示操作日志）
     */
    void onActionOutput(const QString &message, bool isError);

    /**
     * @brief 滚动到消息显示区域底部（确保最新消息可见）
     */
    void updateScrollPosition();

    void onConfirmRequired(const QString& itemId,
                           const QString& targetId,
                           const QString& prompt,
                           const QString& defaultApprove);

private slots:
    // ============================================================
    // 内部 UI 槽函数
    // ============================================================

    /**
     * @brief 发送按钮点击处理
     */
    void onSendClicked();

    /**
     * @brief 输入框文本变化（用于动态调整高度等）
     */
    void onInputTextChanged();

    /**
     * @brief 清空对话按钮点击处理
     */
    void onClearClicked();

protected:
    /**
     * @brief 事件过滤器（用于捕获 Enter 键发送消息）
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    // ============================================================
    // UI 初始化方法
    // ============================================================

    void setupUI();
    void createMessageArea();
    QWidget* createInputArea();
    void createToolbar();

    // ============================================================
    // 消息格式化和内部辅助
    // ============================================================

    /**
     * @brief 添加一条消息到显示区域（内部使用）
     * @param sender 发送者： "user" | "ai" | "system"
     * @param content 消息内容
     * @param isStreaming 是否为流式消息（用于创建占位）
     * @param streamId 流标识（流式消息使用）
     * @return 消息文本光标（用于后续追加）
     */
    QTextCursor addMessage(const QString &sender, const QString &content,
                           bool isStreaming = false, const QString &streamId = QString());

    /**
     * @brief 格式化普通消息（气泡样式）
     */
    QString formatMessage(const QString &sender, const QString &content) const;

    /**
     * @brief 格式化系统消息（居中、斜体）
     */
    QString formatSystemMessage(const QString &content, bool isError) const;

    /**
     * @brief 获取发送者的显示名称
     */
    QString getSenderDisplayName(const QString &sender) const;

    /**
     * @brief 获取发送者对应的颜色
     */
    QString getSenderColor(const QString &sender) const;

    /**
     * @brief 构建发送给 AI 的上下文信息
     */
    QJsonObject buildContext() const;

    /**
     * @brief 将对话历史转换为 JSON 数组
     */
    QJsonArray historyToJson() const;

    // ============================================================
    // UI 控件成员
    // ============================================================

    QTextEdit *m_messageDisplay = nullptr;   ///< 消息显示区域
    QTextEdit *m_inputEdit = nullptr;        ///< 多行输入框
    QPushButton *m_sendButton = nullptr;     ///< 发送按钮
    QPushButton *m_clearButton = nullptr;    ///< 清空对话按钮
    QLabel *m_statusLabel = nullptr;         ///< 连接状态标签
    QToolBar *m_toolbar = nullptr;           ///< 工具栏
    QWidget *m_askBar = nullptr;             ///< chat.ask 提问提示条
    QLabel  *m_askLabel = nullptr;           ///< 提问文本
    bool     m_askMode = false;              ///< 是否处于提问模式
    QString  m_pendingAskItemId;             ///< 待回答的 chat.ask item id
    QPushButton *m_reconnectButton = nullptr; ///< 重连按钮（未连接时显示）

    // ============================================================
    // 数据和状态成员
    // ============================================================

    QPointer<AiConnectionManager> m_connMgr;

    QStringList m_conversationHistory;       ///< 对话历史（纯文本，用于上下文）
    QMap<QString, QTextCursor> m_streamCursors; ///< 流式消息的文本光标（streamId → cursor）
    QMap<QString, QString> m_streamContents; ///< 流式消息的内容缓存

    bool m_isConnected = false;              ///< 是否已连接 Agent
    bool m_autoSendContext = true;           ///< 是否自动发送上下文
    int m_maxHistorySize = 50;               ///< 最大历史消息数量

    // 显示名称配置
    QString m_userName = "我";               ///< 用户显示名称
    QString m_aiName = "AI agent";            ///< AI 显示名称
    QLabel* m_statusDot = nullptr;          /// 状态圆点（替代 emoji）
};

} // namespace SprintIDE

#endif // CHAT_PANEL_H
