// src/build/python_builder.cpp
#include "python_builder.h"
#include "../process_manager.h"
#include "../path_utils.h"
#include <Application.h>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <config_manager.h>

namespace SprintIDE {

QProcessEnvironment PythonBuilder::prepareEnvironment(const BuildTask& task) {
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

    // Python 特定：虚拟环境激活
    QString venv = task.sdkConfig.value("venv").toString();
    if (venv.isEmpty()) {
        // 从全局配置读取 Python 虚拟环境路径
        venv = env.value("PYTHON_VENV");
    }
    if (!venv.isEmpty()) {
        QString binPath = PathUtils::joinPaths({venv, "bin"});
        if (QFile::exists(binPath)) {
            QString path = env.value("PATH");
            env.insert("PATH", binPath + ":" + path);
        }
    }

    return env;
}

bool PythonBuilder::build(const BuildTask& task, BuildResult& result) {
    if (task.sourceFiles.isEmpty()) {
        result.errorMessage = "No Python source file provided.";
        result.builderName = "PythonBuilder";
        return false;
    }
    QString src = PathUtils::absolutePath(task.sourceFiles.first(), task.projectPath);
    if (!QFile::exists(src)) {
        result.errorMessage = "Source file not found: " + src;
        result.builderName = "PythonBuilder";
        return false;
    }

    // 执行语法检查：python -m py_compile
    ProcessManager pm;
    pm.setEnvironment(prepareEnvironment(task));
    ProcessManager::ProcessResult pr = pm.executeSync("python",
                                                      {"-m", "py_compile", src},
                                                      task.projectPath,
                                                      10000);
    result.exitCode = pr.exitCode;
    result.fullOutput = pr.standardOutput + "\n" + pr.standardError;
    result.elapsedMs = pr.executionTime;
    result.success = pr.success && pr.exitCode == 0;
    result.builderName = "PythonBuilder";

    // 提取错误行（简单方式）
    if (!result.success) {
        result.errorLines = pr.standardError.split('\n', Qt::SkipEmptyParts);
    }
    // 可执行文件路径设为源文件路径，用于 run
    result.executablePath = src;
    return true;
}

bool PythonBuilder::clean(const BuildTask& task, BuildResult& result) {
    // 删除 __pycache__ 目录和 .pyc 文件
    QString cacheDir = PathUtils::absolutePath("__pycache__", task.projectPath);
    bool ok = true;
    if (QDir(cacheDir).exists()) {
        if (!QDir(cacheDir).removeRecursively()) ok = false;
    }
    // 删除所有 .pyc 文件（递归扫描）
    QDirIterator it(task.projectPath, QStringList() << "*.pyc",
                    QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        if (!QFile::remove(it.next())) ok = false;
    }
    result.success = ok;
    result.builderName = "PythonBuilder";
    return ok;
}

bool PythonBuilder::run(const BuildTask& task, const BuildResult& buildResult, QString& output) {
    if (buildResult.executablePath.isEmpty() || !QFile::exists(buildResult.executablePath)) {
        output = "Source file not found.";
        return false;
    }
    ProcessManager pm;
    pm.setEnvironment(prepareEnvironment(task));
    ProcessManager::ProcessResult pr = pm.executeSync("python",
                                                      {buildResult.executablePath},
                                                      task.projectPath,
                                                      0);
    output = pr.standardOutput + "\n" + pr.standardError;
    return pr.success && pr.exitCode == 0;
}

} // namespace SprintIDE
