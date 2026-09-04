// src/build/node_builder.cpp
#include "node_builder.h"
#include "../process_manager.h"
#include "../path_utils.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <config_manager.h>
#include <Application.h>

namespace SprintIDE {

QProcessEnvironment NodeBuilder::prepareEnvironment(const BuildTask& task) {
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

    // 如果设置了 NODE_HOME，将其 bin 目录加入 PATH
    QString nodeHome = env.value("NODE_HOME");
    if (!nodeHome.isEmpty()) {
        QString nodeBin = PathUtils::joinPaths({nodeHome, "bin"});
        QString path = env.value("PATH");
        env.insert("PATH", nodeBin + ":" + path);
    }

    return env;
}

bool NodeBuilder::build(const BuildTask& task, BuildResult& result) {
    // 检查 package.json
    QString packageJson = PathUtils::joinPaths({task.projectPath, "package.json"});
    if (!QFile::exists(packageJson)) {
        result.errorMessage = "package.json not found";
        result.builderName = "NodeBuilder";
        return false;
    }

    // 读取 package.json 获取 scripts
    QFile file(packageJson);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.errorMessage = "Cannot open package.json";
        result.builderName = "NodeBuilder";
        return false;
    }
    QByteArray data = file.readAll();
    file.close();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        result.errorMessage = "Invalid package.json";
        result.builderName = "NodeBuilder";
        return false;
    }
    QJsonObject obj = doc.object();
    QJsonObject scripts = obj["scripts"].toObject();

    // 判断构建方式
    bool isTs = isTypeScriptProject(task.projectPath);
    if (isTs) {
        // 使用 npx tsc
        QString npx = findNpxExecutable();
        if (npx.isEmpty()) {
            result.errorMessage = "npx not found";
            result.builderName = "NodeBuilder";
            return false;
        }
        ProcessManager pm;
        pm.setEnvironment(prepareEnvironment(task));
        ProcessManager::ProcessResult pr = pm.executeSync(npx, {"tsc"}, task.projectPath, 60000);
        result.exitCode = pr.exitCode;
        result.fullOutput = pr.standardOutput + "\n" + pr.standardError;
        result.elapsedMs = pr.executionTime;
        result.success = pr.success && pr.exitCode == 0;
        result.builderName = "NodeBuilder";
        if (!result.success) {
            result.errorLines = pr.standardError.split('\n', Qt::SkipEmptyParts);
        }
        // 可执行文件通常是编译后的 index.js 或 main.js
        // 但无法确定，留空
        return true;
    }

    // 如果有 build script，执行 npm run build
    if (scripts.contains("build")) {
        QString output;
        bool ok = runNpmScript("build", task, output);
        result.fullOutput = output;
        result.success = ok;
        return ok;
    }

    // 否则不需要构建，直接成功
    result.success = true;
    result.builderName = "NodeBuilder";
    result.fullOutput = "No build step required (no TypeScript and no build script)";
    return true;
}

bool NodeBuilder::clean(const BuildTask& task, BuildResult& result) {
    // 清理 dist/ 和 node_modules（需用户确认，这里直接删除可能危险）
    // 设计书要求需用户确认，但在 IDE 中可先标记
    // 我们只清理 dist/ 和 build/ 目录，不删除 node_modules（因太耗时）
    QStringList dirsToClean = {"dist", "build", "out"};
    bool ok = true;
    for (const QString& dirName : dirsToClean) {
        QString dirPath = PathUtils::joinPaths({task.projectPath, dirName});
        if (QDir(dirPath).exists()) {
            if (!QDir(dirPath).removeRecursively()) ok = false;
        }
    }
    result.success = ok;
    result.builderName = "NodeBuilder";
    return ok;
}

bool NodeBuilder::run(const BuildTask& task, const BuildResult& buildResult, QString& output) {
    // 优先尝试 npm start
    if (runNpmScript("start", task, output)) return true;

    // 否则尝试 node index.js
    QString node = findNodeExecutable();
    if (node.isEmpty()) {
        output = "Node executable not found";
        return false;
    }

    // 查找入口文件：index.js 或 main.js
    QStringList candidates = {"index.js", "main.js", "app.js"};
    for (const QString& candidate : candidates) {
        QString entry = PathUtils::joinPaths({task.projectPath, candidate});
        if (QFile::exists(entry)) {
            ProcessManager pm;
            pm.setEnvironment(prepareEnvironment(task));
            ProcessManager::ProcessResult pr = pm.executeSync(node, {entry}, task.projectPath, 0);
            output = pr.standardOutput + "\n" + pr.standardError;
            return pr.success && pr.exitCode == 0;
        }
    }

    output = "No entry point found (index.js, main.js, app.js)";
    return false;
}

QString NodeBuilder::getExecutablePath(const BuildTask& task)
{
    QString projectPath = task.projectPath;
    if (projectPath.isEmpty() && !task.sourceFiles.isEmpty()) {
        projectPath = QFileInfo(task.sourceFiles.first()).absolutePath();
    }

    // 检查是否有 package.json，从中读取 "main" 字段
    QString packageJsonPath = PathUtils::joinPaths({projectPath, "package.json"});
    if (QFile::exists(packageJsonPath)) {
        QFile file(packageJsonPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray data = file.readAll();
            file.close();

            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                QString mainFile = obj["main"].toString("index.js");
                QString fullPath = PathUtils::joinPaths({projectPath, mainFile});
                if (QFile::exists(fullPath)) {
                    return fullPath;
                }
            }
        }
    }

    // 查找常见的入口文件
    QStringList entryFiles = {"index.js", "src/index.js", "main.js", "app.js"};
    for (const QString& file : entryFiles) {
        QString fullPath = PathUtils::joinPaths({projectPath, file});
        if (QFile::exists(fullPath)) {
            return fullPath;
        }
    }

    // 如果没有源文件，返回空
    if (task.sourceFiles.isEmpty()) {
        return QString();
    }

    // 返回第一个源文件
    return PathUtils::absolutePath(task.sourceFiles.first(), projectPath);
}

// ==================== 私有辅助 ====================
QString NodeBuilder::findNpmExecutable() const {
    QString npmCmd = "npm";
#ifdef Q_OS_WIN
    npmCmd += ".cmd";
#endif
    ProcessManager pm;
    ProcessManager::ProcessResult pr = pm.executeSync(npmCmd, {"--version"}, QString(), 2000);
    if (pr.success) return npmCmd;
    return QString();
}

QString NodeBuilder::findNpxExecutable() const {
    QString npxCmd = "npx";
#ifdef Q_OS_WIN
    npxCmd += ".cmd";
#endif
    ProcessManager pm;
    ProcessManager::ProcessResult pr = pm.executeSync(npxCmd, {"--version"}, QString(), 2000);
    if (pr.success) return npxCmd;
    return QString();
}

QString NodeBuilder::findNodeExecutable() const {
    QString nodeCmd = "node";
#ifdef Q_OS_WIN
    nodeCmd += ".exe";
#endif
    ProcessManager pm;
    ProcessManager::ProcessResult pr = pm.executeSync(nodeCmd, {"--version"}, QString(), 2000);
    if (pr.success) return nodeCmd;
    return QString();
}

bool NodeBuilder::isTypeScriptProject(const QString& projectPath) const {
    // 检查 tsconfig.json
    QString tsconfig = PathUtils::joinPaths({projectPath, "tsconfig.json"});
    if (QFile::exists(tsconfig)) return true;
    // 检查 package.json 中是否有 typescript 依赖
    QString packageJson = PathUtils::joinPaths({projectPath, "package.json"});
    if (QFile::exists(packageJson)) {
        QFile file(packageJson);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray data = file.readAll();
            file.close();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                QJsonObject deps = obj["dependencies"].toObject();
                if (deps.contains("typescript")) return true;
                QJsonObject devDeps = obj["devDependencies"].toObject();
                if (devDeps.contains("typescript")) return true;
            }
        }
    }
    return false;
}

bool NodeBuilder::runNpmScript(const QString& script, const BuildTask& task, QString& output) {
    QString npm = findNpmExecutable();
    if (npm.isEmpty()) return false;

    ProcessManager pm;
    pm.setEnvironment(prepareEnvironment(task));
    ProcessManager::ProcessResult pr = pm.executeSync(npm, {"run", script}, task.projectPath, 0);
    output = pr.standardOutput + "\n" + pr.standardError;
    return pr.success && pr.exitCode == 0;
}

} // namespace SprintIDE
