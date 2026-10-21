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
#ifndef PYTHON_PROJECT_MODEL_H
#define PYTHON_PROJECT_MODEL_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QSharedPointer>

namespace SprintIDE {

class PythonProjectModel {
public:
    explicit PythonProjectModel(const QString& projectPath);
    bool load();  // 加载项目信息

    // 属性
    QString projectPath() const { return m_projectPath; }
    QString projectName() const { return m_projectName; }
    QString interpreterPath() const { return m_interpreterPath; }
    QString venvPath() const { return m_venvPath; }
    QStringList dependencies() const { return m_dependencies; }
    QStringList devDependencies() const { return m_devDependencies; }
    QString entryPoint() const { return m_entryPoint; }  // 主要入口脚本或模块

private:
    QString m_projectPath;
    QString m_projectName;
    QString m_interpreterPath;
    QString m_venvPath;
    QStringList m_dependencies;
    QStringList m_devDependencies;
    QString m_entryPoint;

    void detectVenv();
    void parseRequirements();
    void parsePyproject();
    void parseSetupPy();
    void detectEntryPoint();
};

} // namespace SprintIDE

#endif // PYTHON_PROJECT_MODEL_H
