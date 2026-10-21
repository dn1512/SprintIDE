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
#ifndef POLICY_ENGINE_H
#define POLICY_ENGINE_H

#include <QString>
#include <QJsonObject>

namespace SprintIDE {

/**
 * @brief 策略引擎
 *
 * 根据"用户配置 + Agent danger hint"决定一个 item 是否需要确认。
 * 规则：只加严不放宽。
 */
class PolicyEngine
{
public:
    enum class Decision {
        Auto,       // 直接执行
        Confirm,    // 需要用户确认
        Deny        // 直接拒绝
    };

    static PolicyEngine& instance();

    /// 判断一个 item 的策略
    Decision checkItem(const QJsonObject& item) const;

    /// 某个 type 是否需要确认（含用户配置 + 默认值）
    bool isConfirmRequired(const QString& type) const;

private:
    PolicyEngine() = default;

    /// "file.delete" → "FileDelete"
    static QString typeToPolicyKey(const QString& type);
};

} // namespace SprintIDE

#endif // POLICY_ENGINE_H
