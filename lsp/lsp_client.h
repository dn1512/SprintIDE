#ifndef LSP_CLIENT_H
#define LSP_CLIENT_H

#include <QObject>
#include <QProcess>
#include <QMap>
#include <QTimer>
#include <QJsonObject>

namespace SprintIDE {

struct LSPDiagnostic {
    QString filePath;
    int line;          // 1-based
    int column;        // 1-based
    QString message;
    QString severity;  // "error", "warning", "information", "hint"
    QString code;
};

struct LSPCompletionItem {
    QString label;
    QString detail;
    QString kind;      // "method", "function", "class", "variable", "keyword", "snippet" 等
    QString insertText;
    int kindCode;      // LSP 原始 kind 码（用于过滤）
};

class LSPClient : public QObject {
    Q_OBJECT

public:
    explicit LSPClient(QObject* parent = nullptr);
    ~LSPClient();

    // 生命周期
    bool startServer(const QString& serverPath = QString());
    void stopServer();
    bool isReady() const;
    bool isServerRunning() const;

    void requestDefinition(const QString& filePath, int line, int column);

    // 文档管理
    void openDocument(const QString& filePath, const QString& content);
    void changeDocument(const QString& filePath, const QString& content);
    void closeDocument(const QString& filePath);
    void saveDocument(const QString& filePath);

    // 设置
    void setProjectRoot(const QString& root);
    void enableClangTidy(bool enable);

    /**
     * @brief 请求代码补全
     * @param filePath 文件路径
     * @param line 行号（0-based）
     * @param column 列号（0-based）
     */
    void requestCompletion(const QString& filePath, int line, int column);
signals:
    void serverStarted();
    void serverStopped();
    void serverError(const QString& error);
    void serverReady();
    void diagnosticsReceived(const QString& filePath, const QList<LSPDiagnostic>& diagnostics);

    /**
     * @brief 补全结果信号
     * @param filePath 文件路径
     * @param items 补全项列表
     */
    void completionReceived(const QString& filePath, const QList<LSPCompletionItem>& items);

    void definitionReceived(const QString& filePath, int line, int column);
private slots:
    void onServerReadyRead();
    void onServerErrorRead();
    void onServerFinished(int exitCode, QProcess::ExitStatus status);
    void onReparseTimer();
    void onInitializeTimeout();

private:
    // 消息处理
    void sendMessage(const QJsonObject& message);
    void parseMessage(const QByteArray& data);
    void handleResponse(const QJsonObject& response);
    void handleNotification(const QJsonObject& notification);
    void handlePublishDiagnostics(const QJsonObject& params);
    void handleDefinitionResponse(const QJsonObject& response);

    // 工具方法
    QString uriFromPath(const QString& path) const;
    QString pathFromUri(const QString& uri) const;
    QByteArray buildMessage(const QJsonObject& message);

    QProcess* m_process;
    int m_messageId;
    bool m_initialized;
    QByteArray m_buffer;
    QMap<int, QString> m_pendingRequests;
    QString m_serverPath;
    QString m_projectRoot;
    QTimer* m_reparseTimer;
    QString m_pendingFilePath;
    QString m_pendingContent;
    QTimer* m_initTimer;
    bool m_enableClangTidy;
    /**
     * @brief 处理补全响应
     */
    void handleCompletionResponse(const QJsonObject& response);
    /**
     * @brief 将 LSP 的 CompletionItemKind 转换为字符串
     */
    QString completionKindToString(int kind) const;

    // ... 现有成员 ...
    /**
     * @brief 补全请求的待处理 ID（用于关联响应）
     */
    int m_completionRequestId = -1;
    /**
     * @brief 当前请求补全的文件路径（用于关联响应）
     */
    QString m_completionFilePath;
};

} // namespace SprintIDE

#endif // LSP_CLIENT_H
