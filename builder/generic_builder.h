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
#ifndef GENERIC_BUILDER_H
#define GENERIC_BUILDER_H

#include "ibuilder.h"
#include <QVariantMap>

namespace SprintIDE {

/**
 * @brief AI 通用适配器，从 BuildTask.aiCustomPayload 读取命令执行
 */
class GenericBuilder : public IBuilder {
public:
    QProcessEnvironment prepareEnvironment(const BuildTask& task) override;
    bool build(const BuildTask& task, BuildResult& result) override;
    bool clean(const BuildTask& task, BuildResult& result) override;
    bool run(const BuildTask& task, const BuildResult& buildResult, QString& output) override;

    /**
     * @brief 保存 AI 生成的通用配置到项目配置
     * @param projectPath 项目路径
     * @param config 配置映射（包含 commands, env_vars 等）
     * @return 是否成功
     */
    static bool saveGenericConfig(const QString& projectPath, const QVariantMap& config);

    /**
     * @brief 安装 SDK（执行安装脚本，并更新项目 SDK 环境）
     * @param toolName 工具名称（如 "openjdk"）
     * @param version 版本
     * @param installScript 安装命令（如 "brew install openjdk"）
     * @return 是否成功
     */
    static bool installSdk(const QString& toolName, const QString& version, const QString& installScript);

    QString getExecutablePath(const BuildTask& task) override;
private:
    /**
     * @brief 执行通用命令（内部辅助）
     * @param cmdMap 命令配置（executable, args, shell, work_dir 等）
     * @param task 原始任务
     * @param result 输出结果
     * @param commandType 用于日志（"build"/"clean"/"run"）
     * @return 是否成功启动（具体结果看 result）
     */
    bool executeGenericCommand(const QVariantMap& cmdMap, const BuildTask& task,
                               BuildResult& result, const QString& commandType);
};

} // namespace SprintIDE

#endif // GENERIC_BUILDER_H
