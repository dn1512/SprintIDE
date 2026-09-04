#ifndef PYTHON_TEST_RUNNER_H
#define PYTHON_TEST_RUNNER_H

#include <QString>
#include <QStringList>
#include <functional>

namespace SprintIDE {

struct PythonTestInfo {
    QString filePath;
    QString className;
    QString methodName;
    QString fullName; // file::class::method
};

class PythonTestRunner {
public:
    // 发现测试
    static QList<PythonTestInfo> discoverTests(const QString& projectPath);
    // 运行测试
    static bool runTest(const QString& projectPath,
                        const PythonTestInfo& test,
                        std::function<void(const QString&)> outputCallback);
    // 调试测试（需 PythonDebugger）
    static bool debugTest(const QString& projectPath,
                          const PythonTestInfo& test);
};

} // namespace SprintIDE

#endif // PYTHON_TEST_RUNNER_H
