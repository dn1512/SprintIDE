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
    IBuilder* delegate = getDelegate(task);
    if (!delegate) {
        result.errorMessage = "No Java build tool detected (Gradle or Maven)";
        result.builderName = "JavaBuilder";
        return false;
    }
    return delegate->build(task, result);
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
