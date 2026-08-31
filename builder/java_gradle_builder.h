// src/build/java_gradle_builder.h
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
