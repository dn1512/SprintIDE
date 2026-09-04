#include "project_resolver.h"
#include "maven_project_model.h"
#include "gradle_project_model.h"
#include "plain_java_project_model.h"
#include "../path_utils.h"
#include <QDir>
#include <QFile>
#include <QDebug>

namespace SprintIDE {

std::unique_ptr<ProjectModel> ProjectResolver::resolveProject(const QString& projectPath)
{
    if (projectPath.isEmpty() || !QDir(projectPath).exists()) {
        return nullptr;
    }

    // 检测 Maven
    if (QFile::exists(PathUtils::joinPaths({projectPath, "pom.xml"}))) {
        qDebug() << "[ProjectResolver] Detected Maven project:" << projectPath;
        return std::make_unique<MavenProjectModel>(projectPath);
    }

    // 检测 Gradle（存在 gradlew 或 build.gradle）
    bool hasGradleWrapper = QFile::exists(PathUtils::joinPaths({projectPath, "gradlew"})) ||
                            QFile::exists(PathUtils::joinPaths({projectPath, "gradlew.bat"}));
    bool hasGradleFile = QFile::exists(PathUtils::joinPaths({projectPath, "build.gradle"})) ||
                         QFile::exists(PathUtils::joinPaths({projectPath, "build.gradle.kts"}));
    if (hasGradleWrapper || hasGradleFile) {
        qDebug() << "[ProjectResolver] Detected Gradle project:" << projectPath;
        return std::make_unique<GradleProjectModel>(projectPath);
    }

    // 否则视为 Plain Java 项目
    qDebug() << "[ProjectResolver] Detected plain Java project:" << projectPath;
    return std::make_unique<PlainJavaProjectModel>(projectPath);
}

} // namespace SprintIDE
