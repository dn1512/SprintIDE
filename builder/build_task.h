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
#ifndef BUILD_TASK_H
#define BUILD_TASK_H

#include <QString>
#include <QStringList>
#include <QVariantMap>
#include "language/language_type.h"

namespace SprintIDE {

/**
 * @brief 通用构建任务结构体
 */
struct BuildTask {
    QString taskId;                     ///< 唯一任务标识
    LanguageType language;              ///< 语言类型
    QString projectPath;                ///< 项目根目录（绝对路径）
    QStringList sourceFiles;            ///< 源文件列表（绝对或相对项目根目录）
    QString outputName;                 ///< 输出文件名（不含路径）
    QString outputDir;                  ///< 输出目录（相对项目根目录）
    bool enableDebug = false;            ///< 是否启用调试信息
    QStringList classpath;              /// 编译/运行的完整类路径（Java）
    QStringList dependencyJars;         /// 依赖的JAR文件列表（Java）

    QVariantMap sdkConfig;              ///< SDK 环境变量（如 JAVA_HOME）
    QVariantMap aiCustomPayload;        ///< AI 通用配置负载（GenericBuilder 专用）
    QStringList extraArgs;              ///< 额外命令行参数


    bool isSpringBoot = false;          /// 是否为 Spring Boot 项目
    bool isWar = false;                 /// 是否为 WAR 项目（仅占位，暂不实现）

    // Java 调试专用
    int debugPort = 5005;               /// JDWP 调试端口
    bool suspendOnStart = true;         /// 启动时是否暂停等待调试器连接
};

} // namespace SprintIDE

#endif // BUILD_TASK_H
