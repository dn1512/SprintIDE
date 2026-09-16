// src/deps/lldb_dap_provider.h
#ifndef LLDB_DAP_PROVIDER_H
#define LLDB_DAP_PROVIDER_H

#include "toolchain_provider.h"

namespace SprintIDE {

/**
 * @brief lldb-dap / lldb-vscode 的提供者
 *
 * 解析顺序：
 *   1. 用户环境变量 LLDB_DAP_PATH 显式指定
 *   2. 系统 PATH 中的 lldb-dap / lldb-vscode
 *   3. Homebrew LLVM（keg-only）：
 *        /opt/homebrew/opt/llvm/bin/  (Apple Silicon)
 *        /usr/local/opt/llvm/bin/     (Intel Mac)
 *   4. Xcode.app 内置（一般没有 lldb-dap，但保留检测）
 *   5. CommandLineTools
 *   6. Linux 常见路径（/usr/bin, /usr/local/bin）
 */
class LldbDapProvider : public ToolchainProvider
{
public:
    QList<ToolchainId> supportedIds() const override;
    ToolchainResolution resolve(ToolchainId id) const override;
    ToolchainInstallHint installHint(ToolchainId id) const override;
};

} // namespace SprintIDE

#endif // LLDB_DAP_PROVIDER_H
