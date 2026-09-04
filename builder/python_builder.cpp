// src/build/python_builder.cpp
#include "python_builder.h"
#include "../process_manager.h"
#include "../path_utils.h"
#include <Application.h>
#include <QDir>
#include <QDirIterator>
#include <QFile>

#include "config_manager.h"
#include "python/python_project_model.h"
#include "project_manager.h"
#include "python/python_dependency_manager.h"

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

    // 若设置了 PYTHON_HOME，则将 ${PYTHON_HOME}/bin 加入 PATH
    QString pythonHome = env.value("PYTHON_HOME");
    if (!pythonHome.isEmpty()) {
        QString binPath = PathUtils::joinPaths({pythonHome, "bin"});
        if (QFile::exists(binPath)) {
            QString path = env.value("PATH");
            env.insert("PATH", binPath + QDir::listSeparator() + path);
        }
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

    bool installDeps = task.aiCustomPayload.value("installDeps", false).toBool();
    if (installDeps) {
        QString interpreter = task.sdkConfig.value("pythonInterpreter").toString();
        if (interpreter.isEmpty()) {
            auto model = ProjectManager::instance()->pythonProjectModel();
            if (model) interpreter = model->interpreterPath();
        }
        if (interpreter.isEmpty()) interpreter = "python3";
        PythonDependencyManager::installDependencies(task.projectPath, interpreter,
                                                     [&](const QString& output) { result.fullOutput += output + "\n"; });
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
    // 获取解释器
    QString python = task.sdkConfig.value("pythonInterpreter").toString();
    if (python.isEmpty()) {
        // 如果未设置，尝试从模型获取（但模型可能在外部未传递，这里作为备选）
        auto model = ProjectManager::instance()->pythonProjectModel();
        if (model) {
            python = model->interpreterPath();
        }
    }
    if (python.isEmpty()) {
        python = "python3";
    }

    // 诊断日志（输出到 output，也会显示在面板）
    output = QString("使用 Python: %1\n").arg(python);

    // 获取入口
    QString entry = task.aiCustomPayload.value("entryPath").toString();
    QString entryType = task.aiCustomPayload.value("entryType").toString();
    if (entry.isEmpty()) {
        entry = buildResult.executablePath;
    }
    if (entry.isEmpty()) {
        output += "错误: 未指定入口点";
        return false;
    }

    QStringList args;
    if (entryType == "module") {
        args << "-m" << entry;
    } else {
        args << entry;
    }
    args << task.aiCustomPayload.value("programArgs").toStringList();

    QProcessEnvironment env = prepareEnvironment(task);
    QVariantMap envMap = task.aiCustomPayload.value("envVars").toMap();
    for (auto it = envMap.begin(); it != envMap.end(); ++it) {
        env.insert(it.key(), it.value().toString());
    }

    ProcessManager pm;
    pm.setEnvironment(env);
    ProcessManager::ProcessResult pr = pm.executeSync(python, args, task.projectPath, 0);
    output += pr.standardOutput + "\n" + pr.standardError;
    return pr.success && pr.exitCode == 0;
}

QString PythonBuilder::getExecutablePath(const BuildTask& task)
{
    // Python 是解释型语言，直接返回源文件路径
    if (!task.sourceFiles.isEmpty()) {
        return PathUtils::absolutePath(task.sourceFiles.first(), task.projectPath);
    }
    return QString();
}

} // namespace SprintIDE
