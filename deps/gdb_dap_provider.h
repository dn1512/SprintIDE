// SPDX-License-Identifier: GPL-3.0-or-later
/*
* SprintIDE - SprintIDE is a cross-platform IDE built from scratch with Qt 6 and C++20. Its core capability is to provide AI Agents with an executable, verifiable code execution environment.
* Copyright (C) 2026  Liu Tengjun (刘腾骏)
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef GDB_DAP_PROVIDER_H
#define GDB_DAP_PROVIDER_H

#include "toolchain_provider.h"

namespace SprintIDE {

/**
 * @brief GDB 调试适配器提供者
 *
 * 自 GDB 14 起，GDB 支持 `--interpreter=dap` 模式，可直接作为 DAP adapter。
 *
 * 解析顺序：
 *   1. 用户环境变量 GDB_DAP_PATH 显式指定
 *   2. 系统 PATH 中的 gdb
 *   3. Homebrew / 常见安装位置
 *
 * 验证规则（比 lldb_dap_provider 更严格）：
 *   - 文件存在、可执行、是普通文件
 *   - 跳过 macOS GUI shim（*.app/Contents/MacOS/*）
 *   - 执行 `gdb --version`，输出里必须含 "GNU gdb" 且主版本 >= 14
 *
 * 为什么要求 14+：
 *   14 之前的 GDB 没有 --interpreter=dap 支持，作为 DAP adapter 启动会失败。
 */
class GdbDapProvider : public ToolchainProvider
{
public:
    QList<ToolchainId> supportedIds() const override;
    ToolchainResolution resolve(ToolchainId id) const override;
    ToolchainInstallHint installHint(ToolchainId id) const override;
};

} // namespace SprintIDE

#endif // GDB_DAP_PROVIDER_H
