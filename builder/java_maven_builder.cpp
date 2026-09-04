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
#include <QTemporaryFile>
#include <QXmlStreamReader>

#include "../config_manager.h"
#include "../application.h"

namespace SprintIDE {

static bool hasLombokDependency(const QString& projectPath)
{
    QString pomFile = PathUtils::joinPaths({projectPath, "pom.xml"});
    if (!QFile::exists(pomFile)) {
        return false;
    }

    QFile file(pomFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    // 检测 <artifactId>lombok</artifactId>
    return content.contains("<artifactId>lombok</artifactId>");
}

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

    // ========== Lombok 兼容性处理 ==========
    // 检测项目是否使用 Lombok，如果是则设置 MAVEN_OPTS
    if (!task.projectPath.isEmpty() && hasLombokDependency(task.projectPath)) {
        // 现有的 MAVEN_OPTS（如果有）
        QString mavenOpts = env.value("MAVEN_OPTS");

        // 添加 --add-exports 参数（Lombok 在 JDK 17+ 需要）
        QString lombokOpts = "--add-exports=jdk.compiler/com.sun.tools.javac.processing=ALL-UNNAMED "
                             "--add-exports=jdk.compiler/com.sun.tools.javac.tree=ALL-UNNAMED "
                             "--add-exports=jdk.compiler/com.sun.tools.javac.util=ALL-UNNAMED";

        if (mavenOpts.isEmpty()) {
            env.insert("MAVEN_OPTS", lombokOpts);
        } else {
            // 如果已有 MAVEN_OPTS，追加
            env.insert("MAVEN_OPTS", mavenOpts + " " + lombokOpts);
        }
        qDebug() << "[JavaMavenBuilder] Added MAVEN_OPTS for Lombok support";
    }

    return env;
}

// ---- 检测 Spring Boot ----
bool JavaMavenBuilder::isSpringBootProject(const QString& projectPath)
{
    QString pomFile = PathUtils::joinPaths({projectPath, "pom.xml"});
    if (!QFile::exists(pomFile)) {
        return false;
    }

    QFile file(pomFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QXmlStreamReader reader(&file);
    bool hasSpringBootPlugin = false;
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && reader.name() == "plugin") {
            // 读取插件内容，寻找 spring-boot-maven-plugin
            QString pluginContent = reader.readElementText();
            if (pluginContent.contains("spring-boot-maven-plugin")) {
                hasSpringBootPlugin = true;
                break;
            }
        }
    }
    file.close();
    return hasSpringBootPlugin;
}

bool JavaMavenBuilder::build(const BuildTask& task, BuildResult& result)
{
    if (task.projectPath.isEmpty()) {
        result.errorMessage = "Project path is empty";
        result.builderName = "JavaMavenBuilder";
        qWarning() << "[JavaMavenBuilder] " << result.errorMessage;
        return false;
    }

    QString pomFile = PathUtils::joinPaths({task.projectPath, "pom.xml"});
    if (!QFile::exists(pomFile)) {
        result.errorMessage = "pom.xml not found in project root";
        result.builderName = "JavaMavenBuilder";
        qWarning() << "[JavaMavenBuilder] " << result.errorMessage;
        return false;
    }

    // 检测是否为 Spring Boot
    bool isSpringBoot = task.isSpringBoot || isSpringBootProject(task.projectPath);
    qDebug() << "[JavaMavenBuilder] Spring Boot detected:" << isSpringBoot;

    // 查找 mvn 可执行文件
    QString mvnCmd = findMavenExecutable();
    qDebug() << "[JavaMavenBuilder] mvn command:" << mvnCmd;

    if (mvnCmd.isEmpty()) {
        result.errorMessage = "Maven (mvn) not found. Please set MAVEN_HOME in settings or add to PATH.";
        result.builderName = "JavaMavenBuilder";
        qWarning() << "[JavaMavenBuilder] " << result.errorMessage;
        return false;
    }

    QProcessEnvironment env = prepareEnvironment(task);

    // 构建命令参数
    QStringList args;
    if (isSpringBoot) {
        args << "clean" << "package";
        qDebug() << "[JavaMavenBuilder] Using 'clean package' for Spring Boot";
    } else {
        args << "clean" << "compile";
        qDebug() << "[JavaMavenBuilder] Using 'clean compile' for standard Java";
    }

    bool skipTests = task.aiCustomPayload.value("skipTests", false).toBool();
    if (skipTests) {
        args << "-DskipTests";
        qDebug() << "[JavaMavenBuilder] Skipping tests";
    }

    // 追加用户额外参数
    args << task.extraArgs;

    // 打印完整命令（便于调试）
    QString fullCommand = mvnCmd + " " + args.join(" ");
    qDebug() << "[JavaMavenBuilder] Executing:" << fullCommand;
    // 同时也输出到输出面板（方便用户看到）
    // 注意：这里不能直接操作 MainWindow，但可以通过信号，不过我们暂时用 qDebug

    ProcessManager pm;
    pm.setEnvironment(env);

    // 增加超时时间到 5 分钟（Spring Boot 打包可能较慢）
    ProcessManager::ProcessResult pr = pm.executeSync(mvnCmd, args, task.projectPath, 300000);

    // 记录执行结果
    qDebug() << "[JavaMavenBuilder] Process finished, exitCode:" << pr.exitCode
             << ", success:" << pr.success
             << ", errorMessage:" << pr.errorMessage;

    result.exitCode = pr.exitCode;
    result.fullOutput = pr.standardOutput + "\n" + pr.standardError;
    result.elapsedMs = pr.executionTime;
    result.success = pr.success && pr.exitCode == 0;
    result.builderName = "JavaMavenBuilder";

    // 解析错误和警告
    if (!result.success) {
        // 输出完整的编译输出到 qDebug（在 Qt Creator 控制台可见）
        qDebug() << "[JavaMavenBuilder] Full output (stdout):" << pr.standardOutput;
        qDebug() << "[JavaMavenBuilder] Full output (stderr):" << pr.standardError;
        QStringList allLines = result.fullOutput.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : allLines) {
            if (line.contains("[ERROR]", Qt::CaseInsensitive)) {
                result.errorLines.append(line);
            } else if (line.contains("[WARNING]", Qt::CaseInsensitive)) {
                result.warningLines.append(line);
            }
        }
        if (result.errorLines.isEmpty()) {
            result.errorLines << pr.standardError.split('\n', Qt::SkipEmptyParts);
        }
        result.errorMessage = pr.errorMessage.isEmpty() ? "Maven build failed" : pr.errorMessage;
        qWarning() << "[JavaMavenBuilder] Build failed:" << result.errorMessage;
    } else {
        qDebug() << "[JavaMavenBuilder] Build succeeded";
        // 查找生成的 JAR
        QString jarPath = getExecutablePath(task);
        if (!jarPath.isEmpty()) {
            result.executablePath = jarPath;
            qDebug() << "[JavaMavenBuilder] Executable JAR:" << jarPath;
        } else {
            qWarning() << "[JavaMavenBuilder] No JAR found in target/";
        }
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

bool JavaMavenBuilder::run(const BuildTask& task, const BuildResult& buildResult, QString& output)
{
    // 如果构建结果中提供了 executablePath 且是 Fat JAR，直接用 java -jar
    if (!buildResult.executablePath.isEmpty() && QFile::exists(buildResult.executablePath)) {
        // 检查是否以 .jar 结尾且文件较大（可能是 Fat JAR）
        QString jarPath = buildResult.executablePath;
        if (jarPath.endsWith(".jar", Qt::CaseInsensitive)) {
            // 直接 java -jar
            QString javaExe = findJavaExecutable();
            if (javaExe.isEmpty()) {
                output = "Java executable not found";
                return false;
            }
            ProcessManager pm;
            pm.setEnvironment(prepareEnvironment(task));
            ProcessManager::ProcessResult pr = pm.executeSync(javaExe, {"-jar", jarPath}, task.projectPath, 0);
            output = pr.standardOutput + "\n" + pr.standardError;
            return pr.success && pr.exitCode == 0;
        }
    }

    // 否则按普通模式运行（依赖 task.aiCustomPayload["mainClass"]）
    QString mainClass = task.aiCustomPayload["mainClass"].toString();
    if (mainClass.isEmpty()) {
        mainClass = extractMainClassFromPom(task.projectPath);
    }
    if (!mainClass.isEmpty()) {
        return runJavaProgram(mainClass, task, output);
    }

    output = "No mainClass specified and no executable jar found";
    return false;
}

QString JavaMavenBuilder::getExecutablePath(const BuildTask& task)
{
    QString projectPath = task.projectPath;
    if (projectPath.isEmpty() && !task.sourceFiles.isEmpty()) {
        projectPath = QFileInfo(task.sourceFiles.first()).absolutePath();
    }

    QDir targetDir(PathUtils::joinPaths({projectPath, "target"}));
    if (!targetDir.exists()) {
        return QString();
    }

    // 优先查找可执行的 JAR（排除 -sources 和 -javadoc）
    QStringList jarFiles = targetDir.entryList({"*.jar"}, QDir::Files);
    // 按文件大小排序（大的可能是 Fat JAR）
    QMap<qint64, QString> sizeMap;
    for (const QString& jar : jarFiles) {
        if (jar.contains("sources") || jar.contains("javadoc")) {
            continue;
        }
        QFileInfo fi(targetDir.absoluteFilePath(jar));
        sizeMap[fi.size()] = jar;
    }
    if (!sizeMap.isEmpty()) {
        // 返回最大的 JAR（通常是 Fat JAR）
        QString largestJar = sizeMap.last();
        return targetDir.absoluteFilePath(largestJar);
    }

    // 没有合适的 JAR，返回空
    return QString();
}

bool JavaMavenBuilder::resolveClasspath(const QString& projectPath, QStringList& classpath) const {
    QString mvn = findMavenExecutable();
    if (mvn.isEmpty()) {
        qWarning() << "Maven not found";
        return false;
    }

    // 创建临时文件存储 classpath
    QTemporaryFile tempFile;
    tempFile.setAutoRemove(true);
    if (!tempFile.open()) return false;
    QString tempPath = tempFile.fileName();
    tempFile.close();

    QStringList args = {"dependency:build-classpath", "-Dmdep.outputFile=" + tempPath};
    ProcessManager pm;
    QProcessEnvironment env = prepareEnvironmentForPath(projectPath); // 需要实现
    pm.setEnvironment(env);
    ProcessManager::ProcessResult result = pm.executeSync(mvn, args, projectPath, 60000);
    if (!result.success || result.exitCode != 0) {
        qWarning() << "Maven dependency resolution failed:" << result.errorMessage;
        return false;
    }

    QFile cpFile(tempPath);
    if (!cpFile.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    QString cpStr = cpFile.readAll().trimmed();
    cpFile.close();
    if (cpStr.isEmpty()) {
        // 至少包含 target/classes
        QString classesDir = PathUtils::joinPaths({projectPath, "target", "classes"});
        if (QFile::exists(classesDir)) classpath.append(classesDir);
        return true;
    }
#ifdef Q_OS_WIN
    classpath = cpStr.split(';');
#else
    classpath = cpStr.split(':');
#endif
    // 确保包含输出目录
    QString classesDir = PathUtils::joinPaths({projectPath, "target", "classes"});
    if (QFile::exists(classesDir) && !classpath.contains(classesDir)) {
        classpath.prepend(classesDir);
    }
    return true;
}

QProcessEnvironment JavaMavenBuilder::prepareEnvironmentForPath(const QString& projectPath) const
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    ConfigManager* cfg = Application::instance().configManager();
    if (cfg) {
        QVariantMap sdkEnv = cfg->value("Sdk/Environment").toMap();
        for (auto it = sdkEnv.begin(); it != sdkEnv.end(); ++it) {
            env.insert(it.key(), it.value().toString());
        }
    }

    QString javaHome = env.value("JAVA_HOME");
    if (!javaHome.isEmpty()) {
        QString javaBin = PathUtils::joinPaths({javaHome, "bin"});
        QString path = env.value("PATH");
        env.insert("PATH", javaBin + ":" + path);
    }

    QString mavenHome = env.value("MAVEN_HOME");
    if (!mavenHome.isEmpty()) {
        QString mavenBin = PathUtils::joinPaths({mavenHome, "bin"});
        QString path = env.value("PATH");
        env.insert("PATH", mavenBin + ":" + path);
    }

    return env;
}

// ==================== 私有辅助 ====================

QString JavaMavenBuilder::findMavenExecutable() const {
    QString mvnCmd = "mvn";
#ifdef Q_OS_WIN
    mvnCmd = "mvn.cmd";
#endif

    // ----- 1. 优先从 ConfigManager（用户设置）读取 MAVEN_HOME -----
    ConfigManager* cfg = Application::instance().configManager();
    if (cfg) {
        QVariantMap sdkEnv = cfg->value("Sdk/Environment").toMap();
        if (sdkEnv.contains("MAVEN_HOME")) {
            QString mavenHome = sdkEnv["MAVEN_HOME"].toString().trimmed();
            if (!mavenHome.isEmpty()) {
                QString mvnPath = PathUtils::joinPaths({mavenHome, "bin", mvnCmd});
                if (QFile::exists(mvnPath)) {
                    qDebug() << "[JavaMavenBuilder] Found mvn from ConfigManager:" << mvnPath;
                    return mvnPath;
                } else {
                    qWarning() << "[JavaMavenBuilder] MAVEN_HOME from ConfigManager points to invalid path:" << mavenHome;
                }
            }
        }
    }

    // ----- 2. 从系统环境变量读取 MAVEN_HOME -----
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString mavenHome = env.value("MAVEN_HOME");
    if (!mavenHome.isEmpty()) {
        QString mvnPath = PathUtils::joinPaths({mavenHome, "bin", mvnCmd});
        if (QFile::exists(mvnPath)) {
            qDebug() << "[JavaMavenBuilder] Found mvn from system MAVEN_HOME:" << mvnPath;
            return mvnPath;
        }
    }

    // ----- 3. 从系统 PATH 查找 -----
    ProcessManager pm;
    ProcessManager::ProcessResult pr = pm.executeSync(mvnCmd, {"--version"}, QString(), 2000);
    if (pr.success) {
        qDebug() << "[JavaMavenBuilder] Found mvn in PATH:" << mvnCmd;
        return mvnCmd;
    }

    // ----- 4. 尝试常见的安装路径（兜底） -----
    QStringList commonPaths;
#ifdef Q_OS_MAC
    commonPaths << "/usr/local/bin/mvn"
                << "/opt/homebrew/bin/mvn"
                << "/usr/local/maven/bin/mvn";
#elif defined(Q_OS_WIN)
    commonPaths << "C:/Program Files/Maven/bin/mvn.cmd"
                << "C:/Program Files (x86)/Maven/bin/mvn.cmd"
                << "C:/maven/bin/mvn.cmd";
#else
    commonPaths << "/usr/bin/mvn"
                << "/usr/local/bin/mvn"
                << "/opt/maven/bin/mvn";
#endif

    for (const QString& path : commonPaths) {
        if (QFile::exists(path)) {
            qDebug() << "[JavaMavenBuilder] Found mvn at common path:" << path;
            return path;
        }
    }

    qWarning() << "[JavaMavenBuilder] Maven (mvn) not found anywhere";
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
