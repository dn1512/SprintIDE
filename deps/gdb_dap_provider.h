// src/deps/gdb_dap_provider.h
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
