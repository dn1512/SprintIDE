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
#ifndef DIAGNOSTIC_FILTER_H
#define DIAGNOSTIC_FILTER_H

#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QVector>

#include "lsp/lsp_client.h"

namespace SprintIDE {

/**
 * @brief LSP 诊断过滤器
 *
 * 职责：
 *   - 按规则决定某条诊断是否应该显示在编辑器
 *   - 硬性规则（黑名单）+ 用户自定义规则
 *   - error 级别永不过滤
 */
class DiagnosticFilter
{
public:

    // ============================================================
    // 过滤模式
    // ============================================================
    enum class Mode {
        ErrorsOnly,   // 默认：只保留 error，warning 全过滤
        ShowAll,      // 保留所有诊断
        Custom        // 走黑名单：默认规则 + 用户规则
    };

    static DiagnosticFilter& instance();

    void setMode(Mode mode) { m_mode = mode; }
    Mode mode() const { return m_mode; }

    /**
     * @brief 是否要显示这条诊断
     * @return true  = 保留（会画到编辑器）
     *         false = 过滤掉
     */
    bool shouldKeep(const LSPDiagnostic& diag) const;

    /**
     * @brief 从配置加载用户自定义规则（在 MainWindow 初始化时调用一次）
     * 配置格式（QSettings / JSON 均可）：
     *   Lsp/IgnoreWarnings = [
     *     "unused-import",
     *     "^W0611:",           // 正则，带 ^ 锚定
     *     "line too long"
     *   ]
     */
    void loadUserRules(const QStringList& rules);

    // 便于调试
    QStringList defaultRules() const;

private:
    DiagnosticFilter();

    struct Rule {
        QString pattern;            // 显示用的原字符串
        QRegularExpression regex;   // 编译后的正则（大小写不敏感）
    };

    Mode m_mode = Mode::ErrorsOnly;

    QVector<Rule> m_defaultRules;
    QVector<Rule> m_userRules;
};

} // namespace SprintIDE

#endif
