#ifndef AI_SERVER_H
#define AI_SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>
#include <QPointer>
#include <QTcpServer>


namespace SprintIDE {

// 前向声明主窗口类，避免循环依赖
class MainWindow;

/**
 * @class AIServer
 * @brief AI 服务端类，负责与 AI Agent 进程进行通信
 *
 * 本类基于 TCP 协议，使用长度前缀（4字节大端整数）封装 JSON-RPC 2.0 消息。
 * 主要功能：
 * 1. 启动本地 TCP 服务（默认 127.0.0.1:9876），等待 AI Agent 连接。
 * 2. 接收 Agent 发来的 JSON-RPC 请求、通知和响应，并转发给 IDE 内部模块。
 * 3. 发送 IDE 主动向 Agent 发起的请求（如自动修复、聊天消息等）。
 * 4. 支持流式数据块传输（用于 AI 流式生成文本）。
 *
 * 设计原则：
 * - 只允许一个 Agent 同时连接（简化逻辑，符合实际使用场景）。
 * - 消息格式：[4字节大端长度][JSON数据（UTF-8编码）]
 * - 所有 JSON 消息遵循 JSON-RPC 2.0 规范。
 */
class AIServer : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象，用于 Qt 内存管理
     */
    explicit AIServer(QObject *parent = nullptr);

    /**
     * @brief 析构函数，负责清理资源
     */
    ~AIServer();

    // ========== 服务生命周期管理 ==========

    /**
     * @brief 启动 TCP 服务器
     * @param port 监听端口，默认 9876
     * @param bindAddress 绑定地址，默认 127.0.0.1（仅本机访问，保证安全）
     * @return 成功启动返回 true，否则 false
     */
    bool startServer(quint16 port = 9876, const QString &bindAddress = "127.0.0.1");

    /**
     * @brief 停止服务器，断开所有连接并释放资源
     */
    void stopServer();

    /**
     * @brief 检查服务器是否正在运行
     * @return 运行中返回 true
     */
    bool isRunning() const;

    /**
     * @brief 检查是否有 Agent 连接
     * @return 已连接返回 true
     */
    bool isAgentConnected() const;

    /**
     * @brief 设置主窗口指针，用于执行具体的 IDE 操作（由外部注入）
     * @param mainWindow 主窗口指针
     */
    void setMainWindow(MainWindow *mainWindow);

    // ========== 向 Agent 发送消息的接口 ==========

    /**
     * @brief 发送一个 JSON-RPC 请求（需要 Agent 返回响应）
     * @param method 方法名，如 "agent.chat"
     * @param params 参数对象（JSON 对象）
     * @param requestId 请求 ID（由调用方分配，用于匹配响应）
     * @return 发送成功返回 true，失败返回 false
     */
    bool sendRequest(const QString &method, const QJsonObject &params, int requestId);

    /**
     * @brief 发送一个 JSON-RPC 通知（无需 Agent 返回响应）
     * @param method 方法名，如 "compileFinished"
     * @param params 参数对象
     * @return 发送成功返回 true
     */
    bool sendNotification(const QString &method, const QJsonObject &params);

    /**
     * @brief 发送错误响应（当 IDE 处理 Agent 请求失败时使用）
     * @param requestId 对应请求的 ID
     * @param code 错误码（按 JSON-RPC 规范定义）
     * @param message 错误描述
     * @return 发送成功返回 true
     */
    bool sendErrorResponse(int requestId, int code, const QString &message);

    /**
     * @brief 发送成功响应（当 IDE 成功处理 Agent 请求时使用）
     * @param requestId 对应请求的 ID
     * @param result 结果对象（JSON 对象）
     * @return 发送成功返回 true
     */
    bool sendResultResponse(int requestId, const QJsonObject &result);

    /**
     * @brief 发送流式数据块（用于响应 Agent 的流式请求）
     * @param requestId 对应请求的 ID
     * @param streamId 流标识符（用于区分多个并行流）
     * @param chunk 文本块内容
     * @return 发送成功返回 true
     */
    bool sendStreamChunk(int requestId, const QString &streamId, const QString &chunk);

    /**
     * @brief 发送流结束信号，表示流式传输完成
     * @param requestId 对应请求的 ID
     * @param streamId 流标识符
     * @param fullContent 完整内容（可选，用于方便 UI 一次性显示）
     * @return 发送成功返回 true
     */
    bool sendStreamEnd(int requestId, const QString &streamId, const QString &fullContent = QString());

signals:
    // ========== 服务器状态信号 ==========

    /**
     * @brief 服务器成功启动信号
     */
    void serverStarted();

    /**
     * @brief 服务器已停止信号
     */
    void serverStopped();

    /**
     * @brief Agent 已连接信号
     */
    void agentConnected();

    /**
     * @brief Agent 已断开连接信号
     */
    void agentDisconnected();

    // ========== 接收来自 Agent 的消息信号 ==========

    /**
     * @brief 收到 Agent 发来的请求（需要 IDE 处理，方法名通常以 "ide." 开头）
     * @param requestId 请求 ID，回复时必须使用相同 ID
     * @param method 方法名
     * @param params 参数对象
     *
     * 外部模块（如 MainWindow）应连接此信号，执行具体操作，
     * 然后调用 sendResultResponse() 或 sendErrorResponse() 回复。
     */
    void ideRequestReceived(int requestId, const QString &method, const QJsonObject &params);

    /**
     * @brief 收到 Agent 发来的通知（无 ID，无需回复）
     * @param method 方法名
     * @param params 参数对象
     */
    void notificationReceived(const QString &method, const QJsonObject &params);

    /**
     * @brief 收到 Agent 对 IDE 先前请求的响应（成功）
     * @param requestId 对应请求的 ID
     * @param result 结果对象
     */
    void responseReceived(int requestId, const QJsonObject &result);

    /**
     * @brief 收到 Agent 对 IDE 先前请求的错误响应
     * @param requestId 对应请求的 ID
     * @param code 错误码
     * @param message 错误描述
     */
    void errorResponseReceived(int requestId, int code, const QString &message);

    /**
     * @brief 收到 Agent 的流式数据块（用于聊天、代码生成等）
     * @param requestId 对应请求的 ID
     * @param streamId 流标识符
     * @param chunk 文本块
     * @param isEnd 是否为最后一块（结束标志）
     * @param fullContent 完整内容（仅在 isEnd=true 时有效）
     */
    void streamDataReceived(int requestId, const QString &streamId,
                            const QString &chunk, bool isEnd, const QString &fullContent);

private slots:
    // ========== TCP 事件处理 ==========

    /**
     * @brief 处理新客户端连接
     * 当有新的 TCP 连接建立时被调用。如果已有 Agent 连接，则拒绝新连接。
     */
    void onNewConnection();

    /**
     * @brief 处理 Socket 数据可读事件
     * 读取 TCP 数据，解析长度前缀和 JSON 内容，处理粘包/半包。
     */
    void onReadyRead();

    /**
     * @brief 处理 Socket 断开连接事件
     * 清理连接状态，发射 agentDisconnected 信号。
     */
    void onSocketDisconnected();

    /**
     * @brief 处理 Socket 错误事件
     * @param error 错误类型
     */
    void onSocketError(QAbstractSocket::SocketError error);

    /**
     * @brief 心跳超时处理（可选）
     * 如果一定时间内没有收到任何数据，可主动断开连接。
     */
    void onHeartbeatTimeout();

private:
    // ========== 内部辅助方法 ==========

    /**
     * @brief 处理接收到的完整消息（已解析出 JSON 对象）
     * @param message 完整的 JSON 字符串
     */
    void handleIncomingMessage(const QString &message);

    /**
     * @brief 处理 JSON-RPC 消息对象
     * 根据消息中的 "id" 和 "method" 字段分发到不同的信号。
     * @param msg 解析后的 JSON 对象
     */
    void processJsonRpcMessage(const QJsonObject &msg);

    /**
     * @brief 向 Agent 发送原始二进制数据
     * @param data 要发送的数据（已经包含长度前缀）
     * @return 发送成功返回 true
     */
    bool sendMessageToAgent(const QByteArray &data);

    /**
     * @brief 构造一条完整消息（长度前缀 + JSON）
     * @param jsonObject 要发送的 JSON 对象
     * @return 包含长度前缀的完整二进制数据
     */
    QByteArray buildMessage(const QJsonObject &jsonObject);

    /**
     * @brief 检查 Agent 是否已连接且 Socket 可写
     * @return 可写返回 true
     */
    bool isAgentWritable() const;

    // ========== 成员变量 ==========

    QTcpServer *m_tcpServer = nullptr;          ///< TCP 服务对象
    QTcpSocket *m_agentSocket = nullptr;        ///< 当前连接的 Agent Socket（只保留一个）
    MainWindow *m_mainWindow = nullptr;          ///< 主窗口指针（弱引用，防止悬挂）
    QTimer *m_heartbeatTimer = nullptr;         ///< 心跳定时器（可选，用于连接保活）

    // 接收缓冲区（用于累积 TCP 流数据，处理粘包/半包）
    QByteArray m_recvBuffer;

    // 运行状态标志
    bool m_isRunning = false;                   ///< 服务器是否正在运行
    bool m_isAgentConnected = false;            ///< 是否有 Agent 连接

    // 服务器配置
    quint16 m_port = 9876;                      ///< 监听端口
    QString m_bindAddress = "127.0.0.1";        ///< 绑定地址（默认仅本机）
};

} // namespace SprintIDE

#endif // AI_SERVER_H
