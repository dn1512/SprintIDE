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
#ifndef LLDB_DAP_PROVIDER_H
#define LLDB_DAP_PROVIDER_H

#include "toolchain_provider.h"

namespace SprintIDE {

/**
 * @brief lldb-dap / lldb-vscode / codelldb 的提供者
 *
 * 解析顺序（从高到低优先级）：
 *   0. 用户环境变量 LLDB_DAP_PATH 显式指定
 *   1. 【内嵌】<bundle>/Contents/Frameworks/lldb/bin/   ← 打包后才有
 *   2. 【用户数据】~/Library/Application Support/SprintIDE/toolchains/lldb/bin/
 *   3. 系统 PATH 中的 lldb-dap / lldb-vscode / codelldb
 *   4. Homebrew LLVM / LLDB 独立 formula
 *   5. VS Code CodeLLDB 扩展（按版本号从新到旧）
 *   6. Xcode.app / CommandLineTools
 *   7. Linux 常见路径
 */
class LldbDapProvider : public ToolchainProvider
{
public:
    QList<ToolchainId> supportedIds() const override;
    ToolchainResolution resolve(ToolchainId id) const override;
    ToolchainInstallHint installHint(ToolchainId id) const override;

private:
    /// 尝试从 app bundle 内嵌目录解析
    ///   macOS:   <app>/Contents/Frameworks/lldb/bin/
    ///   Windows: <app>/lldb/bin/
    ///   Linux:   <app>/../lib/lldb/bin/
    ToolchainResolution resolveBundled() const;

    /// 尝试从用户数据目录解析（为 on-demand 下载预留）
    ///   macOS:   ~/Library/Application Support/SprintIDE/toolchains/lldb/
    ///   Linux:   ~/.local/share/SprintIDE/toolchains/lldb/
    ///   Windows: %APPDATA%/SprintIDE/toolchains/lldb/
    ToolchainResolution resolveUserData() const;
};

} // namespace SprintIDE

#endif // LLDB_DAP_PROVIDER_H
