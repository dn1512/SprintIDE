// src/language/language_type.h
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
