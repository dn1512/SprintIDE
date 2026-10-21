/*
* SprintIDE - SprintIDE is a cross-platform IDE built from scratch with Qt 6 and C++20. Its core capability is to provide AI Agents with an executable, verifiable code execution environment.
* Copyright (C) 2026  Liu Tengjun (刘腾骏)
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
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
