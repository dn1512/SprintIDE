// src/build/python_builder.cpp
#include "python_builder.h"
#include "../process_manager.h"
#include "../path_utils.h"
#include <Application.h>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>

#include "config_manager.h"
#include "python/python_project_model.h"
#include "project_manager.h"
#include "python/python_dependency_manager.h"

namespace SprintIDE {

// ============================================================
// 内部辅助：解析 Python 解释器路径
//   顺序：
//     1. task.sdkConfig["pythonInterpreter"]
//     2. PythonProjectModel::interpreterPath()
//     3. PYTHON_HOME/bin/python3（或 Windows 的 python.exe）
//     4. 兜底：python3
// ============================================================
static QString resolvePythonInterpreter(const BuildTask& task)
{
    // ---- 1. task.sdkConfig ----
    QString python = task.sdkConfig.value("pythonInterpreter").toString();
    if (!python.isEmpty()) return python;

    // ---- 2. PythonProjectModel ----
    auto model = ProjectManager::instance()->pythonProjectModel();
    if (model) {
        QString p = model->interpreterPath();
        if (!p.isEmpty()) return p;
    }

    // ---- 3. PYTHON_HOME ----
    QString pythonHome = task.sdkConfig.value("PYTHON_HOME").toString();
    if (pythonHome.isEmpty()) {
        ConfigManager* cfg = Application::instance().configManager();
        if (cfg) {
            QVariantMap sdkEnv = cfg->value("Sdk/Environment").toMap();
            pythonHome = sdkEnv.value("PYTHON_HOME").toString();
        }
    }
    if (!pythonHome.isEmpty()) {
        // 兼容两种填法：
        //   A. 填目录（如 /usr/local）       → 拼 /bin/python3
        //   B. 填可执行文件（如 /usr/local/bin/python3）→ 直接用
        QFileInfo homeInfo(pythonHome);
        if (homeInfo.isFile() && homeInfo.isExecutable()) {
            // 情况 B：已经是解释器可执行文件
            return pythonHome;
        }
        // 情况 A：当目录处理
#ifdef Q_OS_WIN
        QString candidate = pythonHome + "/python.exe";
#else
        QString candidate = pythonHome + "/bin/python3";
#endif
        if (QFile::exists(candidate)) return candidate;
    }

    // ---- 4. 兜底 ----
    return "python3";
}

// ============================================================
// 内部辅助：为项目级编译解析 Python 入口文件
// ============================================================
static QString resolveProjectEntry(const QString& projectPath)
{
    if (projectPath.isEmpty()) return QString();

    // ---- 1. 项目根目录的常见入口 ----
    QDir projDir(projectPath);
    const QStringList entryNames = {"main.py", "__main__.py", "app.py", "run.py"};
    for (const QString& name : entryNames) {
        QString p = projDir.absoluteFilePath(name);
        if (QFile::exists(p)) return p;
    }

    // ---- 2. src/ 下的入口 ----
    QDir srcDir(projDir.absoluteFilePath("src"));
    if (srcDir.exists()) {
        for (const QString& name : {"main.py", "__main__.py"}) {
            QString p = srcDir.absoluteFilePath(name);
            if (QFile::exists(p)) return p;
        }
    }

    // ---- 3. 兜底：扫第一个 .py，跳过虚拟环境和缓存 ----
    QDirIterator it(projectPath, {"*.py"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString p = it.next();
        if (p.contains("/.venv/")      || p.contains("/venv/") ||
            p.contains("/env/")        || p.contains("/__pycache__/") ||
            p.contains("/.git/")) {
            continue;
        }
        return p;
    }

    return QString();
}

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

bool PythonBuilder::build(const BuildTask& task, BuildResult& result)
{
    result.builderName = "PythonBuilder";

    // ========== 1. 确定要检查的源文件 ==========
    QString src;
    if (!task.sourceFiles.isEmpty()) {
        src = PathUtils::absolutePath(task.sourceFiles.first(), task.projectPath);
    } else {
        src = resolveProjectEntry(task.projectPath);
    }

    if (src.isEmpty()) {
        result.errorMessage = "No Python source file provided "
                              "and no entry file could be found in the project.";
        result.errorLines << result.errorMessage;
        return false;
    }
    if (!QFile::exists(src)) {
        result.errorMessage = "Source file not found: " + src;
        result.errorLines << result.errorMessage;
        return false;
    }

    // ========== 2. 依赖安装（可选） ==========
    bool installDeps = task.aiCustomPayload.value("installDeps", false).toBool();
    if (installDeps) {
        QString interpreter = resolvePythonInterpreter(task);
        PythonDependencyManager::installDependencies(
            task.projectPath, interpreter,
            [&](const QString& output) { result.fullOutput += output + "\n"; });
    }

    // ========== 3. 语法检查 ==========
    // ★ 关键修复：用解析出的解释器，而不是硬编码 "python"
    QString python = resolvePythonInterpreter(task);

    ProcessManager pm;
    pm.setEnvironment(prepareEnvironment(task));
    ProcessManager::ProcessResult pr = pm.executeSync(
        python,
        {"-m", "py_compile", src},
        task.projectPath,
        10000);

    result.exitCode   = pr.exitCode;
    result.fullOutput = QString("使用解释器: %1\n").arg(python)
                      + pr.standardOutput + "\n" + pr.standardError;
    result.elapsedMs  = pr.executionTime;
    result.success    = pr.success && pr.exitCode == 0;

    // 提取错误行
    if (!result.success) {
        result.errorLines = pr.standardError.split('\n', Qt::SkipEmptyParts);
        if (result.errorLines.isEmpty()) {
            // 进程根本没启动（FailedToStart）——补一条明确的错误
            result.errorLines << QString("无法启动 Python 解释器: %1").arg(python);
            result.errorLines << QString("错误信息: %1").arg(pr.errorMessage);
        }
    }

    result.executablePath = src;
    return true;
}

bool PythonBuilder::clean(const BuildTask& task, BuildResult& result) {
    QString cacheDir = PathUtils::absolutePath("__pycache__", task.projectPath);
    bool ok = true;
    if (QDir(cacheDir).exists()) {
        if (!QDir(cacheDir).removeRecursively()) ok = false;
    }
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
    // ★ 复用同一个解释器解析逻辑
    QString python = resolvePythonInterpreter(task);

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
    if (!task.sourceFiles.isEmpty()) {
        return PathUtils::absolutePath(task.sourceFiles.first(), task.projectPath);
    }
    return QString();
}

} // namespace SprintIDE
