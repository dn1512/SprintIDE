// src/deps/toolchain_provider.h
#ifndef TOOLCHAIN_PROVIDER_H
#define TOOLCHAIN_PROVIDER_H

#include "toolchain_types.h"
#include <QString>
#include <QList>
#include <functional>

namespace SprintIDE {

/**
 * @brief 下载进度回调
 * @param progress 0-100 的整数
 */
using ToolchainDownloadProgress = std::function<void(int progress)>;

/**
 * @brief 工具链提供者接口
 *
 * 每个具体工具（lldb-dap、gdb-dap、cmake...）实现一个 Provider。
 * ToolchainManager 依次询问它们能否解析某个 ToolchainId。
 *
 * 设计原则：
 *  - Provider 只负责"找到"或"安装/下载"，不感知 IDE 的业务逻辑
 *  - 解析顺序由 Manager 控制（bundled → user-config → system → PATH）
 *  - 未来加"下载器"时，只需实现 downloadAndInstall，不改动调用方
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

    // ============================================================
    // 自动安装（例如：调用 brew/apt 等系统包管理器）
    // ============================================================
    /// 若为 true，表示 Provider 可以通过系统包管理器自动安装
    virtual bool canAutoInstall(ToolchainId id) const {
        Q_UNUSED(id);
        return false;
    }

    /// 通过系统包管理器安装（可选，默认返回失败）
    virtual bool install(ToolchainId id, QString* errorMsg) {
        Q_UNUSED(id);
        if (errorMsg) *errorMsg = "Auto-install not supported";
        return false;
    }

    // ============================================================
    // 自动下载（例如：从 CDN/GitHub Releases 拉取预编译包）
    // ============================================================
    /// 若为 true，表示 Provider 支持从网络下载预编译工具链
    virtual bool canAutoDownload(ToolchainId id) const {
        Q_UNUSED(id);
        return false;
    }

    /**
     * @brief 下载并安装工具链
     * @param id 工具链
     * @param targetDir 目标目录；空字符串表示使用 Provider 默认位置（通常为
     *                  ~/Library/Application Support/SprintIDE/toolchains/...）
     * @param progress 进度回调（0-100），可为空
     * @param errorMsg 失败时填充错误信息
     * @return 成功返回 true
     */
    virtual bool downloadAndInstall(ToolchainId id,
                                    const QString& targetDir,
                                    ToolchainDownloadProgress progress,
                                    QString* errorMsg) {
        Q_UNUSED(id);
        Q_UNUSED(targetDir);
        Q_UNUSED(progress);
        if (errorMsg) *errorMsg = "Auto-download not supported";
        return false;
    }
};

} // namespace SprintIDE

#endif // TOOLCHAIN_PROVIDER_H
