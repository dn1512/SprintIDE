#ifndef PROTOCOL_VALIDATOR_H
#define PROTOCOL_VALIDATOR_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>

namespace SprintIDE {
namespace Protocol {

/**
 * @brief v1.1 协议校验器
 *
 * 职责：
 *  - 校验 Agent → IDE 消息的顶层信封
 *  - 校验单个 item 的 type + 必填字段
 *  - 未知 type → 返回 isUnknownType=true（上层应静默跳过）
 */
class Validator
{
public:
    struct Result {
        bool    ok            = true;
        QString errorCode;          // 用 protocol_types.h 里的 ErrorCode
        QString errorMessage;
        bool    isUnknownType = false;

        static Result success() { return {}; }

        static Result failure(const QString& code, const QString& msg)
        {
            Result r;
            r.ok           = false;
            r.errorCode    = code;
            r.errorMessage = msg;
            return r;
        }

        static Result unknownType(const QString& type)
        {
            Result r;
            r.ok            = false;
            r.errorCode     = QStringLiteral("UNSUPPORTED_TYPE");
            r.errorMessage  = QStringLiteral("Unknown item type: %1").arg(type);
            r.isUnknownType = true;
            return r;
        }
    };

    /// 校验 Agent → IDE 消息的信封（sessionId / protocolVersion / items）
    static Result validateEnvelope(const QJsonObject& msg);

    /// 校验单个 item（type + 必填字段）
    static Result validateItem(const QJsonObject& item);

    /// 批量校验 items 数组
    static QList<Result> validateItems(const QJsonArray& items);

private:
    /// type → 必填字段列表（v1.1 type 全集）
    static const QMap<QString, QStringList>& requiredFieldsMap();
};

} // namespace Protocol
} // namespace SprintIDE

#endif // PROTOCOL_VALIDATOR_H
