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
#ifndef BUILDER_FACTORY_H
#define BUILDER_FACTORY_H

#include "ibuilder.h"
#include "language/language_type.h"
#include <QMap>

namespace SprintIDE {

/**
 * @brief 构建器工厂，管理不同语言的 IBuilder 实例
 */
class BuilderFactory {
public:
    /**
     * @brief 根据语言类型创建（或返回）对应的构建器
     * @param lang 语言类型
     * @return 构建器指针（由工厂管理，不可 delete）
     */
    static IBuilder* createBuilder(LanguageType lang);

    /**
     * @brief 注册自定义构建器（用于扩展）
     * @param lang 语言类型
     * @param builder 构建器对象（工厂将取得所有权）
     */
    static void registerBuilder(LanguageType lang, IBuilder* builder);

    /**
     * @brief 初始化默认的构建器（在应用启动时调用）
     */
    static void initializeDefaults();

    /**
     * @brief 清理所有注册的构建器（程序退出前调用）
     */
    static void shutdown();

private:
    static QMap<LanguageType, IBuilder*> s_builders;  ///< 语言到构建器的映射
};

} // namespace SprintIDE

#endif // BUILDER_FACTORY_H
