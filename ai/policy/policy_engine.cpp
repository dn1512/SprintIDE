#include "ai/policy/policy_engine.h"
#include "application.h"
#include "config_manager.h"

#include <QMap>
#include <QStringList>

namespace SprintIDE {

PolicyEngine& PolicyEngine::instance()
{
    static PolicyEngine s;
    return s;
}

QString PolicyEngine::typeToPolicyKey(const QString& type)
{
    // "file.delete" "FileDelete"
    QStringList parts = type.split(QLatin1Char('.'));
    QString key;
    for (const QString& p : parts) {
        if (p.isEmpty()) continue;
        key += p.at(0).toUpper() + p.mid(1);
    }
    return key;
}

bool PolicyEngine::isConfirmRequired(const QString& type) const
{
    ConfigManager* cfg = Application::instance().configManager();
    const QString key = QStringLiteral("AI/Policy/") + typeToPolicyKey(type);

    // 默认策略（设计文档 5.4）
    static const QMap<QString, bool> defaults = {
                                                 // 文件
                                                 { QStringLiteral("file.read"),   false },
                                                 { QStringLiteral("file.create"), true  },
                                                 { QStringLiteral("file.edit"),   true  },
                                                 { QStringLiteral("file.delete"), true  },
                                                 { QStringLiteral("file.rename"), true  },

                                                 // 命令
                                                 { QStringLiteral("command.run"),        true },
                                                 { QStringLiteral("command.runShell"),   true },
                                                 { QStringLiteral("command.installDep"), true },

                                                 // IDE 内部
                                                 { QStringLiteral("ide.build"),   false },
                                                 { QStringLiteral("ide.run"),     false },
                                                 { QStringLiteral("ide.stopRun"), false },

                                                 // 查询
                                                 { QStringLiteral("env.query"),       false },
                                                 { QStringLiteral("project.info"),    false },
                                                 { QStringLiteral("project.targets"), false },
                                                 };

    const bool def = defaults.value(type, false);
    return cfg ? cfg->value(key, def).toBool() : def;
}

PolicyEngine::Decision PolicyEngine::checkItem(const QJsonObject& item) const
{
    const QString type   = item.value("type").toString();
    const QString danger = item.value("danger").toString();

    // ---- 1. 整体禁用检查（shell 可被用户全局禁掉） ----
    if (type == QStringLiteral("command.runShell")) {
        ConfigManager* cfg = Application::instance().configManager();
        const bool allowShell = cfg
                                    ? cfg->value("AI/Policy/AllowShell", true).toBool()
                                    : true;
        if (!allowShell) {
            return Decision::Deny;
        }
    }

    // ---- 2. 只加严不放宽 ----
    const bool userWantsConfirm = isConfirmRequired(type);
    const bool agentDangerous   = (danger == QStringLiteral("dangerous"));

    if (userWantsConfirm)  return Decision::Confirm;
    if (agentDangerous)    return Decision::Confirm;

    return Decision::Auto;
}

} // namespace SprintIDE
