#include "ai/executor/system_executor.h"
#include "ai/protocol/protocol_types.h"
#include "mainwindow.h"
#include "application.h"
#include "config_manager.h"

#include <QTimer>
#include <QDebug>

namespace SprintIDE {
namespace Protocol {

SystemExecutor::SystemExecutor(QObject* parent)
    : QObject(parent)
    , m_mainWindow(qobject_cast<MainWindow*>(parent))
{
    if (!m_mainWindow) {
        qWarning() << "[SystemExecutor] MainWindow not provided; "
                      "system.setServerAddress will not be able to restart";
    }
}

ExecutionResult SystemExecutor::execute(const QJsonObject& item)
{
    const QString type = item.value("type").toString();

    if (type == QStringLiteral("system.getServerAddress"))
        return handleGetServerAddress(item);
    if (type == QStringLiteral("system.setServerAddress"))
        return handleSetServerAddress(item);

    return ExecutionResult::fail(
        item.value("id").toString(), type,
        ErrorCode::UnsupportedType,
        QStringLiteral("SystemExecutor cannot handle type: %1").arg(type));
}

// ------------------------------------------------------------
// system.getServerAddress —— 返回当前 AI 连接配置
// ------------------------------------------------------------
ExecutionResult SystemExecutor::handleGetServerAddress(const QJsonObject& item)
{
    const QString itemId = item.value("id").toString();

    ConfigManager* cfg = Application::instance().configManager();
    if (!cfg) {
        return ExecutionResult::fail(
            itemId, QStringLiteral("system.getServerAddress"),
            ErrorCode::InternalError,
            QStringLiteral("ConfigManager unavailable"));
    }

    QJsonObject data;
    data["host"] = cfg->value("AI/Host", "127.0.0.1").toString();
    data["port"] = cfg->value("AI/Port", 9876).toInt();

    return ExecutionResult::ok(
        itemId, QStringLiteral("system.getServerAddress"), data);
}

// ------------------------------------------------------------
// system.setServerAddress —— 修改 AI 连接配置
//
// 语义：
//   - 与当前配置对比
//     * 相同  返回 { changed: false }，不做任何操作
//     * 不同  写入配置，500ms 后重启 AI 连接，返回 { changed: true }
//
// 为什么延迟 500ms：
//   当前 item 的响应还需要用现有连接发回给 Agent，
//   立即断开会导致 Agent 收不到回执。
//
// 重连之后的衔接：
//   Agent 收到 changed=true 后应该在 newHost:newPort 上监听；
//   IDE 500ms 后断旧连新，两边会自动同步。
// ------------------------------------------------------------
ExecutionResult SystemExecutor::handleSetServerAddress(const QJsonObject& item)
{
    const QString itemId = item.value("id").toString();
    const QString newHost = item.value("host").toString();
    const int     newPort = item.value("port").toInt();

    // ---- 参数校验 ----
    if (newHost.isEmpty()) {
        return ExecutionResult::fail(
            itemId, QStringLiteral("system.setServerAddress"),
            ErrorCode::InvalidFields,
            QStringLiteral("Missing or empty 'host'"));
    }
    if (newPort <= 0 || newPort > 65535) {
        return ExecutionResult::fail(
            itemId, QStringLiteral("system.setServerAddress"),
            ErrorCode::InvalidFields,
            QStringLiteral("Invalid 'port' (must be 1-65535), got %1").arg(newPort));
    }

    ConfigManager* cfg = Application::instance().configManager();
    if (!cfg) {
        return ExecutionResult::fail(
            itemId, QStringLiteral("system.setServerAddress"),
            ErrorCode::InternalError,
            QStringLiteral("ConfigManager unavailable"));
    }

    const QString oldHost = cfg->value("AI/Host", "127.0.0.1").toString();
    const int     oldPort = cfg->value("AI/Port", 9876).toInt();

    // ---- 相同 不改 ----
    if (oldHost == newHost && oldPort == newPort) {
        QJsonObject data;
        data["changed"]           = false;
        data["requiresReconnect"] = false;
        data["host"]              = newHost;
        data["port"]              = newPort;
        return ExecutionResult::ok(
            itemId, QStringLiteral("system.setServerAddress"), data);
    }

    // ---- 不同 写配置 ----
    cfg->setValue("AI/Host", newHost);
    cfg->setValue("AI/Port", newPort);

    QJsonObject data;
    data["changed"]           = true;
    data["requiresReconnect"] = true;
    data["oldHost"]           = oldHost;
    data["oldPort"]           = oldPort;
    data["newHost"]           = newHost;
    data["newPort"]           = newPort;

    // ---- 延迟 500ms 重启 AI 连接 ----
    if (m_mainWindow) {
        const quint16 port16 = static_cast<quint16>(newPort);
        QTimer::singleShot(500, m_mainWindow,
            [this, newHost, port16]() {
                if (m_mainWindow) {
                    m_mainWindow->onAIRestartRequested(newHost, port16);
                }
            });
    } else {
        qWarning() << "[SystemExecutor] MainWindow unavailable, "
                      "configuration saved but no reconnect triggered";
    }

    return ExecutionResult::ok(
        itemId, QStringLiteral("system.setServerAddress"), data);
}

} // namespace Protocol
} // namespace SprintIDE