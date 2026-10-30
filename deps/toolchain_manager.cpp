#include "toolchain_manager.h"
#include "toolchain_provider.h"
#include "lldb_dap_provider.h"
#include "toolchain_dialog.h"
#include "gdb_dap_provider.h"

#include <QDebug>
#include <QWidget>

namespace SprintIDE {

ToolchainManager& ToolchainManager::instance()
{
    static ToolchainManager instance;
    return instance;
}

ToolchainManager::ToolchainManager(QObject* parent)
    : QObject(parent)
{
}

ToolchainManager::~ToolchainManager() = default;

void ToolchainManager::registerProvider(std::unique_ptr<ToolchainProvider> provider)
{
    if (!provider) return;
    m_providers.push_back(std::move(provider));
}

void ToolchainManager::ensureDefaultsRegistered()
{
    if (m_defaultsRegistered) return;
    m_defaultsRegistered = true;

    // 注册顺序决定优先级（先注册先尝试）
    registerProvider(std::make_unique<LldbDapProvider>());

    registerProvider(std::make_unique<GdbDapProvider>());
    // 未来：
    // registerProvider(std::make_unique<CppDbgProvider>());
}

ToolchainResolution ToolchainManager::resolve(ToolchainId id)
{
    ensureDefaultsRegistered();

    int key = static_cast<int>(id);
    if (m_cache.contains(key)) {
        return m_cache[key];
    }

    for (auto& p : m_providers) {
        if (!p->supportedIds().contains(id)) continue;
        ToolchainResolution res = p->resolve(id);
        if (res.found) {
            qDebug() << "[ToolchainManager] resolved" << toolchainIdName(id)
            << "->" << res.executablePath
            << "(source:" << res.source << ")";
            m_cache[key] = res;
            return res;
        }
    }

    // 全部失败
    ToolchainResolution fail;
    fail.found = false;
    fail.errorMessage = QString("No provider could resolve %1").arg(toolchainIdName(id));
    qDebug() << "[ToolchainManager]" << toolchainIdName(id) << "not found";
    m_cache[key] = fail;
    return fail;
}

ToolchainInstallHint ToolchainManager::installHint(ToolchainId id) const
{
    for (const auto& p : m_providers) {
        if (p->supportedIds().contains(id)) {
            return p->installHint(id);
        }
    }
    return ToolchainInstallHint();
}

bool ToolchainManager::resolveOrPrompt(ToolchainId id,
                                       QWidget* dialogParent,
                                       ToolchainResolution* outResult)
{
    ToolchainResolution res = resolve(id);
    if (res.found) {
        if (outResult) *outResult = res;
        return true;
    }

    // 未找到 弹窗引导
    ToolchainInstallHint hint = installHint(id);
    ToolchainDialog dlg(hint, id, dialogParent);
    if (dlg.exec() == QDialog::Accepted) {
        // 用户点了"我已安装，重新检测"
        invalidateCache();
        res = resolve(id);
        if (res.found) {
            if (outResult) *outResult = res;
            return true;
        }
    }

    if (outResult) *outResult = res;
    return false;
}

void ToolchainManager::invalidateCache()
{
    m_cache.clear();
    emit resolutionChanged(ToolchainId::Unknown);
}

} // namespace SprintIDE
