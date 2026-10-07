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
