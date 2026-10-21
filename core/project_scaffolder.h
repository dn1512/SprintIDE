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
#ifndef PROJECT_SCAFFOLDER_H
#define PROJECT_SCAFFOLDER_H

#include <QString>
#include <QStringList>

namespace SprintIDE {

/**
 * @brief 项目骨架生成器
 *
 * 职责：给定目标路径 + 语言，在磁盘上生成对应的项目结构。
 * 与 UI 完全解耦，可被 ProjectExecutor（AI 调用）或
 * ProjectCreationDialog（用户 UI）复用。
 *
 * 不负责：
 *   - 项目打开（由 MainWindow::openProject 处理）
 *   - 项目信息收集（由 ProjectExecutor::collectBasicInfo 处理）
 */
class ProjectScaffolder
{
public:
    struct CreateOptions {
        QString projectPath;         ///< 完整路径（含项目名）
        QString projectName;         ///< 项目名（用于文件内显示）
        QString displayName;         ///< 可选，人类可读名
        QString language;            ///< python / cpp / go / rust / node / generic_ai
        bool    createVenv = false;  ///< Python 专属：是否创建 .venv
        QString interpreterPath;     ///< Python 解释器（可选，用于 venv 创建）
        bool    overwriteIfExists = false;
    };

    struct CreateResult {
        bool success = false;
        QString errorCode;           ///< PATH_EXISTS / UNSUPPORTED_LANGUAGE / ...
        QString errorMessage;
        QStringList createdDirs;
        QStringList createdFiles;
    };

    /**
     * @brief 按语言生成项目骨架
     */
    static CreateResult create(const CreateOptions& options);

private:
    static CreateResult createPython(const CreateOptions& opts);
    static CreateResult createCpp(const CreateOptions& opts);
    static CreateResult createGo(const CreateOptions& opts);
    static CreateResult createRust(const CreateOptions& opts);
    static CreateResult createNode(const CreateOptions& opts);
    static CreateResult createGeneric(const CreateOptions& opts);

    // ---- 工具方法 ----
    static bool writeTextFile(const QString& filePath, const QString& content);
    static bool mkpath(const QString& dirPath);
    static QString buildConfigFileContent(const CreateOptions& opts);
    static QString buildDefaultEntryContent(const QString& language,
                                            const QString& projectName);
};

} // namespace SprintIDE

#endif // PROJECT_SCAFFOLDER_H