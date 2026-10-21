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
#ifndef OUTLINE_PROVIDER_H
#define OUTLINE_PROVIDER_H

#include "outline_types.h"
#include <QString>

namespace SprintIDE {

    /**
    * @brief 符号扫描器接口
    *
    * 约定：
    *  - scan() 是纯函数，不访问主线程对象
    *  - 输入全文 + 文件路径，输出符号树
    *  - 大文件由调用方截断（Provider 不负责）
    */
    class IOutlineProvider
    {
        public:
        virtual ~IOutlineProvider() = default;

        virtual QList<OutlineSymbol> scan(const QString& text,
            const QString& filePath) = 0;
    };

} // namespace SprintIDE

#endif