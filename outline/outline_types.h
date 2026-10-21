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
#ifndef OUTLINE_TYPES_H
#define OUTLINE_TYPES_H

#include <QString>
#include <QList>

namespace SprintIDE {

    /**
    * @brief 文件内的一个符号（类/函数/变量/宏）
    *
    * 用途：
    *   - BreadcrumbBar 显示当前光标所在的"路径"
    *   - OutlineOverlay 显示整份文件的符号列表
    */
    struct OutlineSymbol
    {
        enum class Kind {
            Namespace,   // namespace / mod / package
            Class,       // class / struct / enum / interface / trait / union
            Function,    // 全局函数
            Method,      // 类内方法
            Variable,    // 全局变量 / const
            Property,    // 类的成员变量
            Define,      // #define / 宏
        };

        QString name;         // 名字："foo"
        QString typeName;     // 返回类型 / 变量类型："int" / "-> str"
        Kind    kind = Kind::Function;
        int     line = 0;     // 1-based
        int     column = 0;   // 1-based
        int     endLine = 0;  // 关闭行（面包屑联动用），0 = 未计算
        QList<OutlineSymbol> children;

        bool isContainer() const {
            return kind == Kind::Namespace || kind == Kind::Class;
        }
        bool isCallable() const {
            return kind == Kind::Function || kind == Kind::Method;
        }
    };

} // namespace SprintIDE

#endif // OUTLINE_TYPES_H