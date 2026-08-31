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

private:
    QString findMavenExecutable() const;
    QString findJavaExecutable() const;
    QString extractMainClassFromPom(const QString& projectPath) const;
    QString findJarFile(const QString& projectPath) const;
    bool runJavaProgram(const QString& mainClass, const BuildTask& task, QString& output);
};

} // namespace SprintIDE

#endif // JAVA_MAVEN_BUILDER_H
