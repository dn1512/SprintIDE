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
#ifndef LSP_MANAGER_H
#define LSP_MANAGER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QList>
#include <QPointer>

#include "lsp_client.h"
#include "language/language_detector.h"

namespace SprintIDE {

/**
 * @brief LSP 服务器配置信息
 *
 * 描述一个 LSP 服务器的启动命令、参数和初始化选项
 */
struct LSPConfig
{
    QString languageId;      ///< 语言 ID（如 "cpp", "python"）
    QString serverName;      ///< 服务器显示名称（如 "clangd", "pylsp"）
    QString command;         ///< 可执行文件命令（如 "clangd"）
    QStringList arguments;   ///< 启动参数（如 "--background-index"）
    QStringList envPaths;    ///< 额外的环境变量或路径（可选）
    bool hasCompileCommands; ///< 是否需要 compile_commands.json（C++ 专用）

    LSPConfig() : hasCompileCommands(false) {}
    LSPConfig(const QString &lang, const QString &name,
              const QString &cmd, const QStringList &args)
        : languageId(lang), serverName(name), command(cmd), arguments(args),
        hasCompileCommands(false) {}
};

/**
 * @brief LSP 多语言管理器
 *
 * 负责管理多个 LSP 客户端实例，根据文件语言自动选择或启动对应的 LSP 服务器。
 *
 * 使用方式：
 *   LSPManager manager;
 *   manager.initialize();
 *   LSPClient *client = manager.getClientForFile("/project/main.cpp", "/project");
 *   if (client) {
 *       client->openDocument(...);
 *   }
 *
 * 支持的 LSP 服务器：
 *   - C++      : clangd
 *   - Python   : pylsp
 *   - JavaScript/TypeScript : typescript-language-server
 *   - Go       : gopls
 *   - Rust     : rust-analyzer
 *   - Java     : jdtls (需要单独配置)
 *   - C#       : omnisharp (需要单独配置)
 *   - PHP      : intelephense (需要单独配置)
 */
class LSPManager : public QObject
{
    Q_OBJECT

public:
    explicit LSPManager(QObject *parent = nullptr);
    ~LSPManager();

    /**
     * @brief 初始化 LSP 管理器
     *
     * 检测系统中可用的 LSP 服务器，构建服务器配置列表
     */
    void initialize();

    /**
     * @brief 获取或创建文件对应的 LSP 客户端
     * @param filePath 文件路径（用于检测语言和项目根目录）
     * @param projectRoot 项目根目录（用于隔离不同项目的客户端）
     * @return LSP 客户端指针，如果找不到对应的 LSP 服务器则返回 nullptr
     *
     * 内部逻辑：
     *   1. 通过 LanguageDetector 检测文件语言
     *   2. 根据语言查找对应的 LSP 服务器配置
     *   3. 如果该语言的客户端尚未创建，则启动服务器
     *   4. 返回缓存的客户端实例
     */
    LSPClient *getClientForFile(const QString &filePath,
                                const QString &projectRoot);

    /**
     * @brief 获取特定语言和项目的 LSP 客户端
     * @param languageId 语言 ID
     * @param projectRoot 项目根目录
     * @return LSP 客户端指针，如果未创建则返回 nullptr
     */
    LSPClient *getClient(const QString &languageId,
                         const QString &projectRoot) const;

    /**
     * @brief 关闭并清理所有 LSP 客户端
     */
    void shutdownAll();

    /**
     * @brief 获取支持的 LSP 服务器列表
     */
    QList<LSPConfig> availableServers() const;

signals:
    /**
     * @brief 客户端创建信号
     */
    void clientCreated(const QString &languageId, const QString &projectRoot);

    /**
     * @brief 客户端错误信号
     */
    void clientError(const QString &languageId, const QString &error);



    void diagnosticsReceived(const QString &filePath, const QList<LSPDiagnostic> &diagnostics);

private:
    /**
     * @brief 构建内置的 LSP 服务器配置
     */
    void buildDefaultConfigs();

    /**
     * @brief 检测系统中是否安装了指定的 LSP 服务器
     * @param command 命令名（如 "clangd"）
     * @return 找到返回可执行文件的完整路径，否则返回空字符串
     */
    QString findServerExecutable(const QString &command) const;

    /**
     * @brief 生成客户端的唯一键（用于缓存）
     */
    QString makeClientKey(const QString &projectRoot,
                          const QString &languageId) const;

    /**
     * @brief 获取语言对应的 LSP 配置
     */
    LSPConfig getConfigForLanguage(const QString &languageId) const;

    // 获取针对 Python 的 LSP 配置（动态检测）
    LSPConfig getPythonLSPConfig() const;
    // 备用 Python LSP 配置（pylsp）
    LSPConfig getFallbackPythonLSPConfig() const;

    // 成员变量
    QMap<QString, LSPClient *> m_clients;           ///< 客户端缓存（key → client）
    QMap<QString, LSPConfig> m_configs;             ///< 语言配置（languageId → LSPConfig）
    QMap<QString, QString> m_serverCache;           ///< 服务器路径缓存（command → fullPath）

    bool m_initialized = false;
};

} // namespace SprintIDE

#endif // LSP_MANAGER_H
