#include "language_detector.h"

#include <QFileInfo>
#include <QDebug>
#include <QRegularExpression>

namespace SprintIDE {

// ============================================================
// 静态成员初始化
// ============================================================
LanguageDetector* LanguageDetector::m_instance = nullptr;

// ============================================================
// 单例实例获取
// ============================================================
LanguageDetector& LanguageDetector::instance()
{
    if (!m_instance) {
        m_instance = new LanguageDetector();
    }
    return *m_instance;
}

// ============================================================
// 私有构造函数
// ============================================================
LanguageDetector::LanguageDetector()
{
    loadBuiltinLanguages();
    m_initialized = true;
}

// ============================================================
// 加载所有内置语言定义
// ============================================================
void LanguageDetector::loadBuiltinLanguages()
{
    // 清空现有映射（如果有）
    m_languages.clear();
    m_extensionMap.clear();
    m_fileNameMap.clear();
    m_shebangMap.clear();

    // ============================================================
    // 1. C / C++
    // ============================================================
    {
        LanguageInfo lang;
        lang.id = "cpp";
        lang.name = "C++";
        lang.extensions = {".cpp", ".cc", ".cxx", ".c++", ".h", ".hpp", ".hh", ".hxx", ".h++", ".c", ".h", ".ipp"};
        lang.mimeTypes = {"text/x-c++src", "text/x-csrc", "text/x-chdr", "text/x-c++hdr"};

        lang.lineComment = "//";
        lang.blockCommentStart = "/*";
        lang.blockCommentEnd = "*/";

        lang.keywords = {
            "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor",
            "bool", "break", "case", "catch", "char", "char8_t", "char16_t", "char32_t",
            "class", "compl", "concept", "const", "consteval", "constexpr", "constinit",
            "decltype", "delete", "do", "double", "dynamic_cast", "else", "enum",
            "explicit", "export", "extern", "false", "float", "friend", "inline",
            "int", "long", "mutable", "namespace", "new", "noexcept", "not", "not_eq",
            "nullptr", "operator", "or", "or_eq", "private", "protected", "public",
            "register", "reinterpret_cast", "requires", "return", "short", "signed",
            "sizeof", "static", "static_assert", "static_cast", "struct", "switch",
            "template", "this", "thread_local", "throw", "true", "try", "typedef",
            "typeid", "typename", "union", "unsigned", "using", "virtual", "void",
            "volatile", "wchar_t", "while", "xor", "xor_eq"
        };

        lang.controlFlow = {"if", "else", "for", "while", "do", "switch", "case", "default", "break", "continue", "return", "goto"};
        lang.storageTypes = {"auto", "register", "static", "extern", "mutable", "thread_local", "const", "volatile", "constexpr"};
        lang.dataTypes = {"void", "bool", "char", "double", "float", "int", "long", "short", "signed", "unsigned", "wchar_t"};
        lang.builtinFuncs = {"printf", "scanf", "cout", "cin", "endl", "malloc", "free", "new", "delete"};
        lang.preprocessor = {"include", "define", "undef", "if", "ifdef", "ifndef", "else", "elif", "endif", "line", "error", "pragma", "import"};
        lang.constants = {"true", "false", "nullptr"};

        lang.stringDelimiter = "\"";
        lang.charDelimiter = "'";
        lang.caseSensitive = true;
        lang.hasPreprocessor = true;
        lang.indentStyle = "spaces";
        lang.indentSize = 4;

        m_languages[lang.id] = lang;
        for (const QString& ext : lang.extensions) {
            m_extensionMap[normalizeExtension(ext)] = lang.id;
        }
    }

    // ============================================================
    // 2. Python
    // ============================================================
    {
        LanguageInfo lang;
        lang.id = "python";
        lang.name = "Python";
        lang.extensions = {".py", ".pyw", ".pyi"};
        lang.mimeTypes = {"text/x-python", "text/x-python3"};

        lang.lineComment = "#";
        lang.blockCommentStart = "\"\"\"";
        lang.blockCommentEnd = "\"\"\"";

        lang.keywords = {
            "False", "None", "True", "and", "as", "assert", "async", "await", "break",
            "class", "continue", "def", "del", "elif", "else", "except", "finally",
            "for", "from", "global", "if", "import", "in", "is", "lambda", "nonlocal",
            "not", "or", "pass", "raise", "return", "try", "while", "with", "yield"
        };

        lang.controlFlow = {"if", "elif", "else", "for", "while", "break", "continue", "return", "raise", "try", "except", "finally"};
        lang.storageTypes = {"global", "nonlocal", "del"};
        lang.dataTypes = {"int", "float", "str", "bool", "list", "tuple", "dict", "set", "None"};
        lang.builtinFuncs = {"print", "len", "type", "str", "int", "float", "range", "enumerate", "zip", "open", "input", "sorted"};
        lang.preprocessor = {};
        lang.constants = {"True", "False", "None"};

        lang.stringDelimiter = "\"";
        lang.charDelimiter = "'";
        lang.caseSensitive = true;
        lang.hasPreprocessor = false;
        lang.indentStyle = "spaces";
        lang.indentSize = 4;

        m_languages[lang.id] = lang;
        for (const QString& ext : lang.extensions) {
            m_extensionMap[normalizeExtension(ext)] = lang.id;
        }

        // Shebang 映射
        m_shebangMap["python"] = lang.id;
        m_shebangMap["python3"] = lang.id;
    }

    // ============================================================
    // 3. JavaScript
    // ============================================================
    {
        LanguageInfo lang;
        lang.id = "javascript";
        lang.name = "JavaScript";
        lang.extensions = {".js", ".mjs", ".cjs"};
        lang.mimeTypes = {"text/javascript", "application/javascript"};

        lang.lineComment = "//";
        lang.blockCommentStart = "/*";
        lang.blockCommentEnd = "*/";

        lang.keywords = {
            "abstract", "arguments", "await", "boolean", "break", "byte", "case",
            "catch", "char", "class", "const", "continue", "debugger", "default",
            "delete", "do", "double", "else", "enum", "eval", "export", "extends",
            "false", "final", "finally", "float", "for", "function", "goto", "if",
            "implements", "import", "in", "instanceof", "int", "interface", "let",
            "long", "native", "new", "null", "package", "private", "protected",
            "public", "return", "short", "static", "super", "switch", "synchronized",
            "this", "throw", "throws", "transient", "true", "try", "typeof", "var",
            "void", "volatile", "while", "with", "yield"
        };

        lang.controlFlow = {"if", "else", "switch", "case", "default", "for", "while", "do", "break", "continue", "return", "try", "catch", "finally", "throw"};
        lang.storageTypes = {"var", "let", "const", "static", "export", "import"};
        lang.dataTypes = {"number", "string", "boolean", "object", "array", "function", "undefined", "symbol", "bigint"};
        lang.builtinFuncs = {"console.log", "setTimeout", "setInterval", "parseInt", "parseFloat", "JSON.stringify", "JSON.parse"};
        lang.preprocessor = {};
        lang.constants = {"true", "false", "null", "undefined"};

        lang.stringDelimiter = "\"";
        lang.charDelimiter = "'";
        lang.caseSensitive = true;
        lang.hasPreprocessor = false;
        lang.indentStyle = "spaces";
        lang.indentSize = 2;

        m_languages[lang.id] = lang;
        for (const QString& ext : lang.extensions) {
            m_extensionMap[normalizeExtension(ext)] = lang.id;
        }

        m_shebangMap["node"] = lang.id;
    }

    // ============================================================
    // 4. TypeScript
    // ============================================================
    {
        LanguageInfo lang;
        lang.id = "typescript";
        lang.name = "TypeScript";
        lang.extensions = {".ts", ".tsx"};
        lang.mimeTypes = {"text/typescript"};

        lang.lineComment = "//";
        lang.blockCommentStart = "/*";
        lang.blockCommentEnd = "*/";

        lang.keywords = {
            "abstract", "any", "as", "asserts", "bigint", "boolean", "break", "case",
            "catch", "class", "const", "constructor", "continue", "debugger", "declare",
            "default", "delete", "do", "else", "enum", "export", "extends", "false",
            "finally", "for", "from", "function", "if", "implements", "import", "in",
            "infer", "instanceof", "interface", "is", "keyof", "let", "module",
            "namespace", "never", "new", "null", "number", "object", "package",
            "private", "protected", "public", "readonly", "require", "return",
            "static", "string", "super", "switch", "symbol", "this", "throw", "true",
            "try", "type", "typeof", "undefined", "unique", "unknown", "var", "void",
            "while", "with", "yield"
        };

        lang.controlFlow = {"if", "else", "switch", "case", "default", "for", "while", "do", "break", "continue", "return", "try", "catch", "finally", "throw"};
        lang.storageTypes = {"var", "let", "const", "static", "export", "import", "declare"};
        lang.dataTypes = {"string", "number", "boolean", "any", "void", "never", "unknown", "object", "symbol", "bigint"};
        lang.builtinFuncs = {"console.log"};
        lang.preprocessor = {};
        lang.constants = {"true", "false", "null", "undefined"};

        lang.stringDelimiter = "\"";
        lang.charDelimiter = "'";
        lang.caseSensitive = true;
        lang.hasPreprocessor = false;
        lang.indentStyle = "spaces";
        lang.indentSize = 2;

        m_languages[lang.id] = lang;
        for (const QString& ext : lang.extensions) {
            m_extensionMap[normalizeExtension(ext)] = lang.id;
        }
    }

    // ============================================================
    // 5. Java
    // ============================================================
    {
        LanguageInfo lang;
        lang.id = "java";
        lang.name = "Java";
        lang.extensions = {".java"};
        lang.mimeTypes = {"text/x-java"};

        lang.lineComment = "//";
        lang.blockCommentStart = "/*";
        lang.blockCommentEnd = "*/";

        lang.keywords = {
            "abstract", "assert", "boolean", "break", "byte", "case", "catch",
            "char", "class", "const", "continue", "default", "do", "double", "else",
            "enum", "extends", "final", "finally", "float", "for", "goto", "if",
            "implements", "import", "instanceof", "int", "interface", "long", "native",
            "new", "package", "private", "protected", "public", "return", "short",
            "static", "strictfp", "super", "switch", "synchronized", "this", "throw",
            "throws", "transient", "try", "void", "volatile", "while"
        };

        lang.controlFlow = {"if", "else", "switch", "case", "default", "for", "while", "do", "break", "continue", "return", "try", "catch", "finally", "throw"};
        lang.storageTypes = {"public", "private", "protected", "static", "final", "abstract", "synchronized", "volatile", "transient"};
        lang.dataTypes = {"boolean", "byte", "char", "double", "float", "int", "long", "short", "void"};
        lang.builtinFuncs = {"System.out.println", "System.out.print", "main"};
        lang.preprocessor = {};
        lang.constants = {"true", "false", "null"};

        lang.stringDelimiter = "\"";
        lang.charDelimiter = "'";
        lang.caseSensitive = true;
        lang.hasPreprocessor = false;
        lang.indentStyle = "spaces";
        lang.indentSize = 4;

        m_languages[lang.id] = lang;
        for (const QString& ext : lang.extensions) {
            m_extensionMap[normalizeExtension(ext)] = lang.id;
        }
    }

    // ============================================================
    // 6. Go
    // ============================================================
    {
        LanguageInfo lang;
        lang.id = "go";
        lang.name = "Go";
        lang.extensions = {".go"};
        lang.mimeTypes = {"text/x-go"};

        lang.lineComment = "//";
        lang.blockCommentStart = "/*";
        lang.blockCommentEnd = "*/";

        lang.keywords = {
            "break", "case", "chan", "const", "continue", "default", "defer",
            "else", "fallthrough", "for", "func", "go", "goto", "if", "import",
            "interface", "map", "package", "range", "return", "select", "struct",
            "switch", "type", "var"
        };

        lang.controlFlow = {"if", "else", "for", "switch", "case", "default", "break", "continue", "return", "goto", "defer", "fallthrough"};
        lang.storageTypes = {"var", "const", "type", "func", "package", "import"};
        lang.dataTypes = {"bool", "byte", "complex64", "complex128", "error", "float32", "float64", "int", "int8", "int16", "int32", "int64", "rune", "string", "uint", "uint8", "uint16", "uint32", "uint64", "uintptr"};
        lang.builtinFuncs = {"append", "cap", "close", "complex", "copy", "delete", "imag", "len", "make", "new", "panic", "print", "println", "real", "recover"};
        lang.preprocessor = {};
        lang.constants = {"true", "false", "nil", "iota"};

        lang.stringDelimiter = "\"";
        lang.charDelimiter = "'";
        lang.caseSensitive = true;
        lang.hasPreprocessor = false;
        lang.indentStyle = "tabs";
        lang.indentSize = 1;

        m_languages[lang.id] = lang;
        for (const QString& ext : lang.extensions) {
            m_extensionMap[normalizeExtension(ext)] = lang.id;
        }
    }

    // ============================================================
    // 7. Rust
    // ============================================================
    {
        LanguageInfo lang;
        lang.id = "rust";
        lang.name = "Rust";
        lang.extensions = {".rs"};
        lang.mimeTypes = {"text/x-rust"};

        lang.lineComment = "//";
        lang.blockCommentStart = "/*";
        lang.blockCommentEnd = "*/";

        lang.keywords = {
            "abstract", "alignof", "as", "async", "await", "become", "box", "break",
            "const", "continue", "crate", "do", "dyn", "else", "enum", "extern",
            "false", "final", "fn", "for", "if", "impl", "in", "let", "loop", "macro",
            "match", "mod", "move", "mut", "override", "priv", "pub", "ref", "return",
            "self", "Self", "static", "struct", "super", "trait", "true", "try", "type",
            "typeof", "unsafe", "unsized", "use", "virtual", "where", "while", "yield"
        };

        lang.controlFlow = {"if", "else", "for", "while", "loop", "break", "continue", "return", "match"};
        lang.storageTypes = {"pub", "crate", "static", "const", "let", "mut", "ref", "move"};
        lang.dataTypes = {"bool", "char", "i8", "i16", "i32", "i64", "i128", "isize", "u8", "u16", "u32", "u64", "u128", "usize", "f32", "f64", "str", "String"};
        lang.builtinFuncs = {"print", "println", "format", "panic"};
        lang.preprocessor = {};
        lang.constants = {"true", "false"};

        lang.stringDelimiter = "\"";
        lang.charDelimiter = "'";
        lang.caseSensitive = true;
        lang.hasPreprocessor = false;
        lang.indentStyle = "spaces";
        lang.indentSize = 4;

        m_languages[lang.id] = lang;
        for (const QString& ext : lang.extensions) {
            m_extensionMap[normalizeExtension(ext)] = lang.id;
        }
    }

    // ============================================================
    // 8. C#
    // ============================================================
    {
        LanguageInfo lang;
        lang.id = "csharp";
        lang.name = "C#";
        lang.extensions = {".cs"};
        lang.mimeTypes = {"text/x-csharp"};

        lang.lineComment = "//";
        lang.blockCommentStart = "/*";
        lang.blockCommentEnd = "*/";

        lang.keywords = {
            "abstract", "add", "alias", "as", "ascending", "async", "await", "base",
            "bool", "break", "byte", "case", "catch", "char", "checked", "class",
            "const", "continue", "decimal", "default", "delegate", "descending", "do",
            "double", "dynamic", "else", "enum", "event", "explicit", "extern", "false",
            "finally", "fixed", "float", "for", "foreach", "from", "get", "global",
            "goto", "group", "if", "implicit", "in", "int", "interface", "internal",
            "into", "is", "join", "let", "lock", "long", "namespace", "new", "null",
            "object", "operator", "orderby", "out", "override", "params", "partial",
            "private", "protected", "public", "readonly", "ref", "remove", "return",
            "sbyte", "sealed", "select", "set", "short", "sizeof", "stackalloc",
            "static", "string", "struct", "switch", "this", "throw", "true", "try",
            "typeof", "uint", "ulong", "unchecked", "unsafe", "ushort", "using",
            "value", "var", "virtual", "void", "volatile", "where", "while", "yield"
        };

        lang.controlFlow = {"if", "else", "switch", "case", "default", "for", "foreach", "while", "do", "break", "continue", "return", "goto", "try", "catch", "finally", "throw"};
        lang.storageTypes = {"public", "private", "protected", "internal", "static", "readonly", "const", "abstract", "sealed", "virtual", "override", "new", "async", "unsafe"};
        lang.dataTypes = {"bool", "byte", "char", "decimal", "double", "float", "int", "long", "sbyte", "short", "string", "uint", "ulong", "ushort", "void", "object", "dynamic"};
        lang.builtinFuncs = {"Console.WriteLine", "Console.Write"};
        lang.preprocessor = {"if", "else", "elif", "endif", "define", "undef", "warning", "error", "line", "region", "endregion", "pragma"};
        lang.constants = {"true", "false", "null"};

        lang.stringDelimiter = "\"";
        lang.charDelimiter = "'";
        lang.caseSensitive = true;
        lang.hasPreprocessor = true;
        lang.indentStyle = "spaces";
        lang.indentSize = 4;

        m_languages[lang.id] = lang;
        for (const QString& ext : lang.extensions) {
            m_extensionMap[normalizeExtension(ext)] = lang.id;
        }
    }

    // ============================================================
    // 9. Ruby
    // ============================================================
    {
        LanguageInfo lang;
        lang.id = "ruby";
        lang.name = "Ruby";
        lang.extensions = {".rb", ".rake", ".gemspec"};
        lang.mimeTypes = {"text/x-ruby"};

        lang.lineComment = "#";
        lang.blockCommentStart = "=begin";
        lang.blockCommentEnd = "=end";

        lang.keywords = {
            "BEGIN", "END", "alias", "and", "begin", "break", "case", "class",
            "def", "defined?", "do", "else", "elsif", "end", "ensure", "false",
            "for", "if", "in", "module", "next", "nil", "not", "or", "redo", "rescue",
            "retry", "return", "self", "super", "then", "true", "undef", "unless",
            "until", "when", "while", "yield"
        };

        lang.controlFlow = {"if", "else", "elsif", "unless", "case", "when", "for", "while", "until", "break", "next", "redo", "retry", "return", "yield", "begin", "rescue", "ensure"};
        lang.storageTypes = {"class", "module", "def", "alias", "undef"};
        lang.dataTypes = {"Array", "Hash", "String", "Integer", "Float", "TrueClass", "FalseClass", "NilClass", "Symbol", "Proc"};
        lang.builtinFuncs = {"puts", "print", "p", "gets", "require", "load", "include", "extend", "prepend"};
        lang.preprocessor = {};
        lang.constants = {"true", "false", "nil"};

        lang.stringDelimiter = "\"";
        lang.charDelimiter = "'";
        lang.caseSensitive = true;
        lang.hasPreprocessor = false;
        lang.indentStyle = "spaces";
        lang.indentSize = 2;

        m_languages[lang.id] = lang;
        for (const QString& ext : lang.extensions) {
            m_extensionMap[normalizeExtension(ext)] = lang.id;
        }

        m_shebangMap["ruby"] = lang.id;
    }

    // ============================================================
    // 10. PHP
    // ============================================================
    {
        LanguageInfo lang;
        lang.id = "php";
        lang.name = "PHP";
        lang.extensions = {".php", ".php3", ".php4", ".php5", ".phtml"};
        lang.mimeTypes = {"text/x-php", "application/x-httpd-php"};

        lang.lineComment = "//";
        lang.blockCommentStart = "/*";
        lang.blockCommentEnd = "*/";

        lang.keywords = {
            "__CLASS__", "__DIR__", "__FILE__", "__FUNCTION__", "__LINE__",
            "__METHOD__", "__NAMESPACE__", "__TRAIT__", "abstract", "and", "array",
            "as", "break", "callable", "case", "catch", "class", "clone", "const",
            "continue", "declare", "default", "die", "do", "echo", "else", "elseif",
            "empty", "enddeclare", "endfor", "endforeach", "endif", "endswitch",
            "endwhile", "eval", "exit", "extends", "final", "for", "foreach",
            "function", "global", "goto", "if", "implements", "include", "include_once",
            "instanceof", "insteadof", "interface", "isset", "list", "namespace",
            "new", "or", "print", "private", "protected", "public", "require",
            "require_once", "return", "static", "switch", "throw", "trait", "try",
            "unset", "use", "var", "while", "xor", "yield"
        };

        lang.controlFlow = {"if", "elseif", "else", "switch", "case", "default", "for", "foreach", "while", "do", "break", "continue", "return", "goto", "try", "catch", "finally", "throw"};
        lang.storageTypes = {"public", "private", "protected", "static", "final", "abstract", "trait", "interface", "class"};
        lang.dataTypes = {"int", "float", "string", "bool", "array", "object", "callable", "iterable", "void", "mixed"};
        lang.builtinFuncs = {"echo", "print", "var_dump", "print_r", "isset", "empty", "unset", "count", "strlen", "array_push"};
        lang.preprocessor = {};
        lang.constants = {"true", "false", "null", "__FILE__", "__LINE__", "__DIR__"};

        lang.stringDelimiter = "\"";
        lang.charDelimiter = "'";
        lang.caseSensitive = true;
        lang.hasPreprocessor = false;
        lang.indentStyle = "spaces";
        lang.indentSize = 4;

        m_languages[lang.id] = lang;
        for (const QString& ext : lang.extensions) {
            m_extensionMap[normalizeExtension(ext)] = lang.id;
        }

        m_shebangMap["php"] = lang.id;
    }

    // ============================================================
    // 11. Shell Script
    // ============================================================
    {
        LanguageInfo lang;
        lang.id = "shell";
        lang.name = "Shell Script";
        lang.extensions = {".sh", ".bash", ".zsh", ".fish"};
        lang.mimeTypes = {"text/x-shellscript"};

        lang.lineComment = "#";
        lang.blockCommentStart = "";
        lang.blockCommentEnd = "";

        lang.keywords = {
            "case", "do", "done", "elif", "else", "esac", "fi", "for", "function",
            "if", "in", "select", "then", "time", "until", "while"
        };

        lang.controlFlow = {"if", "then", "elif", "else", "fi", "case", "esac", "for", "while", "until", "select", "do", "done", "continue", "break", "return", "exit"};
        lang.storageTypes = {"export", "local", "readonly", "declare", "typeset"};
        lang.dataTypes = {"integer", "string", "array"};
        lang.builtinFuncs = {"echo", "printf", "read", "test", "eval", "exec", "source", "trap", "wait", "sleep"};
        lang.preprocessor = {};
        lang.constants = {"true", "false"};

        lang.stringDelimiter = "\"";
        lang.charDelimiter = "'";
        lang.caseSensitive = true;
        lang.hasPreprocessor = false;
        lang.indentStyle = "spaces";
        lang.indentSize = 2;

        m_languages[lang.id] = lang;
        for (const QString& ext : lang.extensions) {
            m_extensionMap[normalizeExtension(ext)] = lang.id;
        }

        m_shebangMap["bash"] = lang.id;
        m_shebangMap["sh"] = lang.id;
        m_shebangMap["zsh"] = lang.id;
        m_shebangMap["fish"] = lang.id;
    }

    // ============================================================
    // 12. HTML
    // ============================================================
    {
        LanguageInfo lang;
        lang.id = "html";
        lang.name = "HTML";
        lang.extensions = {".html", ".htm", ".xhtml"};
        lang.mimeTypes = {"text/html", "application/xhtml+xml"};

        lang.lineComment = "";
        lang.blockCommentStart = "<!--";
        lang.blockCommentEnd = "-->";

        lang.keywords = {
            "html", "head", "title", "body", "div", "span", "p", "a", "img",
            "ul", "ol", "li", "table", "tr", "td", "th", "form", "input",
            "button", "script", "style", "link", "meta", "h1", "h2", "h3",
            "h4", "h5", "h6", "br", "hr", "strong", "em", "b", "i", "u"
        };

        lang.controlFlow = {};
        lang.storageTypes = {};
        lang.dataTypes = {};
        lang.builtinFuncs = {};
        lang.preprocessor = {};
        lang.constants = {};

        lang.stringDelimiter = "\"";
        lang.charDelimiter = "'";
        lang.caseSensitive = false;
        lang.hasPreprocessor = false;
        lang.indentStyle = "spaces";
        lang.indentSize = 2;

        m_languages[lang.id] = lang;
        for (const QString& ext : lang.extensions) {
            m_extensionMap[normalizeExtension(ext)] = lang.id;
        }
    }

    // ============================================================
    // 13. CSS
    // ============================================================
    {
        LanguageInfo lang;
        lang.id = "css";
        lang.name = "CSS";
        lang.extensions = {".css", ".scss", ".sass", ".less"};
        lang.mimeTypes = {"text/css"};

        lang.lineComment = "";
        lang.blockCommentStart = "/*";
        lang.blockCommentEnd = "*/";

        lang.keywords = {
            "color", "background", "font", "border", "margin", "padding",
            "width", "height", "display", "position", "top", "right", "bottom", "left",
            "flex", "grid", "float", "clear", "text-align", "font-size",
            "line-height", "vertical-align", "z-index", "opacity", "overflow"
        };

        lang.controlFlow = {};
        lang.storageTypes = {};
        lang.dataTypes = {};
        lang.builtinFuncs = {};
        lang.preprocessor = {};
        lang.constants = {};

        lang.stringDelimiter = "\"";
        lang.charDelimiter = "'";
        lang.caseSensitive = true;
        lang.hasPreprocessor = false;
        lang.indentStyle = "spaces";
        lang.indentSize = 2;

        m_languages[lang.id] = lang;
        for (const QString& ext : lang.extensions) {
            m_extensionMap[normalizeExtension(ext)] = lang.id;
        }
    }

    // ============================================================
    // 14. JSON
    // ============================================================
    {
        LanguageInfo lang;
        lang.id = "json";
        lang.name = "JSON";
        lang.extensions = {".json", ".jsonc"};
        lang.mimeTypes = {"application/json"};

        lang.lineComment = "//";
        lang.blockCommentStart = "/*";
        lang.blockCommentEnd = "*/";

        lang.keywords = {};
        lang.controlFlow = {};
        lang.storageTypes = {};
        lang.dataTypes = {};
        lang.builtinFuncs = {};
        lang.preprocessor = {};
        lang.constants = {"true", "false", "null"};

        lang.stringDelimiter = "\"";
        lang.charDelimiter = "";
        lang.caseSensitive = true;
        lang.hasPreprocessor = false;
        lang.indentStyle = "spaces";
        lang.indentSize = 2;

        m_languages[lang.id] = lang;
        for (const QString& ext : lang.extensions) {
            m_extensionMap[normalizeExtension(ext)] = lang.id;
        }
    }

    // ============================================================
    // YAML (新增)
    // ============================================================
    {
        LanguageInfo lang;
        lang.id = "yaml";
        lang.name = "YAML";
        lang.extensions = {".yml", ".yaml"};
        lang.mimeTypes = {"text/yaml", "application/x-yaml"};

        lang.lineComment = "#";
        lang.blockCommentStart = "";
        lang.blockCommentEnd = "";

        lang.keywords = {}; // YAML 本身没有关键字，但可以将某些特殊值高亮
        lang.controlFlow = {};
        lang.storageTypes = {};
        lang.dataTypes = {};
        lang.builtinFuncs = {};
        lang.preprocessor = {};
        lang.constants = {"true", "false", "on", "off", "yes", "no", "null", "~"};

        lang.stringDelimiter = "\"";
        lang.charDelimiter = "'";
        lang.caseSensitive = true;
        lang.hasPreprocessor = false;
        lang.indentStyle = "spaces";
        lang.indentSize = 2;

        m_languages[lang.id] = lang;
        for (const QString& ext : lang.extensions) {
            m_extensionMap[normalizeExtension(ext)] = lang.id;
        }
    }

    // ============================================================
    // Properties (新增)
    // ============================================================
    {
        LanguageInfo lang;
        lang.id = "properties";
        lang.name = "Properties";
        lang.extensions = {".properties"};
        lang.mimeTypes = {"text/x-java-properties"};

        lang.lineComment = "#";
        lang.blockCommentStart = "";
        lang.blockCommentEnd = "";

        lang.keywords = {};
        lang.controlFlow = {};
        lang.storageTypes = {};
        lang.dataTypes = {};
        lang.builtinFuncs = {};
        lang.preprocessor = {};
        lang.constants = {};

        lang.stringDelimiter = "\"";
        lang.charDelimiter = "";
        lang.caseSensitive = false;
        lang.hasPreprocessor = false;
        lang.indentStyle = "spaces";
        lang.indentSize = 2;

        m_languages[lang.id] = lang;
        for (const QString& ext : lang.extensions) {
            m_extensionMap[normalizeExtension(ext)] = lang.id;
        }
    }

    // ============================================================
    // 15. XML
    // ============================================================
    {
        LanguageInfo lang;
        lang.id = "xml";
        lang.name = "XML";
        lang.extensions = {".xml", ".xsd", ".xslt", ".xhtml"};
        lang.mimeTypes = {"application/xml", "text/xml"};

        lang.lineComment = "";
        lang.blockCommentStart = "<!--";
        lang.blockCommentEnd = "-->";

        lang.keywords = {};
        lang.controlFlow = {};
        lang.storageTypes = {};
        lang.dataTypes = {};
        lang.builtinFuncs = {};
        lang.preprocessor = {};
        lang.constants = {};

        lang.stringDelimiter = "\"";
        lang.charDelimiter = "'";
        lang.caseSensitive = true;
        lang.hasPreprocessor = false;
        lang.indentStyle = "spaces";
        lang.indentSize = 2;

        m_languages[lang.id] = lang;
        for (const QString& ext : lang.extensions) {
            m_extensionMap[normalizeExtension(ext)] = lang.id;
        }
    }

    // ============================================================
    // 16. Markdown
    // ============================================================
    {
        LanguageInfo lang;
        lang.id = "markdown";
        lang.name = "Markdown";
        lang.extensions = {".md", ".markdown", ".mdown", ".mkd"};
        lang.mimeTypes = {"text/markdown"};

        lang.lineComment = "";
        lang.blockCommentStart = "";
        lang.blockCommentEnd = "";

        lang.keywords = {};
        lang.controlFlow = {};
        lang.storageTypes = {};
        lang.dataTypes = {};
        lang.builtinFuncs = {};
        lang.preprocessor = {};
        lang.constants = {};

        lang.stringDelimiter = "\"";
        lang.charDelimiter = "";
        lang.caseSensitive = false;
        lang.hasPreprocessor = false;
        lang.indentStyle = "spaces";
        lang.indentSize = 2;

        m_languages[lang.id] = lang;
        for (const QString& ext : lang.extensions) {
            m_extensionMap[normalizeExtension(ext)] = lang.id;
        }
    }

    // ============================================================
    // 17. Plain Text (默认)
    // ============================================================
    {
        LanguageInfo lang;
        lang.id = "text";
        lang.name = "Plain Text";
        lang.extensions = {".txt", ".log", ".csv", ".tsv"};
        lang.mimeTypes = {"text/plain"};

        lang.lineComment = "";
        lang.blockCommentStart = "";
        lang.blockCommentEnd = "";

        lang.keywords = {};
        lang.controlFlow = {};
        lang.storageTypes = {};
        lang.dataTypes = {};
        lang.builtinFuncs = {};
        lang.preprocessor = {};
        lang.constants = {};

        lang.stringDelimiter = "";
        lang.charDelimiter = "";
        lang.caseSensitive = false;
        lang.hasPreprocessor = false;
        lang.indentStyle = "spaces";
        lang.indentSize = 4;

        m_languages[lang.id] = lang;
        for (const QString& ext : lang.extensions) {
            m_extensionMap[normalizeExtension(ext)] = lang.id;
        }
    }

    // ============================================================
    // 特殊文件名映射（不依赖扩展名）
    // ============================================================
    m_fileNameMap["Makefile"] = "text";        // 没有很好的 Makefile 语言支持
    m_fileNameMap["CMakeLists.txt"] = "text";
    m_fileNameMap["Dockerfile"] = "text";
    m_fileNameMap[".gitignore"] = "text";
    m_fileNameMap[".env"] = "text";
    m_fileNameMap["README"] = "markdown";
    m_fileNameMap["LICENSE"] = "text";

    qDebug() << "[LanguageDetector] 已加载" << m_languages.size() << "种语言定义";
}

// ============================================================
// 根据文件路径检测语言
// ============================================================
LanguageInfo LanguageDetector::detectLanguage(const QString& filePath) const
{
    if (filePath.isEmpty()) {
        return getLanguage("text");
    }

    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();
    QString extension = fileInfo.suffix();

    // 1. 优先检查特殊文件名
    auto fileNameIt = m_fileNameMap.find(fileName);
    if (fileNameIt != m_fileNameMap.end()) {
        return getLanguage(fileNameIt.value());
    }

    // 2. 检查扩展名（如果有点号，规范化处理）
    if (!extension.isEmpty()) {
        // v8.5：.ts 扩展名歧义处理
        //   Qt Linguist 翻译源文件用 .ts（XML 格式），与 TypeScript 冲突。
        //   读文件头区分：
        //     以 <?xml 开头 Qt 翻译文件（xml 语言，不触发 TS LSP）
        //     否则         正常扩展名映射（typescript）
        if (extension.compare(QLatin1String("ts"), Qt::CaseInsensitive) == 0
            && isQtTranslationFile(filePath)) {
            return getLanguage(QLatin1String("xml"));
        }

        LanguageInfo lang = detectLanguageByExtension(extension);
        if (lang.id != "text") {
            return lang;
        }
    }

    // 3. 如果文件存在且可读，尝试读取第一行检测 Shebang
    if (fileInfo.exists() && fileInfo.isReadable()) {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString firstLine = QString::fromUtf8(file.readLine());
            file.close();
            LanguageInfo lang = detectLanguageByContent(firstLine);
            if (lang.id != "text") {
                return lang;
            }
        }
    }

    // 4. 返回默认语言
    return getLanguage("text");
}

// ============================================================
// 根据扩展名检测语言
// ============================================================
LanguageInfo LanguageDetector::detectLanguageByExtension(const QString& extension) const
{
    QString normalized = normalizeExtension(extension);

    auto it = m_extensionMap.find(normalized);
    if (it != m_extensionMap.end()) {
        return getLanguage(it.value());
    }

    return getLanguage("text");
}

// ============================================================
// 根据内容检测语言（Shebang 行）
// ============================================================
LanguageInfo LanguageDetector::detectLanguageByContent(const QString& content) const
{
    if (content.isEmpty()) {
        return getLanguage("text");
    }

    // 只检查第一行（Shebang）
    QString firstLine = content.trimmed();
    if (firstLine.startsWith("#!")) {
        QString shebang = firstLine.mid(2).trimmed();

        // 检查 Shebang 映射
        for (auto it = m_shebangMap.constBegin(); it != m_shebangMap.constEnd(); ++it) {
            if (shebang.contains(it.key())) {
                return getLanguage(it.value());
            }
        }

        // 处理特殊情况：/usr/bin/env python
        QRegularExpression envRegex(R"(/usr/bin/env\s+(\w+))");
        QRegularExpressionMatch match = envRegex.match(shebang);
        if (match.hasMatch()) {
            QString interpreter = match.captured(1);
            for (auto it = m_shebangMap.constBegin(); it != m_shebangMap.constEnd(); ++it) {
                if (interpreter.contains(it.key()) || it.key().contains(interpreter)) {
                    return getLanguage(it.value());
                }
            }
        }
    }

    return getLanguage("text");
}

// ============================================================
// 根据文件名检测语言（特殊文件）
// ============================================================
LanguageInfo LanguageDetector::detectLanguageByFileName(const QString& fileName) const
{
    if (fileName.isEmpty()) {
        return getLanguage("text");
    }

    // 检查特殊文件名映射
    auto it = m_fileNameMap.find(fileName);
    if (it != m_fileNameMap.end()) {
        return getLanguage(it.value());
    }

    // 否则根据扩展名检测
    QFileInfo info(fileName);
    return detectLanguageByExtension(info.suffix());
}

// ============================================================
// 获取所有支持的语言列表
// ============================================================
QStringList LanguageDetector::availableLanguages() const
{
    return m_languages.keys();
}

// ============================================================
// 根据语言 ID 获取语言信息
// ============================================================
LanguageInfo LanguageDetector::getLanguage(const QString& languageId) const
{
    auto it = m_languages.find(languageId);
    if (it != m_languages.end()) {
        return it.value();
    }

    // 返回默认语言（Plain Text）
    return m_languages.value("text");
}

// ============================================================
// 判断是否支持某种语言
// ============================================================
bool LanguageDetector::supportsLanguage(const QString& languageId) const
{
    return m_languages.contains(languageId);
}

// ============================================================
// 获取语言的显示名称
// ============================================================
QString LanguageDetector::getLanguageDisplayName(const QString& languageId) const
{
    LanguageInfo lang = getLanguage(languageId);
    return lang.name;
}

// ============================================================
// 获取语言的注释符号
// ============================================================
bool LanguageDetector::getCommentSymbols(const QString& languageId,
                                         QString& lineComment,
                                         QString& blockStart,
                                         QString& blockEnd) const
{
    LanguageInfo lang = getLanguage(languageId);
    if (lang.id == "text") {
        return false;
    }

    lineComment = lang.lineComment;
    blockStart = lang.blockCommentStart;
    blockEnd = lang.blockCommentEnd;
    return true;
}

// ============================================================
// 判断是否支持多行注释
// ============================================================
bool LanguageDetector::supportsBlockComment(const QString& languageId) const
{
    LanguageInfo lang = getLanguage(languageId);
    return !lang.blockCommentStart.isEmpty() && !lang.blockCommentEnd.isEmpty();
}

// ============================================================
// 获取关键字列表
// ============================================================
QStringList LanguageDetector::getKeywords(const QString& languageId) const
{
    LanguageInfo lang = getLanguage(languageId);
    return lang.keywords;
}

// ============================================================
// 手动注册或覆盖语言规则
// ============================================================
void LanguageDetector::registerLanguage(const LanguageInfo& info)
{
    if (info.id.isEmpty()) {
        qWarning() << "[LanguageDetector] 无法注册语言：ID 为空";
        return;
    }

    // 如果语言已存在，先清理旧的扩展名映射
    if (m_languages.contains(info.id)) {
        LanguageInfo oldInfo = m_languages[info.id];
        for (const QString& ext : oldInfo.extensions) {
            m_extensionMap.remove(normalizeExtension(ext));
        }
    }

    // 注册语言
    m_languages[info.id] = info;
    for (const QString& ext : info.extensions) {
        m_extensionMap[normalizeExtension(ext)] = info.id;
    }

    qDebug() << "[LanguageDetector] 已注册语言:" << info.id << "(" << info.name << ")";
}

// ============================================================
// 重新加载内置语言定义
// ============================================================
void LanguageDetector::reloadBuiltinLanguages()
{
    loadBuiltinLanguages();
    qDebug() << "[LanguageDetector] 已重新加载内置语言定义";
}

// ============================================================
// 规范化扩展名
// ============================================================
QString LanguageDetector::normalizeExtension(const QString& extension) const
{
    QString normalized = extension.trimmed();

    // 移除开头的点号（如果有）
    if (normalized.startsWith('.')) {
        normalized = normalized.mid(1);
    }

    // 转换为小写（忽略大小写）
    return normalized.toLower();
}

// ============================================================
// 判断 .ts 文件是否为 Qt Linguist 翻译源文件
// ============================================================
bool LanguageDetector::isQtTranslationFile(const QString& filePath) const
{
    QFileInfo fi(filePath);
    if (!fi.exists() || !fi.isReadable()) {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    // 前 256 字节足够判断（XML 声明固定在前几十字节）
    const QByteArray head = file.read(256);
    file.close();

    // 跳过 UTF-8 BOM
    int i = 0;
    if (head.size() >= 3
        && static_cast<unsigned char>(head[0]) == 0xEF
        && static_cast<unsigned char>(head[1]) == 0xBB
        && static_cast<unsigned char>(head[2]) == 0xBF) {
        i = 3;
    }

    // 跳过前导空白
    while (i < head.size()) {
        const char c = head.at(i);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            ++i;
        } else {
            break;
        }
    }

    // TypeScript 代码不会以 XML 声明开头（注释是 // 或 /*）。
    // 只要以 <?xml 开头 视为 Qt Linguist 翻译源文件。
    return head.mid(i, 5) == QByteArray("<?xml");
}

} // namespace SprintIDE
