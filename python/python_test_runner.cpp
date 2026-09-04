#include "python_test_runner.h"
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>

namespace SprintIDE {

QList<PythonTestInfo> PythonTestRunner::discoverTests(const QString& projectPath) {
    QList<PythonTestInfo> tests;
    QDir dir(projectPath);
    QStringList testFiles = dir.entryList(QStringList() << "test_*.py" << "*_test.py", QDir::Files);
    // 也可递归扫描子目录，这里简化
    for (const QString& file : testFiles) {
        QString fullPath = dir.absoluteFilePath(file);
        // 使用 pytest --collect-only -q 获取测试列表
        QProcess proc;
        proc.setWorkingDirectory(projectPath);
        proc.start("pytest", { "--collect-only", "-q", fullPath });
        if (proc.waitForFinished(5000) && proc.exitCode() == 0) {
            QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());
            // 解析行如 "test_file.py::TestClass::test_method"
            QRegularExpression re(R"(([^:]+)::([^:]+)::([^\s]+))");
            auto it = re.globalMatch(output);
            while (it.hasNext()) {
                auto match = it.next();
                PythonTestInfo info;
                info.filePath = match.captured(1);
                info.className = match.captured(2);
                info.methodName = match.captured(3);
                info.fullName = match.captured(0);
                tests << info;
            }
        }
    }
    return tests;
}

bool PythonTestRunner::runTest(const QString& projectPath,
                               const PythonTestInfo& test,
                               std::function<void(const QString&)> outputCallback) {
    QProcess proc;
    proc.setWorkingDirectory(projectPath);
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start("pytest", { "-v", test.fullName });
    if (!proc.waitForStarted(5000)) return false;
    while (proc.state() == QProcess::Running) {
        if (proc.bytesAvailable()) {
            QString out = QString::fromLocal8Bit(proc.readAll());
            if (outputCallback) outputCallback(out);
        }
        proc.waitForReadyRead(100);
    }
    proc.waitForFinished();
    return proc.exitCode() == 0;
}

bool PythonTestRunner::debugTest(const QString& projectPath, const PythonTestInfo& test) {
    // 需要 PythonDebugger 支持，此处暂未实现，留空
    Q_UNUSED(projectPath);
    Q_UNUSED(test);
    return false;
}

} // namespace SprintIDE
