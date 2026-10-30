// SPDX-License-Identifier: GPL-3.0-or-later
/*
* SprintIDE - SprintIDE is a cross-platform IDE built from scratch with Qt 6 and C++20. Its core capability is to provide AI Agents with an executable, verifiable code execution environment.
* Copyright (C) 2026  Liu Tengjun (刘腾骏)
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef LSP_CLIENT_H
#define LSP_CLIENT_H

#include <QObject>
#include <QProcess>
#include <QMap>
#include <QTimer>
#include <QJsonObject>

#include "project_manager.h"
#include "java/project_model.h"

namespace SprintIDE {

struct LSPDiagnostic {
    QString filePath;
    int line;          // 1-based
    int column;        // 1-based
    int endLine = 0;   // 诊断结束行（1-based，0 表示未设置）
    int endColumn = 0; // 诊断结束列（1-based，0 表示未设置）
    QString message;
    QString severity;  // "error", "warning", "information", "hint"
    QString code;
    QString source;    // 诊断来源
};

struct LSPCompletionItem {
    QString label;
    QString detail;
    QString kind;      // "method", "function", "class", "variable", "keyword", "snippet" 等
    QString insertText;
    int kindCode;      // LSP 原始 kind 码（用于过滤）
};

/// 一个文本编辑操作（对应 LSP 的 TextEdit）
struct LSPTextEdit {
    int startLine    = 0;    // 0-based（LSP 原生）
    int startColumn  = 0;
    int endLine      = 0;
    int endColumn    = 0;
    QString newText;
};

/// 一个文件的编辑集
struct LSPWorkspaceEdit {
    QString filePath;                 // 本地绝对路径
    QList<LSPTextEdit> edits;
};

/// 一个可用的代码动作
struct LSPCodeAction {
    QString title;                    // 菜单项显示文字："Insert #include <cstdio>"
    QString kind;                     // 可选："quickfix", "refactor.rewrite" …
    bool isPreferred = false;
    // 简化：只保留 workspace edit；LSP command 形式暂不支持
    QList<LSPWorkspaceEdit> edits;
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

    /**
     * @brief 请求某位置的代码动作（Quick Fix）
     * @param filePath 文件路径
     * @param startLine    诊断起止行（0-based）
     * @param startColumn  诊断起止列（0-based）
     * @param endLine
     * @param endColumn
     * @param diagnostics  该行的诊断列表（原样透传给 LSP）
     */
    void requestCodeAction(const QString& filePath,
                           int startLine, int startColumn,
                           int endLine,   int endColumn,
                           const QList<LSPDiagnostic>& diagnostics);

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

    /**
     * @brief 设置自定义的 initializationOptions（用于 LSP 初始化）
     * @param options JSON 对象
     */
    void setInitializationOptions(const QJsonObject& options);

    void setCustomCommand(const QString& command) { m_serverPath = command; }
    void setCustomArguments(const QStringList& args) { m_customArguments = args; }

    /// v8.5：设置语言特定的启动参数（来自 LSPManager 的 LSPConfig.arguments）
    ///   优先级：m_customArguments > m_arguments > （空 走 clangd/jdtls 兜底）
    void setArguments(const QStringList& args) { m_arguments = args; }

    /// 修复 Rust 跳转：设置当前 client 的语言 ID（"cpp"/"rust"/"python"...）
    void setLanguageId(const QString& id) { m_languageId = id; }

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

    /**
     * @brief 代码动作响应（可为空列表，表示没有可用修复）
     */
    void codeActionReceived(const QString& filePath,
                            const QList<LSPCodeAction>& actions);
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

    void handleCodeActionResponse(const QJsonObject& response);
    int  m_codeActionRequestId = -1;
    QString m_codeActionFilePath;

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
    QStringList m_customArguments;
    bool m_enableClangTidy;

    // v8.5：来自 LSPManager 的语言特定启动参数
    //   clangd / jdtls / 其他服务器的参数由 LSPManager 的 LSPConfig.arguments 提供，
    //   LSPClient 不再硬编码。只有 m_customArguments（Python 等特殊场景）会覆盖它。
    QStringList m_arguments;
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

    QJsonObject m_customInitializationOptions;   // 自定义初始化选项

    /// 修复 Rust 跳转：当前 client 服务的语言 ID
    ///   用于 didOpen 时填充 textDocument.languageId
    QString m_languageId;

};

} // namespace SprintIDE

#endif // LSP_CLIENT_H
