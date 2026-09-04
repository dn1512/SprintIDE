#ifndef JAVA_MAVEN_BUILDER_H
#define JAVA_MAVEN_BUILDER_H

#include "ibuilder.h"

namespace SprintIDE {

/**
 * @brief Java Maven 构建器
 */
class JavaMavenBuilder : public IBuilder {
public:
    QProcessEnvironment prepareEnvironment(const BuildTask& task) override;
    bool build(const BuildTask& task, BuildResult& result) override;
    bool clean(const BuildTask& task, BuildResult& result) override;
    bool run(const BuildTask& task, const BuildResult& buildResult, QString& output) override;
    QString getExecutablePath(const BuildTask& task) override;

    /**
     * @brief 解析 maven 项目的 classpath
     * @param projectPath 项目路径
     * @param classpath 输出：类路径列表
     * @return true 成功
    */
    bool resolveClasspath(const QString& projectPath, QStringList& classpath) const;

    // 检测是否为 Spring Boot 项目（基于 pom.xml）
    static bool isSpringBootProject(const QString& projectPath);

private:
    QString findMavenExecutable() const;
    QString findJavaExecutable() const;
    QString extractMainClassFromPom(const QString& projectPath) const;
    QString findJarFile(const QString& projectPath) const;
    bool runJavaProgram(const QString& mainClass, const BuildTask& task, QString& output);
    /**
     * @brief 为指定项目路径准备环境变量（不依赖 BuildTask）
     * @param projectPath 项目根目录
     * @return 环境变量对象
     */
    QProcessEnvironment prepareEnvironmentForPath(const QString& projectPath) const;
};

} // namespace SprintIDE

#endif // JAVA_MAVEN_BUILDER_H
