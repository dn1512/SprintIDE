#include "python_virtual_env.h"
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QDebug>

namespace SprintIDE {

QString PythonVirtualEnv::detectVenv(const QString& projectPath) {
    // 优先级：.venv > venv > env
    QStringList candidates = { ".venv", "venv", "env" };
    for (const QString& name : candidates) {
        QString path = QDir(projectPath).absoluteFilePath(name);
        if (isValidVenv(path)) {
            return path;
        }
    }
    // 检查环境变量
    QByteArray venv = qgetenv("VIRTUAL_ENV");
    if (!venv.isEmpty()) {
        QString path = QString::fromLocal8Bit(venv);
        if (isValidVenv(path)) return path;
    }
    return QString();
}

bool PythonVirtualEnv::createVenv(const QString& projectPath, const QString& pythonPath) {
    QString python = pythonPath.isEmpty() ? "python3" : pythonPath;
    QStringList args = { "-m", "venv", ".venv" };
    QProcess proc;
    proc.setWorkingDirectory(projectPath);
    proc.start(python, args);
    if (!proc.waitForFinished(30000)) {
        qWarning() << "Failed to create venv: timeout";
        return false;
    }
    if (proc.exitCode() != 0) {
        qWarning() << "Failed to create venv: " << proc.readAllStandardError();
        return false;
    }
    return true;
}

QString PythonVirtualEnv::activateScriptPath(const QString& venvPath) {
    if (venvPath.isEmpty()) return QString();
#ifdef Q_OS_WIN
    return QDir(venvPath).absoluteFilePath("Scripts/activate.bat");
#else
    return QDir(venvPath).absoluteFilePath("bin/activate");
#endif
}

QString PythonVirtualEnv::interpreterPath(const QString& venvPath) {
    if (venvPath.isEmpty()) return QString();
#ifdef Q_OS_WIN
    QString py = QDir(venvPath).absoluteFilePath("Scripts/python.exe");
#else
    QString py = QDir(venvPath).absoluteFilePath("bin/python");
#endif
    return QFile::exists(py) ? py : QString();
}

QString PythonVirtualEnv::pipPath(const QString& venvPath) {
    if (venvPath.isEmpty()) return QString();
#ifdef Q_OS_WIN
    QString pip = QDir(venvPath).absoluteFilePath("Scripts/pip.exe");
#else
    QString pip = QDir(venvPath).absoluteFilePath("bin/pip");
#endif
    return QFile::exists(pip) ? pip : QString();
}

bool PythonVirtualEnv::isValidVenv(const QString& venvPath) {
    if (venvPath.isEmpty()) return false;
    QDir dir(venvPath);
    if (!dir.exists()) return false;
    // 检查是否存在 bin/python 或 Scripts/python.exe
#ifdef Q_OS_WIN
    return dir.exists("Scripts/python.exe");
#else
    return dir.exists("bin/python") || dir.exists("bin/python3");
#endif
}

} // namespace SprintIDE
