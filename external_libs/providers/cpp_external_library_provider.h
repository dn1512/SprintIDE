#ifndef CPP_EXTERNAL_LIBRARY_PROVIDER_H
#define CPP_EXTERNAL_LIBRARY_PROVIDER_H

#include "../external_library_provider.h"
#include <QSet>

namespace SprintIDE {

    class CppExternalLibraryProvider : public IExternalLibraryProvider
    {
        public:
        QList<ExternalLibraryNode> collect(const QString& projectPath) override;

        private:
        void parseCompileCommands(const QString& ccPath, QSet<QString>& out) const;
        QString findQtDir(const QString& projectPath) const;
    };

} // namespace SprintIDE

#endif // CPP_EXTERNAL_LIBRARY_PROVIDER_H