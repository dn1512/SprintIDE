#ifndef CPP_OUTLINE_PROVIDER_H
#define CPP_OUTLINE_PROVIDER_H

#include "outline_provider.h"

namespace SprintIDE {

    class CppOutlineProvider : public IOutlineProvider
    {
        public:
        QList<OutlineSymbol> scan(const QString& text, const QString& filePath) override;

        private:
        static int braceDelta(const QString& line);
    };

} // namespace SprintIDE

#endif