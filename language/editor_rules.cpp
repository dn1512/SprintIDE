#include "language/editor_rules.h"

namespace SprintIDE {

// ============================================================
// C / C++ / Objective-C
// ============================================================
static EditorRules makeCppLike()
{
    EditorRules r;
    r.lineComment        = "//";
    r.blockCommentStart  = "/*";
    r.blockCommentEnd    = "*/";

    r.indentTriggers     = {"{"};
    r.dedentTriggers     = {"}"};
    r.dedentLineStarters = {};           // C 语言不做输入时反缩进

    r.autoPairs = {
        {'(', ')'}, {'[', ']'}, {'{', '}'},
        {'"', '"'}, {'\'', '\''}
    };
    r.autoBraceOnNewline = true;
    return r;
}

// ============================================================
// Python
// ============================================================
static EditorRules makePython()
{
    EditorRules r;
    r.lineComment        = "#";
    r.blockCommentStart  = "\"\"\"";     // 实际上三引号是 docstring，但注释快捷键用它也合理
    r.blockCommentEnd    = "\"\"\"";

    r.indentTriggers     = {":"};        // if x: / def f(): / for i in ...:
    r.dedentTriggers     = {};           // Python 没有显式闭合符
    r.dedentLineStarters = {"else", "elif", "except", "finally", "case"};

    r.autoPairs = {
        {'(', ')'}, {'[', ']'}, {'{', '}'},
        {'"', '"'}, {'\'', '\''}
    };
    r.autoBraceOnNewline = false;        // ←←← 关键：禁止大括号换行
    return r;
}

// ============================================================
// JavaScript / TypeScript
// ============================================================
static EditorRules makeJavaScript()
{
    EditorRules r = makeCppLike();
    // 基本同 C++，但换行风格上 JS 更常用 } 后跟 else 在同一行
    // 暂时不加特殊处理
    return r;
}

// ============================================================
// Java / C#
// ============================================================
static EditorRules makeJavaLike()
{
    return makeCppLike();
}

// ============================================================
// Go
// ============================================================
static EditorRules makeGo()
{
    EditorRules r;
    r.lineComment        = "//";
    r.blockCommentStart  = "/*";
    r.blockCommentEnd    = "*/";

    r.indentTriggers     = {"{"};
    r.dedentTriggers     = {"}"};
    r.dedentLineStarters = {};

    r.autoPairs = {
        {'(', ')'}, {'[', ']'}, {'{', '}'},
        {'"', '"'}, {'`', '`'}
    };
    r.autoBraceOnNewline = true;
    return r;
}

// ============================================================
// Rust
// ============================================================
static EditorRules makeRust()
{
    return makeCppLike();
}

// ============================================================
// Ruby
// ============================================================
static EditorRules makeRuby()
{
    EditorRules r;
    r.lineComment        = "#";
    r.blockCommentStart  = "=begin";
    r.blockCommentEnd    = "=end";

    r.indentTriggers     = {
        "do", "then", "def", "class", "module",
        "if", "unless", "case", "begin", "while", "until", "for"
    };
    r.dedentTriggers     = {"end"};
    r.dedentLineStarters = {"else", "elsif", "when", "rescue", "ensure"};

    r.autoPairs = {
        {'(', ')'}, {'[', ']'}, {'{', '}'},
        {'"', '"'}, {'\'', '\''}
    };
    r.autoBraceOnNewline = false;
    return r;
}

// ============================================================
// Shell
// ============================================================
static EditorRules makeShell()
{
    EditorRules r;
    r.lineComment        = "#";
    r.blockCommentStart  = "";
    r.blockCommentEnd    = "";

    r.indentTriggers     = {"then", "do", "{", "("};
    r.dedentTriggers     = {"fi", "done", "esac", "}", ")"};
    r.dedentLineStarters = {"else", "elif", "fi", "done", "esac"};

    r.autoPairs = {
        {'(', ')'}, {'[', ']'}, {'{', '}'},
        {'"', '"'}, {'\'', '\''}, {'`', '`'}
    };
    r.autoBraceOnNewline = false;
    return r;
}

// ============================================================
// PHP
// ============================================================
static EditorRules makePhp()
{
    EditorRules r = makeCppLike();
    r.lineComment = "//";     // PHP 也支持 #，但 // 更通用
    return r;
}

// ============================================================
// HTML / XML
// ============================================================
static EditorRules makeMarkup()
{
    EditorRules r;
    r.lineComment        = "";
    r.blockCommentStart  = "<!--";
    r.blockCommentEnd    = "-->";

    r.indentTriggers     = {};
    r.dedentTriggers     = {};
    r.dedentLineStarters = {};

    r.autoPairs = {
        {'(', ')'}, {'[', ']'},
        {'"', '"'}, {'\'', '\''}
        // 注意：HTML 里不自动配对 < 和 >，因为会干扰自闭合标签
    };
    r.autoBraceOnNewline = false;
    return r;
}

// ============================================================
// CSS / SCSS / LESS
// ============================================================
static EditorRules makeCss()
{
    EditorRules r;
    r.lineComment        = "";
    r.blockCommentStart  = "/*";
    r.blockCommentEnd    = "*/";

    r.indentTriggers     = {"{"};
    r.dedentTriggers     = {"}"};
    r.dedentLineStarters = {};

    r.autoPairs = {
        {'(', ')'}, {'[', ']'}, {'{', '}'},
        {'"', '"'}, {'\'', '\''}
    };
    r.autoBraceOnNewline = true;
    return r;
}

// ============================================================
// JSON
// ============================================================
static EditorRules makeJson()
{
    EditorRules r;
    r.lineComment        = "";
    r.blockCommentStart  = "/*";
    r.blockCommentEnd    = "*/";

    r.indentTriggers     = {"{", "["};
    r.dedentTriggers     = {"}", "]"};
    r.dedentLineStarters = {};

    r.autoPairs = {
        {'(', ')'}, {'[', ']'}, {'{', '}'},
        {'"', '"'}
    };
    r.autoBraceOnNewline = true;
    return r;
}

// ============================================================
// YAML
// ============================================================
static EditorRules makeYaml()
{
    EditorRules r;
    r.lineComment        = "#";
    r.blockCommentStart  = "";
    r.blockCommentEnd    = "";

    r.indentTriggers     = {":"};
    r.dedentTriggers     = {};
    r.dedentLineStarters = {};

    r.autoPairs = {
        {'(', ')'}, {'[', ']'}, {'{', '}'},
        {'"', '"'}, {'\'', '\''}
    };
    r.autoBraceOnNewline = false;
    return r;
}

// ============================================================
// Properties
// ============================================================
static EditorRules makeProperties()
{
    EditorRules r;
    r.lineComment        = "#";
    r.blockCommentStart  = "";
    r.blockCommentEnd    = "";

    r.indentTriggers     = {};
    r.dedentTriggers     = {};
    r.dedentLineStarters = {};

    r.autoPairs = {
        {'(', ')'}, {'[', ']'}, {'{', '}'},
        {'"', '"'}
    };
    r.autoBraceOnNewline = false;
    return r;
}

// ============================================================
// Markdown / Plain Text：关闭所有自动行为
// ============================================================
static EditorRules makePlain()
{
    EditorRules r;
    r.lineComment        = "";
    r.blockCommentStart  = "";
    r.blockCommentEnd    = "";

    r.indentTriggers     = {};
    r.dedentTriggers     = {};
    r.dedentLineStarters = {};

    r.autoPairs          = {};           // 纯文本不做任何自动配对
    r.autoBraceOnNewline = false;
    return r;
}

// ============================================================
// 工厂
// ============================================================
EditorRules EditorRules::forLanguage(const QString& id)
{
    // ---- C / C++ 系列 ----
    if (id == "cpp" || id == "c")              return makeCppLike();

    // ---- 脚本语言 ----
    if (id == "python")                        return makePython();
    if (id == "ruby")                          return makeRuby();
    if (id == "shell")                         return makeShell();
    if (id == "php")                           return makePhp();

    // ---- 编译型 + 类 C 语法 ----
    if (id == "java")                          return makeJavaLike();
    if (id == "csharp")                        return makeJavaLike();
    if (id == "go")                            return makeGo();
    if (id == "rust")                          return makeRust();
    if (id == "javascript" || id == "typescript") return makeJavaScript();

    // ---- 标记类 ----
    if (id == "html" || id == "xml")           return makeMarkup();
    if (id == "css")                           return makeCss();

    // ---- 数据/配置 ----
    if (id == "json")                          return makeJson();
    if (id == "yaml")                          return makeYaml();
    if (id == "properties")                    return makeProperties();

    // ---- 默认：纯文本，关闭所有自动行为 ----
    if (id == "markdown" || id == "text")      return makePlain();

    // ---- 未知语言 → 保守策略 ----
    return makePlain();
}

// ============================================================
// 便捷判断
// ============================================================
bool EditorRules::endsWithIndentTrigger(const QString& trimmedLine) const
{
    if (trimmedLine.isEmpty()) return false;
    for (const QString& t : indentTriggers) {
        if (trimmedLine.endsWith(t)) return true;
    }
    return false;
}

bool EditorRules::startsWithDedentTrigger(const QString& trimmedLine) const
{
    if (trimmedLine.isEmpty()) return false;
    for (const QString& t : dedentTriggers) {
        if (trimmedLine.startsWith(t)) return true;
    }
    return false;
}

bool EditorRules::lineStartsWithDedentKeyword(const QString& trimmedLine) const
{
    if (trimmedLine.isEmpty()) return false;
    for (const QString& kw : dedentLineStarters) {
        // 要求后面跟空格、冒号或结束，避免 "elsewhere" 被误判为 "else"
        if (trimmedLine == kw) return true;
        if (trimmedLine.startsWith(kw)) {
            const QChar next = trimmedLine.at(kw.length());
            if (next.isSpace() || next == ':') return true;
        }
    }
    return false;
}

} // namespace SprintIDE
