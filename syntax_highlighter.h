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
#ifndef SYNTAX_HIGHLIGHTER_H
#define SYNTAX_HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

/**
 * @class SyntaxHighlighter
 * @brief C++语法高亮器类
 *
 * 使用QSyntaxHighlighter实现C++代码的语法高亮
 * 支持关键字、字符串、注释、数字等不同元素的着色
 * 采用正则表达式匹配，不依赖libclang，适合儿童学习环境
 */

namespace SprintIDE {

class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象（通常是QTextDocument）
     *
     * 初始化语法高亮规则，设置不同语法元素的格式
     */
    explicit SyntaxHighlighter(QTextDocument *parent = nullptr);

    /**
     * @brief 设置主题
     * @param isDarkTheme 是否为深色主题
     *
     * 根据主题调整语法高亮的颜色
     */
    void setTheme(bool isDarkTheme);

protected:
    /**
     * @brief 重写高亮函数
     * @param text 需要高亮的文本
     *
     * 对文本进行语法分析，应用不同的格式规则
     */
    void highlightBlock(const QString &text) override;

private:

    /**
     * @brief 高亮函数声明
     * @param text 需要高亮的文本
     *
     * 专门处理函数声明的高亮，确保函数名正确着色
     */
    void highlightFunctionDeclarations(const QString &text);
    /**
     * @brief 初始化高亮规则
     *
     * 设置C++关键字、数据类型、注释等语法元素的匹配规则
     */
    void initHighlightRules();

    /**
     * @brief 初始化C++多行注释的高亮
     *
     * 设置多行注释（/* ... * /）的高亮规则
     */
    void initMultiLineCommentRules();

    // ========== 高亮规则结构体 ==========

    /**
     * @struct HighlightRule
     * @brief 高亮规则结构体
     *
     * 包含正则表达式和对应的文本格式
     */
    struct HighlightRule {
        QRegularExpression pattern;   ///< 匹配模式的正则表达式
        QTextCharFormat format;       ///< 文本格式（颜色、字体等）
    };

    QVector<HighlightRule> m_highlightRules;  ///< 高亮规则列表

    // 多行注释规则
    QRegularExpression m_commentStartExpression;  ///< 多行注释开始标记
    QRegularExpression m_commentEndExpression;    ///< 多行注释结束标记
    QTextCharFormat m_multiLineCommentFormat;     ///< 多行注释格式

    // ========== 文本格式定义 ==========
    QTextCharFormat m_keywordFormat;          ///< 一般关键字格式
    QTextCharFormat m_controlFlowFormat;      ///< 控制流关键字格式
    QTextCharFormat m_storageClassFormat;     ///< 存储类关键字格式
    QTextCharFormat m_typeFormat;             ///< 数据类型格式
    QTextCharFormat m_stdTypeFormat;          ///< 标准库类型格式
    QTextCharFormat m_functionFormat;         ///< 函数格式
    QTextCharFormat m_predefinedFunctionFormat; ///< 预定义函数格式
    QTextCharFormat m_commentFormat;          ///< 单行注释格式
    QTextCharFormat m_stringFormat;           ///< 字符串格式
    QTextCharFormat m_numberFormat;           ///< 数字格式
    QTextCharFormat m_preprocessorFormat;     ///< 预处理器格式

    // ========== C++关键字列表 ==========
    QStringList m_keywords;                   ///< C++关键字列表
    QStringList m_controlFlowKeywords;        ///< 控制流关键字列表
    QStringList m_storageClassKeywords;       ///< 存储类关键字列表
    QStringList m_types;                      ///< C++数据类型列表
    QStringList m_stdTypes;                   ///< C++标准库类型列表
    QStringList m_predefinedFunctions;        ///< C++预定义函数列表
    QStringList m_preprocessorKeywords;       ///< 预处理器关键字列表

    bool m_isDarkTheme;          ///< 当前是否为深色主题
};

}
#endif // SYNTAX_HIGHLIGHTER_H
