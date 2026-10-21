// SPDX-License-Identifier: GPL-3.0-or-later
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
