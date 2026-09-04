#include "java_builder.h"
#include "../path_utils.h"
#include <QFile>
#include <QDir>
#include <config_manager.h>
#include <Application.h>

namespace SprintIDE {

QProcessEnvironment JavaBuilder::prepareEnvironment(const BuildTask& task) {
    IBuilder* delegate = getDelegate(task);
    if (delegate) {
        return delegate->prepareEnvironment(task);
    }
    // 如果没有检测到任何构建工具，返回默认环境（仅全局 SDK）
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    ConfigManager* cfg = Application::instance().configManager();
    if (cfg) {
        QVariantMap sdkEnv = cfg->value("Sdk/Environment").toMap();
        for (auto it = sdkEnv.begin(); it != sdkEnv.end(); ++it) {
            env.insert(it.key(), it.value().toString());
        }
    }
    return env;
}

bool JavaBuilder::build(const BuildTask& task, BuildResult& result) {
    BuildTask mutableTask = task;

    // 检测 Spring Boot（如果是 Maven 项目）
    if (isMavenProject(mutableTask.projectPath)) {
        mutableTask.isSpringBoot = JavaMavenBuilder::isSpringBootProject(mutableTask.projectPath);
    }

    // Gradle 检测暂不实现，后续可扩展

    // 先解析依赖，填充 classpath
    if (mutableTask.classpath.isEmpty()) {
        // 直接传入 mutableTask，resolveDependencies 会填充其 classpath 和 dependencyJars
        if (!resolveDependencies(mutableTask)) {
            result.errorMessage = "Failed to resolve dependencies";
            result.builderName = "JavaBuilder";
            return false;
        }
        // 此时 mutableTask.classpath 和 mutableTask.dependencyJars 已被填充
    }
    // 然后获取具体 Builder 并执行
    IBuilder* delegate = getDelegate(mutableTask);
    if (!delegate) {
        result.errorMessage = "No Java build tool detected (Gradle or Maven)";
        result.builderName = "JavaBuilder";
        return false;
    }
    return delegate->build(mutableTask, result);
}

bool JavaBuilder::clean(const BuildTask& task, BuildResult& result) {
    IBuilder* delegate = getDelegate(task);
    if (!delegate) {
        result.errorMessage = "No Java build tool detected";
        result.builderName = "JavaBuilder";
        return false;
    }
    return delegate->clean(task, result);
}

bool JavaBuilder::run(const BuildTask& task, const BuildResult& buildResult, QString& output) {
    IBuilder* delegate = getDelegate(task);
    if (!delegate) {
        output = "No Java build tool detected";
        return false;
    }
    return delegate->run(task, buildResult, output);
}

QString JavaBuilder::getExecutablePath(const BuildTask& task)
{
    IBuilder* delegate = getDelegate(task);
    if (delegate) {
        return delegate->getExecutablePath(task);
    }

    // 没有检测到构建工具，尝试查找 JAR
    QString projectPath = task.projectPath;
    if (projectPath.isEmpty() && !task.sourceFiles.isEmpty()) {
        projectPath = QFileInfo(task.sourceFiles.first()).absolutePath();
    }

    // 尝试在 target/ 和 build/libs/ 中查找
    QStringList searchDirs = {
        PathUtils::joinPaths({projectPath, "target"}),
        PathUtils::joinPaths({projectPath, "build", "libs"})
    };

    for (const QString& dirPath : searchDirs) {
        QDir dir(dirPath);
        if (dir.exists()) {
            QStringList jars = dir.entryList({"*.jar"}, QDir::Files);
            if (!jars.isEmpty()) {
                return dir.absoluteFilePath(jars.first());
            }
        }
    }

    return QString();
}

bool JavaBuilder::resolveDependencies(BuildTask& task) const {
    if (task.projectPath.isEmpty()) {
        qWarning() << "resolveDependencies: projectPath is empty";
        return false;
    }
    // 检测项目类型
    if (isGradleProject(task.projectPath)) {
        JavaGradleBuilder gradleBuilder;
        // 调用 Gradle 的依赖解析（需要新增一个方法）
        QStringList cp;
        if (gradleBuilder.resolveClasspath(task.projectPath, cp)) {
            task.classpath = cp;
            return true;
        }
        return false;
    } else if (isMavenProject(task.projectPath)) {
        JavaMavenBuilder mavenBuilder;
        QStringList cp;
        if (mavenBuilder.resolveClasspath(task.projectPath, cp)) {
            task.classpath = cp;
            return true;
        }
        return false;
    }
    // 没有构建工具，可能是 Plain Java，尝试扫描 lib
    // 这里简单处理，如果存在 lib 目录，添加其中的 jar
    QDir libDir(task.projectPath + "/lib");
    if (libDir.exists()) {
        QStringList jars = libDir.entryList({"*.jar"}, QDir::Files);
        for (const QString& jar : jars) {
            task.dependencyJars.append(libDir.absoluteFilePath(jar));
        }
        task.classpath = task.dependencyJars;
        // 添加编译输出目录（通常是 out 或 target/classes）
        if (QDir(task.projectPath + "/out").exists()) {
            task.classpath.append(task.projectPath + "/out");
        } else if (QDir(task.projectPath + "/target/classes").exists()) {
            task.classpath.append(task.projectPath + "/target/classes");
        }
        return true;
    }
    // 否则无法解析
    return false;
}

// ==================== 私有辅助 ====================

bool JavaBuilder::isGradleProject(const QString& projectPath) const {
    if (projectPath.isEmpty()) return false;
    // 检查 gradlew 或 gradlew.bat
#ifdef Q_OS_WIN
    QString gradlew = "gradlew.bat";
#else
    QString gradlew = "gradlew";
#endif
    QString gradlewPath = PathUtils::joinPaths({projectPath, gradlew});
    if (QFile::exists(gradlewPath)) return true;

    // 检查 build.gradle 或 build.gradle.kts
    QString buildGradle = PathUtils::joinPaths({projectPath, "build.gradle"});
    if (QFile::exists(buildGradle)) return true;
    QString buildGradleKts = PathUtils::joinPaths({projectPath, "build.gradle.kts"});
    if (QFile::exists(buildGradleKts)) return true;

    return false;
}

bool JavaBuilder::isMavenProject(const QString& projectPath) const {
    if (projectPath.isEmpty()) return false;
    QString pomXml = PathUtils::joinPaths({projectPath, "pom.xml"});
    return QFile::exists(pomXml);
}

IBuilder* JavaBuilder::getDelegate(const BuildTask& task) const {
    QString projectPath = task.projectPath;
    if (projectPath.isEmpty()) {
        // 如果任务没有指定项目路径，尝试从源文件推断
        if (!task.sourceFiles.isEmpty()) {
            QFileInfo fi(task.sourceFiles.first());
            projectPath = fi.absolutePath();
        }
    }

    // 检测 Gradle
    if (isGradleProject(projectPath)) {
        // 可以在这里设置一些 task 特定的参数（如传递环境变量），但不用
        return const_cast<JavaGradleBuilder*>(&m_gradleBuilder);
    }

    // 检测 Maven
    if (isMavenProject(projectPath)) {
        return const_cast<JavaMavenBuilder*>(&m_mavenBuilder);
    }

    // 都没有
    return nullptr;
}

} // namespace SprintIDE
