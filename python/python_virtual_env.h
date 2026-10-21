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
#ifndef PYTHON_VIRTUAL_ENV_H
#define PYTHON_VIRTUAL_ENV_H

#include <QString>
#include <QStringList>

namespace SprintIDE {

class PythonVirtualEnv {
public:
    // 检测项目路径下的虚拟环境
    static QString detectVenv(const QString& projectPath);
    // 创建虚拟环境（使用 system 或指定 python）
    static bool createVenv(const QString& projectPath, const QString& pythonPath = QString());
    // 获取激活脚本路径（用于终端）
    static QString activateScriptPath(const QString& venvPath);
    // 获取虚拟环境中的 Python 解释器路径
    static QString interpreterPath(const QString& venvPath);
    // 获取虚拟环境中的 pip 路径
    static QString pipPath(const QString& venvPath);
    // 判断是否为有效的虚拟环境
    static bool isValidVenv(const QString& venvPath);
};

} // namespace SprintIDE

#endif // PYTHON_VIRTUAL_ENV_H
