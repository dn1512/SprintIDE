#include "cpp_builder.h"
#include "process_manager.h"
#include "compile_manager.h"
#include "path_utils.h"
#include "cpp/cpp_project_model.h"

#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDirIterator>
#include <QProcess>
#include <QProcessEnvironment>

#include <config_manager.h>
#include "application.h"

namespace SprintIDE {

// ====================================================================
// prepareEnvironment
// ====================================================================
QProcessEnvironment CppBuilder::prepareEnvironment(const BuildTask& task)
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    ConfigManager* cfg = Application::instance().configManager();
    if (cfg) {
        QVariantMap sdkEnv = cfg->value("Sdk/Environment").toMap();
        for (auto it = sdkEnv.begin(); it != sdkEnv.end(); ++it) {
            env.insert(it.key(), it.value().toString());
        }
    }
    for (auto it = task.sdkConfig.begin(); it != task.sdkConfig.end(); ++it) {
        env.insert(it.key(), it.value().toString());
    }
    return env;
}

// ====================================================================
// detectCompiler（静态，供外部复用）
// ====================================================================
QString CppBuilder::detectCompiler()
{
    QStringList candidates;
#ifdef Q_OS_WIN
    candidates << "cl.exe" << "clang++.exe" << "g++.exe" << "clang.exe" << "gcc.exe";
#else
    candidates << "clang++" << "g++" << "clang" << "gcc";
#endif
    for (const QString& c : candidates) {
        ProcessManager pm;
        ProcessManager::ProcessResult pr = pm.executeSync(c, {"--version"}, QString(), 2000);
        if (pr.success) return c;
    }
    return QString();
}

// ====================================================================
// build —— 分发到 CMake 或 单文件
// ====================================================================
bool CppBuilder::build(const BuildTask& task, BuildResult& result)
{
    result.builderName = "CppBuilder";

    CppProjectModel model(task.projectPath);
    model.load();

    qDebug() << "[CppBuilder] build(): projectPath =" << task.projectPath
             << "buildSystem =" << model.buildSystemName();

    if (model.buildSystem() == CppBuildSystem::CMake) {
        return buildWithCMake(task, result);
    }
    return buildSingleFile(task, result);
}

// ====================================================================
// buildWithCMake
// ====================================================================
bool CppBuilder::buildWithCMake(const BuildTask& task, BuildResult& result)
{
    CppProjectModel model(task.projectPath);
    model.load();

    const QString cmake = CppProjectModel::findCMakeExecutable();
    if (cmake.isEmpty()) {
        result.errorMessage = "CMake not found in PATH. "
                              "Please install CMake and ensure it is on your PATH.";
        result.success = false;
        return false;
    }

    const QString buildDir = model.buildDirectory();
    if (!QDir().mkpath(buildDir)) {
        result.errorMessage = QString("Cannot create build directory: %1").arg(buildDir);
        result.success = false;
        return false;
    }

    QProcessEnvironment env = prepareEnvironment(task);
    const QString workDir = task.projectPath.isEmpty() ? QDir::currentPath() : task.projectPath;

    // ================= 1. 配置阶段 =================
    const QString cacheFile = QDir(buildDir).absoluteFilePath("CMakeCache.txt");
    const QString wantedBuildType = task.enableDebug ? "Debug" : "Release";

    // 检查是否需要重新 configure
    bool needConfigure = !QFile::exists(cacheFile);
    QString cachedBuildType;

    if (!needConfigure) {
        // 读 CMakeCache.txt 里现有的 CMAKE_BUILD_TYPE
        QFile cache(cacheFile);
        if (cache.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&cache);
            while (!in.atEnd()) {
                const QString line = in.readLine();
                if (line.startsWith("CMAKE_BUILD_TYPE:")) {
                    int eq = line.indexOf('=');
                    if (eq > 0) {
                        cachedBuildType = line.mid(eq + 1).trimmed();
                        break;
                    }
                }
            }
            cache.close();
        }

        if (cachedBuildType != wantedBuildType) {
            qDebug() << "[CppBuilder] build type 不匹配: cache ="
                     << (cachedBuildType.isEmpty() ? QString("<empty>") : cachedBuildType)
                     << "期望 =" << wantedBuildType
                     << "→ 强制重新 configure";
            needConfigure = true;
        } else {
            qDebug() << "[CppBuilder] build type 匹配 (" << wantedBuildType
                     << ")，跳过 configure";
        }
    }

    if (needConfigure) {
        QStringList configArgs;
        configArgs << "-S" << workDir
                   << "-B" << buildDir
                   << "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
                   << "-DCMAKE_BUILD_TYPE=" + wantedBuildType;   // 统一传，不再 if-else

        // 生成器：优先 Ninja，不传 -DCMAKE_MAKE_PROGRAM
        const QString ninja = CppProjectModel::findNinjaExecutable();
        if (!ninja.isEmpty()) {
            configArgs << "-G" << "Ninja";
            configArgs << "-DCMAKE_MAKE_PROGRAM=" + ninja;   // 关键修复
            qDebug() << "[CppBuilder] 使用 Ninja 生成器（系统 ninja:" << ninja << "）";
        } else {
            const QString make = CppProjectModel::findMakeExecutable();
            if (!make.isEmpty()) {
                configArgs << "-G" << "Unix Makefiles";
                configArgs << "-DCMAKE_MAKE_PROGRAM=" + make;   // 同样处理 make
                qDebug() << "[CppBuilder] 使用 Unix Makefiles 生成器";
            } else {
                qWarning() << "[CppBuilder] 未找到 Ninja / Make";
            }
        }

        // 用户额外参数
        for (const QString& a : task.extraArgs) {
            configArgs << a;
        }

        qDebug() << "[CppBuilder] CMake configure:" << cmake << configArgs;

        ProcessManager pm;
        pm.setEnvironment(env);
        ProcessManager::ProcessResult pr =
            pm.executeSync(cmake, configArgs, workDir, 120000);

        result.fullOutput += pr.standardOutput + "\n" + pr.standardError;
        result.elapsedMs += pr.executionTime;

        if (!pr.success || pr.exitCode != 0) {
            result.exitCode = pr.exitCode;
            result.success = false;
            BuildResult parsed = parseOutput(result.fullOutput, workDir);
            result.errorLines = parsed.errorLines;
            result.warningLines = parsed.warningLines;
            return true;
        }
    }

    // ================= 2. 构建阶段 =================
    QStringList buildArgs;
    buildArgs << "--build" << buildDir;

    // 验证 target 有效性（核心修复）
    QString requestedTarget = task.outputName;
    QStringList validTargets = model.targetNames();
    QString actualTarget;

    if (!requestedTarget.isEmpty() && validTargets.contains(requestedTarget)) {
        actualTarget = requestedTarget;
        qDebug() << "[CppBuilder] 使用指定 target:" << actualTarget;
    } else {
        if (!requestedTarget.isEmpty()) {
            qDebug() << "[CppBuilder] 请求的 target" << requestedTarget
                     << "不存在，改用默认 target";
        }
        actualTarget = model.defaultTarget();
        qDebug() << "[CppBuilder] 使用默认 target:" << actualTarget
                 << "（可选 target 列表:" << validTargets << "）";
    }

    if (!actualTarget.isEmpty()) {
        buildArgs << "--target" << actualTarget;
    }

#ifdef Q_OS_WIN
    buildArgs << "--" << "/m";
#else
    buildArgs << "--" << "-j4";
#endif

    qDebug() << "[CppBuilder] CMake build:" << cmake << buildArgs;

    ProcessManager pm;
    pm.setEnvironment(env);
    ProcessManager::ProcessResult pr =
        pm.executeSync(cmake, buildArgs, workDir, 600000);

    result.fullOutput += pr.standardOutput + "\n" + pr.standardError;
    result.exitCode = pr.exitCode;
    result.elapsedMs += pr.executionTime;
    result.success = pr.success && pr.exitCode == 0;

    BuildResult parsed = parseOutput(result.fullOutput, workDir);
    result.errorLines = parsed.errorLines;
    result.warningLines = parsed.warningLines;

    // ================= 3. 定位可执行文件 =================
    if (result.success) {
        QString target = actualTarget;
        if (target.isEmpty()) {
            target = model.defaultTarget();
        }
        QString exe = model.guessExecutablePath(target);
        result.executablePath = exe;
        if (exe.isEmpty()) {
            qWarning() << "[CppBuilder] Build succeeded but executable not located."
                       << "target =" << target;
        }
    }

    return true;
}

// ====================================================================
// buildSingleFile —— 原逻辑保留
// ====================================================================
bool CppBuilder::buildSingleFile(const BuildTask& task, BuildResult& result)
{
    // 1. 确定编译器
    QString compiler = task.sdkConfig.value("compiler").toString();
    if (compiler.isEmpty()) {
        compiler = autoSelectCompiler(task);
        if (compiler.isEmpty()) {
            result.errorMessage = "No C++ compiler found.";
            result.success = false;
            return false;
        }
    }

    // 2. 生成参数
    QStringList args = generateCompilerArgs(task);
    QString workDir = task.projectPath.isEmpty() ? QDir::currentPath() : task.projectPath;

    // 3. 准备环境
    QProcessEnvironment env = prepareEnvironment(task);

    // 4. 执行编译
    m_processManager.setEnvironment(env);
    ProcessManager::ProcessResult pr = m_processManager.executeSync(compiler, args, workDir, 60000);

    // 5. 填充结果
    result.exitCode = pr.exitCode;
    result.fullOutput = pr.standardOutput + "\n" + pr.standardError;
    result.elapsedMs = pr.executionTime;
    result.success = pr.success && pr.exitCode == 0;

    // 6. 解析输出
    BuildResult parsed = parseOutput(result.fullOutput, task.projectPath);
    result.errorLines = parsed.errorLines;
    result.warningLines = parsed.warningLines;

    // 7. 成功则确定可执行文件路径
    if (result.success) {
        result.executablePath = getExecutablePath(task);
    }

    return true;
}

// ====================================================================
// clean
// ====================================================================
bool CppBuilder::clean(const BuildTask& task, BuildResult& result)
{
    result.builderName = "CppBuilder";

    CppProjectModel model(task.projectPath);
    model.load();

    if (model.buildSystem() == CppBuildSystem::CMake) {
        return cleanWithCMake(task, result);
    }

    // ---- 非 CMake：沿用原有删除产物逻辑 ----
    QString outDir = PathUtils::absolutePath(task.outputDir, task.projectPath);
    QDir dir(outDir);
    if (!dir.exists()) {
        result.success = true;
        return true;
    }

    QStringList filters;
    filters << "*.o" << "*.obj" << "*.exe" << "*.out" << "*.a" << "*.so" << "*.dylib";
    QStringList files = dir.entryList(filters, QDir::Files);
    bool ok = true;
    for (const QString& f : files) {
        if (!dir.remove(f)) ok = false;
    }
    result.success = ok;
    return ok;
}

// ====================================================================
// cleanWithCMake
// ====================================================================
bool CppBuilder::cleanWithCMake(const BuildTask& task, BuildResult& result)
{
    CppProjectModel model(task.projectPath);
    model.load();

    const QString cmake = CppProjectModel::findCMakeExecutable();
    const QString buildDir = model.buildDirectory();

    if (cmake.isEmpty() || !QDir(buildDir).exists()) {
        // CMake 不可用或尚未构建 → 直接删除 build 目录
        if (QDir(buildDir).exists()) {
            QDir(buildDir).removeRecursively();
        }
        result.success = true;
        return true;
    }

    QStringList args;
    args << "--build" << buildDir << "--target" << "clean";

    m_processManager.setEnvironment(prepareEnvironment(task));
    ProcessManager::ProcessResult pr =
        m_processManager.executeSync(cmake, args, task.projectPath, 60000);

    result.fullOutput = pr.standardOutput + "\n" + pr.standardError;
    result.exitCode = pr.exitCode;
    result.success = pr.success && pr.exitCode == 0;
    return result.success;
}

// ====================================================================
// run —— 保留原逻辑（同步执行；UI 端的异步由 MainWindow 负责）
// ====================================================================
bool CppBuilder::run(const BuildTask& task, const BuildResult& buildResult, QString& output)
{
    if (buildResult.executablePath.isEmpty() || !QFile::exists(buildResult.executablePath)) {
        output = "Executable not found.";
        return false;
    }
    m_processManager.setEnvironment(prepareEnvironment(task));
    ProcessManager::ProcessResult pr =
        m_processManager.executeSync(buildResult.executablePath,
                                     task.extraArgs,
                                     task.projectPath,
                                     0);
    output = pr.standardOutput + "\n" + pr.standardError;
    return pr.success && pr.exitCode == 0;
}

// ====================================================================
// getExecutablePath
// ====================================================================
QString CppBuilder::getExecutablePath(const BuildTask& task)
{
    CppProjectModel model(task.projectPath);
    model.load();

    if (model.buildSystem() == CppBuildSystem::CMake) {
        // 验证 target 有效性（与 buildWithCMake 一致）
        QString target = task.outputName;
        QStringList validTargets = model.targetNames();

        if (!target.isEmpty() && validTargets.contains(target)) {
            qDebug() << "[CppBuilder] getExecutablePath: 使用指定 target:" << target;
        } else {
            if (!target.isEmpty()) {
                qDebug() << "[CppBuilder] getExecutablePath: 请求的 target"
                         << target << "不存在，改用默认";
            }
            target = model.defaultTarget();
            qDebug() << "[CppBuilder] getExecutablePath: 使用默认 target:" << target;
        }
        return model.guessExecutablePath(target);
    }

    // ---- 非 CMake：沿用原逻辑 ----
    QString outDir = PathUtils::absolutePath(task.outputDir, task.projectPath);
    QString outName = task.outputName;

    if (outName.isEmpty() && !task.sourceFiles.isEmpty()) {
        outName = QFileInfo(task.sourceFiles.first()).completeBaseName();
    }
    if (outName.isEmpty()) {
        outName = "a.out";
    }

#ifdef Q_OS_WIN
    if (!outName.endsWith(".exe", Qt::CaseInsensitive)) {
        outName += ".exe";
    }
#endif
    return PathUtils::joinPaths({outDir, outName});
}

// ====================================================================
// ensureCompileCommands
// ====================================================================
bool CppBuilder::ensureCompileCommands(const QString& projectPath)
{
    CppProjectModel model(projectPath);
    model.load();

    if (model.buildSystem() == CppBuildSystem::CMake) {
        const QString buildDir = model.buildDirectory();
        const QString builtJson = QDir(buildDir).absoluteFilePath("compile_commands.json");
        const QString rootJson  = QDir(projectPath).absoluteFilePath("compile_commands.json");

        // ---- 1. build 目录里已存在 → 只做软链，不再 configure ----
        if (QFile::exists(builtJson)) {
            if (QFile::exists(rootJson)) QFile::remove(rootJson);
            if (!QFile::link(builtJson, rootJson)) {
                QFile::copy(builtJson, rootJson);
            }
            return true;
        }

        // ---- 2. 不存在 → 不主动 configure，交给用户主动编译时触发 ----
        // 这样避免"仅仅打开项目/文件"就触发 cmake，进而可能弹出 GUI
        qDebug() << "[CppBuilder] compile_commands.json 尚未生成，"
                 << "等待用户主动编译时生成。";
        return false;
    }

    // ---- 非 CMake：沿用旧的手工生成逻辑 ----
    QDir projectDir(projectPath);
    QString jsonPath = projectDir.filePath("compile_commands.json");
    if (QFile::exists(jsonPath)) return true;

    QStringList cppFiles;
    QDirIterator it(projectPath, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString file = it.next();
        if (file.endsWith(".cpp") || file.endsWith(".cxx") ||
            file.endsWith(".cc") || file.endsWith(".c")) {
            if (file.contains("/build/") || file.contains("/.git/")) continue;
            cppFiles.append(file);
        }
    }
    if (cppFiles.isEmpty()) return false;

    const QString compiler = detectCompiler();
    if (compiler.isEmpty()) return false;

    QJsonArray entries;
    for (const QString& file : cppFiles) {
        QString relativeFile = QDir(projectPath).relativeFilePath(file);
        QString includes = "-I" + projectDir.filePath("include") + " -I" + projectDir.filePath(".");
        QString command = QString("%1 -c -std=c++17 -g %2 %3 -o %4.o")
                              .arg(compiler, relativeFile, includes,
                                   relativeFile.left(relativeFile.lastIndexOf('.')));

        QJsonObject entry;
        entry["directory"] = projectPath;
        entry["file"] = relativeFile;
        entry["command"] = command;
        entries.append(entry);
    }

    QJsonDocument doc(entries);
    QFile jsonFile(jsonPath);
    if (!jsonFile.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    jsonFile.write(doc.toJson());
    jsonFile.close();
    return true;
}

// ====================================================================
// 私有辅助
// ====================================================================
QStringList CppBuilder::generateCompilerArgs(const BuildTask& task) const
{
    QStringList args;
    QString standard = task.sdkConfig.value("cppStandard", "c++17").toString();
    args << "-std=" + standard;
    if (task.enableDebug) args << "-g";
    args << "-Wall" << "-Wextra";

    QStringList includes = task.sdkConfig.value("includePaths").toStringList();
    for (const QString& inc : includes) {
        args << "-I" + PathUtils::absolutePath(inc, task.projectPath);
    }
    for (const QString& src : task.sourceFiles) {
        args << PathUtils::absolutePath(src, task.projectPath);
    }
    QString outDir = PathUtils::absolutePath(task.outputDir, task.projectPath);
    QString outName = task.outputName;
    if (outName.isEmpty()) {
        if (!task.sourceFiles.isEmpty())
            outName = QFileInfo(task.sourceFiles.first()).completeBaseName();
        else
            outName = "a.out";
    }
#ifdef Q_OS_WIN
    if (!outName.endsWith(".exe")) outName += ".exe";
#endif
    args << "-o" + PathUtils::joinPaths({outDir, outName});
    return args;
}

QString CppBuilder::autoSelectCompiler(const BuildTask& task) const
{
    Q_UNUSED(task);
    return CppBuilder::detectCompiler();
}

BuildResult CppBuilder::parseOutput(const QString& output, const QString& projectPath) const
{
    BuildResult result;
    CompileResult cr = CompileManager::parseCompilerOutput(output, projectPath);
    result.errorLines = cr.errors;
    result.warningLines = cr.warnings;
    result.success = cr.success;
    return result;
}

} // namespace SprintIDE
