#include "run_configuration.h"
#include <QJsonArray>

namespace SprintIDE {

QVariantMap RunConfiguration::toVariantMap() const
{
    QVariantMap map;
    map["name"] = name;
    map["mainClass"] = mainClass;
    map["workingDirectory"] = workingDir;
    map["jvmArgs"] = jvmArgs;
    map["programArgs"] = programArgs;
    map["environment"] = environmentVariables;
    map["isFatJar"] = isFatJar;
    map["fatJarPath"] = fatJarPath;
    return map;
}

RunConfiguration RunConfiguration::fromVariantMap(const QVariantMap& map)
{
    RunConfiguration config;
    config.name = map.value("name").toString();
    config.mainClass = map.value("mainClass").toString();
    config.workingDir = map.value("workingDirectory").toString();
    config.jvmArgs = map.value("jvmArgs").toStringList();
    config.programArgs = map.value("programArgs").toStringList();
    config.environmentVariables = map.value("environment").toMap();
    config.isFatJar = map.value("isFatJar").toBool();
    config.fatJarPath = map.value("fatJarPath").toString();
    return config;
}

} // namespace SprintIDE
