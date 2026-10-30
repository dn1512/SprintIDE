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
#ifndef EDITOR_RULES_H
#define EDITOR_RULES_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QPair>
#include <QChar>

namespace SprintIDE {

/**
 * @brief 每种语言的编辑器交互行为规则
 *
 * 与 LanguageInfo（语法高亮）解耦：
 *   - LanguageInfo  高亮颜色、关键字列表
 *   - EditorRules   注释符、缩进触发、自动配对、大括号换行
 *
 * 使用方式（只在打开文件时调用一次）：
 *   m_editorRules = EditorRules::forLanguage("python");
 */
struct EditorRules
{
    // ============================================================
    // 0. 块结构风格
    //   用途：findLogicalLineIndent 的"闭合行特判"只对 Brace 语言生效
    //   - Python / YAML：靠缩进，不用括号判定锚点
    //   - Ruby / Shell：靠 end / do / then / fi 等关键字
    //   - 其余：默认 Brace
    // ============================================================
    enum class BlockStyle {
        Brace,      // { } 标记块：C/C++/Java/JS/TS/Go/Rust/CSS/JSON/Shell
        Indent,     // 缩进层级：Python / YAML
        Keyword,    // 关键字闭合：Ruby
    };
    BlockStyle blockStyle = BlockStyle::Brace;
    // ============================================================
    // 1. 注释
    // ============================================================
    QString lineComment;            // "//" "#" "--" ";" 空字符串 = 不支持
    QString blockCommentStart;      // "/*" "<!--" 空字符串 = 不支持
    QString blockCommentEnd;        // "*/" "-->" 空字符串 = 不支持

    // ============================================================
    // 2. 缩进触发（按回车时使用）
    // ============================================================
    // 上一行"去掉空白后"以其中任意一个字符串结尾 新行缩进 +1 级
    // 例：C/C++ 的 "{"，Python 的 ":"，Shell 的 "then"、"do"
    QStringList indentTriggers;

    // 上一行"去掉空白后"以其中任意一个字符串开头 新行缩进 -1 级
    // 例：C 的 "}"，Shell 的 "fi" "done" "esac"，Ruby 的 "end"
    QStringList dedentTriggers;

    // 该行的行首 token 命中时，该行自身要减少一级缩进（用于输入时补正）
    // 例：Python 的 "else" "elif" "except" "finally" "case"
    QStringList dedentLineStarters;

    // ============================================================
    // 3. 自动配对
    // ============================================================
    // 输入 (  [  {  "  '  ` 时自动补全右半边并把光标放中间
    QList<QPair<QChar, QChar>> autoPairs;

    // ============================================================
    // 4. 大括号换行（按 { 时是否触发三行展开）
    // ============================================================
    // true  输入 { 时插入 "{\n    <光标>\n}"（C/C++/Java/JS/Go/Rust/C#）
    // false { 只做一个普通字符（Python/Shell/Ruby/Markdown 等）
    bool autoBraceOnNewline = false;

    // ============================================================
    // 5. 上下文保护
    // ============================================================
    // 光标位于字符串字面量内部时，禁用 autoPairs / autoBraceOnNewline
    // 用于修复 f"{...}"、R"(...)"、"..." 里按 { 被强行展开的问题
    bool suppressInsideString = true;

    // 光标位于行注释右侧时，禁用所有自动行为
    bool suppressInsideLineComment = true;

    // ============================================================
    // 6. 工厂方法：根据语言 ID 返回规则
    // ============================================================
    // languageId 来自 LanguageDetector（"cpp" "python" "javascript" ...）
    static EditorRules forLanguage(const QString& languageId);

    // 便捷判断
    bool endsWithIndentTrigger(const QString& trimmedLine) const;
    bool startsWithDedentTrigger(const QString& trimmedLine) const;
    bool lineStartsWithDedentKeyword(const QString& trimmedLine) const;

    // 是否需要在字符串/注释内禁用自动行为
    bool needsContextGuard() const {
        return suppressInsideString || suppressInsideLineComment;
    }
};

} // namespace SprintIDE

#endif // EDITOR_RULES_H
