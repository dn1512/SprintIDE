// SPDX-License-Identifier: GPL-3.0-or-later
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
