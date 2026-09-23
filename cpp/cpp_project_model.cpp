#include "cpp_project_model.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
#include <QProcess>
#include <QProcessEnvironment>
#include <QDebug>
#include <QCoreApplication> 
#include <algorithm>
#include <functional>    // for std::greater
#include <QHash>

namespace SprintIDE {

// ====================================================================
// 内嵌工具链根目录（cmake / ninja 打包后落点）
//
// 打包结构：
//   macOS: SprintIDE.app/Contents/Frameworks/toolchain/
//   Win:   SprintIDE/toolchain/
//   Linux: SprintIDE/bin/../lib/toolchain/
//
// 与 lldb_dap_provider.cpp 的 bundledToolchainRoot() 保持一致风格，
// 但目标目录不同（lldb 单独放 Frameworks/lldb，构建工具放 Frameworks/toolchain）。
// ====================================================================
static QString bundledToolchainRoot()
{
    if (!QCoreApplication::instance()) return QString();

    QDir appDir(QCoreApplication::applicationDirPath());
    if (appDir.path().isEmpty()) return QString();

#ifdef Q_OS_MAC
    // applicationDirPath() = .../SprintIDE.app/Contents/MacOS
    QDir macosDir = appDir;
    if (macosDir.dirName() == "MacOS") {
        macosDir.cdUp();          // → Contents
        return macosDir.absoluteFilePath("Frameworks/toolchain");
    }
    // 非 bundle 运行（Qt Creator 直接跑）：没有内嵌路径
    return QString();
#elif defined(Q_OS_WIN)
    return appDir.absoluteFilePath("toolchain");
#else
    return appDir.absoluteFilePath("../lib/toolchain");
#endif
}

// ====================================================================
// 构造
// ====================================================================
CppProjectModel::CppProjectModel(const QString& projectPath)
    : m_projectPath(projectPath)
{
}

// ====================================================================
// load：扫描项目目录，识别构建系统
// ====================================================================
bool CppProjectModel::load()
{
    m_targets.clear();
    m_sourceFiles.clear();
    m_cmakeListsPath.clear();
    m_makefilePath.clear();
    m_projectName.clear();
    m_buildSystem = CppBuildSystem::Unknown;

    if (m_projectPath.isEmpty() || !QDir(m_projectPath).exists()) {
        return false;
    }

    detectBuildSystem();

    switch (m_buildSystem) {
    case CppBuildSystem::CMake:
        m_cmakeListsPath = QDir(m_projectPath).absoluteFilePath("CMakeLists.txt");
        scanCMakeProjectName();
        scanCMakeTargets();
        break;
    case CppBuildSystem::Makefile: {
        const QStringList candidates = {"Makefile", "makefile", "GNUmakefile"};
        for (const QString& name : candidates) {
            QString p = QDir(m_projectPath).absoluteFilePath(name);
            if (QFile::exists(p)) {
                m_makefilePath = p;
                break;
            }
        }
        m_projectName = QFileInfo(m_projectPath).fileName();
        break;
    }
    case CppBuildSystem::SingleFile:
    case CppBuildSystem::Unknown:
    default:
        m_projectName = QFileInfo(m_projectPath).fileName();
        break;
    }

    // 在扫描源码之前先探测 Qt（依赖 m_cmakeListsPath 已经设置好）
    detectQtProject();

    scanSourceFiles();
    return true;
}

// ====================================================================
// detectBuildSystem
// ====================================================================
void CppProjectModel::detectBuildSystem()
{
    QDir dir(m_projectPath);

    if (dir.exists("CMakeLists.txt")) {
        m_buildSystem = CppBuildSystem::CMake;
        return;
    }
    if (dir.exists("Makefile") || dir.exists("makefile") || dir.exists("GNUmakefile")) {
        m_buildSystem = CppBuildSystem::Makefile;
        return;
    }

    // 检查是否存在 C/C++ 源文件 → SingleFile
    QDirIterator it(m_projectPath,
                    {"*.cpp", "*.cxx", "*.cc", "*.c", "*.h", "*.hpp"},
                    QDir::Files,
                    QDirIterator::Subdirectories);
    if (it.hasNext()) {
        m_buildSystem = CppBuildSystem::SingleFile;
        return;
    }

    m_buildSystem = CppBuildSystem::Unknown;
}

// ====================================================================
// buildSystemName
// ====================================================================
QString CppProjectModel::buildSystemName() const
{
    switch (m_buildSystem) {
    case CppBuildSystem::CMake:      return "CMake";
    case CppBuildSystem::Makefile:   return "Makefile";
    case CppBuildSystem::SingleFile: return "SingleFile";
    case CppBuildSystem::Unknown:
    default:                         return "Unknown";
    }
}

// ====================================================================
// buildDirectory
// ====================================================================
QString CppProjectModel::buildDirectory() const
{
    if (m_buildSystem == CppBuildSystem::CMake) {
        return QDir(m_projectPath).absoluteFilePath("build");
    }
    // Makefile / SingleFile 就地构建
    return m_projectPath;
}

// ====================================================================
// compileCommandsPath
// ====================================================================
QString CppProjectModel::compileCommandsPath() const
{
    if (m_buildSystem == CppBuildSystem::CMake) {
        return QDir(buildDirectory()).absoluteFilePath("compile_commands.json");
    }
    return QDir(m_projectPath).absoluteFilePath("compile_commands.json");
}

// ====================================================================
// 扫描 CMake project(...) 取工程名
// ====================================================================
void CppProjectModel::scanCMakeProjectName()
{
    QFile file(m_cmakeListsPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_projectName = QFileInfo(m_projectPath).fileName();
        return;
    }
    QTextStream in(&file);
    static QRegularExpression re(R"(^\s*project\s*\(\s*([A-Za-z0-9_\-\.]+))");
    while (!in.atEnd()) {
        QString line = in.readLine();
        // 去掉注释
        int commentIdx = line.indexOf('#');
        if (commentIdx >= 0) line = line.left(commentIdx);

        QRegularExpressionMatch m = re.match(line);
        if (m.hasMatch()) {
            m_projectName = m.captured(1);
            file.close();
            return;
        }
    }
    file.close();
    m_projectName = QFileInfo(m_projectPath).fileName();
}

// ====================================================================
// 扫描 CMake 目标：add_executable / add_library
// ====================================================================
void CppProjectModel::scanCMakeTargets()
{
    QFile file(m_cmakeListsPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    // 移除 # 行注释
    content.replace(QRegularExpression(R"(#[^\n]*)"), "");

    // ============================================================
    // 1. 收集简单的 CMake 变量（不做完整解析，只处理常见形态）
    // ============================================================
    QHash<QString, QString> vars;

    // project(X ...)  →  PROJECT_NAME = X
    {
        static QRegularExpression reProj(R"(project\s*\(\s*([A-Za-z0-9_\-\.]+))");
        auto m = reProj.match(content);
        if (m.hasMatch()) {
            vars["PROJECT_NAME"] = m.captured(1);
        }
    }

    // set(VAR value)  →  VAR = value    （单值形式，忽略 list）
    {
        static QRegularExpression reSet(
            R"(set\s*\(\s*([A-Za-z0-9_]+)\s+([A-Za-z0-9_\-\.]+)\s*\))");
        auto it = reSet.globalMatch(content);
        while (it.hasNext()) {
            auto m = it.next();
            vars[m.captured(1)] = m.captured(2);
        }
    }

    // 一层变量展开（迭代两次，处理 a→b, b→c 这种简单链）
    auto expandVars = [&vars](const QString& in) {
        QString out = in;
        for (int pass = 0; pass < 2; ++pass) {
            bool changed = false;
            for (auto it = vars.begin(); it != vars.end(); ++it) {
                const QString token = "${" + it.key() + "}";
                if (out.contains(token)) {
                    out.replace(token, it.value());
                    changed = true;
                }
            }
            if (!changed) break;
        }
        return out;
    };

    // ============================================================
    // 2. 扫 add_executable / add_library，第一参数允许 ${VAR}
    //    —— 改用更宽松的字符类 [^\s\)]+，捕获后再展开
    // ============================================================
    static QRegularExpression reExe(
        R"(add_executable\s*\(\s*([^\s\)]+)([^\)]*)\))",
        QRegularExpression::DotMatchesEverythingOption);
    auto it = reExe.globalMatch(content);
    while (it.hasNext()) {
        auto m = it.next();
        CppTarget t;
        t.name = expandVars(m.captured(1));
        if (t.name.isEmpty()) continue;
        t.type = "executable";
        QString rest = m.captured(2).trimmed();
        if (!rest.isEmpty()) {
            for (const QString& s : rest.split(QRegularExpression(R"(\s+)"),
                                               Qt::SkipEmptyParts)) {
                t.sources << s;
            }
        }
        m_targets.append(t);
    }

    static QRegularExpression reLib(
        R"(add_library\s*\(\s*([^\s\)]+)\s*(STATIC|SHARED|MODULE)?([^\)]*)\))",
        QRegularExpression::DotMatchesEverythingOption);
    auto it2 = reLib.globalMatch(content);
    while (it2.hasNext()) {
        auto m = it2.next();
        CppTarget t;
        t.name = expandVars(m.captured(1));
        if (t.name.isEmpty()) continue;
        t.type = "library";
        QString rest = m.captured(3).trimmed();
        if (!rest.isEmpty()) {
            for (const QString& s : rest.split(QRegularExpression(R"(\s+)"),
                                               Qt::SkipEmptyParts)) {
                t.sources << s;
            }
        }
        m_targets.append(t);
    }

    qDebug() << "[CppProjectModel] CMake targets found:" << m_targets.size()
             << "vars expanded:" << vars
             << "in" << m_cmakeListsPath;
}

// ====================================================================
// 扫描源码文件（用于回退编译模式）
// ====================================================================
void CppProjectModel::scanSourceFiles()
{
    QStringList filters = {"*.cpp", "*.cxx", "*.cc", "*.c"};
    QDirIterator it(m_projectPath, filters, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString path = it.next();
        // 跳过 build 目录内的产物
        if (path.contains("/build/") || path.contains("/.git/")) continue;
        m_sourceFiles << path;
    }
}

void CppProjectModel::detectQtProject()
{
    m_isQtProject = false;
    m_qtMajorVersion = 0;

    // 情况 A：CMake 项目 —— 读 CMakeLists.txt 找 find_package(Qt5 / Qt6
    if (m_buildSystem == CppBuildSystem::CMake) {
        QFile f(m_cmakeListsPath);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;
        QTextStream in(&f);
        const QString content = in.readAll();
        f.close();

        // 允许 find_package(Qt6 ...)、find_package(Qt5 COMPONENTS ...)
        static QRegularExpression re(R"(find_package\s*\(\s*Qt([56])\b)");
        auto m = re.match(content);
        if (m.hasMatch()) {
            m_isQtProject   = true;
            m_qtMajorVersion = m.captured(1).toInt();
            qDebug() << "[CppProjectModel] 检测到 Qt" << m_qtMajorVersion
                     << "项目:" << m_projectPath;
            return;
        }
        // 兜底：有些人直接写 target_link_libraries(xxx Qt6::Widgets)
        if (content.contains(QRegularExpression(R"(Qt[56]::)"))) {
            m_isQtProject = true;
            m_qtMajorVersion = content.contains("Qt6::") ? 6 : 5;
            return;
        }
    }

    // 情况 B：qmake 项目 —— 根目录存在 *.pro
    QDir dir(m_projectPath);
    if (!dir.entryList({"*.pro"}, QDir::Files).isEmpty()) {
        m_isQtProject = true;   // 版本未知，保持 0
        qDebug() << "[CppProjectModel] 检测到 qmake 项目:" << m_projectPath;
    }
}

// ====================================================================
// targetNames / defaultTarget
// ====================================================================
QStringList CppProjectModel::targetNames() const
{
    QStringList names;
    for (const CppTarget& t : m_targets) {
        names << t.name;
    }
    return names;
}

QString CppProjectModel::defaultTarget() const
{
    for (const CppTarget& t : m_targets) {
        if (t.type == "executable") {
            return t.name;
        }
    }
    // 没有 executable，则返回第一个 target
    if (!m_targets.isEmpty()) {
        return m_targets.first().name;
    }
    return QString();
}

// ====================================================================
// 推测可执行文件路径
// ====================================================================
QString CppProjectModel::guessExecutablePath(const QString& targetName) const
{
    if (m_buildSystem != CppBuildSystem::CMake) {
        return QString();
    }

    const QString buildDir = buildDirectory();
    const QString cacheFile = QDir(buildDir).absoluteFilePath("CMakeCache.txt");

    QString runtimeDir;
    if (QFile::exists(cacheFile)) {
        runtimeDir = readCMakeRuntimeOutputDir(buildDir);
    }

    // 所有可能藏着产物的目录
    QStringList searchDirs;
    if (!runtimeDir.isEmpty()) {
        searchDirs << runtimeDir;
    }
    searchDirs << buildDir
               << QDir(buildDir).absoluteFilePath("bin")
               << QDir(buildDir).absoluteFilePath("Debug")
               << QDir(buildDir).absoluteFilePath("Release")
               << QDir(buildDir).absoluteFilePath("cmake-build-debug")
               << QDir(buildDir).absoluteFilePath("cmake-build-release");

    // 若没有指定 target，candidateNames 为空，后续"按名字匹配"会跳过
    QStringList candidateNames;
    if (!targetName.isEmpty()) {
        candidateNames << targetName;
#ifdef Q_OS_WIN
        candidateNames << targetName + ".exe";
#endif
    }

    // ============================================================
    // 第一步：按名字匹配
    // ============================================================
    for (const QString& dir : searchDirs) {
        QDir d(dir);
        if (!d.exists()) continue;

#ifdef Q_OS_MAC
        // Qt GUI 应用：<dir>/<target>.app/Contents/MacOS/<target>
        if (!targetName.isEmpty()) {
            const QString appBundle = d.absoluteFilePath(targetName + ".app");
            const QFileInfo bundleInfo(appBundle);
            if (bundleInfo.isDir()) {
                const QString inner =
                    QDir(appBundle).absoluteFilePath("Contents/MacOS/" + targetName);
                if (QFile::exists(inner) && QFileInfo(inner).isExecutable()) {
                    qDebug() << "[CppProjectModel] 命中 .app bundle:" << inner;
                    return inner;
                }
            }
        }
#endif

        for (const QString& name : candidateNames) {
            const QString full = d.absoluteFilePath(name);
            if (QFile::exists(full) && QFileInfo(full).isExecutable()) {
                return full;
            }
        }
    }

#ifdef Q_OS_MAC
    // ============================================================
    // 第二步（macOS 专属 fallback）：扫描所有 *.app bundle
    //
    // 用于 targetName 为空、或 targetName 与 .app 名不一致的场景。
    // 典型触发：CMakeLists 里写 add_executable(${PROJECT_NAME} ...)，
    //           target 名无法静态解析 → targetName 为空。
    // ============================================================
    {
        for (const QString& root : searchDirs) {
            QDir r(root);
            if (!r.exists()) continue;

            const QFileInfoList bundles = r.entryInfoList(
                {"*.app"}, QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QFileInfo& b : bundles) {
                // "SprintIDE.app" → "SprintIDE"
                const QString appName = b.completeBaseName();
                const QString inner =
                    b.absoluteFilePath() + "/Contents/MacOS/" + appName;
                if (QFile::exists(inner) && QFileInfo(inner).isExecutable()) {
                    qDebug() << "[CppProjectModel] fallback 扫到 .app:" << inner;
                    return inner;
                }
            }
        }
    }
#endif

    // ============================================================
    // 第三步：扫描构建根目录下的裸可执行文件（非 bundle）
    //   —— Linux / Windows 主路径，macOS 上作为最后兜底
    // ============================================================
    QDir buildRoot(buildDir);
    if (buildRoot.exists()) {
        const QFileInfoList entries =
            buildRoot.entryInfoList(QDir::Files | QDir::Executable,
                                    QDir::Time | QDir::DirsFirst);
        for (const QFileInfo& fi : entries) {
            const QString s = fi.suffix().toLower();
            if (s == "so" || s == "dylib" || s == "a" || s == "o" || s == "cmake") {
                continue;
            }
            if (fi.isExecutable()) {
                return fi.absoluteFilePath();
            }
        }
    }

    return QString();
}

// ====================================================================
// 静态：isCppProject
// ====================================================================
bool CppProjectModel::isCppProject(const QString& path)
{
    QDir dir(path);
    if (!dir.exists()) return false;
    if (dir.exists("CMakeLists.txt")) return true;
    if (dir.exists("Makefile") || dir.exists("makefile") || dir.exists("GNUmakefile")) return true;

    QDirIterator it(path,
                    {"*.cpp", "*.cxx", "*.cc", "*.c"},
                    QDir::Files,
                    QDirIterator::Subdirectories);
    return it.hasNext();
}

// ====================================================================
// 静态：工具探测
// ====================================================================
static QString findInPathAndCommonLocations(const QStringList& names,
                                            const QStringList& extraPaths)
{
    // 1. 先扫 PATH
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString pathEnv = env.value("PATH");
    QStringList pathDirs = pathEnv.split(QDir::listSeparator(), Qt::SkipEmptyParts);
    pathDirs << extraPaths;

    for (const QString& dir : pathDirs) {
        for (const QString& name : names) {
            QString full = QDir(dir).absoluteFilePath(name);
            QFileInfo fi(full);
            if (fi.exists() && fi.isExecutable()) {
                return full;
            }
        }
    }
    return QString();
}

QString CppProjectModel::findCMakeExecutable()
{
    // ================================================================
    // 0. 用户显式配置优先：环境变量 CMAKE_HOME
    // ================================================================
    {
        QProcessEnvironment sysEnv = QProcessEnvironment::systemEnvironment();
        QString cmakeHome = sysEnv.value("CMAKE_HOME").trimmed();
        if (!cmakeHome.isEmpty()) {
            QStringList homeCandidates = {
                QDir(cmakeHome).absoluteFilePath("bin/cmake"),  // CMAKE_HOME 指向安装根
                QDir(cmakeHome).absoluteFilePath("cmake"),      // CMAKE_HOME 指向 bin
                cmakeHome                                       // CMAKE_HOME 直接指向可执行文件
            };
            for (const QString& c : homeCandidates) {
                QFileInfo fi(c);
                if (fi.exists() && fi.isExecutable() && fi.isFile()
                    && !c.contains(".app/Contents/MacOS/")) {
                    qDebug() << "[CppProjectModel] 使用 CMAKE_HOME 指定的 cmake:" << c;
                    return c;
                }
            }
            qWarning() << "[CppProjectModel] CMAKE_HOME 配置无效:" << cmakeHome;
        }
    }

    // ================================================================
    // 1. 内嵌工具链（app bundle 内的 Frameworks/toolchain，打包后可用）
    //    开发期目录不存在，直接跳过
    // ================================================================
    {
        const QString bundledRoot = bundledToolchainRoot();
        if (!bundledRoot.isEmpty()) {
            QString bundled = bundledRoot + "/bin/cmake";
#ifdef Q_OS_WIN
            bundled += ".exe";
#endif
            QFileInfo fi(bundled);
            if (fi.exists() && fi.isExecutable() && fi.isFile()) {
                // 快速验证一次（防止文件被截断/损坏）
                QProcess proc;
                proc.setProgram(bundled);
                proc.setArguments({"--version"});
                proc.setProcessChannelMode(QProcess::MergedChannels);
                proc.start();
                if (proc.waitForStarted(3000) && proc.waitForFinished(5000)) {
                    QString out = QString::fromLocal8Bit(proc.readAllStandardOutput());
                    if (out.contains("cmake version", Qt::CaseInsensitive)) {
                        qDebug() << "[CppProjectModel] ✓ 使用内嵌 cmake:" << bundled;
                        return bundled;
                    }
                    qWarning() << "[CppProjectModel] 内嵌 cmake 输出异常:"
                               << out.left(120);
                } else {
                    proc.kill();
                    proc.waitForFinished(500);
                    qWarning() << "[CppProjectModel] 内嵌 cmake 启动/运行失败:" << bundled;
                }
            }
        }
    }

    // ================================================================
    // 2. 收集候选（PATH + well-known 目录）
    // ================================================================
    QStringList candidates;

    QString cmakeName = "cmake";
#ifdef Q_OS_WIN
    cmakeName = "cmake.exe";
#endif

    // ---- 1a. 从 PATH 中查找 ----
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QStringList pathDirs = env.value("PATH").split(QDir::listSeparator(),
                                                   Qt::SkipEmptyParts);
    for (const QString& dir : pathDirs) {
        QString full = QDir(dir).absoluteFilePath(cmakeName);
        QFileInfo fi(full);
        if (fi.exists() && fi.isExecutable()) {
            candidates << full;
        }
    }

    // ---- 1b. Homebrew / MacPorts / Linux 标准路径 ----
    QStringList wellKnownDirs;
#ifdef Q_OS_MAC
    wellKnownDirs << "/opt/homebrew/bin"
                  << "/usr/local/bin"
                  << "/usr/bin";
#elif defined(Q_OS_WIN)
    wellKnownDirs << "C:/Program Files/CMake/bin"
                  << "C:/Program Files (x86)/CMake/bin";
#else
    wellKnownDirs << "/usr/bin"
                  << "/usr/local/bin"
                  << "/opt/cmake/bin";
#endif

    for (const QString& dir : wellKnownDirs) {
        QString full = QDir(dir).absoluteFilePath(cmakeName);
        QFileInfo fi(full);
        if (fi.exists() && fi.isExecutable()) {
            candidates << full;
        }
    }

    // ---- 去重 ----
    QStringList uniqueCandidates;
    for (const QString& c : candidates) {
        if (!uniqueCandidates.contains(c)) {
            uniqueCandidates << c;
        }
    }

    // ================================================================
    // 3. 逐个验证
    // ================================================================
    for (const QString& cand : uniqueCandidates) {

        // 过滤所有 macOS GUI shim
        if (cand.contains(".app/Contents/MacOS/")) {
            qDebug() << "[CppProjectModel] 跳过 GUI shim:" << cand;
            continue;
        }

        // 实际调用一次 cmake --version，5 秒内返回且输出匹配才接受
        QProcess proc;
        proc.setProgram(cand);
        proc.setArguments({"--version"});
        proc.setProcessChannelMode(QProcess::MergedChannels);
        proc.start();

        if (!proc.waitForStarted(3000)) {
            qDebug() << "[CppProjectModel] ✗ 启动失败:" << cand;
            proc.kill();
            proc.waitForFinished(500);
            continue;
        }
        if (!proc.waitForFinished(5000)) {
            qDebug() << "[CppProjectModel] ✗ 超时（可能是 GUI）:" << cand;
            proc.kill();
            proc.waitForFinished(500);
            continue;
        }

        QString out = QString::fromLocal8Bit(proc.readAllStandardOutput());
        if (out.contains("cmake version", Qt::CaseInsensitive)) {
            qDebug() << "[CppProjectModel] ✓ 使用 cmake:" << cand;
            return cand;
        } else {
            qDebug() << "[CppProjectModel] ✗ 输出不匹配:" << cand
                     << "->" << out.left(80);
        }
    }

    qWarning() << "[CppProjectModel] 未找到命令行版 cmake。";
    qWarning() << "  → macOS: brew install cmake";
    qWarning() << "  → 或在 [设置 → 环境变量] 里配置 CMAKE_HOME";
    return QString();
}

QString CppProjectModel::findMakeExecutable()
{
    QStringList names;
#ifdef Q_OS_WIN
    names << "mingw32-make.exe" << "make.exe" << "nmake.exe";
#else
    names << "make" << "gmake";
#endif
    QStringList extraPaths = {
        "/opt/homebrew/bin",
        "/usr/local/bin",
        "/usr/bin"
    };
    return findInPathAndCommonLocations(names, extraPaths);
}

QString CppProjectModel::findNinjaExecutable()
{
    QStringList names;
#ifdef Q_OS_WIN
    names << "ninja.exe" << "ninja";
#else
    names << "ninja";
#endif

    QStringList candidates;

    // ---------------- 0. 内嵌工具链（app bundle 内） ----------------
    //   打包后优先使用，开发期路径不存在则自动跳过
    {
        const QString bundledRoot = bundledToolchainRoot();
        if (!bundledRoot.isEmpty()) {
            QString bundled = bundledRoot + "/bin/ninja";
#ifdef Q_OS_WIN
            bundled += ".exe";
#endif
            candidates << bundled;
        }
    }

    // ---------------- 1. Homebrew 标准路径（最稳） ----------------
#ifdef Q_OS_MAC
    // Intel Mac
    candidates << "/usr/local/bin/ninja";
    candidates << "/usr/local/Cellar/ninja/1.13.2/bin/ninja";  // 万一没 link
    // Apple Silicon
    candidates << "/opt/homebrew/bin/ninja";
    // Cellar 里的最新版（用 QDir 遍历匹配）
    {
        QDir cellar("/usr/local/Cellar/ninja");
        if (cellar.exists()) {
            QStringList vers = cellar.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
            // 版本号排序（简单的字符串逆序即可，因为 "1.10" > "1.9" 在字典序下不成立，但这里够用）
            std::sort(vers.begin(), vers.end(), std::greater<QString>());
            for (const QString& v : vers) {
                candidates << "/usr/local/Cellar/ninja/" + v + "/bin/ninja";
            }
        }
        QDir cellar2("/opt/homebrew/Cellar/ninja");
        if (cellar2.exists()) {
            QStringList vers = cellar2.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
            std::sort(vers.begin(), vers.end(), std::greater<QString>());
            for (const QString& v : vers) {
                candidates << "/opt/homebrew/Cellar/ninja/" + v + "/bin/ninja";
            }
        }
    }
#elif defined(Q_OS_WIN)
    candidates << "C:/Program Files/ninja/ninja.exe";
    candidates << "C:/Program Files (x86)/ninja/ninja.exe";
#else
    candidates << "/usr/bin/ninja" << "/usr/local/bin/ninja";
#endif

    // ---------------- 2. PATH 里的（跳过 Qt） ----------------
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QStringList pathDirs = env.value("PATH").split(QDir::listSeparator(),
                                                   Qt::SkipEmptyParts);
    for (const QString& dir : pathDirs) {
        // 跳过 Qt 相关路径
        if (dir.contains("Qt", Qt::CaseInsensitive)) continue;
        for (const QString& name : names) {
            candidates << QDir(dir).absoluteFilePath(name);
        }
    }

    // ---------------- 3. 逐个验证 ----------------
    for (const QString& cand : candidates) {
        // 跳过 Qt 路径（双保险）
        if (cand.contains("Qt", Qt::CaseInsensitive)) {
            qDebug() << "[CppProjectModel] 跳过 Qt ninja:" << cand;
            continue;
        }
        // 跳过 GUI shim
        if (cand.contains(".app/Contents/MacOS/")) continue;

        QFileInfo fi(cand);
        if (!fi.exists() || !fi.isExecutable() || !fi.isFile()) continue;

        QProcess proc;
        proc.setProgram(cand);
        proc.setArguments({"--version"});
        proc.setProcessChannelMode(QProcess::MergedChannels);
        proc.start();

        if (!proc.waitForStarted(2000)) {
            proc.kill(); proc.waitForFinished(200);
            continue;
        }
        if (!proc.waitForFinished(3000)) {
            proc.kill(); proc.waitForFinished(200);
            continue;
        }

        QString out = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
        // ninja --version 输出形如 "1.13.2" 或 "1.11.1.git.kitware.jobserver-1"
        static QRegularExpression verRe(R"(^\d+\.\d+)");
        if (verRe.match(out).hasMatch()) {
            qDebug() << "[CppProjectModel] ✓ 使用 ninja:" << cand << "version:" << out;
            return cand;
        } else {
            qDebug() << "[CppProjectModel] ✗ ninja 候选输出异常:" << cand
                     << "->" << out.left(80);
        }
    }

    qWarning() << "[CppProjectModel] 未找到可用的 ninja。"
               << "请安装：brew install ninja";
    return QString();
}

// ====================================================================
// 静态：读取 CMakeCache.txt 里的 CMAKE_RUNTIME_OUTPUT_DIRECTORY
// ====================================================================
QString CppProjectModel::readCMakeRuntimeOutputDir(const QString& buildDir)
{
    const QString cacheFile = QDir(buildDir).absoluteFilePath("CMakeCache.txt");
    QFile file(cacheFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.startsWith("CMAKE_RUNTIME_OUTPUT_DIRECTORY")) {
            int eq = line.indexOf('=');
            if (eq > 0) {
                QString val = line.mid(eq + 1).trimmed();
                file.close();
                if (!val.isEmpty()) {
                    // 可能是相对路径，转为相对 buildDir
                    QFileInfo fi(val);
                    if (fi.isAbsolute()) return val;
                    return QDir(buildDir).absoluteFilePath(val);
                }
            }
        }
    }
    file.close();
    return QString();
}

} // namespace SprintIDE
