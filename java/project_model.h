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
