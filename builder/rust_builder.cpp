// src/build/rust_builder.cpp
#include "rust_builder.h"
#include "../process_manager.h"
#include "../path_utils.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QRegularExpression>   // ← 添加此头文件
#include <config_manager.h>
#include <Application.h>

namespace SprintIDE {

QProcessEnvironment RustBuilder::prepareEnvironment(const BuildTask& task) {
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

    // 如果设置了 CARGO_HOME，将其 bin 目录加入 PATH
    QString cargoHome = env.value("CARGO_HOME");
    if (!cargoHome.isEmpty()) {
        QString cargoBin = PathUtils::joinPaths({cargoHome, "bin"});
        QString path = env.value("PATH");
        env.insert("PATH", cargoBin + ":" + path);
    }
    return env;
}

bool RustBuilder::build(const BuildTask& task, BuildResult& result) {
    // 检测 Cargo.toml
    QString cargoFilePath = PathUtils::joinPaths({task.projectPath, "Cargo.toml"});
    if (!QFile::exists(cargoFilePath)) {
        result.errorMessage = "Cargo.toml not found in project root";
        result.builderName = "RustBuilder";
        return false;
    }

    QString cargo = findCargoExecutable();
    if (cargo.isEmpty()) {
        result.errorMessage = "Cargo not found";
        result.builderName = "RustBuilder";
        return false;
    }

    // 构建命令：cargo build [--release]
    QStringList args;
    args << "build";
    if (!task.enableDebug) {
        args << "--release";
    }
    args << task.extraArgs;

    ProcessManager pm;
    pm.setEnvironment(prepareEnvironment(task));
    ProcessManager::ProcessResult pr = pm.executeSync(cargo, args, task.projectPath, 60000);

    result.exitCode = pr.exitCode;
    result.fullOutput = pr.standardOutput + "\n" + pr.standardError;
    result.elapsedMs = pr.executionTime;
    result.success = pr.success && pr.exitCode == 0;
    result.builderName = "RustBuilder";

    if (!result.success) {
        result.errorLines = pr.standardError.split('\n', Qt::SkipEmptyParts);
    }

    // 确定可执行文件路径
    if (result.success) {
        QString targetDir = task.enableDebug ? "debug" : "release";
        QString projectName = QFileInfo(task.projectPath).fileName(); // 默认
        QString exePath = PathUtils::joinPaths({task.projectPath, "target", targetDir, projectName});
#ifdef Q_OS_WIN
        exePath += ".exe";
#endif

        // 尝试从 Cargo.toml 读取 name
        QFile cargoFile(cargoFilePath);   // ← 修复：使用不同的变量名
        if (cargoFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&cargoFile);
            QString content = stream.readAll();
            cargoFile.close();

            QRegularExpression regex(R"(name\s*=\s*['\"]([^'\"]+)['\"])");   // ← 现在可用
            QRegularExpressionMatch match = regex.match(content);            // ← 现在可用
            if (match.hasMatch()) {
                projectName = match.captured(1);
                exePath = PathUtils::joinPaths({task.projectPath, "target", targetDir, projectName});
#ifdef Q_OS_WIN
                exePath += ".exe";
#endif
            }
        }

        if (QFile::exists(exePath)) {
            result.executablePath = exePath;
        }
    }

    return true;
}

bool RustBuilder::clean(const BuildTask& task, BuildResult& result) {
    QString cargo = findCargoExecutable();
    if (cargo.isEmpty()) {
        result.errorMessage = "Cargo not found";
        result.builderName = "RustBuilder";
        return false;
    }

    ProcessManager pm;
    pm.setEnvironment(prepareEnvironment(task));
    ProcessManager::ProcessResult pr = pm.executeSync(cargo, {"clean"}, task.projectPath, 30000);

    result.exitCode = pr.exitCode;
    result.fullOutput = pr.standardOutput + "\n" + pr.standardError;
    result.elapsedMs = pr.executionTime;
    result.success = pr.success && pr.exitCode == 0;
    result.builderName = "RustBuilder";

    if (!result.success) {
        result.errorLines = pr.standardError.split('\n', Qt::SkipEmptyParts);
    }
    return true;
}

bool RustBuilder::run(const BuildTask& task, const BuildResult& buildResult, QString& output) {
    // 优先使用 cargo run
    QString cargo = findCargoExecutable();
    if (!cargo.isEmpty()) {
        ProcessManager pm;
        pm.setEnvironment(prepareEnvironment(task));
        ProcessManager::ProcessResult pr = pm.executeSync(cargo, {"run"}, task.projectPath, 0);
        output = pr.standardOutput + "\n" + pr.standardError;
        return pr.success && pr.exitCode == 0;
    }

    // 否则尝试直接运行可执行文件
    if (buildResult.executablePath.isEmpty() || !QFile::exists(buildResult.executablePath)) {
        output = "Executable not found";
        return false;
    }

    ProcessManager pm;
    pm.setEnvironment(prepareEnvironment(task));
    ProcessManager::ProcessResult pr = pm.executeSync(buildResult.executablePath, task.extraArgs, task.projectPath, 0);
    output = pr.standardOutput + "\n" + pr.standardError;
    return pr.success && pr.exitCode == 0;
}

QString RustBuilder::getExecutablePath(const BuildTask& task)
{
    QString projectPath = task.projectPath;
    if (projectPath.isEmpty() && !task.sourceFiles.isEmpty()) {
        projectPath = QFileInfo(task.sourceFiles.first()).absolutePath();
    }
    if (projectPath.isEmpty()) return QString();

    // ============================================================
    // 1. 向上找 workspace 根
    //    Cargo workspace 的 target/ 在根，不在成员目录。
    //    例：workspace/ 下有 workspace/Cargo.toml 和 workspace/member/Cargo.toml，
    //        执行 cargo build 在 workspace/member 下，产物落在 workspace/target/。
    // ============================================================
    QString cargoRoot = projectPath;
    {
        QDir dir(projectPath);
        while (!dir.isRoot()) {
            if (QFile::exists(dir.absoluteFilePath("Cargo.toml"))) {
                cargoRoot = dir.absolutePath();
                break;
            }
            if (!dir.cdUp()) break;
        }
    }

    // ============================================================
    // 2. 决定 bin 名
    //    a) task.outputName 非空 → 用它（onRun 里按源文件选好 target）
    //    b) 否则从 Cargo.toml 的 [package] name 读
    //    c) 再兜底目录名 / 源文件名
    // ============================================================
    QString outName = task.outputName;

    if (outName.isEmpty()) {
        const QString cargoToml = QDir(cargoRoot).absoluteFilePath("Cargo.toml");
        QFile f(cargoToml);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream s(&f);
            const QString content = s.readAll();
            f.close();

            // [package] 段里的 name
            // 注：用 DotMatchesEverything + Multiline，跨行匹配 [package] 到下一个 [
            static const QRegularExpression pkgRe(
                R"re(\[package\][^\[]*?^name\s*=\s*"([^"]+)")re",
                QRegularExpression::MultilineOption
                | QRegularExpression::DotMatchesEverythingOption);
            auto m = pkgRe.match(content);
            if (m.hasMatch()) {
                outName = m.captured(1);
            }
        }
    }

    if (outName.isEmpty()) {
        outName = QFileInfo(projectPath).fileName();
    }
    if (outName.isEmpty() && !task.sourceFiles.isEmpty()) {
        outName = QFileInfo(task.sourceFiles.first()).completeBaseName();
    }
    if (outName.isEmpty()) return QString();

    #ifdef Q_OS_WIN
    if (!outName.endsWith(".exe", Qt::CaseInsensitive)) {
        outName += ".exe";
    }
    #endif

    // ============================================================
    // 3. profile：debug / release
    //    与 build() 里的判断一致：task.enableDebug == false 时 --release
    // ============================================================
    const QString subDir = task.enableDebug ? "debug" : "release";

    return PathUtils::joinPaths({cargoRoot, "target", subDir, outName});
}

QString RustBuilder::findCargoExecutable() const {
    QString cargoCmd = "cargo";
#ifdef Q_OS_WIN
    cargoCmd += ".exe";
#endif
    ProcessManager pm;
    ProcessManager::ProcessResult pr = pm.executeSync(cargoCmd, {"--version"}, QString(), 2000);
    if (pr.success) {
        return cargoCmd;
    }
    // 检查 CARGO_HOME
    QString cargoHome = QProcessEnvironment::systemEnvironment().value("CARGO_HOME");
    if (!cargoHome.isEmpty()) {
        QString cargoExe = PathUtils::joinPaths({cargoHome, "bin", "cargo"});
#ifdef Q_OS_WIN
        cargoExe += ".exe";
#endif
        if (QFile::exists(cargoExe)) return cargoExe;
    }
    return QString();
}

} // namespace SprintIDE
