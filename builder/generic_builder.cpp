// src/build/generic_builder.cpp
#include "generic_builder.h"
#include "../process_manager.h"
#include "../path_utils.h"
#include "../project_config.h"
#include <Application.h>
#include <QDebug>
#include <QDir>
#include <config_manager.h>

namespace SprintIDE {

// ==================== prepareEnvironment ====================
QProcessEnvironment GenericBuilder::prepareEnvironment(const BuildTask& task) {
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

    // 合并 aiCustomPayload 中的 env_vars
    QVariantMap envVars = task.aiCustomPayload["env_vars"].toMap();
    for (auto it = envVars.begin(); it != envVars.end(); ++it) {
        env.insert(it.key(), it.value().toString());
    }

    return env;
}

// ==================== build ====================
bool GenericBuilder::build(const BuildTask& task, BuildResult& result) {
    QVariantMap commands = task.aiCustomPayload["commands"].toMap();
    QVariantMap buildCmd = commands["build"].toMap();
    if (buildCmd.isEmpty()) {
        result.errorMessage = "No 'build' command defined in aiCustomPayload";
        result.builderName = "GenericBuilder";
        return false;
    }
    return executeGenericCommand(buildCmd, task, result, "build");
}

// ==================== clean ====================
bool GenericBuilder::clean(const BuildTask& task, BuildResult& result) {
    QVariantMap commands = task.aiCustomPayload["commands"].toMap();
    QVariantMap cleanCmd = commands["clean"].toMap();
    if (cleanCmd.isEmpty()) {
        result.errorMessage = "No 'clean' command defined";
        result.builderName = "GenericBuilder";
        return false;
    }
    return executeGenericCommand(cleanCmd, task, result, "clean");
}

// ==================== run ====================
bool GenericBuilder::run(const BuildTask& task, const BuildResult& buildResult, QString& output) {
    QVariantMap commands = task.aiCustomPayload["commands"].toMap();
    QVariantMap runCmd = commands["run"].toMap();
    if (runCmd.isEmpty()) {
        output = "No 'run' command defined";
        return false;
    }
    BuildResult dummy;
    bool ok = executeGenericCommand(runCmd, task, dummy, "run");
    output = dummy.fullOutput;
    return ok && dummy.success;
}

// ==================== executeGenericCommand (private) ====================
bool GenericBuilder::executeGenericCommand(const QVariantMap& cmdMap,
                                           const BuildTask& task,
                                           BuildResult& result,
                                           const QString& commandType) {
    QString executable = cmdMap["executable"].toString();
    if (executable.isEmpty()) {
        result.errorMessage = "Missing 'executable' in command";
        result.builderName = "GenericBuilder";
        return false;
    }
    QStringList args = cmdMap["args"].toStringList();
    bool useShell = cmdMap["shell"].toBool();
    QString workDir = task.projectPath;
    if (cmdMap.contains("work_dir")) {
        workDir = PathUtils::absolutePath(cmdMap["work_dir"].toString(), task.projectPath);
        if (!QDir(workDir).exists()) workDir = task.projectPath;
    }

    // 准备环境
    QProcessEnvironment env = prepareEnvironment(task);

    // 执行
    ProcessManager pm;
    pm.setEnvironment(env);
    ProcessManager::ProcessResult pr = pm.executeSync(executable, args, workDir, 60000);

    // 填充结果
    result.exitCode = pr.exitCode;
    result.fullOutput = pr.standardOutput + "\n" + pr.standardError;
    result.elapsedMs = pr.executionTime;
    result.success = pr.success && pr.exitCode == 0;
    result.builderName = "GenericBuilder";

    // 简单错误提取（全部作为错误行）
    if (!result.success) {
        result.errorLines = pr.standardError.split('\n', Qt::SkipEmptyParts);
        // 也可尝试识别 stdout 中的错误
        result.errorLines += pr.standardOutput.split('\n', Qt::SkipEmptyParts);
    }

    // 如果命令是 run，尝试从输出中提取可执行路径？此处不处理，由上层决定。

    qDebug() << "[AI_BUILD] Executing:" << executable << args;
    qDebug() << "[AI_BUILD] ExitCode:" << pr.exitCode;
    qDebug() << "[AI_BUILD] Stderr:" << pr.standardError;

    return true; // 启动成功
}

// ==================== saveGenericConfig ====================
bool GenericBuilder::saveGenericConfig(const QString& projectPath, const QVariantMap& config) {
    ProjectConfig projConfig;
    QString configFile = projectPath + "/.lightcppproj";
    QStringList errors;
    if (!projConfig.loadFromFile(configFile, errors)) {
        qWarning() << "Failed to load project config for saving generic config:" << errors.join(", ");
        return false;
    }
    projConfig.setGenericBuildConfig(config);
    return projConfig.saveToFile(configFile, errors);
}

// ==================== installSdk ====================
bool GenericBuilder::installSdk(const QString& toolName, const QString& version,
                                const QString& installScript) {
    Q_UNUSED(toolName);
    Q_UNUSED(version);
    // 执行安装脚本
    ProcessManager pm;
    ProcessManager::ProcessResult pr = pm.executeSync(installScript, {}, QDir::homePath(), 120000);
    if (!pr.success) {
        qWarning() << "SDK installation failed:" << pr.standardError;
        return false;
    }
    // 成功安装后，可以解析输出确定路径，但这里简化，仅返回成功
    // 实际可调用 saveGenericConfig 更新项目配置中的 SDK 环境
    return true;
}

QString GenericBuilder::getExecutablePath(const BuildTask& task)
{
    // 从 AI 配置中读取 run.executable
    QVariantMap config = task.aiCustomPayload;
    if (config.isEmpty()) {
        return QString();
    }

    QVariantMap commands = config["commands"].toMap();
    QVariantMap runCmd = commands["run"].toMap();
    QString executable = runCmd["executable"].toString();

    if (executable.isEmpty()) {
        return QString();
    }

    // 如果可执行文件是相对路径，基于项目路径解析
    if (!QFileInfo(executable).isAbsolute()) {
        QString projectPath = task.projectPath;
        if (projectPath.isEmpty() && !task.sourceFiles.isEmpty()) {
            projectPath = QFileInfo(task.sourceFiles.first()).absolutePath();
        }
        executable = PathUtils::absolutePath(executable, projectPath);
    }

    return executable;
}

} // namespace SprintIDE
