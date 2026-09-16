// src/deps/toolchain_provider.h
#ifndef TOOLCHAIN_PROVIDER_H
#define TOOLCHAIN_PROVIDER_H

#include "toolchain_types.h"
#include <QString>
#include <QList>

namespace SprintIDE {

/**
 * @brief 工具链提供者接口
 *
 * 每个具体工具（lldb-dap、gdb-dap、cmake...）实现一个 Provider。
 * ToolchainManager 依次询问它们能否解析某个 ToolchainId。
 *
 * 设计原则：
 *  - Provider 只负责"找到"或"安装"，不感知 IDE 的业务逻辑
 *  - 解析顺序由 Manager 控制（bundled → user-config → system → PATH）
 *  - 未来加"下载器"时，只需新增一个 Provider，不改动既有代码
 */
class ToolchainProvider
{
public:
    virtual ~ToolchainProvider() = default;

    /// 本 Provider 负责哪些 ToolchainId
    virtual QList<ToolchainId> supportedIds() const = 0;

    /// 尝试解析：返回可执行文件路径，失败时 resolution.found = false
    virtual ToolchainResolution resolve(ToolchainId id) const = 0;

    /// 返回"如何安装"的引导信息（用于弹窗）
    virtual ToolchainInstallHint installHint(ToolchainId id) const = 0;

    /// 若为 true，表示 Provider 可以主动触发安装（未来 on-demand 下载用）
    virtual bool canAutoInstall(ToolchainId id) const { Q_UNUSED(id); return false; }

    /// 主动安装（可选，默认返回失败）
    virtual bool install(ToolchainId id, QString* errorMsg) {
        Q_UNUSED(id);
        if (errorMsg) *errorMsg = "Auto-install not supported";
        return false;
    }
};

} // namespace SprintIDE

#endif // TOOLCHAIN_PROVIDER_H
