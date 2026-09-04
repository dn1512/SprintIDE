#include "python_dependency_manager.h"
#include <QProcess>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QRegularExpression>

namespace SprintIDE {

bool PythonDependencyManager::installDependencies(const QString& projectPath,
                                                  const QString& interpreterPath,
                                                  std::function<void(const QString&)> progress) {
    // 检测是否存在 pyproject.toml (poetry)
    bool hasPoetry = QFile::exists(projectPath + "/pyproject.toml") &&
                     QFile::exists(projectPath + "/poetry.lock");
    QString cmd = interpreterPath.isEmpty() ? "python3" : interpreterPath;
    QStringList args;

    if (hasPoetry) {
        // 使用 poetry
        cmd = "poetry";
        args << "install";
    } else {
        // 使用 pip
        args << "-m" << "pip" << "install" << "-r" << "requirements.txt";
        // 如果 requirements.txt 不存在，则直接安装 pyproject.toml 中的依赖（简化）
        if (!QFile::exists(projectPath + "/requirements.txt")) {
            args = { "-m", "pip", "install", "." };
        }
    }

    QProcess proc;
    proc.setWorkingDirectory(projectPath);
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(cmd, args);
    if (!proc.waitForStarted(5000)) {
        qWarning() << "Failed to start dependency install";
        return false;
    }

    while (proc.state() == QProcess::Running) {
        if (proc.bytesAvailable()) {
            QString output = QString::fromLocal8Bit(proc.readAll());
            if (progress) progress(output);
        }
        proc.waitForReadyRead(100);
    }
    proc.waitForFinished();
    if (proc.exitCode() != 0) {
        qWarning() << "Dependency install failed: " << proc.readAllStandardError();
        return false;
    }
    return true;
}

bool PythonDependencyManager::addDependency(const QString& projectPath,
                                            const QString& package,
                                            const QString& version,
                                            const QString& interpreterPath) {
    // 检测项目管理工具
    bool hasPoetry = QFile::exists(projectPath + "/pyproject.toml") &&
                     QFile::exists(projectPath + "/poetry.lock");
    if (hasPoetry) {
        QStringList args = { "add", package };
        if (!version.isEmpty()) args << version;
        QProcess proc;
        proc.setWorkingDirectory(projectPath);
        proc.start("poetry", args);
        return proc.waitForFinished(30000) && proc.exitCode() == 0;
    } else {
        // 使用 pip
        QString cmd = interpreterPath.isEmpty() ? "python3" : interpreterPath;
        QStringList args = { "-m", "pip", "install" };
        if (!version.isEmpty()) args << package + "==" + version;
        else args << package;
        QProcess proc;
        proc.setWorkingDirectory(projectPath);
        proc.start(cmd, args);
        proc.waitForFinished(30000);
        if (proc.exitCode() != 0) return false;
        // 追加到 requirements.txt
        QFile req(projectPath + "/requirements.txt");
        if (req.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&req);
            out << (version.isEmpty() ? package : package + "==" + version) << "\n";
            req.close();
        }
        return true;
    }
}

QStringList PythonDependencyManager::listOutdated(const QString& interpreterPath) {
    QString cmd = interpreterPath.isEmpty() ? "python3" : interpreterPath;
    QProcess proc;
    proc.start(cmd, { "-m", "pip", "list", "--outdated" });
    if (!proc.waitForFinished(30000)) return QStringList();
    QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());
    QStringList lines = output.split('\n');
    QStringList outdated;
    QRegularExpression re(R"(([a-zA-Z0-9_\-]+)\s+(\S+)\s+(\S+))");
    for (const QString& line : lines) {
        auto match = re.match(line);
        if (match.hasMatch()) {
            outdated << match.captured(1) + " (" + match.captured(2) + " -> " + match.captured(3) + ")";
        }
    }
    return outdated;
}

bool PythonDependencyManager::updatePackage(const QString& projectPath,
                                            const QString& package,
                                            const QString& interpreterPath) {
    QString cmd = interpreterPath.isEmpty() ? "python3" : interpreterPath;
    QProcess proc;
    proc.setWorkingDirectory(projectPath);
    proc.start(cmd, { "-m", "pip", "install", "--upgrade", package });
    return proc.waitForFinished(30000) && proc.exitCode() == 0;
}

bool PythonDependencyManager::ensurePylsp(const QString& interpreterPath)
{
    QString cmd = interpreterPath.isEmpty() ? "python3" : interpreterPath;
    QProcess check;
    check.start(cmd, {"-c", "import pylsp"});
    check.waitForFinished(2000);
    if (check.exitCode() == 0) return true;

    // 安装 pylsp
    QProcess install;
    install.start(cmd, {"-m", "pip", "install", "python-lsp-server"});
    install.waitForFinished(30000);
    return install.exitCode() == 0;
}

} // namespace SprintIDE
