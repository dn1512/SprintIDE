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
#ifndef TOOLCHAIN_MANAGER_H
#define TOOLCHAIN_MANAGER_H

#include "toolchain_types.h"
#include <QObject>
#include <QMap>
#include <QString>
#include <memory>
#include <vector>

namespace SprintIDE {

class ToolchainProvider;

/**
 * @brief 工具链管理器（单例）
 *
 * 统一管理所有外部工具（lldb-dap、cmake、ninja 等）的发现、解析、安装引导。
 *
 * 使用示例：
 *   auto res = ToolchainManager::instance().resolve(ToolchainId::LldbDap);
 *   if (res.found) {
 *       // 用 res.executablePath 启动 adapter
 *   } else {
 *       // 弹出 ToolchainDialog 引导用户
 *   }
 */
class ToolchainManager : public QObject
{
    Q_OBJECT
public:
    static ToolchainManager& instance();

    /// 注册 Provider（按注册顺序依次尝试，先注册优先级更高）
    void registerProvider(std::unique_ptr<ToolchainProvider> provider);

    /// 解析工具链（带缓存，一次进程内不重复扫描）
    ToolchainResolution resolve(ToolchainId id);

    /// 获取安装引导信息（用于 ToolchainDialog）
    ToolchainInstallHint installHint(ToolchainId id) const;

    /// 清空缓存（例如用户在设置里手动改了路径之后）
    void invalidateCache();

    /// 一次性便捷方法：解析 + 若未找到则弹窗引导
    /// @return 成功解析到可执行文件则返回 true，否则返回 false（用户已看到弹窗）
    bool resolveOrPrompt(ToolchainId id,
                         QWidget* dialogParent = nullptr,
                         ToolchainResolution* outResult = nullptr);

signals:
    /// 解析结果变化（例如用户手动配置了路径后刷新）
    void resolutionChanged(SprintIDE::ToolchainId id);

private:
    explicit ToolchainManager(QObject* parent = nullptr);
    ~ToolchainManager();

    Q_DISABLE_COPY(ToolchainManager)

    std::vector<std::unique_ptr<ToolchainProvider>> m_providers;
    QMap<int, ToolchainResolution> m_cache;   // key = static_cast<int>(ToolchainId)
    bool m_defaultsRegistered = false;

    void ensureDefaultsRegistered();
};

} // namespace SprintIDE

Q_DECLARE_METATYPE(SprintIDE::ToolchainId)

#endif // TOOLCHAIN_MANAGER_H
