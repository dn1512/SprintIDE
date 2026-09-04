#pragma once

#include "run_configuration.h"
#include <QList>
#include <QString>

namespace SprintIDE {

class RunConfigManager {
public:
    static RunConfigManager& instance();

    // 加载/保存项目运行配置
    bool loadConfigs(const QString& projectPath);
    bool saveConfigs(const QString& projectPath) const;

    QList<RunConfiguration> configs() const { return m_configs; }
    void setConfigs(const QList<RunConfiguration>& configs);

    void addConfig(const RunConfiguration& config);
    void removeConfig(const QString& name);
    RunConfiguration* findConfig(const QString& name);

private:
    RunConfigManager() = default;
    QList<RunConfiguration> m_configs;
    QString m_currentProjectPath;
};

} // namespace SprintIDE
