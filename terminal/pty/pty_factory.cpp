// SPDX-License-Identifier: GPL-3.0-or-later
#include "pty_factory.h"

#ifdef Q_OS_WIN
#  include "win_conpty_process.h"
#else
#  include "unix_pty_process.h"
#endif

namespace SprintIDE {

    std::unique_ptr<IPtyProcess> PtyFactory::create(QObject* parent)
    {
        #ifdef Q_OS_WIN
        if (!ConPtyApi::instance().available) return nullptr;
        return std::make_unique<WinConPtyProcess>(parent);
        #else
        return std::make_unique<UnixPtyProcess>(parent);
        #endif
    }

    bool PtyFactory::isPtySupported()
    {
        #ifdef Q_OS_WIN
        return ConPtyApi::instance().available;
        #else
        return true;
        #endif
    }

} // namespace SprintIDE