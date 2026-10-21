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
#pragma once

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QVariantMap>

namespace SprintIDE {

// 项目类型
enum class JavaProjectType {
    Plain,      // 无构建工具
    Maven,
    Gradle
};

// 项目模型抽象基类
class ProjectModel {
public:
    virtual ~ProjectModel() = default;

    // 基本信息
    virtual QString projectPath() const = 0;
    virtual QString projectName() const = 0;
    virtual JavaProjectType type() const = 0;
    virtual QString jdkPath() const = 0;
    virtual QString languageLevel() const = 0;  // "8", "11", "17", "21"

    // 源码与输出
    virtual QStringList sourceDirectories() const = 0;   // 如 src/main/java
    virtual QStringList resourceDirectories() const = 0; // src/main/resources
    virtual QString outputDirectory() const = 0;         // target/classes 或 build/classes
    virtual QString testOutputDirectory() const = 0;

    // 依赖
    virtual QStringList dependencyJars() const = 0;      // 所有编译依赖的jar路径（绝对）
    virtual QStringList classpath() const = 0;           // 完整的类路径（包含输出目录）
    virtual QStringList testClasspath() const = 0;

    // 运行相关
    virtual QString defaultMainClass() const = 0;        // 自动检测的主类
    virtual QStringList jvmArgs() const = 0;
    virtual QStringList programArgs() const = 0;
    virtual QVariantMap environmentVariables() const = 0;

    // 是否支持 Fat JAR（Spring Boot 等）
    virtual bool isFatJar() const { return false; }
    virtual QString fatJarPath() const { return QString(); }
};

} // namespace SprintIDE
