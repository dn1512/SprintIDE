// src/build/go_builder.cpp
#include "go_builder.h"
#include "../process_manager.h"
#include "../path_utils.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <config_manager.h>
#include <Application.h>

namespace SprintIDE {

QProcessEnvironment GoBuilder::prepareEnvironment(const BuildTask& task) {
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

    // 设置 GOPATH 如果未设置
    if (!env.contains("GOPATH")) {
        QString home = QDir::homePath();
        env.insert("GOPATH", PathUtils::joinPaths({home, "go"}));
    }

    // 如果设置了 GOROOT，将其 bin 目录加入 PATH
    QString goroot = env.value("GOROOT");
    if (!goroot.isEmpty()) {
        QString goBin = PathUtils::joinPaths({goroot, "bin"});
        QString path = env.value("PATH");
        env.insert("PATH", goBin + ":" + path);
    }

    return env;
}

bool GoBuilder::build(const BuildTask& task, BuildResult& result) {
    // 构建命令：go build -o outputName .
    QString goExe = findGoExecutable();
    if (goExe.isEmpty()) {
        result.errorMessage = "Go executable not found";
        result.builderName = "GoBuilder";
        return false;
    }

    // 确定输出目录和名称
    QString outDir = PathUtils::absolutePath(task.outputDir, task.projectPath);
    QString outName = task.outputName.isEmpty() ? "project" : task.outputName;
    if (task.sourceFiles.size() > 1) {
        // 多个源文件，可能直接编译项目（当前目录）
        // 我们不处理多文件单独编译，视为整个项目
    }
    // 如果 sourceFiles 非空，只编译第一个？但 Go 通常编译整个包，所以忽略 sourceFiles，直接 go build .
    // 但可以支持用户指定 main.go 路径
    QString buildTarget = ".";
    if (!task.sourceFiles.isEmpty()) {
        // 使用第一个源文件所在的目录作为包路径
        QString src = PathUtils::absolutePath(task.sourceFiles.first(), task.projectPath);
        buildTarget = QFileInfo(src).absolutePath();
    }

    // 准备参数
    QStringList args;
    args << "build";
    if (!task.enableDebug) {
        args << "-ldflags" << "-s -w"; // 去除调试信息
    }
    args << "-o" << PathUtils::joinPaths({outDir, outName});
    args << buildTarget;

    // 环境
    QProcessEnvironment env = prepareEnvironment(task);

    ProcessManager pm;
    pm.setEnvironment(env);
    ProcessManager::ProcessResult pr = pm.executeSync(goExe, args, task.projectPath, 60000);

    result.exitCode = pr.exitCode;
    result.fullOutput = pr.standardOutput + "\n" + pr.standardError;
    result.elapsedMs = pr.executionTime;
    result.success = pr.success && pr.exitCode == 0;
    result.builderName = "GoBuilder";

    if (!result.success) {
        result.errorLines = pr.standardError.split('\n', Qt::SkipEmptyParts);
    }

    // 可执行文件路径
    if (result.success) {
        QString exePath = PathUtils::joinPaths({outDir, outName});
#ifdef Q_OS_WIN
        exePath += ".exe";
#endif
        if (QFile::exists(exePath)) {
            result.executablePath = exePath;
        } else {
            // 可能输出文件名与项目名不同，尝试查找
            // 这里简化，如果不存在，尝试 go build 默认输出（当前目录下的可执行文件）
            QString defaultExe = PathUtils::joinPaths({task.projectPath, QFileInfo(task.projectPath).fileName()});
#ifdef Q_OS_WIN
            defaultExe += ".exe";
#endif
            if (QFile::exists(defaultExe)) {
                result.executablePath = defaultExe;
            }
        }
    }

    return true;
}

bool GoBuilder::clean(const BuildTask& task, BuildResult& result) {
    // 删除 bin/ 目录
    QString outDir = PathUtils::absolutePath(task.outputDir, task.projectPath);
    QDir dir(outDir);
    bool ok = true;
    if (dir.exists()) {
        // 清空目录，但保留目录本身
        QStringList files = dir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& entry : files) {
            QString entryPath = dir.absoluteFilePath(entry);
            QFileInfo info(entryPath);
            if (info.isFile()) {
                if (!QFile::remove(entryPath)) ok = false;
            } else if (info.isDir()) {
                if (!QDir(entryPath).removeRecursively()) ok = false;
            }
        }
    }
    result.success = ok;
    result.builderName = "GoBuilder";
    return ok;
}

bool GoBuilder::run(const BuildTask& task, const BuildResult& buildResult, QString& output) {
    if (buildResult.executablePath.isEmpty() || !QFile::exists(buildResult.executablePath)) {
        // 尝试在输出目录中找默认名称
        QString outDir = PathUtils::absolutePath(task.outputDir, task.projectPath);
        QString exeName = task.outputName.isEmpty() ? "project" : task.outputName;
#ifdef Q_OS_WIN
        exeName += ".exe";
#endif
        QString exePath = PathUtils::joinPaths({outDir, exeName});
        if (!QFile::exists(exePath)) {
            output = "Executable not found";
            return false;
        }
        return runProgram(exePath, task, output);
    }
    return runProgram(buildResult.executablePath, task, output);
}

// ==================== 私有辅助 ====================
bool GoBuilder::runProgram(const QString& executable, const BuildTask& task, QString& output) {
    ProcessManager pm;
    pm.setEnvironment(prepareEnvironment(task));
    ProcessManager::ProcessResult pr = pm.executeSync(executable, task.extraArgs, task.projectPath, 0);
    output = pr.standardOutput + "\n" + pr.standardError;
    return pr.success && pr.exitCode == 0;
}

QString GoBuilder::findGoExecutable() const {
    QString goCmd = "go";
#ifdef Q_OS_WIN
    goCmd += ".exe";
#endif
    ProcessManager pm;
    ProcessManager::ProcessResult pr = pm.executeSync(goCmd, {"version"}, QString(), 2000);
    if (pr.success) {
        return goCmd;
    }
    // 检查环境变量 GOROOT
    QString goroot = QProcessEnvironment::systemEnvironment().value("GOROOT");
    if (!goroot.isEmpty()) {
        QString goExe = PathUtils::joinPaths({goroot, "bin", "go"});
#ifdef Q_OS_WIN
        goExe += ".exe";
#endif
        if (QFile::exists(goExe)) return goExe;
    }
    return QString();
}

} // namespace SprintIDE
