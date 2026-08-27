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

    // 文档管理
    void openDocument(const QString& filePath, const QString& content);
    void changeDocument(const QString& filePath, const QString& content);
    void closeDocument(const QString& filePath);
    void saveDocument(const QString& filePath);

    // 设置
    void setProjectRoot(const QString& root);
    void enableClangTidy(bool enable);

signals:
    void serverStarted();
    void serverStopped();
    void serverError(const QString& error);
    void serverReady();
    void diagnosticsReceived(const QString& filePath, const QList<LSPDiagnostic>& diagnostics);

private slots:
    void onServerReadyRead();
    void onServerErrorRead();
    void onServerFinished(int exitCode, QProcess::ExitStatus status);
    void onReparseTimer();

private:
    // 消息处理
    void sendMessage(const QJsonObject& message);
    void parseMessage(const QByteArray& data);
    void handleResponse(const QJsonObject& response);
    void handleNotification(const QJsonObject& notification);
    void handlePublishDiagnostics(const QJsonObject& params);

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
    bool m_enableClangTidy;
};

} // namespace SprintIDE

#endif // LSP_CLIENT_H
