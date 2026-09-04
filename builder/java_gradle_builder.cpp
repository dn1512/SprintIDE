// src/build/java_gradle_builder.cpp
#include "java_gradle_builder.h"
#include "../process_manager.h"
#include "../path_utils.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QRegularExpression>
#include <config_manager.h>
#include <Application.h>

namespace SprintIDE {

// ==================== prepareEnvironment ====================
QProcessEnvironment JavaGradleBuilder::prepareEnvironment(const BuildTask& task) {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    // 从全局配置加载 SDK 环境变量
    ConfigManager* cfg = Application::instance().configManager();
    if (cfg) {
        QVariantMap sdkEnv = cfg->value("Sdk/Environment").toMap();
        for (auto it = sdkEnv.begin(); it != sdkEnv.end(); ++it) {
            env.insert(it.key(), it.value().toString());
        }
    }

    // 合并任务中的 sdkConfig
    for (auto it = task.sdkConfig.begin(); it != task.sdkConfig.end(); ++it) {
        env.insert(it.key(), it.value().toString());
    }

    // 如果设置了 JAVA_HOME，将其 bin 目录加入 PATH
    QString javaHome = env.value("JAVA_HOME");
    if (!javaHome.isEmpty()) {
        QString javaBin = PathUtils::joinPaths({javaHome, "bin"});
        QString path = env.value("PATH");
        env.insert("PATH", javaBin + ":" + path);
    }

    // 如果设置了 GRADLE_HOME，将其 bin 目录加入 PATH
    QString gradleHome = env.value("GRADLE_HOME");
    if (!gradleHome.isEmpty()) {
        QString gradleBin = PathUtils::joinPaths({gradleHome, "bin"});
        QString path = env.value("PATH");
        env.insert("PATH", gradleBin + ":" + path);
    }

    return env;
}

// ==================== build ====================
bool JavaGradleBuilder::build(const BuildTask& task, BuildResult& result) {
    QString gradlew = getGradleWrapperPath(task.projectPath);
    if (gradlew.isEmpty()) {
        result.errorMessage = "gradlew not found in project root";
        result.builderName = "JavaGradleBuilder";
        return false;
    }

    QStringList args;
    // 如果 extraArgs 有内容，优先使用；否则默认 build -x test
    if (!task.extraArgs.isEmpty()) {
        args = task.extraArgs;
    } else {
        args << "build" << "-x" << "test";
    }

    QProcessEnvironment env = prepareEnvironment(task);
    ProcessManager pm;
    pm.setEnvironment(env);

    ProcessManager::ProcessResult pr = pm.executeSync(gradlew, args, task.projectPath, 120000);

    result.exitCode = pr.exitCode;
    result.fullOutput = pr.standardOutput + "\n" + pr.standardError;
    result.elapsedMs = pr.executionTime;
    result.success = pr.success && pr.exitCode == 0;
    result.builderName = "JavaGradleBuilder";

    if (!result.success) {
        result.errorLines = pr.standardError.split('\n', Qt::SkipEmptyParts);
        result.errorLines += pr.standardOutput.split('\n', Qt::SkipEmptyParts);
    } else {
        // 提取警告
        for (const QString& line : pr.standardOutput.split('\n', Qt::SkipEmptyParts)) {
            if (line.contains("warning")) {
                result.warningLines.append(line);
            }
        }
        // 查找可执行文件（JAR）
        result.executablePath = getExecutablePath(task);
    }

    return true;
}

// ==================== clean ====================
bool JavaGradleBuilder::clean(const BuildTask& task, BuildResult& result) {
    QString gradlew = getGradleWrapperPath(task.projectPath);
    if (gradlew.isEmpty()) {
        result.errorMessage = "gradlew not found";
        result.builderName = "JavaGradleBuilder";
        return false;
    }

    ProcessManager pm;
    pm.setEnvironment(prepareEnvironment(task));
    ProcessManager::ProcessResult pr = pm.executeSync(gradlew, {"clean"}, task.projectPath, 30000);

    result.exitCode = pr.exitCode;
    result.fullOutput = pr.standardOutput + "\n" + pr.standardError;
    result.elapsedMs = pr.executionTime;
    result.success = pr.success && pr.exitCode == 0;
    result.builderName = "JavaGradleBuilder";

    if (!result.success) {
        result.errorLines = pr.standardError.split('\n', Qt::SkipEmptyParts);
    }
    return true;
}

// ==================== run ====================
bool JavaGradleBuilder::run(const BuildTask& task, const BuildResult& buildResult, QString& output) {
    // 优先使用 aiCustomPayload 中的 mainClass
    QString mainClass = task.aiCustomPayload["mainClass"].toString();
    if (mainClass.isEmpty()) {
        // 尝试从 build.gradle 解析
        mainClass = extractMainClass(task.projectPath);
    }

    // 如果有 mainClass，使用 java -cp 运行
    if (!mainClass.isEmpty()) {
        // 查找 Java 可执行文件
        QString javaExe = findJavaExecutable();
        if (javaExe.isEmpty()) {
            output = "Java executable not found (set JAVA_HOME)";
            return false;
        }

        // 查找 classpath（通常是 build/classes/java/main 和 build/resources/main）
        QString classesDir = PathUtils::joinPaths({task.projectPath, "build", "classes", "java", "main"});
        QString resourcesDir = PathUtils::joinPaths({task.projectPath, "build", "resources", "main"});
        QString classpath = classesDir + ":" + resourcesDir;
        // 也可以加入依赖的 jars，但简化处理

        // 准备环境
        QProcessEnvironment env = prepareEnvironment(task);

        ProcessManager pm;
        pm.setEnvironment(env);
        ProcessManager::ProcessResult pr = pm.executeSync(javaExe,
                                                          {"-cp", classpath, mainClass},
                                                          task.projectPath,
                                                          0);
        output = pr.standardOutput + "\n" + pr.standardError;
        return pr.success && pr.exitCode == 0;
    }

    // 否则，尝试使用 gradlew run
    QString gradlew = getGradleWrapperPath(task.projectPath);
    if (gradlew.isEmpty()) {
        output = "gradlew not found and mainClass not specified";
        return false;
    }

    ProcessManager pm;
    pm.setEnvironment(prepareEnvironment(task));
    ProcessManager::ProcessResult pr = pm.executeSync(gradlew, {"run"}, task.projectPath, 0);
    output = pr.standardOutput + "\n" + pr.standardError;
    return pr.success && pr.exitCode == 0;
}

// ==================== 私有辅助 ====================
QString JavaGradleBuilder::getGradleWrapperPath(const QString& projectPath) const {
    // Windows 使用 gradlew.bat，其他使用 gradlew
#ifdef Q_OS_WIN
    QString wrapper = "gradlew.bat";
#else
    QString wrapper = "gradlew";
#endif
    QString fullPath = PathUtils::joinPaths({projectPath, wrapper});
    if (QFile::exists(fullPath)) {
        // 确保可执行权限（Unix）
#ifdef Q_OS_UNIX
        QFile file(fullPath);
        file.setPermissions(file.permissions() | QFileDevice::ExeUser | QFileDevice::ExeGroup | QFileDevice::ExeOther);
#endif
        return fullPath;
    }
    return QString();
}

QString JavaGradleBuilder::extractMainClass(const QString& projectPath) const {
    // 读取 build.gradle 或 build.gradle.kts
    QStringList possibleFiles = { "build.gradle", "build.gradle.kts" };
    for (const QString& fileName : possibleFiles) {
        QString filePath = PathUtils::joinPaths({projectPath, fileName});
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            QString content = stream.readAll();
            file.close();

            // 查找 mainClassName = '...' 或 mainClass = '...'
            QRegularExpression regex(R"(mainClassName\s*=\s*['\"]([^'\"]+)['\"])");
            QRegularExpressionMatch match = regex.match(content);
            if (match.hasMatch()) {
                return match.captured(1);
            }
            // 尝试 application 插件中的 mainClass
            QRegularExpression regex2(R"(mainClass\s*=\s*['\"]([^'\"]+)['\"])");
            QRegularExpressionMatch match2 = regex2.match(content);
            if (match2.hasMatch()) {
                return match2.captured(1);
            }
        }
    }
    return QString();
}

QString JavaGradleBuilder::findJavaExecutable() const {
    // 优先使用 JAVA_HOME
    QString javaHome = QProcessEnvironment::systemEnvironment().value("JAVA_HOME");
    if (!javaHome.isEmpty()) {
        QString javaExe = PathUtils::joinPaths({javaHome, "bin", "java"});
#ifdef Q_OS_WIN
        javaExe += ".exe";
#endif
        if (QFile::exists(javaExe)) {
            return javaExe;
        }
    }
    // 尝试 PATH 中的 java
    QString javaCmd = "java";
#ifdef Q_OS_WIN
    javaCmd += ".exe";
#endif
    ProcessManager pm;
    ProcessManager::ProcessResult pr = pm.executeSync(javaCmd, {"-version"}, QString(), 2000);
    if (pr.success) {
        return javaCmd;
    }
    return QString();
}

QString JavaGradleBuilder::getExecutablePath(const BuildTask& task)
{
    // Gradle 项目的 JAR 文件通常在 build/libs/ 目录下
    QString projectPath = task.projectPath;
    if (projectPath.isEmpty() && !task.sourceFiles.isEmpty()) {
        projectPath = QFileInfo(task.sourceFiles.first()).absolutePath();
    }

    QDir libsDir(PathUtils::joinPaths({projectPath, "build", "libs"}));
    if (!libsDir.exists()) {
        return QString();
    }

    // 查找 JAR 文件（优先选择包含项目名的文件）
    QStringList jarFiles = libsDir.entryList({"*.jar"}, QDir::Files);
    if (jarFiles.isEmpty()) {
        return QString();
    }

    // 优先选择与项目名匹配的 JAR
    QString projectName = task.outputName;
    if (projectName.isEmpty()) {
        projectName = QFileInfo(projectPath).fileName();
    }

    for (const QString& jar : jarFiles) {
        if (jar.contains(projectName, Qt::CaseInsensitive)) {
            return libsDir.absoluteFilePath(jar);
        }
    }

    // 否则返回第一个 JAR
    return libsDir.absoluteFilePath(jarFiles.first());
}

bool JavaGradleBuilder::resolveClasspath(const QString& projectPath, QStringList& classpath) const {
    QString gradlew = getGradleWrapperPath(projectPath);
    if (gradlew.isEmpty()) {
        qWarning() << "gradlew not found";
        return false;
    }

    // 执行 gradlew -q dependencies --configuration compileClasspath
    ProcessManager pm;
    QProcessEnvironment env = prepareEnvironmentForPath(projectPath); // 需单独实现
    pm.setEnvironment(env);
    QStringList args = {"-q", "dependencies", "--configuration", "compileClasspath"};
    ProcessManager::ProcessResult result = pm.executeSync(gradlew, args, projectPath, 60000);

    if (!result.success || result.exitCode != 0) {
        qWarning() << "Gradle dependency resolution failed:" << result.errorMessage;
        return false;
    }

    // 解析输出提取 jar 路径
    QRegularExpression jarRegex(R"(([^ ]+\.jar))");
    QString output = result.standardOutput;
    QRegularExpressionMatchIterator it = jarRegex.globalMatch(output);
    QStringList jars;
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString jar = match.captured(1);
        if (QFile::exists(jar)) {
            jars.append(jar);
        }
    }
    classpath = QSet<QString>(jars.begin(), jars.end()).values();
    // 添加编译输出目录
    QString classesDir = PathUtils::joinPaths({projectPath, "build", "classes", "java", "main"});
    if (QFile::exists(classesDir)) {
        classpath.prepend(classesDir);
    }
    return true;
}

QProcessEnvironment JavaGradleBuilder::prepareEnvironmentForPath(const QString& projectPath) const
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    // 从全局配置加载 SDK 环境变量
    ConfigManager* cfg = Application::instance().configManager();
    if (cfg) {
        QVariantMap sdkEnv = cfg->value("Sdk/Environment").toMap();
        for (auto it = sdkEnv.begin(); it != sdkEnv.end(); ++it) {
            env.insert(it.key(), it.value().toString());
        }
    }

    // 如果设置了 JAVA_HOME，将其 bin 目录加入 PATH
    QString javaHome = env.value("JAVA_HOME");
    if (!javaHome.isEmpty()) {
        QString javaBin = PathUtils::joinPaths({javaHome, "bin"});
        QString path = env.value("PATH");
        env.insert("PATH", javaBin + ":" + path);
    }

    // 如果设置了 GRADLE_HOME，将其 bin 目录加入 PATH
    QString gradleHome = env.value("GRADLE_HOME");
    if (!gradleHome.isEmpty()) {
        QString gradleBin = PathUtils::joinPaths({gradleHome, "bin"});
        QString path = env.value("PATH");
        env.insert("PATH", gradleBin + ":" + path);
    }

    return env;
}

} // namespace SprintIDE
