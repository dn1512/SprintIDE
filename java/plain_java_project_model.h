#pragma once

#include "project_model.h"

namespace SprintIDE {

class PlainJavaProjectModel : public ProjectModel {
public:
    explicit PlainJavaProjectModel(const QString& projectPath);
    ~PlainJavaProjectModel() = default;

    QString projectPath() const override { return m_projectPath; }
    QString projectName() const override { return m_projectName; }
    JavaProjectType type() const override { return JavaProjectType::Plain; }
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
    void scanSourcesAndLibs();

    QString m_projectPath;
    QString m_projectName;
    QString m_jdkPath;
    QString m_languageLevel = "17";
    QStringList m_sourceDirs = {"src"};
    QStringList m_resourceDirs = {"resources"};
    QString m_outputDir = "out";
    QString m_testOutputDir = "out/test";
    QStringList m_dependencyJars;
    QString m_mainClass;
    QStringList m_jvmArgs;
    QStringList m_programArgs;
    QVariantMap m_env;
};

} // namespace SprintIDE
