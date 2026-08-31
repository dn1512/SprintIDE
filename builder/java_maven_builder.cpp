#include "java_maven_builder.h"
#include "../process_manager.h"
#include "../path_utils.h"
#include "../config_manager.h"
#include "../application.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QRegularExpression>

namespace SprintIDE {

QProcessEnvironment JavaMavenBuilder::prepareEnvironment(const BuildTask& task) {
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

    // 如果设置了 MAVEN_HOME，将其 bin 目录加入 PATH
    QString mavenHome = env.value("MAVEN_HOME");
    if (!mavenHome.isEmpty()) {
        QString mavenBin = PathUtils::joinPaths({mavenHome, "bin"});
        QString path = env.value("PATH");
        env.insert("PATH", mavenBin + ":" + path);
    }

    return env;
}

bool JavaMavenBuilder::build(const BuildTask& task, BuildResult& result) {
    if (task.projectPath.isEmpty()) {
        result.errorMessage = "Project path is empty";
        result.builderName = "JavaMavenBuilder";
        return false;
    }

    // 检查 pom.xml
    QString pomFile = PathUtils::joinPaths({task.projectPath, "pom.xml"});
    if (!QFile::exists(pomFile)) {
        result.errorMessage = "pom.xml not found in project root";
        result.builderName = "JavaMavenBuilder";
        return false;
    }

    // 查找 mvn
    QString mvn = findMavenExecutable();
    if (mvn.isEmpty()) {
        result.errorMessage = "Maven (mvn) not found. Please set MAVEN_HOME.";
        result.builderName = "JavaMavenBuilder";
        return false;
    }

    // 构建命令：mvn clean compile
    QStringList args;
    args << "clean" << "compile";
    // 跳过测试
    args << "-DskipTests";
    args << task.extraArgs;

    // 准备环境
    QProcessEnvironment env = prepareEnvironment(task);

    ProcessManager pm;
    pm.setEnvironment(env);
    ProcessManager::ProcessResult pr = pm.executeSync(mvn, args, task.projectPath, 120000);

    result.exitCode = pr.exitCode;
    result.fullOutput = pr.standardOutput + "\n" + pr.standardError;
    result.elapsedMs = pr.executionTime;
    result.success = pr.success && pr.exitCode == 0;
    result.builderName = "JavaMavenBuilder";

    if (!result.success) {
        result.errorLines = pr.standardError.split('\n', Qt::SkipEmptyParts);
        result.errorLines += pr.standardOutput.split('\n', Qt::SkipEmptyParts);
    }

    return true;
}

bool JavaMavenBuilder::clean(const BuildTask& task, BuildResult& result) {
    QString mvn = findMavenExecutable();
    if (mvn.isEmpty()) {
        result.errorMessage = "Maven (mvn) not found";
        result.builderName = "JavaMavenBuilder";
        return false;
    }

    ProcessManager pm;
    pm.setEnvironment(prepareEnvironment(task));
    ProcessManager::ProcessResult pr = pm.executeSync(mvn, {"clean"}, task.projectPath, 30000);

    result.exitCode = pr.exitCode;
    result.fullOutput = pr.standardOutput + "\n" + pr.standardError;
    result.elapsedMs = pr.executionTime;
    result.success = pr.success && pr.exitCode == 0;
    result.builderName = "JavaMavenBuilder";

    if (!result.success) {
        result.errorLines = pr.standardError.split('\n', Qt::SkipEmptyParts);
    }
    return true;
}

bool JavaMavenBuilder::run(const BuildTask& task, const BuildResult& buildResult, QString& output) {
    // 优先使用 aiCustomPayload 中的 mainClass
    QString mainClass = task.aiCustomPayload["mainClass"].toString();
    if (mainClass.isEmpty()) {
        mainClass = extractMainClassFromPom(task.projectPath);
    }

    // 如果有 mainClass，使用 java -cp 运行
    if (!mainClass.isEmpty()) {
        return runJavaProgram(mainClass, task, output);
    }

    // 查找生成的 jar 文件
    QString jarFile = findJarFile(task.projectPath);
    if (!jarFile.isEmpty()) {
        QString javaExe = findJavaExecutable();
        if (!javaExe.isEmpty()) {
            ProcessManager pm;
            pm.setEnvironment(prepareEnvironment(task));
            ProcessManager::ProcessResult pr = pm.executeSync(javaExe,
                                                              {"-jar", jarFile},
                                                              task.projectPath,
                                                              0);
            output = pr.standardOutput + "\n" + pr.standardError;
            return pr.success && pr.exitCode == 0;
        }
    }

    output = "No mainClass specified and no executable jar found";
    return false;
}

// ==================== 私有辅助 ====================

QString JavaMavenBuilder::findMavenExecutable() const {
    QString mvnCmd = "mvn";
#ifdef Q_OS_WIN
    mvnCmd += ".cmd";
#endif

    // 先检查环境变量 MAVEN_HOME
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString mavenHome = env.value("MAVEN_HOME");
    if (!mavenHome.isEmpty()) {
        QString mvnPath = PathUtils::joinPaths({mavenHome, "bin", mvnCmd});
        if (QFile::exists(mvnPath)) return mvnPath;
    }

    // 检查 PATH
    ProcessManager pm;
    ProcessManager::ProcessResult pr = pm.executeSync(mvnCmd, {"--version"}, QString(), 2000);
    if (pr.success) return mvnCmd;

    return QString();
}

QString JavaMavenBuilder::findJavaExecutable() const {
    QString javaHome = QProcessEnvironment::systemEnvironment().value("JAVA_HOME");
    if (!javaHome.isEmpty()) {
        QString javaExe = PathUtils::joinPaths({javaHome, "bin", "java"});
#ifdef Q_OS_WIN
        javaExe += ".exe";
#endif
        if (QFile::exists(javaExe)) return javaExe;
    }
    return "java";
}

QString JavaMavenBuilder::extractMainClassFromPom(const QString& projectPath) const {
    QString pomFile = PathUtils::joinPaths({projectPath, "pom.xml"});
    if (!QFile::exists(pomFile)) return QString();

    QFile file(pomFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    // 查找 mainClass
    QRegularExpression regex(R"(<mainClass>([^<]+)</mainClass>)");
    QRegularExpressionMatch match = regex.match(content);
    if (match.hasMatch()) {
        return match.captured(1).trimmed();
    }

    // 查找 maven-jar-plugin 的 mainClass
    QRegularExpression regex2(R"(<mainClass>([^<]+)</mainClass>)");
    // 简单搜索
    return QString();
}

QString JavaMavenBuilder::findJarFile(const QString& projectPath) const {
    // 查找 target/*.jar
    QString targetDir = PathUtils::joinPaths({projectPath, "target"});
    QDir dir(targetDir);
    if (!dir.exists()) return QString();

    QStringList filters;
    filters << "*.jar";
    QStringList jars = dir.entryList(filters, QDir::Files);
    if (jars.isEmpty()) return QString();

    // 优先选择不带 "-sources" 和 "-javadoc" 的 jar
    for (const QString& jar : jars) {
        if (!jar.contains("sources") && !jar.contains("javadoc")) {
            return PathUtils::joinPaths({targetDir, jar});
        }
    }

    // 如果没有，取第一个
    return PathUtils::joinPaths({targetDir, jars.first()});
}

bool JavaMavenBuilder::runJavaProgram(const QString& mainClass, const BuildTask& task, QString& output) {
    QString javaExe = findJavaExecutable();
    if (javaExe.isEmpty()) {
        output = "Java executable not found";
        return false;
    }

    // 查找 classpath（target/classes）
    QString classesDir = PathUtils::joinPaths({task.projectPath, "target", "classes"});
    if (!QFile::exists(classesDir)) {
        output = "Classes directory not found. Please build first.";
        return false;
    }

    // 查找依赖的 jar
    QString libDir = PathUtils::joinPaths({task.projectPath, "target", "lib"});
    QString classpath = classesDir;
    if (QFile::exists(libDir)) {
        QDir dir(libDir);
        QStringList jars = dir.entryList(QStringList() << "*.jar", QDir::Files);
        for (const QString& jar : jars) {
            classpath += ":" + PathUtils::joinPaths({libDir, jar});
        }
    }

    ProcessManager pm;
    pm.setEnvironment(prepareEnvironment(task));
    ProcessManager::ProcessResult pr = pm.executeSync(javaExe,
                                                      {"-cp", classpath, mainClass},
                                                      task.projectPath,
                                                      0);
    output = pr.standardOutput + "\n" + pr.standardError;
    return pr.success && pr.exitCode == 0;
}

} // namespace SprintIDE
