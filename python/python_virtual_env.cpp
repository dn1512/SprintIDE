#include "python_virtual_env.h"
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QDebug>
#include <QRegularExpression>

namespace SprintIDE {

// ============================================================
// 判定 venv：只检查 pyvenv.cfg 存在
//   v8.6: 不再检查 bin/python 是否可执行
//     - 兼容 macOS 下 bin/python 是 symlink 且指向的目标可能被卸载
//     - 兼容用户项目里 venv 从别的机器拷贝过来、解释器还没重建
// ============================================================
bool PythonVirtualEnv::isValidVenv(const QString& venvPath) {
    if (venvPath.isEmpty()) return false;
    QDir dir(venvPath);
    if (!dir.exists()) return false;
    return dir.exists("pyvenv.cfg");
}

// ============================================================
// 在 dirPath 下查找 venv 子目录
//   顺序：.venv > venv > env（与 detectVenv 保持一致）
// ============================================================
QString PythonVirtualEnv::findVenvInDir(const QString& dirPath)
{
    if (dirPath.isEmpty()) return QString();
    QDir dir(dirPath);
    if (!dir.exists()) return QString();

    for (const QString& name : {".venv", "venv", "env"}) {
        const QString candidate = dir.absoluteFilePath(name);
        if (QDir(candidate).exists() && isValidVenv(candidate)) {
            return candidate;
        }
    }
    return QString();
}

QString PythonVirtualEnv::detectVenv(const QString& projectPath) {
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

// ============================================================
// 定位 venv 内的解释器
//   v8.6: 用 exists() || isSymLink() 兼容断链 symlink
//     正常情况下 bin/python 是指向系统 python3 的 symlink
//     若目标已被卸载，exists() 会返回 false，但 isSymLink() 仍为 true，
//     此时返回该路径让调用方在运行阶段自然失败（而不是配置阶段就被拦下）
// ============================================================
QString PythonVirtualEnv::interpreterPath(const QString& venvPath) {
    if (venvPath.isEmpty()) return QString();

#ifdef Q_OS_WIN
    const QStringList candidates = {
        "Scripts/python.exe",
        "Scripts/python3.exe",
    };
#else
    const QStringList candidates = {
        "bin/python",
        "bin/python3",
        "bin/python3.13",
        "bin/python3.12",
        "bin/python3.11",
        "bin/python3.10",
        "bin/python3.9",
    };
#endif

    for (const QString& rel : candidates) {
        const QString py = QDir(venvPath).absoluteFilePath(rel);
        const QFileInfo fi(py);
        if (fi.exists() || fi.isSymLink()) {
            return fi.absoluteFilePath();
        }
    }
    return QString();
}

QString PythonVirtualEnv::pipPath(const QString& venvPath) {
    if (venvPath.isEmpty()) return QString();
#ifdef Q_OS_WIN
    QString pip = QDir(venvPath).absoluteFilePath("Scripts/pip.exe");
#else
    QString pip = QDir(venvPath).absoluteFilePath("bin/pip");
#endif
    QFileInfo fi(pip);
    return (fi.exists() || fi.isSymLink()) ? fi.absoluteFilePath() : QString();
}

// ============================================================
// v8.6 新增：从解释器路径反推 venv 根
// ============================================================
QString PythonVirtualEnv::venvRootFromInterpreter(const QString& interpreterPath)
{
    if (interpreterPath.isEmpty()) return QString();

    const QFileInfo fi(interpreterPath);
    // 兼容断链 symlink
    if (!fi.exists() && !fi.isSymLink()) return QString();

    const QString parentDirPath = fi.absolutePath();
    const QString parentDirName = QFileInfo(parentDirPath).fileName();

#ifdef Q_OS_WIN
    if (parentDirName.compare("Scripts", Qt::CaseInsensitive) != 0) {
        return QString();
    }
#else
    if (parentDirName != "bin") {
        return QString();
    }
#endif

    static const QRegularExpression pyRe(
        R"(^python(3(\.\d+)?)?(\.exe)?$)",
        QRegularExpression::CaseInsensitiveOption);
    if (!pyRe.match(fi.fileName()).hasMatch()) {
        return QString();
    }

    return QFileInfo(parentDirPath).absolutePath();
}

} // namespace SprintIDE