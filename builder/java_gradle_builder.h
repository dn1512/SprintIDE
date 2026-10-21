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
#ifndef JAVA_GRADLE_BUILDER_H
#define JAVA_GRADLE_BUILDER_H

#include "ibuilder.h"

namespace SprintIDE {

/**
 * @brief Java Gradle 构建器，使用项目中的 gradlew 脚本
 */
class JavaGradleBuilder : public IBuilder {
public:
    QProcessEnvironment prepareEnvironment(const BuildTask& task) override;
    bool build(const BuildTask& task, BuildResult& result) override;
    bool clean(const BuildTask& task, BuildResult& result) override;
    bool run(const BuildTask& task, const BuildResult& buildResult, QString& output) override;
    QString getExecutablePath(const BuildTask& task) override;
    /**
     * @brief 解析 Gradle 项目的 classpath
     * @param projectPath 项目路径
     * @param classpath 输出：类路径列表
     * @return true 成功
     */
    bool resolveClasspath(const QString& projectPath, QStringList& classpath) const;

    /**
     * @brief 为指定项目路径准备环境变量（不依赖 BuildTask）
     * @param projectPath 项目根目录
     * @return 环境变量对象
     */
    QProcessEnvironment prepareEnvironmentForPath(const QString& projectPath) const;
private:
    /**
     * @brief 获取 gradlew 可执行文件路径（自动加 .bat 后缀）
     */
    QString getGradleWrapperPath(const QString& projectPath) const;

    /**
     * @brief 解析 build.gradle 获取 mainClassName（如果未在 aiCustomPayload 中指定）
     */
    QString extractMainClass(const QString& projectPath) const;

    /**
     * @brief 查找 Java 可执行文件（优先使用 JAVA_HOME）
     */
    QString findJavaExecutable() const;
};

} // namespace SprintIDE

#endif // JAVA_GRADLE_BUILDER_H
