// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef RUST_OUTLINE_PROVIDER_H
#define RUST_OUTLINE_PROVIDER_H

#include "outline_provider.h"

namespace SprintIDE {

    class RustOutlineProvider : public IOutlineProvider
    {
        public:
        QList<OutlineSymbol> scan(const QString& text,
            const QString& filePath) override;

        private:
        static int braceDelta(const QString& line);
    };

} // namespace SprintIDE

#endif // RUST_OUTLINE_PROVIDER_H