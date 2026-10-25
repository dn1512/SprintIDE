// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "pty_types.h"
#include <memory>

namespace SprintIDE {

    class PtyFactory
    {
        public:
        // 自动按平台选择实现；返回 nullptr 表示当前平台不支持 PTY
        static std::unique_ptr<IPtyProcess> create(QObject* parent = nullptr);

        // 是否支持真正的 PTY（false 时 TerminalPanel 应降级到"命令执行器"模式）
        static bool isPtySupported();
    };

} // namespace SprintIDE