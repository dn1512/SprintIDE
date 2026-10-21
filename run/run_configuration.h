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
