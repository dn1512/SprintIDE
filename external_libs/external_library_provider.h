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
#ifndef EXTERNAL_LIBRARY_PROVIDER_H
#define EXTERNAL_LIBRARY_PROVIDER_H

#include <QString>
#include "external_library_types.h"

namespace SprintIDE {

    /**
    * @brief 外部库收集器抽象接口
    *
    * 约定：
    *  - collect() 必须是线程安全的纯函数（可放到 QtConcurrent 里跑）
    *  - 不得访问 MainWindow 主线程对象
    *  - 不得修改任何全局状态
    */
    class IExternalLibraryProvider
    {
        public:
        virtual ~IExternalLibraryProvider() = default;

        /// @param projectPath 项目根目录绝对路径
        /// @return 顶层的 Root 节点列表（一般 1~2 个，空表示未检测到外部库）
        virtual QList<ExternalLibraryNode> collect(const QString& projectPath) = 0;
    };

} // namespace SprintIDE

#endif // EXTERNAL_LIBRARY_PROVIDER_H