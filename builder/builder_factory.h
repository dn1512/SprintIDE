// src/build/builder_factory.h
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
