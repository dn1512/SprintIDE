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