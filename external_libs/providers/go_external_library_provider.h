#ifndef GO_EXTERNAL_LIBRARY_PROVIDER_H
#define GO_EXTERNAL_LIBRARY_PROVIDER_H

#include "../external_library_provider.h"

namespace SprintIDE {

    class GoExternalLibraryProvider : public IExternalLibraryProvider
    {
        public:
        QList<ExternalLibraryNode> collect(const QString& projectPath) override;
    };

} // namespace SprintIDE

#endif // GO_EXTERNAL_LIBRARY_PROVIDER_H