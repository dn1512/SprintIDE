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
#ifndef LANGUAGE_TYPE_H
#define LANGUAGE_TYPE_H

#include <QString>

namespace SprintIDE {

/**
 * @brief 语言类型枚举，扩展用于构建系统
 */
enum class LanguageType {
    Unknown,
    Cpp,
    Qt,
    Python,
    Java,
    Go,
    Rust,
    JavaScript,
    TypeScript,
    CSharp,
    Ruby,
    Php,
    Shell,
    Html,
    Css,
    Json,
    Xml,
    Markdown,
    PlainText,
    Yaml,
    Properties,
    Generic_AI      // 保留   // 通用 AI 驱动语言
};

/**
 * @brief 将 LanguageType 转为可读字符串
 */
inline QString languageTypeToString(LanguageType type) {
    switch (type) {
    case LanguageType::Unknown:   return "Unknown";
    case LanguageType::Cpp:       return "Cpp";
    case LanguageType::Qt:        return "Qt";
    case LanguageType::Python:    return "Python";
    case LanguageType::Java:      return "Java";
    case LanguageType::Go:        return "Go";
    case LanguageType::Rust:      return "Rust";
    case LanguageType::JavaScript:return "JavaScript";
    case LanguageType::TypeScript:return "TypeScript";
    case LanguageType::CSharp:    return "CSharp";
    case LanguageType::Ruby:      return "Ruby";
    case LanguageType::Php:       return "Php";
    case LanguageType::Shell:     return "Shell";
    case LanguageType::Html:      return "Html";
    case LanguageType::Css:       return "Css";
    case LanguageType::Json:      return "Json";
    case LanguageType::Xml:       return "Xml";
    case LanguageType::Markdown:  return "Markdown";
    case LanguageType::PlainText: return "PlainText";
    case LanguageType::Yaml: return "YAML";
    case LanguageType::Properties: return "Properties";
    case LanguageType::Generic_AI:return "Generic_AI";
    default: return "Unknown";
    }
}

} // namespace SprintIDE

#endif // LANGUAGE_TYPE_H
