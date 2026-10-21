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
#ifndef GO_BUILDER_H
#define GO_BUILDER_H

#include "ibuilder.h"

namespace SprintIDE {

/**
 * @brief Go 语言构建器
 */
class GoBuilder : public IBuilder {
public:
    QProcessEnvironment prepareEnvironment(const BuildTask& task) override;
    bool build(const BuildTask& task, BuildResult& result) override;
    bool clean(const BuildTask& task, BuildResult& result) override;
    bool run(const BuildTask& task, const BuildResult& buildResult, QString& output) override;
    QString getExecutablePath(const BuildTask& task) override;

private:
    /**
     * @brief 查找 go 可执行文件
     */
    QString findGoExecutable() const;

    /**
     * @brief 运行可执行文件（内部辅助）
     * @param executable 可执行文件路径
     * @param task 任务（用于环境和参数）
     * @param output 输出内容
     * @return 是否成功
     */
    bool runProgram(const QString& executable, const BuildTask& task, QString& output);
};

} // namespace SprintIDE

#endif // GO_BUILDER_H
