// src/deps/toolchain_types.h
#ifndef TOOLCHAIN_TYPES_H
#define TOOLCHAIN_TYPES_H

#include <QString>
#include <QStringList>

namespace SprintIDE {

/**
 * @brief 工具链类型（用于 registry 索引）
 */
enum class ToolchainId {
    Unknown,
    LldbDap,      ///< lldb-dap / lldb-vscode（macOS / Linux）
    GdbDap,       ///< gdb --interpreter=dap（Linux / 未来 Windows MinGW）
    CppDbg,       ///< OpenDebugAD7（Windows MSVC）
    CMake,        ///< CMake 构建工具（预留）
    Ninja,        ///< Ninja 构建工具（预留）
    Make,         ///< make 构建工具（预留）
};

/**
 * @brief 一个工具链的解析结果
 */
struct ToolchainResolution {
    bool found = false;              ///< 是否找到
    QString executablePath;          ///< 可执行文件完整路径
    QString version;                 ///< 版本字符串（尽力获取，可能为空）
    QString source;                  ///< 来源："system" / "homebrew" / "bundled" / "user-configured"
    QString errorMessage;            ///< 失败时的诊断信息
};

/**
 * @brief 安装引导信息
 */
struct ToolchainInstallHint {
    QString title;                   ///< 例如 "需要 LLDB 调试适配器"
    QString description;             ///< 详述
    QString command;                 ///< 例如 "brew install llvm"
    QString detailUrl;               ///< 详情链接（可选）
};

/// 便于调试日志：ToolchainId -> 字符串
inline QString toolchainIdName(ToolchainId id)
{
    switch (id) {
    case ToolchainId::LldbDap: return "LldbDap";
    case ToolchainId::GdbDap:  return "GdbDap";
    case ToolchainId::CppDbg:  return "CppDbg";
    case ToolchainId::CMake:   return "CMake";
    case ToolchainId::Ninja:   return "Ninja";
    case ToolchainId::Make:    return "Make";
    default:                   return "Unknown";
    }
}

} // namespace SprintIDE

#endif // TOOLCHAIN_TYPES_H
