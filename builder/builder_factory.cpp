// src/build/builder_factory.cpp
#include "builder_factory.h"
#include "cpp_builder.h"
#include "python_builder.h"
#include "java_gradle_builder.h"
#include "go_builder.h"
#include "rust_builder.h"
#include "node_builder.h"
#include "generic_builder.h"
#include "java_maven_builder.h"
#include "java_builder.h"
#include <QDebug>

namespace SprintIDE {

QMap<LanguageType, IBuilder*> BuilderFactory::s_builders;

void BuilderFactory::initializeDefaults() {
    registerBuilder(LanguageType::Cpp, new CppBuilder());
    registerBuilder(LanguageType::Python, new PythonBuilder());
    registerBuilder(LanguageType::Java, new JavaBuilder());
    registerBuilder(LanguageType::Go, new GoBuilder());
    registerBuilder(LanguageType::Rust, new RustBuilder());
    registerBuilder(LanguageType::JavaScript, new NodeBuilder());
    registerBuilder(LanguageType::TypeScript, new NodeBuilder());
    registerBuilder(LanguageType::Generic_AI, new GenericBuilder());

    // 可选：其他语言也可注册 NodeBuilder 或 GenericBuilder
    // 例如 CSharp 等，暂时不实现，可回退到 GenericBuilder
}

IBuilder* BuilderFactory::createBuilder(LanguageType lang) {
    if (s_builders.contains(lang)) {
        return s_builders[lang];
    }
    // 如果未注册，但语言是 Generic_AI，返回通用构建器
    if (lang == LanguageType::Generic_AI && s_builders.contains(LanguageType::Generic_AI)) {
        return s_builders[LanguageType::Generic_AI];
    }
    // 默认返回 nullptr，上层应处理
    return nullptr;
}

void BuilderFactory::registerBuilder(LanguageType lang, IBuilder* builder) {
    if (s_builders.contains(lang)) {
        delete s_builders[lang];
    }
    s_builders[lang] = builder;
}

void BuilderFactory::shutdown() {
    for (auto it = s_builders.begin(); it != s_builders.end(); ++it) {
        delete it.value();
    }
    s_builders.clear();
}

} // namespace SprintIDE
