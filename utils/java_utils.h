#ifndef JAVA_UTILS_H
#define JAVA_UTILS_H

#include <QString>
#include <QProcess>
#include <QDebug>

namespace SprintIDE {

struct JavaInfo {
    QString path;      // java 可执行文件的完整路径
    QString home;        // JAVA_HOME 路径（JDK根目录）
    int majorVersion;  // 如 21, 17, 11
    bool isValid() const { return !path.isEmpty() && !home.isEmpty() && majorVersion >= 17; }
};

class JavaUtils {
public:
    // 扫描系统，返回最佳的 Java 版本信息
    static JavaInfo detectJava();

    // 检查是否满足最低版本要求
    static bool isVersionCompatible(int majorVersion);

    // 生成 JDTLS 启动脚本（wrapper）
    static bool ensureJdtlsWrapper(const QString& projectRoot,
                                   const JavaInfo& javaInfo,
                                   QString& wrapperPath);

    // 创建最小 config.ini 文件（在指定目录中）
    static bool createMinimalConfig(const QString& configDir);
    // 从版本字符串中解析 major 版本号（内部辅助）
    static int parseMajorVersion(const QString& versionOutput);
    // 对指定 java 可执行文件执行 -version 并解析
    static int parseMajorVersionFromJava(const QString& javaPath);

    /**
     * @brief 为 Maven 项目生成 Eclipse 元数据（.project, .classpath）
     * @param projectPath 项目根目录
     * @param mavenHome 可选，指定 MAVEN_HOME（为空则从配置或 PATH 查找）
     * @return true 如果成功或已存在
     */
    static bool ensureEclipseMetadata(const QString& projectPath, const QString& mavenHome = QString());
private:
    static JavaInfo parseJavaVersion(const QString& javaPath);
    static QStringList getCommonJavaPaths();
};

} // namespace SprintIDE

#endif
