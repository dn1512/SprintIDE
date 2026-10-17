#ifndef NODE_EXTERNAL_LIBRARY_PROVIDER_H
#define NODE_EXTERNAL_LIBRARY_PROVIDER_H

#include "../external_library_provider.h"

namespace SprintIDE {

    class NodeExternalLibraryProvider : public IExternalLibraryProvider
    {
        public:
        QList<ExternalLibraryNode> collect(const QString& projectPath) override;
    };

} // namespace SprintIDE

#endif // NODE_EXTERNAL_LIBRARY_PROVIDER_H