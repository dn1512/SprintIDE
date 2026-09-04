#include "python_formatter.h"
#include <QProcess>
#include <QFile>
#include <QTemporaryFile>
#include <QDebug>

namespace SprintIDE {

bool PythonFormatter::isAvailable() {
    QProcess proc;
    proc.start("black", { "--version" });
    proc.waitForFinished(2000);
    return proc.exitCode() == 0;
}

bool PythonFormatter::formatFile(const QString& filePath, QString& error) {
    if (!QFile::exists(filePath)) {
        error = "File does not exist";
        return false;
    }
    if (!isAvailable()) {
        error = "Black not found. Please install: pip install black";
        return false;
    }
    QProcess proc;
    proc.start("black", { filePath });
    if (!proc.waitForFinished(30000)) {
        error = "Timeout";
        return false;
    }
    if (proc.exitCode() != 0) {
        error = QString::fromLocal8Bit(proc.readAllStandardError());
        return false;
    }
    return true;
}

} // namespace SprintIDE
