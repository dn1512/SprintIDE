#pragma once

#include "project_model.h"
#include <QXmlStreamReader>

namespace SprintIDE {

class MavenProjectModel : public ProjectModel {
public:
    explicit MavenProjectModel(const QString& projectPath);
    ~MavenProjectModel() = default;

    // 重写 ProjectModel 接口
    QString projectPath() const override { return m_projectPath; }
    QString projectName() const override { return m_projectName; }
    JavaProjectType type() const override { return JavaProjectType::Maven; }
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
    bool parsePom();                   // 解析 pom.xml
    void resolveDependencies();        // 调用 mvn dependency:build-classpath
    void detectJavaVersion();          // 从 JAVA_HOME 或配置读取

    QString m_projectPath;
    QString m_projectName;
    QString m_jdkPath;
    QString m_languageLevel = "17";
    QStringList m_sourceDirs = {"src/main/java"};
    QStringList m_resourceDirs = {"src/main/resources"};
    QString m_outputDir = "target/classes";
    QString m_testOutputDir = "target/test-classes";
    QStringList m_dependencyJars;
    QString m_mainClass;
    QStringList m_jvmArgs;
    QStringList m_programArgs;
    QVariantMap m_env;

    bool m_parsed = false;
};

} // namespace SprintIDE
