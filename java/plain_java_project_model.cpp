#include "plain_java_project_model.h"
#include "../path_utils.h"
#include "../config_manager.h"
#include "../application.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QProcessEnvironment>
#include <QDirIterator>
#include <QRegularExpression>

namespace SprintIDE {

PlainJavaProjectModel::PlainJavaProjectModel(const QString& projectPath)
    : m_projectPath(projectPath)
{
    m_projectName = QFileInfo(projectPath).fileName();

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

    scanSourcesAndLibs();
}

void PlainJavaProjectModel::scanSourcesAndLibs()
{
    // 检查是否存在 src/main/java 或者 src
    QDir projectDir(m_projectPath);
    if (projectDir.exists("src/main/java")) {
        m_sourceDirs = {"src/main/java"};
        m_resourceDirs = {"src/main/resources"};
        m_outputDir = "target/classes";  // 模仿 Maven
    } else if (projectDir.exists("src")) {
        m_sourceDirs = {"src"};
        // 检查是否有 resources
        if (projectDir.exists("resources")) {
            m_resourceDirs = {"resources"};
        }
        m_outputDir = "out";
    } else {
        // 没有 src，将当前目录视为源码目录（简单情况）
        m_sourceDirs = {"."};
        m_outputDir = "out";
    }

    // 扫描 lib 目录下的 jar
    QStringList libDirs = {"lib", "libs", "jars"};
    for (const QString& libDirName : libDirs) {
        QDir libDir(PathUtils::joinPaths({m_projectPath, libDirName}));
        if (libDir.exists()) {
            QStringList jars = libDir.entryList({"*.jar"}, QDir::Files);
            for (const QString& jar : jars) {
                m_dependencyJars.append(libDir.absoluteFilePath(jar));
            }
        }
    }

    // 扫描源码目录找主类
    for (const QString& srcDir : m_sourceDirs) {
        QDir src(PathUtils::joinPaths({m_projectPath, srcDir}));
        if (src.exists()) {
            // 递归查找 *.java 含 main 方法
            QList<QFileInfo> files;
            QDirIterator it(src.absolutePath(), QDirIterator::Subdirectories);
            while (it.hasNext()) {
                it.next();
                if (it.fileInfo().isFile() && it.fileInfo().suffix() == "java") {
                    files.append(it.fileInfo());
                }
            }
            for (const QFileInfo& fi : files) {
                QFile f(fi.absoluteFilePath());
                if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QString content = f.readAll();
                    if (content.contains("public static void main")) {
                        // 提取类名（文件名无扩展）
                        QString className = fi.baseName();
                        // 尝试从文件内容提取包名，拼接完整类名
                        QRegularExpression pkgRegex(R"(package\s+([^;]+);)");
                        QRegularExpressionMatch pkgMatch = pkgRegex.match(content);
                        if (pkgMatch.hasMatch()) {
                            m_mainClass = pkgMatch.captured(1) + "." + className;
                        } else {
                            m_mainClass = className;
                        }
                        break;
                    }
                }
            }
        }
        if (!m_mainClass.isEmpty()) break;
    }
}

QStringList PlainJavaProjectModel::classpath() const
{
    QStringList cp;
    cp << PathUtils::joinPaths({m_projectPath, m_outputDir});
    cp << m_dependencyJars;
    return cp;
}

QStringList PlainJavaProjectModel::testClasspath() const
{
    QStringList cp = classpath();
    cp << PathUtils::joinPaths({m_projectPath, m_testOutputDir});
    return cp;
}

} // namespace SprintIDE
