#include "python_run_config.h"

namespace SprintIDE {

QVariantMap PythonRunConfig::toVariantMap() const {
    QVariantMap map;
    map["name"] = name;
    map["entryType"] = static_cast<int>(entryType);
    map["entryPath"] = entryPath;
    map["interpreterPath"] = interpreterPath;
    map["workingDirectory"] = workingDirectory;
    map["programArgs"] = programArgs;
    map["envVars"] = envVars;
    map["installDeps"] = installDeps;
    return map;
}

PythonRunConfig PythonRunConfig::fromVariantMap(const QVariantMap& map) {
    PythonRunConfig cfg;
    cfg.name = map["name"].toString();
    cfg.entryType = static_cast<PythonEntryType>(map["entryType"].toInt());
    cfg.entryPath = map["entryPath"].toString();
    cfg.interpreterPath = map["interpreterPath"].toString();
    cfg.workingDirectory = map["workingDirectory"].toString();
    cfg.programArgs = map["programArgs"].toStringList();
    cfg.envVars = map["envVars"].toMap();
    cfg.installDeps = map["installDeps"].toBool();
    return cfg;
}

} // namespace SprintIDE
