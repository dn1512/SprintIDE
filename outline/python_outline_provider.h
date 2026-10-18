#ifndef PYTHON_OUTLINE_PROVIDER_H
#define PYTHON_OUTLINE_PROVIDER_H

#include "outline_provider.h"

namespace SprintIDE {

    class PythonOutlineProvider : public IOutlineProvider
    {
        public:
        QList<OutlineSymbol> scan(const QString& text, const QString& filePath) override;
    };

} // namespace SprintIDE

#endif