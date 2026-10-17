#ifndef EXTERNAL_LIBRARY_PROVIDER_FACTORY_H
#define EXTERNAL_LIBRARY_PROVIDER_FACTORY_H

#include <memory>
#include <QString>
#include "external_library_provider.h"

namespace SprintIDE {

    class ExternalLibraryProviderFactory
    {
        public:
        /// 根据项目目录里的标识文件挑选 provider；无法识别返回 nullptr
        static std::unique_ptr<IExternalLibraryProvider> forProject(const QString& projectPath);
    };

} // namespace SprintIDE

#endif // EXTERNAL_LIBRARY_PROVIDER_FACTORY_H