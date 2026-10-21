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
#ifndef BUILD_RESULT_H
#define BUILD_RESULT_H

#include <QString>
#include <QStringList>

namespace SprintIDE {

/**
 * @brief 构建结果结构体
 */
struct BuildResult {
    bool success = false;               ///< 是否成功
    int exitCode = -1;                  ///< 退出码
    QString fullOutput;                 ///< 完整输出（stdout + stderr）
    QString executablePath;             ///< 生成的可执行文件路径（若有）
    QStringList errorLines;             ///< 错误行（解析后）
    QStringList warningLines;           ///< 警告行（解析后）
    qint64 elapsedMs = 0;               ///< 耗时（毫秒）
    QString builderName;                ///< 处理此任务的 Builder 名称
    QString errorMessage;               ///< 错误信息（一般失败时）
};

} // namespace SprintIDE

#endif // BUILD_RESULT_H
