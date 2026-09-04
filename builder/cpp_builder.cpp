// src/build/cpp_builder.cpp
#include "cpp_builder.h"
#include "process_manager.h"
#include "compile_manager.h"   // 复用 parseCompilerOutput
#include "path_utils.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <config_manager.h>
#include "application.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDirIterator>


namespace SprintIDE {

// ==================== prepareEnvironment ====================
QProcessEnvironment CppBuilder::prepareEnvironment(const BuildTask& task) {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    // 从全局配置加载 SDK 环境变量
    ConfigManager* cfg = Application::instance().configManager();
    if (cfg) {
        QVariantMap sdkEnv = cfg->value("Sdk/Environment").toMap();
        for (auto it = sdkEnv.begin(); it != sdkEnv.end(); ++it) {
            env.insert(it.key(), it.value().toString());
        }
    }

    // 合并任务中的 sdkConfig（兼容旧方式）
    for (auto it = task.sdkConfig.begin(); it != task.sdkConfig.end(); ++it) {
        env.insert(it.key(), it.value().toString());
    }

    return env;
}

// ==================== build ====================
bool CppBuilder::build(const BuildTask& task, BuildResult& result) {
    // 1. 确定编译器
    QString compiler = task.sdkConfig.value("compiler").toString();
    if (compiler.isEmpty()) {
        compiler = autoSelectCompiler(task);
        if (compiler.isEmpty()) {
            result.errorMessage = "No C++ compiler found.";
            result.builderName = "CppBuilder";
            return false;
        }
    }

    // 2. 生成参数
    QStringList args = generateCompilerArgs(task);
    QString workDir = task.projectPath.isEmpty() ? QDir::currentPath() : task.projectPath;

    // 3. 准备环境
    QProcessEnvironment env = prepareEnvironment(task);

    // 4. 执行编译
    ProcessManager pm;
    pm.setEnvironment(env);
    ProcessManager::ProcessResult pr = pm.executeSync(compiler, args, workDir, 30000);

    // 5. 填充结果
    result.exitCode = pr.exitCode;
    result.fullOutput = pr.standardOutput + "\n" + pr.standardError;
    result.elapsedMs = pr.executionTime;
    result.success = pr.success && pr.exitCode == 0;
    result.builderName = "CppBuilder";

    // 6. 解析输出
    BuildResult parsed = parseOutput(result.fullOutput, task.projectPath);
    result.errorLines = parsed.errorLines;
    result.warningLines = parsed.warningLines;

    // 7. 如果成功，确定可执行文件路径
    if (result.success) {
        QString outDir = PathUtils::absolutePath(task.outputDir, task.projectPath);
        QString outName = task.outputName;
        if (outName.isEmpty()) {
            if (!task.sourceFiles.isEmpty()) {
                outName = QFileInfo(task.sourceFiles.first()).completeBaseName();
            } else {
                outName = "a.out";
            }
        }
#ifdef Q_OS_WIN
        if (!outName.endsWith(".exe")) outName += ".exe";
#endif
        result.executablePath = PathUtils::joinPaths({outDir, outName});
    }

    return true; // 启动成功，结果看 result.success
}

// ==================== clean ====================
bool CppBuilder::clean(const BuildTask& task, BuildResult& result) {
    QString outDir = PathUtils::absolutePath(task.outputDir, task.projectPath);
    QDir dir(outDir);
    if (!dir.exists()) {
        result.success = true;
        result.builderName = "CppBuilder";
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
    result.builderName = "CppBuilder";
    return ok;
}

// ==================== run ====================
bool CppBuilder::run(const BuildTask& task, const BuildResult& buildResult, QString& output) {
    if (buildResult.executablePath.isEmpty() || !QFile::exists(buildResult.executablePath)) {
        output = "Executable not found.";
        return false;
    }
    ProcessManager pm;
    pm.setEnvironment(prepareEnvironment(task));
    ProcessManager::ProcessResult pr = pm.executeSync(buildResult.executablePath,
                                                      task.extraArgs,
                                                      task.projectPath,
                                                      0);
    output = pr.standardOutput + "\n" + pr.standardError;
    return pr.success && pr.exitCode == 0;
}

bool CppBuilder::ensureCompileCommands(const QString& projectPath) {
    QDir projectDir(projectPath);
    QString jsonPath = projectDir.filePath("compile_commands.json");
    if (QFile::exists(jsonPath)) {
        return true; // 已存在，无需重复生成
    }

    // 递归查找所有 .cpp/.cxx/.cc/.c 文件
    QStringList cppFiles;
    QDirIterator it(projectPath, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString file = it.next();
        if (file.endsWith(".cpp") || file.endsWith(".cxx") ||
            file.endsWith(".cc") || file.endsWith(".c")) {
            cppFiles.append(file);
        }
    }

    if (cppFiles.isEmpty()) {
        // 没有源文件，不生成（或生成空文件？建议不生成）
        return false;
    }

    QJsonArray entries;
    for (const QString& file : cppFiles) {
        QString relativeFile = QDir(projectPath).relativeFilePath(file);
        // 简单编译命令（可扩展，如从项目配置读取 include 路径）
        QString includes = "-I" + projectDir.filePath("include") + " -I" + projectDir.filePath(".");
        QString command = QString("clang++ -c -std=c++17 -g %1 %2 -o %3.o")
                              .arg(relativeFile)
                              .arg(includes)
                              .arg(relativeFile.left(relativeFile.lastIndexOf('.')));

        QJsonObject entry;
        entry["directory"] = projectPath;
        entry["file"] = relativeFile;
        entry["command"] = command;
        entries.append(entry);
    }

    QJsonDocument doc(entries);
    QFile jsonFile(jsonPath);
    if (!jsonFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    jsonFile.write(doc.toJson());
    jsonFile.close();
    return true;
}

// ==================== 私有辅助 ====================
QStringList CppBuilder::generateCompilerArgs(const BuildTask& task) const {
    QStringList args;
    // C++ 标准
    QString standard = task.sdkConfig.value("cppStandard", "c++17").toString();
    args << "-std=" + standard;
    // 调试
    if (task.enableDebug) args << "-g";
    // 警告
    args << "-Wall" << "-Wextra";
    // 包含路径（从 sdkConfig 中读取 "includePaths" 列表）
    QStringList includes = task.sdkConfig.value("includePaths").toStringList();
    for (const QString& inc : includes) {
        args << "-I" + PathUtils::absolutePath(inc, task.projectPath);
    }
    // 源文件（绝对路径）
    for (const QString& src : task.sourceFiles) {
        args << PathUtils::absolutePath(src, task.projectPath);
    }
    // 输出文件
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

QString CppBuilder::autoSelectCompiler(const BuildTask& task) const {
    Q_UNUSED(task);
    QStringList candidates = {"g++", "clang++", "gcc", "clang"};
    for (const QString& c : candidates) {
        ProcessManager pm;
        ProcessManager::ProcessResult pr = pm.executeSync(c, {"--version"}, QString(), 2000);
        if (pr.success) return c;
    }
    return QString();
}

BuildResult CppBuilder::parseOutput(const QString& output, const QString& projectPath) const {
    BuildResult result;
    // 复用 CompileManager 的静态解析函数
    CompileResult cr = CompileManager::parseCompilerOutput(output, projectPath);
    result.errorLines = cr.errors;
    result.warningLines = cr.warnings;
    result.success = cr.success;
    return result;
}

QString CppBuilder::getExecutablePath(const BuildTask& task)
{
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

} // namespace SprintIDE
