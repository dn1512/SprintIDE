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
#ifndef LANGUAGE_HIGHLIGHTER_H
#define LANGUAGE_HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>
#include <QColor>
#include <QMap>

#include "language_detector.h"

namespace SprintIDE {

/**
 * @brief 高亮规则结构体
 *
 * 定义一条语法高亮规则：匹配模式 + 应用格式
 */
struct HighlightRule
{
    QRegularExpression pattern;     ///< 匹配模式（正则表达式）
    QTextCharFormat format;         ///< 应用格式（颜色、字体等）
    int priority;                   ///< 优先级（数值越大越优先，用于规则冲突时）

    HighlightRule() : priority(0) {}
    HighlightRule(const QRegularExpression& p, const QTextCharFormat& f, int pri = 0)
        : pattern(p), format(f), priority(pri) {}
};

/**
 * @brief 多语言语法高亮器
 *
 * 基于 LanguageDetector 提供的语言信息，动态生成高亮规则。
 * 支持 17 种语言的语法高亮，包括：
 * - C/C++, Python, JavaScript, TypeScript, Java, Go, Rust, C#
 * - Ruby, PHP, Shell, HTML, CSS, JSON, XML, Markdown, Plain Text
 *
 * 使用方式：
 *   LanguageHighlighter *highlighter = new LanguageHighlighter(document);
 *   highlighter->setLanguageByFilePath("main.cpp");  // 自动检测语言
 *   // 或直接指定语言 ID
 *   highlighter->setLanguage("python");
 *
 *   // 切换主题
 *   highlighter->setTheme(true);  // 深色主题
 */
class LanguageHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父文档对象
     */
    explicit LanguageHighlighter(QTextDocument *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~LanguageHighlighter() override;

    /**
     * @brief 根据文件路径设置语言
     * @param filePath 文件路径
     *
     * 内部调用 LanguageDetector::detectLanguage() 自动检测语言
     */
    void setLanguageByFilePath(const QString& filePath);

    /**
     * @brief 根据语言 ID 设置语言
     * @param languageId 语言 ID，如 "cpp", "python", "javascript"
     *
     * 如果语言 ID 不存在，自动回退到 "text"（纯文本）
     */
    void setLanguage(const QString& languageId);

    /**
     * @brief 获取当前语言信息
     */
    LanguageInfo currentLanguage() const;

    /**
     * @brief 获取当前语言 ID
     */
    QString currentLanguageId() const;

    /**
     * @brief 设置主题（深色/浅色）
     * @param isDark 是否为深色主题
     *
     * 切换主题会重新生成所有高亮规则的颜色
     */
    void setTheme(bool isDark);

    /**
     * @brief 检查是否为深色主题
     */
    bool isDarkTheme() const;

    /**
     * @brief 重新高亮整个文档
     *
     * 当语言或主题变化后，调用此方法刷新所有高亮
     */
    // void rehighlight();

    /**
     * @brief 获取语言显示名称
     */
    QString languageDisplayName() const;

    /**
     * @brief 重新高亮整个文档
     */
    void rehighlightDocument();

signals:
    /**
     * @brief 语言变化信号
     * @param languageId 新的语言 ID
     * @param languageName 新的语言名称
     */
    void languageChanged(const QString& languageId, const QString& languageName);

protected:
    /**
     * @brief 高亮文本块（QSyntaxHighlighter 纯虚函数）
     * @param text 需要高亮的文本
     *
     * 这是语法高亮的核心方法，对每个文本块调用
     */
    void highlightBlock(const QString& text) override;

private:
    /**
     * @brief 根据语言信息更新高亮规则
     * @param info 语言信息
     *
     * 根据语言的关键字、数据类型、注释符号等生成高亮规则列表
     */
    void updateRules(const LanguageInfo& info);

    /**
     * @brief 初始化默认颜色方案
     * @param isDark 是否为深色主题
     *
     * 设置所有语法元素的默认颜色
     */
    void initDefaultColors(bool isDark);

    /**
     * @brief 创建关键字格式
     * @param color 颜色
     * @param bold 是否加粗
     * @param italic 是否斜体
     */
    QTextCharFormat createFormat(const QColor& color,
                                 bool bold = false,
                                 bool italic = false) const;

    /**
     * @brief 生成关键字高亮规则
     * @param keywords 关键字列表
     * @param format 格式
     * @param priority 优先级
     *
     * 将关键字列表转换为正则表达式并生成规则
     */
    QVector<HighlightRule> createKeywordRules(const QStringList& keywords,
                                              const QTextCharFormat& format,
                                              int priority = 1) const;

    /**
     * @brief 处理多行注释
     * @param text 当前文本块
     * @param startPattern 开始模式
     * @param endPattern 结束模式
     * @param format 格式
     *
     * 处理跨行的多行注释，使用 QSyntaxHighlighter 的状态机制
     */
    void handleMultiLineComment(const QString& text,
                                const QRegularExpression& startPattern,
                                const QRegularExpression& endPattern,
                                const QTextCharFormat& format);


    void highlightYaml(const QString& text);

    void highlightProperties(const QString& text);

    // ---- 成员变量 ----

    // 当前语言信息
    LanguageInfo m_currentLanguage;

    // 高亮规则列表
    QVector<HighlightRule> m_rules;

    // 多行注释规则
    QRegularExpression m_commentStartPattern;   ///< 多行注释开始
    QRegularExpression m_commentEndPattern;     ///< 多行注释结束

    // 颜色方案
    bool m_isDark = true;

    // ---- 语法元素颜色（根据主题动态变化） ----
    QColor m_keywordColor;          ///< 关键字颜色
    QColor m_controlFlowColor;      ///< 控制流关键字颜色
    QColor m_storageTypeColor;      ///< 存储类型关键字颜色
    QColor m_dataTypeColor;         ///< 数据类型颜色
    QColor m_builtinFuncColor;      ///< 内置函数颜色
    QColor m_commentColor;          ///< 注释颜色
    QColor m_stringColor;           ///< 字符串颜色
    QColor m_charColor;             ///< 字符颜色
    QColor m_numberColor;           ///< 数字颜色
    QColor m_preprocessorColor;     ///< 预处理器颜色
    QColor m_constantColor;         ///< 常量颜色
    QColor m_functionColor;         ///< 函数名颜色
    QColor m_classColor;            ///< 类名颜色

    // ---- 格式缓存 ----
    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_controlFlowFormat;
    QTextCharFormat m_storageTypeFormat;
    QTextCharFormat m_dataTypeFormat;
    QTextCharFormat m_builtinFuncFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_multiLineCommentFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_charFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_preprocessorFormat;
    QTextCharFormat m_constantFormat;
    QTextCharFormat m_functionFormat;
    QTextCharFormat m_classFormat;
};

} // namespace SprintIDE

#endif // LANGUAGE_HIGHLIGHTER_H
