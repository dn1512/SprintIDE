// src/build/python_builder.cpp
#include "python_builder.h"
#include "../process_manager.h"
#include "../path_utils.h"
#include <Application.h>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#include "config_manager.h"
#include "python/python_project_model.h"
#include "project_manager.h"
#include "python/python_dependency_manager.h"

namespace SprintIDE {

    // ============================================================
    // 文件内辅助：在 PATH 里按平台候选名查找 Python 解释器
    //   Windows: python.exe python3.exe py.exe（Python Launcher）
    //   Unix   : python3 python
    // 返回空串 = 找不到
    // ============================================================
    static QString findPythonInPath()
    {
        #ifdef Q_OS_WIN
        const QStringList candidates = {
            QStringLiteral("python.exe"),
            QStringLiteral("python3.exe"),
            QStringLiteral("py.exe"),
        };
        #else
        const QStringList candidates = {
            QStringLiteral("python3"),
            QStringLiteral("python"),
        };
        #endif
        for (const QString& name : candidates) {
            const QString p = QStandardPaths::findExecutable(name);
            if (!p.isEmpty()) {
                qDebug() << "[Python] PATH 中找到解释器:" << name << "->" << p;
                return p;
            }
        }
        return QString();
    }

    // ============================================================
    // 跨平台 Python 解释器解析（供 PythonBuilder 与 MainWindow 共用）
    //   顺序：
    //     1. task.sdkConfig["pythonInterpreter"]  —— 显式指定（绝对路径或裸名）
    //     2. PythonProjectModel::interpreterPath() —— 项目 / venv 探测
    //     3. PYTHON_HOME —— 支持填目录或可执行文件本身
    //     4. PATH 查找 —— 跨平台兜底
    //     5. 平台习惯名 —— 都找不到时返回，让调用方给出明确错误
    // ============================================================
    QString PythonBuilder::resolvePythonInterpreter(const BuildTask& task)
    {
        // ---- 1. task.sdkConfig["pythonInterpreter"] ----
        const QString explicitPy = task.sdkConfig.value("pythonInterpreter").toString();
        if (!explicitPy.isEmpty()) {
            const QFileInfo fi(explicitPy);
            if (fi.isFile() && fi.isExecutable()) {
                return explicitPy;
            }
            // 用户可能只写了裸名，如 "py" / "python.exe"
            const QString found = QStandardPaths::findExecutable(explicitPy);
            if (!found.isEmpty()) {
                return found;
            }
            qWarning() << "[Python] pythonInterpreter 指定了但找不到:" << explicitPy;
        }

        // ---- 2. PythonProjectModel::interpreterPath() ----
        if (auto model = ProjectManager::instance()->pythonProjectModel()) {
            const QString p = model->interpreterPath();
            if (!p.isEmpty()) {
                const QFileInfo fi(p);
                if (fi.isFile() && fi.isExecutable()) {
                    return p;
                }
            }
        }

        // ---- 3. PYTHON_HOME（支持填目录或可执行文件） ----
        QString pythonHome = task.sdkConfig.value("PYTHON_HOME").toString();
        if (pythonHome.isEmpty()) {
            if (auto* cfg = Application::instance().configManager()) {
                const QVariantMap sdkEnv = cfg->value("Sdk/Environment").toMap();
                pythonHome = sdkEnv.value("PYTHON_HOME").toString();
            }
        }
        if (!pythonHome.isEmpty()) {
            const QFileInfo homeInfo(pythonHome);

            // 3a. 用户填的是可执行文件本身
            if (homeInfo.isFile() && homeInfo.isExecutable()) {
                return pythonHome;
            }

            // 3b. 用户填的是安装目录 按平台试多个候选
            #ifdef Q_OS_WIN
            const QStringList localCandidates = {
                pythonHome + "/python.exe",
                pythonHome + "/python3.exe",
            };
            #else
            const QStringList localCandidates = {
                pythonHome + "/bin/python3",
                pythonHome + "/bin/python",
                pythonHome + "/python3",
            };
            #endif
            for (const QString& c : localCandidates) {
                if (QFile::exists(c)) {
                    qDebug() << "[Python] 从 PYTHON_HOME 命中:" << c;
                    return c;
                }
            }
            qWarning() << "[Python] PYTHON_HOME 已设置但没找到解释器:" << pythonHome;
        }

        // ---- 4. PATH 查找（跨平台兜底） ----
        const QString fromPath = findPythonInPath();
        if (!fromPath.isEmpty()) {
            return fromPath;
        }

        // ---- 5. 真的找不到 返回平台习惯名，让调用方报明确错误 ----
        #ifdef Q_OS_WIN
        qWarning() << "[Python] 未找到任何 Python 解释器，"
        "请安装 Python 或在设置里配置 PYTHON_HOME / pythonInterpreter";
        return QStringLiteral("python.exe");
        #else
        qWarning() << "[Python] 未找到任何 Python 解释器，"
        "请安装 Python 或在设置里配置 PYTHON_HOME / pythonInterpreter";
        return QStringLiteral("python3");
        #endif
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
        //   v8.6：Windows 上路径分隔符是 '\\'，原来只检查 '/' 会失效
        QDirIterator it(projectPath, {"*.py"}, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString p = it.next();
            if (p.contains("/.venv/")         || p.contains("/venv/") ||
                p.contains("/env/")           || p.contains("/__pycache__/") ||
                p.contains("/.git/")          ||
                p.contains("\\venv\\")        || p.contains("\\env\\") ||
                p.contains("\\__pycache__\\") || p.contains("\\.git\\")) {
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

        // ============================================================
        //  把 PYTHON_HOME 对应的 bin 目录注入 PATH
        //    - Windows: python.exe 在 PYTHON_HOME\ 根目录
        //               pip / 脚本 在 PYTHON_HOME\Scripts\
        //    - Unix   : 在 PYTHON_HOME/bin/
        // ============================================================
        const QString pythonHome = env.value("PYTHON_HOME");
        if (!pythonHome.isEmpty()) {
            QStringList binPaths;
            #ifdef Q_OS_WIN
            binPaths << pythonHome
            << PathUtils::joinPaths({pythonHome, "Scripts"});
            #else
            binPaths << PathUtils::joinPaths({pythonHome, "bin"});
            #endif
            for (const QString& binPath : binPaths) {
                if (QDir(binPath).exists()) {
                    const QString path = env.value("PATH");
                    env.insert("PATH", binPath + QDir::listSeparator() + path);
                }
            }
        }

        // Python 特定：虚拟环境激活
        QString venv = task.sdkConfig.value("venv").toString();
        if (venv.isEmpty()) {
            venv = env.value("PYTHON_VENV");
        }
        if (!venv.isEmpty()) {
            #ifdef Q_OS_WIN
            const QString binPath = PathUtils::joinPaths({venv, "Scripts"});
            #else
            const QString binPath = PathUtils::joinPaths({venv, "bin"});
            #endif
            if (QDir(binPath).exists()) {
                const QString path = env.value("PATH");
                env.insert("PATH", binPath + QDir::listSeparator() + path);
                env.insert("VIRTUAL_ENV", venv);
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
        // 用跨平台解析出来的解释器，而不是硬编码 "python"
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
        // 复用同一个解释器解析逻辑
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