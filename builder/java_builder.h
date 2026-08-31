#ifndef JAVA_BUILDER_H
#define JAVA_BUILDER_H

#include "ibuilder.h"
#include "java_gradle_builder.h"
#include "java_maven_builder.h"

namespace SprintIDE {

/**
 * @brief Java 构建器 - 自动检测使用 Gradle 还是 Maven
 *
 * 根据项目根目录中是否存在 gradlew 或 pom.xml 自动选择合适的构建工具。
 */
class JavaBuilder : public IBuilder {
public:
    JavaBuilder() = default;
    ~JavaBuilder() override = default;

    // 继承自 IBuilder
    QProcessEnvironment prepareEnvironment(const BuildTask& task) override;
    bool build(const BuildTask& task, BuildResult& result) override;
    bool clean(const BuildTask& task, BuildResult& result) override;
    bool run(const BuildTask& task, const BuildResult& buildResult, QString& output) override;

private:
    /**
     * @brief 检测项目是否使用 Gradle
     * @param projectPath 项目根目录
     * @return true 如果存在 gradlew 或 gradlew.bat 或 build.gradle
     */
    bool isGradleProject(const QString& projectPath) const;

    /**
     * @brief 检测项目是否使用 Maven
     * @param projectPath 项目根目录
     * @return true 如果存在 pom.xml
     */
    bool isMavenProject(const QString& projectPath) const;

    /**
     * @brief 根据项目类型返回对应的 IBuilder 实例
     * @param task 构建任务（用于检测）
     * @return 对应的构建器指针（由本类持有，无需 delete）
     */
    IBuilder* getDelegate(const BuildTask& task) const;

    // 两个具体的构建器实例（作为成员，由本类管理生命周期）
    JavaGradleBuilder m_gradleBuilder;
    JavaMavenBuilder   m_mavenBuilder;
};

} // namespace SprintIDE

#endif // JAVA_BUILDER_H
