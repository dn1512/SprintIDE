#include "java_utils.h"
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QProcess>
#include <QDebug>
#include <QStandardPaths>
#include <algorithm>
#include <project_config.h>

#include "application.h"
#include "config_manager.h"
#include "process_manager.h"
#include "project_config.h"


namespace SprintIDE {

// ============================================================
// 辅助函数：通过MAVEN/gradle查找lombok支持
// ============================================================
static QString findLombokJar()
{
    QStringList searchPaths;
    QString home = QDir::homePath();

    // 1. Maven 本地仓库
    QString m2Repo = home + "/.m2/repository/org/projectlombok/lombok";
    QDir m2Dir(m2Repo);
    if (m2Dir.exists()) {
        QStringList versions = m2Dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        // 按版本号降序排列（数字降序）
        std::sort(versions.begin(), versions.end(),
                  [](const QString& a, const QString& b) {
                      // 使用版本号比较，例如 "1.18.30" 和 "1.18.28"
                      // 简单比较字符串可能不符合语义，但可接受，或者用 QVersionNumber
                      return a > b; // 降序
                  });
        for (const QString& ver : versions) {
            QString jar = m2Dir.absoluteFilePath(ver) + "/lombok-" + ver + ".jar";
            if (QFile::exists(jar)) {
                return jar;
            }
        }
    }

    // 2. Gradle 缓存
    QString gradleCache = home + "/.gradle/caches/modules-2/files-2.1/org.projectlombok/lombok";
    QDir gradleDir(gradleCache);
    if (gradleDir.exists()) {
        QStringList versions = gradleDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        std::sort(versions.begin(), versions.end(), [](const QString& a, const QString& b) { return a > b; });
        for (const QString& ver : versions) {
            QDir verDir(gradleDir.absoluteFilePath(ver));
            QStringList hashDirs = verDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QString& hash : hashDirs) {
                QString jar = verDir.absoluteFilePath(hash) + "/lombok-" + ver + ".jar";
                if (QFile::exists(jar)) {
                    return jar;
                }
            }
        }
    }

    // 3. 常见系统路径
#ifdef Q_OS_WIN
    searchPaths << "C:/Program Files/Lombok/lombok.jar"
                << "C:/lombok/lombok.jar";
#elif defined(Q_OS_MAC)
    searchPaths << "/usr/local/share/java/lombok.jar"
                << "/opt/homebrew/share/java/lombok.jar";
#else
    searchPaths << "/usr/share/java/lombok.jar"
                << "/usr/local/share/java/lombok.jar"
                << "/opt/lombok/lombok.jar";
#endif

    // 4. 环境变量
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (env.contains("LOMBOK_HOME")) {
        QString lombokHome = env.value("LOMBOK_HOME");
        searchPaths << lombokHome + "/lombok.jar";
    }

    // 5. 用户配置
    ConfigManager* cfg = Application::instance().configManager();
    if (cfg) {
        QString customPath = cfg->value("Lombok/Path").toString();
        if (!customPath.isEmpty()) {
            searchPaths.prepend(customPath);
        }
    }

    for (const QString& path : searchPaths) {
        if (QFile::exists(path)) {
            return path;
        }
    }

    qWarning() << "未找到 Lombok JAR，请安装 Lombok 或在设置中指定路径。";
    return QString();
}

// ============================================================
// 辅助函数：通过执行 java -XshowSettings:properties 获取 java.home
// ============================================================
static QString getJavaHomeFromJava(const QString& javaPath)
{
    QProcess process;
    process.start(javaPath, {"-XshowSettings:properties", "-version"});
    if (!process.waitForFinished(3000)) {
        return QString();
    }
    QString output = process.readAllStandardError();
    output += process.readAllStandardOutput();
    QRegularExpression regex("java\\.home = (.*)");
    QRegularExpressionMatch match = regex.match(output);
    if (match.hasMatch()) {
        return match.captured(1).trimmed();
    }
    return QString();
}

// 核心方法：检测 Java 17+，优先从全局配置读取
JavaInfo JavaUtils::detectJava()
{
    JavaInfo info;

    // ========== 第一步：从全局配置中读取 JAVA_HOME ==========
    ConfigManager* cfg = Application::instance().configManager();
    if (cfg) {
        QVariantMap sdkEnv = cfg->value("Sdk/Environment").toMap();
        if (sdkEnv.contains("JAVA_HOME")) {
            QString javaHome = sdkEnv["JAVA_HOME"].toString().trimmed();
            if (!javaHome.isEmpty()) {
                QFileInfo homeInfo(javaHome);
                if (homeInfo.exists() && homeInfo.isDir()) {
                    // 构建 java 可执行文件路径
                    QString javaPath = javaHome + "/bin/java";
#ifdef Q_OS_WIN
                    javaPath = javaHome + "/bin/java.exe";
#endif
                    if (QFile::exists(javaPath)) {
                        // 验证版本
                        QProcess process;
                        process.start(javaPath, {"-version"});
                        if (process.waitForFinished(3000)) {
                            QString output = process.readAllStandardError();
                            if (output.isEmpty()) output = process.readAllStandardOutput();
                            int major = parseMajorVersion(output);
                            if (major >= 17) {
                                info.path = javaPath;
                                info.home = javaHome;
                                info.majorVersion = major;
                                return info;
                            }
                        }
                    }
                }
            }
        }
    }

    // ========== 第二步：从系统 PATH 中查找 java ==========
    QString javaPath = QStandardPaths::findExecutable("java");
    if (!javaPath.isEmpty()) {
        // 获取真实路径（解析符号链接）
        QString realPath = QFileInfo(javaPath).canonicalFilePath();
        if (realPath.isEmpty()) realPath = javaPath;

        // 获取版本
        QProcess process;
        process.start(realPath, {"-version"});
        if (process.waitForFinished(3000)) {
            QString output = process.readAllStandardError();
            if (output.isEmpty()) output = process.readAllStandardOutput();
            int major = parseMajorVersion(output);
            if (major >= 17) {
                // 获取 java.home
                QString javaHome = getJavaHomeFromJava(realPath);
                if (javaHome.isEmpty()) {
                    // 备用：从路径推断（仅当 java 在 bin 目录下）
                    QFileInfo binInfo(realPath);
                    QDir binDir = binInfo.dir();
                    if (binDir.dirName().toLower() == "bin") {
                        javaHome = binDir.absolutePath();
                    } else {
                        // 可能是 jre/bin/java，返回上一层
                        javaHome = binDir.absolutePath();
                    }
                }
                // 验证 javaHome 存在
                if (!javaHome.isEmpty() && QFile::exists(javaHome + "/lib") && QFile::exists(javaHome + "/bin/java")) {
                    info.path = realPath;
                    info.home = javaHome;
                    info.majorVersion = major;
                    return info;
                }
            }
        }
    }

    // ========== 第三步：常见安装路径扫描（macOS / Linux / Windows） ==========
    QStringList candidatePaths;

#ifdef Q_OS_MAC
    // macOS：使用 /usr/libexec/java_home
    QProcess javaHomeProc;
    javaHomeProc.start("/usr/libexec/java_home");
    if (javaHomeProc.waitForFinished(2000)) {
        QString home = QString::fromLocal8Bit(javaHomeProc.readAllStandardOutput()).trimmed();
        if (!home.isEmpty() && QFile::exists(home + "/bin/java")) {
            QProcess verProc;
            verProc.start(home + "/bin/java", {"-version"});
            if (verProc.waitForFinished(2000)) {
                QString verOut = verProc.readAllStandardError();
                if (verOut.isEmpty()) verOut = verProc.readAllStandardOutput();
                int major = parseMajorVersion(verOut);
                if (major >= 17) {
                    info.path = home + "/bin/java";
                    info.home = home;
                    info.majorVersion = major;
                    return info;
                }
            }
        }
    }
    // Homebrew JDK 21
    QStringList brewPaths = {"/opt/homebrew/Cellar/openjdk", "/usr/local/Cellar/openjdk"};
    for (const QString& base : brewPaths) {
        QDir dir(base);
        if (dir.exists()) {
            QStringList versions = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QString& ver : versions) {
                QString home = dir.absoluteFilePath(ver) + "/libexec/openjdk.jdk/Contents/Home";
                if (QFile::exists(home + "/bin/java")) {
                    // 验证版本
                    QProcess proc;
                    proc.start(home + "/bin/java", {"-version"});
                    if (proc.waitForFinished(2000)) {
                        QString out = proc.readAllStandardError();
                        if (out.isEmpty()) out = proc.readAllStandardOutput();
                        int major = parseMajorVersion(out);
                        if (major >= 17) {
                            info.path = home + "/bin/java";
                            info.home = home;
                            info.majorVersion = major;
                            return info;
                        }
                    }
                }
            }
        }
    }
#elif defined(Q_OS_LINUX)
    // Linux：扫描 /usr/lib/jvm
    QStringList jvmPaths = {"/usr/lib/jvm", "/usr/local/lib/jvm"};
    for (const QString& base : jvmPaths) {
        QDir dir(base);
        if (dir.exists()) {
            QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QString& sub : subdirs) {
                QString home = dir.absoluteFilePath(sub);
                if (QFile::exists(home + "/bin/java")) {
                    QProcess proc;
                    proc.start(home + "/bin/java", {"-version"});
                    if (proc.waitForFinished(2000)) {
                        QString out = proc.readAllStandardError();
                        if (out.isEmpty()) out = proc.readAllStandardOutput();
                        int major = parseMajorVersion(out);
                        if (major >= 17) {
                            info.path = home + "/bin/java";
                            info.home = home;
                            info.majorVersion = major;
                            return info;
                        }
                    }
                }
            }
        }
    }
#elif defined(Q_OS_WIN)
    // Windows：扫描常见安装目录
    QStringList winPaths = {"C:/Program Files/Java", "C:/Program Files (x86)/Java"};
    for (const QString& base : winPaths) {
        QDir dir(base);
        if (dir.exists()) {
            QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QString& sub : subdirs) {
                if (sub.contains("jdk", Qt::CaseInsensitive) || sub.contains("openjdk", Qt::CaseInsensitive)) {
                    QString home = dir.absoluteFilePath(sub);
                    QString javaExe = home + "/bin/java.exe";
                    if (QFile::exists(javaExe)) {
                        QProcess proc;
                        proc.start(javaExe, {"-version"});
                        if (proc.waitForFinished(2000)) {
                            QString out = proc.readAllStandardError();
                            if (out.isEmpty()) out = proc.readAllStandardOutput();
                            int major = parseMajorVersion(out);
                            if (major >= 17) {
                                info.path = javaExe;
                                info.home = home;
                                info.majorVersion = major;
                                return info;
                            }
                        }
                    }
                }
            }
        }
    }
#endif

    // 未找到有效 Java
    qDebug() << "JavaUtils: 未找到有效的 Java 17+";
    return info;
}

JavaInfo JavaUtils::parseJavaVersion(const QString& javaPath)
{
    JavaInfo result;
    result.path = javaPath;

    QProcess process;
    process.setProgram(javaPath);
    process.setArguments({"-version"});
    process.start();
    if (!process.waitForFinished(3000)) {
        return result;
    }

    QByteArray output = process.readAllStandardError();
    if (output.isEmpty()) {
        output = process.readAllStandardOutput();
    }

    QString versionStr = QString::fromUtf8(output);
    QRegularExpression regex(R"((?:java|openjdk) version "(\d+)\.)");
    QRegularExpressionMatch match = regex.match(versionStr);
    if (match.hasMatch()) {
        result.majorVersion = match.captured(1).toInt();
    } else {
        QRegularExpression oldRegex(R"("1\.(\d+)\.)");
        QRegularExpressionMatch oldMatch = oldRegex.match(versionStr);
        if (oldMatch.hasMatch()) {
            result.majorVersion = oldMatch.captured(1).toInt();
        }
    }
    return result;
}

QStringList JavaUtils::getCommonJavaPaths()
{
    QStringList paths;
#ifdef Q_OS_MAC
    paths << "/usr/bin/java"
          << "/Library/Java/JavaVirtualMachines/jdk-21.jdk/Contents/Home/bin/java"
          << "/Library/Java/JavaVirtualMachines/jdk-17.jdk/Contents/Home/bin/java"
          << "/opt/homebrew/bin/java"
          << "/usr/local/bin/java";
#elif defined(Q_OS_WIN)
    paths << "C:/Program Files/Java/jdk-21/bin/java.exe"
          << "C:/Program Files/Java/jdk-17/bin/java.exe"
          << "C:/Program Files/Java/jdk-11/bin/java.exe"
          << "C:/Program Files (x86)/Java/jdk-21/bin/java.exe";
#else
    paths << "/usr/bin/java"
          << "/usr/local/bin/java"
          << "/opt/java/bin/java"
          << "/usr/lib/jvm/java-21-openjdk/bin/java"
          << "/usr/lib/jvm/java-17-openjdk/bin/java";
#endif
    return paths;
}

bool JavaUtils::isVersionCompatible(int majorVersion)
{
    return majorVersion >= 17;
}

bool JavaUtils::ensureJdtlsWrapper(const QString& projectRoot,
                                   const JavaInfo& javaInfo,
                                   QString& wrapperPath)
{
    if (!javaInfo.isValid()) return false;

    // 查找 equinox launcher
    QDir pluginsDir(QDir::homePath() + "/.jdtls/plugins");
    if (!pluginsDir.exists()) return false;
    QStringList jars = pluginsDir.entryList({"org.eclipse.equinox.launcher_*.jar"}, QDir::Files);
    if (jars.isEmpty()) return false;
    jars.sort();
    QString launcherJar = pluginsDir.absoluteFilePath(jars.last());

    // 创建项目内的 .SprintIDE/jdtls 目录
    QDir projectDir(projectRoot);
    QDir jdtlsDir = projectDir.absoluteFilePath(".SprintIDE/jdtls");
    if (!jdtlsDir.mkpath(".")) return false;

    // ----- 查找 Lombok JAR 并构造 -javaagent 参数 -----
    QString lombokJar = findLombokJar();  // 新增函数
    QString javaAgentArg;
    if (!lombokJar.isEmpty()) {
        javaAgentArg = " -javaagent:\"" + lombokJar + "\"";
    }

#ifdef Q_OS_WIN
    wrapperPath = jdtlsDir.absoluteFilePath("jdtls_wrapper.bat");
    QString scriptContent = "@echo off\r\n";
    scriptContent += "set JAVA_HOME=\"" + javaInfo.home + "\"\r\n";
    scriptContent += "\"" + javaInfo.path + "\"" + javaAgentArg + " -jar \"" + launcherJar + "\" %*\r\n";
#else
    wrapperPath = jdtlsDir.absoluteFilePath("jdtls_wrapper.sh");
    QString scriptContent = "#!/bin/bash\n";
    scriptContent += "export JAVA_HOME=\"" + javaInfo.home + "\"\n";
    scriptContent += "exec \"" + javaInfo.path + "\"" + javaAgentArg + " -jar \"" + launcherJar + "\" \"$@\"\n";
#endif

    QFile scriptFile(wrapperPath);
    if (!scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    scriptFile.write(scriptContent.toUtf8());
    scriptFile.close();

#ifndef Q_OS_WIN
    QFile::setPermissions(wrapperPath, QFile::ExeUser | QFile::ReadUser | QFile::WriteUser);
#endif

    return true;
}

bool JavaUtils::createMinimalConfig(const QString& configDir)
{
    QDir dir(configDir);
    if (!dir.mkpath(".")) return false;
    QString configPath = dir.absoluteFilePath("config.ini");
    if (QFile::exists(configPath)) return true;

    // 尝试复制模板
    QString templateConfig;
#ifdef Q_OS_MAC
    templateConfig = QDir::homePath() + "/.jdtls/config_mac/config.ini";
#elif defined(Q_OS_LINUX)
    templateConfig = QDir::homePath() + "/.jdtls/config_linux/config.ini";
#elif defined(Q_OS_WIN)
    templateConfig = QDir::homePath() + "/.jdtls/config_win/config.ini";
#endif
    if (QFile::exists(templateConfig)) {
        if (QFile::copy(templateConfig, configPath)) return true;
    }

    // 生成最小配置（确保包含 org.eclipse.core.runtime）
    QString content =
        "osgi.bundles=org.eclipse.equinox.common@2:start, "
        "org.eclipse.update.configurator@3:start, "
        "org.eclipse.core.runtime@start, "
        "org.eclipse.jdt.ls.core@start\n"
        "osgi.bundles.defaultStartLevel=4\n"
        "osgi.configuration.area=$INSTALL_DIR/configuration\n"
        "eclipse.product=org.eclipse.jdt.ls.core.product\n"
        "osgi.framework=file:$INSTALL_DIR/plugins/org.eclipse.osgi_*.jar\n"
        "osgi.splashPath=platform:/base/plugins/org.eclipse.jdt.ls.core\n"
        "eclipse.application=org.eclipse.jdt.ls.core.id1\n"
        "osgi.clean=true\n";
    QFile file(configPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    file.write(content.toUtf8());
    file.close();
    return true;
}

int JavaUtils::parseMajorVersion(const QString& versionOutput)
{
    // 匹配 "java version "21.0.6" 或 "openjdk version "17.0.12""
    QRegularExpression regex(R"((?:java|openjdk) version "(\d+)\.)");
    QRegularExpressionMatch match = regex.match(versionOutput);
    if (match.hasMatch()) {
        bool ok = false;
        int major = match.captured(1).toInt(&ok);
        if (ok) return major;
    }

    // 匹配旧格式 "java version "1.8.0_271" -> 8
    QRegularExpression oldRegex(R"("1\.(\d+)\.)");
    QRegularExpressionMatch oldMatch = oldRegex.match(versionOutput);
    if (oldMatch.hasMatch()) {
        bool ok = false;
        int major = oldMatch.captured(1).toInt(&ok);
        if (ok) return major;
    }

    return -1;
}

int JavaUtils::parseMajorVersionFromJava(const QString& javaPath)
{
    if (javaPath.isEmpty()) return -1;

    QProcess process;
    process.start(javaPath, {"-version"});
    if (!process.waitForFinished(3000)) {
        return -1;
    }

    QString output = process.readAllStandardError();
    if (output.isEmpty()) {
        output = process.readAllStandardOutput();
    }

    return parseMajorVersion(output);
}

bool JavaUtils::ensureEclipseMetadata(const QString& projectPath, const QString& mavenHome) {
    QString pomFile = projectPath + "/pom.xml";
    if (!QFile::exists(pomFile)) {
        return true; // 不是 Maven 项目，无需处理
    }

    // 检查是否已存在
    if (QFile::exists(projectPath + "/.project") && QFile::exists(projectPath + "/.classpath")) {
        return true;
    }

    // ----- 读取项目/全局配置中的 MAVEN_HOME -----
    // QString mavenHome;
    // // 优先从项目配置读取
    // ProjectConfig projConfig;
    // QString configFile = projectPath + "/.lightcppproj";
    // QStringList errors;
    // if (projConfig.loadFromFile(configFile, errors)) {
    //     QVariantMap sdkEnv = projConfig.sdkEnvironment();
    //     if (sdkEnv.contains("MAVEN_HOME")) {
    //         mavenHome = sdkEnv["MAVEN_HOME"].toString();
    //     }
    // }
    // // 如果项目配置没有，尝试全局配置
    // if (mavenHome.isEmpty()) {
    //     ConfigManager* cfg = Application::instance().configManager();
    //     if (cfg) {
    //         QVariantMap sdkEnv = cfg->value("Sdk/Environment").toMap();
    //         if (sdkEnv.contains("MAVEN_HOME")) {
    //             mavenHome = sdkEnv["MAVEN_HOME"].toString();
    //         }
    //     }
    // }

    // 确定 mvn 可执行文件
    QString mvnExecutable = "mvn";
    if (!mavenHome.isEmpty()) {
        QString mvnPath = QDir(mavenHome).absoluteFilePath("bin/mvn");
#ifdef Q_OS_WIN
        mvnPath += ".cmd";
#endif
        if (QFile::exists(mvnPath)) {
            mvnExecutable = mvnPath;
        }
    } else {
        // 从项目配置或全局配置读取 MAVEN_HOME
        ConfigManager* cfg = Application::instance().configManager();
        if (cfg) {
            QVariantMap sdkEnv = cfg->value("Sdk/Environment").toMap();
            if (sdkEnv.contains("MAVEN_HOME")) {
                QString mh = sdkEnv["MAVEN_HOME"].toString();
                if (!mh.isEmpty()) {
                    QString mvnPath = QDir(mh).absoluteFilePath("bin/mvn");
#ifdef Q_OS_WIN
                    mvnPath += ".cmd";
#endif
                    if (QFile::exists(mvnPath)) {
                        mvnExecutable = mvnPath;
                    }
                }
            }
        }
    }

    // 执行 mvn eclipse:eclipse
    ProcessManager pm;
    ProcessManager::ProcessResult result = pm.executeSync(
        mvnExecutable,
        {"eclipse:eclipse"},
        projectPath,
        60000  // 60 秒超时
        );

    return result.success;
}

} // namespace SprintIDE
