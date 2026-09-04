#pragma once

#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace SprintIDE {

class RunConfiguration {
public:
    RunConfiguration() = default;

    QString name;
    QString mainClass;
    QString workingDir;
    QStringList jvmArgs;
    QStringList programArgs;
    QVariantMap environmentVariables;
    bool isFatJar = false;
    QString fatJarPath;
    int debugPort = 5005;
    bool enableDebug = false;

    // 序列化
    QVariantMap toVariantMap() const;
    static RunConfiguration fromVariantMap(const QVariantMap& map);
};

} // namespace SprintIDE
