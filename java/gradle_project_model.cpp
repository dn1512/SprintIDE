#include "gradle_project_model.h"
#include "../path_utils.h"
#include "../process_manager.h"
#include "../config_manager.h"
#include "../application.h"
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QTemporaryFile>
#include <QRegularExpressionMatchIterator>

namespace SprintIDE {

GradleProjectModel::GradleProjectModel(const QString& projectPath)
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

    parseGradleProperties();
    resolveDependenciesGradle();
}

void GradleProjectModel::parseGradleProperties()
{
    // 查找 gradle.properties 读取 java.version
    QString propFile = PathUtils::joinPaths({m_projectPath, "gradle.properties"});
    if (QFile::exists(propFile)) {
        QFile file(propFile);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (line.startsWith("java.version=")) {
                    m_languageLevel = line.mid(13).trimmed();
                    break;
                }
            }
        }
    }

    // 扫描 build.gradle 或 build.gradle.kts 找 mainClass
    QStringList gradleFiles = {"build.gradle", "build.gradle.kts"};
    for (const QString& f : gradleFiles) {
        QString filePath = PathUtils::joinPaths({m_projectPath, f});
        if (QFile::exists(filePath)) {
            QFile file(filePath);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString content = file.readAll();
                // 查找 mainClassName = '...' 或 mainClass.set('...')
                QRegularExpression regex(R"(mainClassName\s*=\s*['\"]([^'\"]+)['\"])");
                QRegularExpressionMatch match = regex.match(content);
                if (match.hasMatch()) {
                    m_mainClass = match.captured(1);
                    break;
                }
                // 尝试 application 插件的 mainClass
                QRegularExpression regex2(R"(mainClass\s*=\s*['\"]([^'\"]+)['\"])");
                QRegularExpressionMatch match2 = regex2.match(content);
                if (match2.hasMatch()) {
                    m_mainClass = match2.captured(1);
                    break;
                }
            }
        }
    }

    // 如果未找到，尝试扫描 src/main/java 中的 main 方法
    if (m_mainClass.isEmpty()) {
        QDir srcDir(PathUtils::joinPaths({m_projectPath, "src", "main", "java"}));
        if (srcDir.exists()) {
            QStringList javaFiles = srcDir.entryList({"*.java"}, QDir::Files | QDir::NoDotAndDotDot);
            for (const QString& file : javaFiles) {
                QFile f(srcDir.absoluteFilePath(file));
                if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QString content = f.readAll();
                    if (content.contains("public static void main")) {
                        QString className = file.left(file.lastIndexOf('.'));
                        m_mainClass = className;
                        break;
                    }
                }
            }
        }
    }
}

void GradleProjectModel::resolveDependenciesGradle()
{
    // 使用 gradlew -q dependencies --configuration compileClasspath 获取
    QString gradlew = "gradlew";
#ifdef Q_OS_WIN
    gradlew = "gradlew.bat";
#endif
    QString gradlewPath = PathUtils::joinPaths({m_projectPath, gradlew});
    if (!QFile::exists(gradlewPath)) {
        qWarning() << "gradlew not found in project root";
        return;
    }

    // 执行 gradlew -q dependencies --configuration compileClasspath
    ProcessManager pm;
    // 设置环境变量（JAVA_HOME 等）
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!m_jdkPath.isEmpty()) {
        env.insert("JAVA_HOME", m_jdkPath);
    }
    pm.setEnvironment(env);

    QStringList args = {"-q", "dependencies", "--configuration", "compileClasspath"};
    ProcessManager::ProcessResult result = pm.executeSync(gradlewPath, args, m_projectPath, 60000);

    if (!result.success || result.exitCode != 0) {
        qWarning() << "Gradle dependency resolution failed:" << result.errorMessage;
        return;
    }

    QString output = result.standardOutput;
    // 解析输出，提取以 "--- " 开头或包含 " -> " 的行
    // 更稳健：查找 "compileClasspath" 之后的内容，然后提取文件路径
    // 简单做法：查找所有 .jar 路径
    QRegularExpression jarRegex(R"(([^ ]+\.jar))");
    QRegularExpressionMatchIterator it = jarRegex.globalMatch(output);
    QStringList jars;
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString jar = match.captured(1);
        if (QFile::exists(jar)) {
            jars.append(jar);
        }
    }

    // 去重
    m_dependencyJars = QSet<QString>(jars.begin(), jars.end()).values();

    // 添加 build/classes/java/main
    m_dependencyJars.append(PathUtils::joinPaths({m_projectPath, "build", "classes", "java", "main"}));
}

QStringList GradleProjectModel::classpath() const
{
    QStringList cp;
    cp << PathUtils::joinPaths({m_projectPath, "build", "classes", "java", "main"});
    cp << m_dependencyJars;
    return cp;
}

QStringList GradleProjectModel::testClasspath() const
{
    QStringList cp = classpath();
    cp << PathUtils::joinPaths({m_projectPath, "build", "classes", "java", "test"});
    return cp;
}

} // namespace SprintIDE
