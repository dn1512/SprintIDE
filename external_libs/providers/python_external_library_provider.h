#ifndef PYTHON_EXTERNAL_LIBRARY_PROVIDER_H
#define PYTHON_EXTERNAL_LIBRARY_PROVIDER_H

#include "../external_library_provider.h"

namespace SprintIDE {

    class PythonExternalLibraryProvider : public IExternalLibraryProvider
    {
        public:
        QList<ExternalLibraryNode> collect(const QString& projectPath) override;

        private:
        QString findSitePackages(const QString& projectPath) const;
    };

} // namespace SprintIDE

#endif // PYTHON_EXTERNAL_LIBRARY_PROVIDER_H