#ifndef RUST_EXTERNAL_LIBRARY_PROVIDER_H
#define RUST_EXTERNAL_LIBRARY_PROVIDER_H

#include "../external_library_provider.h"

namespace SprintIDE {

    class RustExternalLibraryProvider : public IExternalLibraryProvider
    {
        public:
        QList<ExternalLibraryNode> collect(const QString& projectPath) override;
    };

} // namespace SprintIDE

#endif // RUST_EXTERNAL_LIBRARY_PROVIDER_H