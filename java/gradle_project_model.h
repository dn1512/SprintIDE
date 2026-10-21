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

#include "project_model.h"

namespace SprintIDE {

class GradleProjectModel : public ProjectModel {
public:
    explicit GradleProjectModel(const QString& projectPath);
    ~GradleProjectModel() = default;

    QString projectPath() const override { return m_projectPath; }
    QString projectName() const override { return m_projectName; }
    JavaProjectType type() const override { return JavaProjectType::Gradle; }
    QString jdkPath() const override { return m_jdkPath; }
    QString languageLevel() const override { return m_languageLevel; }
    QStringList sourceDirectories() const override { return m_sourceDirs; }
    QStringList resourceDirectories() const override { return m_resourceDirs; }
    QString outputDirectory() const override { return m_outputDir; }
    QString testOutputDirectory() const override { return m_testOutputDir; }
    QStringList dependencyJars() const override { return m_dependencyJars; }
    QStringList classpath() const override;
    QStringList testClasspath() const override;
    QString defaultMainClass() const override { return m_mainClass; }
    QStringList jvmArgs() const override { return m_jvmArgs; }
    QStringList programArgs() const override { return m_programArgs; }
    QVariantMap environmentVariables() const override { return m_env; }

private:
    void parseGradleProperties();
    void resolveDependenciesGradle();

    QString m_projectPath;
    QString m_projectName;
    QString m_jdkPath;
    QString m_languageLevel = "17";
    QStringList m_sourceDirs = {"src/main/java"};
    QStringList m_resourceDirs = {"src/main/resources"};
    QString m_outputDir = "build/classes/java/main";
    QString m_testOutputDir = "build/classes/java/test";
    QStringList m_dependencyJars;
    QString m_mainClass;
    QStringList m_jvmArgs;
    QStringList m_programArgs;
    QVariantMap m_env;
};

} // namespace SprintIDE
