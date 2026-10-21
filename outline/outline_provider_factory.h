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
#ifndef OUTLINE_PROVIDER_FACTORY_H
#define OUTLINE_PROVIDER_FACTORY_H

#include "outline_provider.h"
#include <memory>

namespace SprintIDE {

    class OutlineProviderFactory
    {
        public:
        /// 根据 languageId（来自 LanguageHighlighter）挑 provider
        /// 返回 nullptr 表示该语言暂不支持
        static std::unique_ptr<IOutlineProvider> forLanguage(const QString& languageId);
    };

} // namespace SprintIDE

#endif