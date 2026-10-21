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
#ifndef PYTHON_TEST_RUNNER_H
#define PYTHON_TEST_RUNNER_H

#include <QString>
#include <QStringList>
#include <functional>

namespace SprintIDE {

struct PythonTestInfo {
    QString filePath;
    QString className;
    QString methodName;
    QString fullName; // file::class::method
};

class PythonTestRunner {
public:
    // 发现测试
    static QList<PythonTestInfo> discoverTests(const QString& projectPath);
    // 运行测试
    static bool runTest(const QString& projectPath,
                        const PythonTestInfo& test,
                        std::function<void(const QString&)> outputCallback);
    // 调试测试（需 PythonDebugger）
    static bool debugTest(const QString& projectPath,
                          const PythonTestInfo& test);
};

} // namespace SprintIDE

#endif // PYTHON_TEST_RUNNER_H
