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
#ifndef DAP_TYPES_H
#define DAP_TYPES_H

#include <QString>
#include <QList>

namespace SprintIDE {
namespace Dap {

/**
 * @brief 源文件位置
 */
struct Source {
    QString name;
    QString path;           ///< 绝对路径
    int sourceReference = 0;///< 0 表示文件；>0 表示源码由 adapter 提供
};

/**
 * @brief 断点（setBreakpoints 的响应）
 */
struct Breakpoint {
    int id = 0;             ///< adapter 内部 ID
    bool verified = false;
    int line = 0;           ///< 1-based
    int column = 0;         ///< 1-based
    QString message;
    QString sourcePath;
};

/**
 * @brief 栈帧
 */
struct StackFrame {
    int id = 0;             ///< frameId，用于 scopes/variables 请求
    QString name;           ///< 函数名
    Source source;
    int line = 0;           ///< 1-based
    int column = 0;
};

/**
 * @brief 作用域（Locals / Globals 等）
 */
struct Scope {
    QString name;               ///< "Locals" / "Globals" / "Registers"
    int variablesReference = 0;
    bool expensive = false;
};

/**
 * @brief 变量
 */
struct Variable {
    QString name;
    QString value;
    QString type;
    int variablesReference = 0; ///< >0 表示可展开（结构体/数组/指针）
    bool isContainer() const { return variablesReference > 0; }
};

/**
 * @brief 线程
 */
struct Thread {
    int id = 0;
    QString name;
};

/**
 * @brief stopped 事件的 body
 */
struct StoppedInfo {
    QString reason;             ///< "breakpoint" / "step" / "entry" / "exception" / "pause"
    int threadId = 0;
    QString description;
    QString text;
    bool allThreadsStopped = false;
};

} // namespace Dap
} // namespace SprintIDE

#endif // DAP_TYPES_H
