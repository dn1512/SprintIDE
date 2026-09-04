#ifndef PYTHON_RUN_CONFIG_H
#define PYTHON_RUN_CONFIG_H

#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace SprintIDE {

enum class PythonEntryType { Script, Module };

struct PythonRunConfig {
    QString name;
    PythonEntryType entryType = PythonEntryType::Script;
    QString entryPath;        // 脚本路径或模块名
    QString interpreterPath;
    QString workingDirectory;
    QStringList programArgs;
    QVariantMap envVars;
    bool installDeps = false;

    QVariantMap toVariantMap() const;
    static PythonRunConfig fromVariantMap(const QVariantMap& map);
};

} // namespace SprintIDE

#endif // PYTHON_RUN_CONFIG_H
