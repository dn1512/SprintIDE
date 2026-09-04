#ifndef PYTHON_DEPENDENCY_MANAGER_H
#define PYTHON_DEPENDENCY_MANAGER_H

#include <QString>
#include <QStringList>
#include <functional>

namespace SprintIDE {

class PythonDependencyManager {
public:
    // 安装依赖，progress 回调用于输出日志
    static bool installDependencies(const QString& projectPath,
                                    const QString& interpreterPath,
                                    std::function<void(const QString&)> progress = nullptr);
    static bool addDependency(const QString& projectPath,
                              const QString& package,
                              const QString& version = QString(),
                              const QString& interpreterPath = QString());
    static QStringList listOutdated(const QString& interpreterPath);
    static bool updatePackage(const QString& projectPath,
                              const QString& package,
                              const QString& interpreterPath = QString());
    static bool ensurePylsp(const QString& interpreterPath = QString());
};

} // namespace SprintIDE

#endif // PYTHON_DEPENDENCY_MANAGER_H
