// src/cpp/cpp_project_model.h
#ifndef CPP_PROJECT_MODEL_H
#define CPP_PROJECT_MODEL_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QDir>

namespace SprintIDE {

/**
 * @brief C++ 项目构建系统类型
 */
enum class CppBuildSystem {
    Unknown,      ///< 未识别（回退到单文件 g++ 编译）
    CMake,        ///< 存在 CMakeLists.txt
    Makefile,     ///< 存在 Makefile / makefile / GNUmakefile
    SingleFile    ///< 无构建文件，视作单文件项目
};

/**
 * @brief CMake 目标信息
 */
struct CppTarget {
    QString name;         ///< 目标名（add_executable 的第一个参数）
    QString type;         ///< "executable" / "library" / "unknown"
    QStringList sources;  ///< 直接列出的源文件（可能不完整）
};

/**
 * @brief C++ 项目模型
 *
 * 职责：
 *  - 检测项目根目录下的构建系统类型
 *  - 提供 CMake 构建所需信息（build 目录、compile_commands.json 路径）
 *  - 扫描源码文件（供回退编译模式使用）
 *  - 探测系统中可用的构建工具（cmake / make / ninja）
 *
 * 与 Java 的 ProjectModel、Python 的 PythonProjectModel 保持接口风格一致。
 */
class CppProjectModel {
public:
    explicit CppProjectModel(const QString& projectPath);
    ~CppProjectModel() = default;

    // ---------- 加载 / 重载 ----------
    /// 扫描项目目录，识别构建系统
    bool load();

    // ---------- 基本信息 ----------
    QString projectPath() const { return m_projectPath; }
    QString projectName() const { return m_projectName; }
    CppBuildSystem buildSystem() const { return m_buildSystem; }
    /// 返回人类可读的构建系统名（"CMake" / "Makefile" / "SingleFile" / "Unknown"）
    QString buildSystemName() const;

    // ---------- 构建系统相关路径 ----------
    QString cmakeListsPath() const { return m_cmakeListsPath; }
    QString makefilePath() const { return m_makefilePath; }
    /// 默认构建目录：<projectPath>/build（CMake 时）或 <projectPath>（Makefile 时）
    QString buildDirectory() const;
    /// <buildDirectory>/compile_commands.json
    QString compileCommandsPath() const;

    // ---------- CMake 目标 ----------
    QStringList targetNames() const;
    QString defaultTarget() const;  ///< 优先返回 executable 类型的目标
    /// 给定 target 名，推测其在构建目录下的可执行文件绝对路径（可能为空）
    QString guessExecutablePath(const QString& targetName) const;

    // ---------- 源文件 ----------
    QStringList sourceFiles() const { return m_sourceFiles; }

    // ---------- 静态工具 ----------
    /// 判断给定路径是否像一个 C++ 项目（存在 CMakeLists.txt / Makefile / 源文件）
    static bool isCppProject(const QString& path);

    /// 在 PATH、Homebrew、常见安装位置查找 cmake
    static QString findCMakeExecutable();
    /// 在 PATH、常见位置查找 make
    static QString findMakeExecutable();
    /// 在 PATH、常见位置查找 ninja（可选）
    static QString findNinjaExecutable();

    /// 在 CMakeCache.txt 中读取 CMAKE_RUNTIME_OUTPUT_DIRECTORY
    static QString readCMakeRuntimeOutputDir(const QString& buildDir);

    /// 是否识别为 Qt 项目（CMakeLists 含 find_package(Qt5/Qt6，或存在 *.pro）
    bool isQtProject() const { return m_isQtProject; }

    /// Qt 主版本号（5 / 6），非 Qt 项目为 0
    int qtMajorVersion() const { return m_qtMajorVersion; }

private:
    void detectBuildSystem();
    void scanCMakeTargets();
    void scanCMakeProjectName();
    void scanSourceFiles();
    /// 在 load() 末尾调用：扫描 CMakeLists.txt / *.pro，判断是否 Qt
    void detectQtProject();

    QString m_projectPath;
    CppBuildSystem m_buildSystem = CppBuildSystem::Unknown;
    QString m_cmakeListsPath;
    QString m_makefilePath;
    QString m_projectName;
    QList<CppTarget> m_targets;
    QStringList m_sourceFiles;
    bool m_isQtProject = false;
    int  m_qtMajorVersion = 0;
};

} // namespace SprintIDE

#endif // CPP_PROJECT_MODEL_H
