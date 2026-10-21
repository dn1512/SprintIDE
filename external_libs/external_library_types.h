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
#ifndef EXTERNAL_LIBRARY_TYPES_H
#define EXTERNAL_LIBRARY_TYPES_H

#include <QString>
#include <QList>

namespace SprintIDE {

    /**
    * @brief 外部库节点的通用数据结构
    *
    * 所有 Provider 都产出这种节点，由 ProjectTreeProxyModel 渲染
    */
    struct ExternalLibraryNode
    {
        enum class Kind {
            Root,        ///< 顶层环境名（"site-packages"、"Cargo 依赖"）
            Group,       ///< 中间分类（"系统头文件"、"Qt"）
            Package,     ///< 库/包（numpy、serde 等），可展开
            File,        ///< 可打开的源码文件
            Opaque,      ///< 二进制（.so / .dll / jar），双击用系统打开
        };

        QString displayName;
        QString absolutePath;   ///< 双击/复制路径用；Root/Group 可为空
        Kind    kind = Kind::Package;
        QList<ExternalLibraryNode> children;
    };

} // namespace SprintIDE

#endif // EXTERNAL_LIBRARY_TYPES_H