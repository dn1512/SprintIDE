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
#ifndef LANGUAGE_DETECTOR_H
#define LANGUAGE_DETECTOR_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVector>
#include <QSet>

namespace SprintIDE {

/**
 * @brief 语言信息结构体
 *
 * 存储一种编程语言的所有识别信息和语法规则，
 * 用于语法高亮、代码补全和 AI 上下文提供。
 */
struct LanguageInfo
{
    QString id;                 ///< 语言唯一标识符，如 "cpp", "python", "javascript"
    QString name;               ///< 语言显示名称，如 "C++", "Python", "JavaScript"
    QStringList extensions;     ///< 文件扩展名列表，如 {".cpp", ".cc", ".cxx"}
    QStringList mimeTypes;      ///< MIME 类型列表，如 {"text/x-c++src"}

    // 注释符号
    QString lineComment;        ///< 单行注释符号，如 "//"
    QString blockCommentStart;  ///< 多行注释开始符号，如 "/*"
    QString blockCommentEnd;    ///< 多行注释结束符号，如 "*/"

    // 关键字分类（用于语法高亮）
    QStringList keywords;       ///< 一般关键字，如 "if", "else", "return"
    QStringList controlFlow;    ///< 控制流关键字，如 "for", "while", "switch"
    QStringList storageTypes;   ///< 存储类型关键字，如 "static", "const", "extern"
    QStringList dataTypes;      ///< 数据类型关键字，如 "int", "float", "string"
    QStringList builtinFuncs;   ///< 内置函数，如 "print", "len", "sizeof"

    // 预处理器/特殊关键字
    QStringList preprocessor;   ///< 预处理器指令，如 "#include", "#define"
    QStringList constants;      ///< 常量关键字，如 "true", "false", "null"

    // 字符串和字符
    QString stringDelimiter;    ///< 字符串定界符，如 "\""
    QString charDelimiter;      ///< 字符定界符，如 "'"

    // 其他属性
    bool caseSensitive;         ///< 是否区分大小写
    bool hasPreprocessor;       ///< 是否有预处理器
    QString indentStyle;        ///< 缩进风格，如 "spaces" 或 "tabs"
    int indentSize;             ///< 缩进大小（空格数）

    // 构造函数
    LanguageInfo() : caseSensitive(true), hasPreprocessor(false),
        indentStyle("spaces"), indentSize(4) {}
};

/**
 * @brief 语言检测器（单例）
 *
 * 负责根据文件扩展名或内容识别编程语言，
 * 并提供所有支持语言的规则信息。
 *
 * 使用方式：
 *   LanguageInfo lang = LanguageDetector::instance().detectLanguage("main.cpp");
 *   // lang.id == "cpp", lang.name == "C++"
 *
 *   LanguageInfo lang2 = LanguageDetector::instance().detectLanguageByExtension(".py");
 *   // lang2.id == "python"
 */
class LanguageDetector
{
public:
    /**
     * @brief 获取单例实例
     */
    static LanguageDetector& instance();

    // 禁止拷贝和赋值
    LanguageDetector(const LanguageDetector&) = delete;
    LanguageDetector& operator=(const LanguageDetector&) = delete;

    /**
     * @brief 根据文件路径检测语言
     * @param filePath 文件路径（可以是绝对或相对路径）
     * @return 检测到的语言信息，如果无法识别则返回默认语言（Plain Text）
     *
     * 检测优先级：
     * 1. 文件扩展名匹配
     * 2. 文件名匹配（如 Makefile, CMakeLists.txt）
     * 3. 内容检测（如 Shebang 行）
     */
    LanguageInfo detectLanguage(const QString& filePath) const;

    /**
     * @brief 根据文件扩展名检测语言
     * @param extension 文件扩展名（可以带或不带点号，如 ".cpp" 或 "cpp"）
     * @return 检测到的语言信息，如果无法识别则返回默认语言
     */
    LanguageInfo detectLanguageByExtension(const QString& extension) const;

    /**
     * @brief 根据文件内容检测语言
     * @param content 文件内容（至少第一行，用于 Shebang 检测）
     * @return 检测到的语言信息，如果无法识别则返回默认语言
     *
     * 主要用于 Shebang 行检测：
     *   #!/usr/bin/env python  Python
     *   #!/bin/bash           Shell
     *   #!/usr/bin/node       JavaScript
     */
    LanguageInfo detectLanguageByContent(const QString& content) const;

    /**
     * @brief 根据文件名检测语言（处理特殊文件名）
     * @param fileName 文件名（如 "Makefile", "CMakeLists.txt"）
     * @return 检测到的语言信息
     */
    LanguageInfo detectLanguageByFileName(const QString& fileName) const;

    /**
     * @brief 获取所有支持的语言列表
     * @return 语言 ID 列表，如 {"cpp", "python", "javascript", ...}
     */
    QStringList availableLanguages() const;

    /**
     * @brief 根据语言 ID 获取语言信息
     * @param languageId 语言 ID，如 "cpp"
     * @return 语言信息，如果不存在则返回默认语言
     */
    LanguageInfo getLanguage(const QString& languageId) const;

    /**
     * @brief 判断是否支持某种语言
     * @param languageId 语言 ID
     * @return 是否支持
     */
    bool supportsLanguage(const QString& languageId) const;

    /**
     * @brief 获取语言的显示名称
     * @param languageId 语言 ID
     * @return 显示名称，如 "C++"
     */
    QString getLanguageDisplayName(const QString& languageId) const;

    /**
     * @brief 获取语言的注释符号
     * @param languageId 语言 ID
     * @param lineComment 输出参数：单行注释符号
     * @param blockStart 输出参数：块注释开始符号
     * @param blockEnd 输出参数：块注释结束符号
     * @return 是否成功获取
     */
    bool getCommentSymbols(const QString& languageId,
                           QString& lineComment,
                           QString& blockStart,
                           QString& blockEnd) const;

    /**
     * @brief 判断某种语言是否支持多行注释
     */
    bool supportsBlockComment(const QString& languageId) const;

    /**
     * @brief 获取某种语言的关键字列表（用于语法高亮）
     */
    QStringList getKeywords(const QString& languageId) const;

    /**
     * @brief 手动注册或覆盖语言规则
     * @param info 语言信息
     *
     * 可用于扩展或自定义语言规则，如添加新的文件扩展名。
     */
    void registerLanguage(const LanguageInfo& info);

    /**
     * @brief 重新加载内置语言定义（重置所有自定义注册）
     */
    void reloadBuiltinLanguages();

private:
    // 私有构造函数（单例模式）
    LanguageDetector();
    ~LanguageDetector() = default;

    /**
     * @brief 加载内置语言定义
     *
     * 初始化所有支持的语言规则，包括：
     * - C/C++ (cpp)
     * - Python (python)
     * - JavaScript/TypeScript (javascript, typescript)
     * - Java (java)
     * - Go (go)
     * - Rust (rust)
     * - C# (csharp)
     * - Ruby (ruby)
     * - PHP (php)
     * - Shell (shell)
     * - HTML (html)
     * - CSS (css)
     * - JSON (json)
     * - XML (xml)
     * - Markdown (markdown)
     * - Plain Text (text)
     */
    void loadBuiltinLanguages();

    /**
     * @brief 检测 Shebang 行
     * @param firstLine 文件的第一行
     * @return 如果匹配到 Shebang 则返回语言 ID，否则返回空字符串
     */
    QString detectShebang(const QString& firstLine) const;

    /**
    * @brief 规范化扩展名（统一为小写，确保带点号）
    */
   QString normalizeExtension(const QString& extension) const;

   /**
    * @brief 判断 .ts 文件是否为 Qt Linguist 翻译源文件
    *
    * Qt 翻译文件用 .ts 扩展名（XML 格式），与 TypeScript 冲突。
    * 通过读文件头判断：以 <?xml 开头 Qt 翻译文件。
    * 仅在扩展名为 .ts 时调用，避免其他文件的 IO 开销。
    */
   bool isQtTranslationFile(const QString& filePath) const;

    // 成员变量
    QMap<QString, LanguageInfo> m_languages;        ///< 语言 ID 语言信息
    QMap<QString, QString> m_extensionMap;          ///< 扩展名 语言 ID
    QMap<QString, QString> m_fileNameMap;           ///< 特殊文件名 语言 ID
    QMap<QString, QString> m_shebangMap;            ///< Shebang 关键词 语言 ID

    bool m_initialized = false;                     ///< 是否已初始化

    static LanguageDetector* m_instance;            ///< 单例实例
};

} // namespace SprintIDE

#endif // LANGUAGE_DETECTOR_H
