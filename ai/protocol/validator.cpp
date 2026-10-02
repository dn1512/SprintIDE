#include "ai/protocol/validator.h"
#include "ai/protocol/protocol_types.h"

namespace SprintIDE {
namespace Protocol {

// ------------------------------------------------------------
// type → 必填字段表（v1.1 全集）
// ------------------------------------------------------------
const QMap<QString, QStringList>& Validator::requiredFieldsMap()
{
    static const QMap<QString, QStringList> s_map = {
                                                     // ---- chat.* ----
                                                     { QStringLiteral("chat.ask"),     { QStringLiteral("question") } },
                                                     { QStringLiteral("chat.confirm"), { QStringLiteral("targetId"),
                                                                                       QStringLiteral("prompt") } },
                                                     { QStringLiteral("chat.notify"),  { QStringLiteral("level"),
                                                                                      QStringLiteral("message") } },

                                                     // ---- file.* ----
                                                     { QStringLiteral("file.read"),    { QStringLiteral("id"),
                                                                                    QStringLiteral("path") } },
                                                     { QStringLiteral("file.create"),  { QStringLiteral("path"),
                                                                                      QStringLiteral("content") } },
                                                     { QStringLiteral("file.delete"),  { QStringLiteral("path") } },
                                                     { QStringLiteral("file.rename"),  { QStringLiteral("oldPath"),
                                                                                      QStringLiteral("newPath") } },
                                                     { QStringLiteral("file.edit"),    { QStringLiteral("path"),
                                                                                    QStringLiteral("mode") } },

                                                     // ---- command.* ----
                                                     { QStringLiteral("command.run"),        { QStringLiteral("id"),
                                                                                      QStringLiteral("program"),
                                                                                      QStringLiteral("arguments") } },
                                                     { QStringLiteral("command.runShell"),   { QStringLiteral("id"),
                                                                                           QStringLiteral("script") } },
                                                     { QStringLiteral("command.installDep"), { QStringLiteral("id"),
                                                                                             QStringLiteral("manager"),
                                                                                             QStringLiteral("packages") } },

                                                     // ---- ide.* ----
                                                     { QStringLiteral("ide.build"),     { QStringLiteral("id") } },
                                                     { QStringLiteral("ide.run"),       { QStringLiteral("id") } },
                                                     { QStringLiteral("ide.stopRun"),   { QStringLiteral("id") } },
                                                     { QStringLiteral("ide.debug"),     { QStringLiteral("id") } },
                                                     { QStringLiteral("ide.stopDebug"), { QStringLiteral("id") } },

                                                     // ---- project.* ----
                                                     { QStringLiteral("project.info"),    { QStringLiteral("id") } },
                                                     { QStringLiteral("project.targets"), { QStringLiteral("id") } },

                                                     // ---- env.* ----
                                                     { QStringLiteral("env.query"),  { QStringLiteral("id"),
                                                                                    QStringLiteral("items") } },
                                                     { QStringLiteral("env.setSdk"), { QStringLiteral("variables") } },

                                                     // ---- meta.* ----
                                                     { QStringLiteral("meta.batch"), { QStringLiteral("id"),
                                                                                     QStringLiteral("atomic"),
                                                                                     QStringLiteral("items") } },
                                                     { QStringLiteral("meta.abort"), { QStringLiteral("reason") } },
                                                     { QStringLiteral("meta.done"),  { } },
                                                     };
    return s_map;
}

// ------------------------------------------------------------
// 信封校验
// ------------------------------------------------------------
Validator::Result Validator::validateEnvelope(const QJsonObject& msg)
{
    if (!msg.contains(QStringLiteral("sessionId")) ||
        msg.value(QStringLiteral("sessionId")).toString().isEmpty()) {
        return Result::failure(ErrorCode::InvalidFields,
                               QStringLiteral("Missing or empty 'sessionId'"));
    }

    if (!msg.contains(QStringLiteral("protocolVersion"))) {
        return Result::failure(ErrorCode::InvalidFields,
                               QStringLiteral("Missing 'protocolVersion'"));
    }

    if (!msg.contains(QStringLiteral("items")) ||
        !msg.value(QStringLiteral("items")).isArray()) {
        return Result::failure(ErrorCode::InvalidFields,
                               QStringLiteral("'items' must be an array"));
    }

    return Result::success();
}

// ------------------------------------------------------------
// 单个 item 校验
// ------------------------------------------------------------
Validator::Result Validator::validateItem(const QJsonObject& item)
{
    const QString type = item.value(QStringLiteral("type")).toString();
    if (type.isEmpty()) {
        return Result::failure(ErrorCode::InvalidFields,
                               QStringLiteral("Missing 'type'"));
    }

    const auto& table = requiredFieldsMap();
    if (!table.contains(type)) {
        return Result::unknownType(type);
    }

    const QStringList& required = table.value(type);
    for (const QString& field : required) {
        if (!item.contains(field)) {
            return Result::failure(
                ErrorCode::InvalidFields,
                QStringLiteral("Missing required field '%1' for type '%2'")
                    .arg(field, type));
        }
    }

    return Result::success();
}

// ------------------------------------------------------------
// 批量校验
// ------------------------------------------------------------
QList<Validator::Result> Validator::validateItems(const QJsonArray& items)
{
    QList<Result> results;
    results.reserve(items.size());

    for (const QJsonValue& v : items) {
        if (!v.isObject()) {
            results.append(Result::failure(ErrorCode::InvalidFields,
                                           QStringLiteral("Item is not an object")));
            continue;
        }
        results.append(validateItem(v.toObject()));
    }
    return results;
}

} // namespace Protocol
} // namespace SprintIDE
