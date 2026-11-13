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

#ifndef PYTHON_VIRTUAL_ENV_H
#define PYTHON_VIRTUAL_ENV_H

#include <QString>
#include <QStringList>

namespace SprintIDE {

class PythonVirtualEnv {
public:
    static QString detectVenv(const QString& projectPath);
    static bool createVenv(const QString& projectPath, const QString& pythonPath = QString());
    static QString activateScriptPath(const QString& venvPath);
    static QString interpreterPath(const QString& venvPath);
    static QString pipPath(const QString& venvPath);
    static bool isValidVenv(const QString& venvPath);

    // v8.6 新增：在 dirPath 下查找 venv / .venv / env 子目录，返回第一个命中的绝对路径
    //   命中判据 = 目录内存在 pyvenv.cfg（不检查解释器是否可执行）
    static QString findVenvInDir(const QString& dirPath);

    // v8.6 新增：从解释器路径反推 venv 根
    //   仅当路径形如 <root>/bin/python*（Unix）
    //   或 <root>\Scripts\python*.exe（Windows）时返回 root，否则空串。
    //   兼容断链 symlink：bin/python 是符号链接但目标已失效时仍能反推。
    static QString venvRootFromInterpreter(const QString& interpreterPath);
};

} // namespace SprintIDE

#endif // PYTHON_VIRTUAL_ENV_H