#include "maven_project_model.h"
#include "../path_utils.h"
#include "../process_manager.h"
#include "../config_manager.h"
#include "../application.h"
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QRegularExpression>
#include <QTemporaryFile>

namespace SprintIDE {

MavenProjectModel::MavenProjectModel(const QString& projectPath)
    : m_projectPath(projectPath)
{
    // 从项目路径提取项目名
    m_projectName = QFileInfo(projectPath).fileName();

    // 读取 JDK 配置（从全局设置或环境变量）
    ConfigManager* cfg = Application::instance().configManager();
    if (cfg) {
        QVariantMap sdkEnv = cfg->value("Sdk/Environment").toMap();
        if (sdkEnv.contains("JAVA_HOME")) {
            m_jdkPath = sdkEnv["JAVA_HOME"].toString();
        }
    }
    if (m_jdkPath.isEmpty()) {
        m_jdkPath = QProcessEnvironment::systemEnvironment().value("JAVA_HOME");
    }

    // 解析 pom.xml
    parsePom();
    // 解析依赖（第一次可能耗时，可异步，但这里同步）
    resolveDependencies();
}

bool MavenProjectModel::parsePom()
{
    QString pomFile = PathUtils::joinPaths({m_projectPath, "pom.xml"});
    if (!QFile::exists(pomFile)) {
        qWarning() << "pom.xml not found:" << pomFile;
        return false;
    }

    QFile file(pomFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open pom.xml:" << file.errorString();
        return false;
    }

    QXmlStreamReader reader(&file);
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            if (reader.name() == "artifactId") {
                m_projectName = reader.readElementText();
            } else if (reader.name() == "mainClass") {
                // 可能存在于 <properties> 或 <build><plugins> 中
                // 简单查找，不深入解析
            } else if (reader.name() == "maven.compiler.source") {
                m_languageLevel = reader.readElementText();
            } else if (reader.name() == "maven.compiler.target") {
                // 保持与 source 一致
            }
        }
    }

    // 如果未读到主类，尝试常见模式
    if (m_mainClass.isEmpty()) {
        // 扫描 src/main/java 下的 *Application.java
        QDir srcDir(PathUtils::joinPaths({m_projectPath, "src", "main", "java"}));
        if (srcDir.exists()) {
            QStringList javaFiles = srcDir.entryList({"*.java"}, QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
            for (const QString& file : javaFiles) {
                QFile f(srcDir.absoluteFilePath(file));
                if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QString content = f.readAll();
                    if (content.contains("public static void main")) {
                        // 从文件路径推导类名
                        QString className = file.left(file.lastIndexOf('.'));
                        // 可能需要包名，但暂不处理
                        m_mainClass = className;
                        break;
                    }
                }
            }
        }
    }

    m_parsed = true;
    return true;
}

void MavenProjectModel::resolveDependencies()
{
    // 调用 mvn dependency:build-classpath 获取类路径
    QString mvnCmd = "mvn";
#ifdef Q_OS_WIN
    mvnCmd = "mvn.cmd";
#endif

    // 从配置或环境获取 MAVEN_HOME
    ConfigManager* cfg = Application::instance().configManager();
    QString mavenHome;
    if (cfg) {
        QVariantMap sdkEnv = cfg->value("Sdk/Environment").toMap();
        if (sdkEnv.contains("MAVEN_HOME")) {
            mavenHome = sdkEnv["MAVEN_HOME"].toString();
        }
    }
    if (mavenHome.isEmpty()) {
        mavenHome = QProcessEnvironment::systemEnvironment().value("MAVEN_HOME");
    }
    if (!mavenHome.isEmpty()) {
        QString mvnPath = PathUtils::joinPaths({mavenHome, "bin", mvnCmd});
        if (QFile::exists(mvnPath)) {
            mvnCmd = mvnPath;
        }
    }

    // 使用临时文件存储类路径
    QTemporaryFile tempFile;
    tempFile.setAutoRemove(true);
    if (!tempFile.open()) {
        qWarning() << "Cannot create temp file for maven classpath";
        return;
    }
    QString tempPath = tempFile.fileName();
    tempFile.close();

    QStringList args = {"dependency:build-classpath", "-Dmdep.outputFile=" + tempPath};
    ProcessManager pm;
    // 设置工作目录
    ProcessManager::ProcessResult result = pm.executeSync(mvnCmd, args, m_projectPath, 60000);

    if (!result.success || result.exitCode != 0) {
        qWarning() << "Maven dependency resolution failed:" << result.errorMessage;
        return;
    }

    // 读取临时文件
    QFile cpFile(tempPath);
    if (cpFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString classpathStr = cpFile.readAll().trimmed();
        if (!classpathStr.isEmpty()) {
            // 按路径分隔符分割
#ifdef Q_OS_WIN
            m_dependencyJars = classpathStr.split(';');
#else
            m_dependencyJars = classpathStr.split(':');
#endif
        }
        cpFile.close();
    }

    // 如果类路径为空，尝试常见仓库
    if (m_dependencyJars.isEmpty()) {
        // 至少包含 target/classes
        m_dependencyJars.append(PathUtils::joinPaths({m_projectPath, "target", "classes"}));
    }
}

QStringList MavenProjectModel::classpath() const
{
    QStringList cp;
    cp << PathUtils::joinPaths({m_projectPath, "target", "classes"});
    cp << m_dependencyJars;
    return cp;
}

QStringList MavenProjectModel::testClasspath() const
{
    QStringList cp = classpath();
    cp << PathUtils::joinPaths({m_projectPath, "target", "test-classes"});
    return cp;
}

} // namespace SprintIDE
