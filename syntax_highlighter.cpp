#include "syntax_highlighter.h"
#include <QDebug>

namespace SprintIDE {

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent),
    m_isDarkTheme(true)
{
    // 初始化高亮规则
    initHighlightRules();
    initMultiLineCommentRules();

    // 默认使用浅色主题
    setTheme(true);
}

void SyntaxHighlighter::setTheme(bool isDarkTheme)
{
    m_isDarkTheme = isDarkTheme;

    // 根据主题设置颜色
    if (isDarkTheme) {
        // 深色主题颜色（VS Code风格）

        // 一般关键字 - 蓝色
        m_keywordFormat.setForeground(QColor(135, 206, 250));
        m_keywordFormat.setFontWeight(QFont::Normal);

        // 控制流关键字 - 粉色
        m_controlFlowFormat.setForeground(QColor(197, 134, 192));
        m_controlFlowFormat.setFontWeight(QFont::Bold);

        // 存储类关键字 - 浅蓝色
        m_storageClassFormat.setForeground(QColor(79, 193, 255));
        m_storageClassFormat.setFontWeight(QFont::Normal);

        // 数据类型 - 青色
        m_typeFormat.setForeground(QColor(78, 201, 176));
        m_typeFormat.setFontWeight(QFont::Normal);

        // 标准库类型 - 浅青色
        m_stdTypeFormat.setForeground(QColor(78, 201, 176));
        m_stdTypeFormat.setFontWeight(QFont::Normal);
        m_stdTypeFormat.setFontItalic(true);  // 斜体区分

        // 用户函数 - 黄色
        m_functionFormat.setForeground(QColor(220, 220, 170));
        m_functionFormat.setFontWeight(QFont::Normal);

        // 预定义函数 - 浅黄色
        m_predefinedFunctionFormat.setForeground(QColor(220, 220, 170));
        m_predefinedFunctionFormat.setFontWeight(QFont::Normal);

        // 注释 - 绿色
        m_commentFormat.setForeground(QColor(106, 153, 85));
        m_commentFormat.setFontItalic(true);

        // 字符串 - 橙色
        m_stringFormat.setForeground(QColor(206, 145, 120));

        // 数字 - 浅绿色
        m_numberFormat.setForeground(QColor(181, 206, 168));

        // 预处理器 - 紫色
        m_preprocessorFormat.setForeground(QColor(200, 120, 150));
        m_preprocessorFormat.setFontWeight(QFont::Normal);

        // 多行注释 - 绿色
        m_multiLineCommentFormat.setForeground(QColor(106, 153, 85));
        m_multiLineCommentFormat.setFontItalic(true);

    } else {
        // 浅色主题颜色

        // 一般关键字 - 深蓝色
        m_keywordFormat.setForeground(QColor(135, 206, 250));
        m_keywordFormat.setFontWeight(QFont::Bold);

        // 控制流关键字 - 深紫色
        m_controlFlowFormat.setForeground(QColor(221, 32, 221));
        m_controlFlowFormat.setFontWeight(QFont::Bold);

        // 存储类关键字 - 蓝色
        m_storageClassFormat.setForeground(QColor(0, 0, 255));
        m_storageClassFormat.setFontWeight(QFont::Normal);

        // 数据类型 - 深青色
        m_typeFormat.setForeground(QColor(43, 145, 175));
        m_typeFormat.setFontWeight(QFont::Bold);

        // 标准库类型 - 深青色斜体
        m_stdTypeFormat.setForeground(QColor(43, 145, 175));
        m_stdTypeFormat.setFontWeight(QFont::Normal);
        m_stdTypeFormat.setFontItalic(true);

        // 用户函数 - 紫色
        m_functionFormat.setForeground(QColor(120, 0, 120));
        m_functionFormat.setFontWeight(QFont::Normal);

        // 预定义函数 - 深紫色
        m_predefinedFunctionFormat.setForeground(QColor(120, 0, 120));
        m_predefinedFunctionFormat.setFontWeight(QFont::Normal);

        // 注释 - 深绿色
        m_commentFormat.setForeground(QColor(0, 128, 0));
        m_commentFormat.setFontItalic(true);

        // 字符串 - 棕色
        m_stringFormat.setForeground(QColor(163, 21, 21));

        // 数字 - 深绿色
        m_numberFormat.setForeground(QColor(9, 134, 88));

        // 预处理器 - 紫色
        m_preprocessorFormat.setForeground(QColor(128, 0, 128));
        m_preprocessorFormat.setFontWeight(QFont::Bold);

        // 多行注释 - 深绿色
        m_multiLineCommentFormat.setForeground(QColor(0, 128, 0));
        m_multiLineCommentFormat.setFontItalic(true);
    }

    // 重新初始化高亮规则
    m_highlightRules.clear();
    initHighlightRules();

    // 重新高亮所有文本
    rehighlight();
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    // 先处理多行注释
    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1) {
        startIndex = text.indexOf(m_commentStartExpression);
    }

    while (startIndex >= 0) {
        QRegularExpressionMatch match = m_commentEndExpression.match(text, startIndex);
        int endIndex = match.capturedStart();
        int commentLength = 0;

        if (endIndex == -1) {
            // 注释跨越多行
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            // 注释在本行结束
            commentLength = endIndex - startIndex + match.capturedLength();
        }

        setFormat(startIndex, commentLength, m_multiLineCommentFormat);
        startIndex = text.indexOf(m_commentStartExpression, startIndex + commentLength);
    }

    // 应用单行高亮规则
    for (const HighlightRule &rule : m_highlightRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();

            // 检查匹配的文本是否已经在注释或字符串中
            bool isAlreadyFormatted = false;
            QTextCharFormat existingFormat = format(match.capturedStart());

            // 如果已经设置了格式（比如注释或字符串），跳过
            if (existingFormat != QTextCharFormat()) {
                isAlreadyFormatted = true;
            }

            if (!isAlreadyFormatted) {
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }
    }

    // 特别处理函数声明，确保函数名正确高亮
    highlightFunctionDeclarations(text);
}

void SyntaxHighlighter::highlightFunctionDeclarations(const QString &text)
{
    // 正则表达式匹配函数声明模式
    // 例如: void functionName( 或 int functionName(
    QRegularExpression functionPattern("\\b([a-zA-Z_][a-zA-Z0-9_]*\\s+)+([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\(");
    QRegularExpressionMatchIterator matchIterator = functionPattern.globalMatch(text);

    while (matchIterator.hasNext()) {
        QRegularExpressionMatch match = matchIterator.next();

        // 获取整个匹配
        QString fullMatch = match.captured(0);
        QString functionName = match.captured(2);  // 第二个捕获组是函数名

        if (!functionName.isEmpty()) {
            // 计算函数名的位置
            int functionNameStart = text.indexOf(functionName, match.capturedStart());

            // 检查函数名是否已经被高亮（比如作为关键字）
            QTextCharFormat existingFormat = format(functionNameStart);

            // 如果函数名还没有被高亮，或者当前格式是关键字格式
            if (existingFormat == QTextCharFormat() ||
                existingFormat == m_keywordFormat ||
                existingFormat == m_typeFormat ||
                existingFormat == m_stdTypeFormat) {

                // 应用函数名高亮
                setFormat(functionNameStart, functionName.length(), m_functionFormat);
            }
        }
    }
}

void SyntaxHighlighter::initHighlightRules()
{
    // 清空现有规则
    m_highlightRules.clear();

    // ========== 初始化C++关键字分类 ==========

    // 控制流关键字
    m_controlFlowKeywords = {
        "if", "else", "for", "while", "do", "switch", "case", "default",
        "break", "continue", "return", "goto", "try", "catch", "throw", "finally"
    };

    // 存储类关键字
    m_storageClassKeywords = {
        "auto", "register", "static", "extern", "mutable", "thread_local",
        "const", "volatile", "constexpr", "consteval", "constinit"
    };

    // 一般C++关键字（排除已分类的）
    m_keywords = {
        "alignas", "alignof", "and", "and_eq", "asm", "bitand", "bitor",
        "bool", "char", "char8_t", "char16_t", "char32_t", "class", "compl",
        "concept", "const_cast", "co_await", "co_return", "co_yield", "decltype",
        "delete", "dynamic_cast", "enum", "explicit", "export", "false", "friend",
        "inline", "namespace", "new", "noexcept", "not", "not_eq", "nullptr",
        "operator", "or", "or_eq", "private", "protected", "public",
        "reinterpret_cast", "requires", "sizeof", "static_assert", "static_cast",
        "struct", "template", "this", "true", "typedef", "typeid", "typename",
        "union", "using", "virtual", "wchar_t", "xor", "xor_eq"
    };

    // C++基本数据类型
    m_types = {
        "void", "bool", "char", "double", "float", "int", "long", "short",
        "signed", "unsigned", "wchar_t", "char8_t", "char16_t", "char32_t"
    };

    // C++标准库类型
    m_stdTypes = {
        "string", "vector", "list", "map", "set", "unordered_map", "unordered_set",
        "array", "deque", "queue", "stack", "pair", "tuple", "optional",
        "variant", "any", "function", "shared_ptr", "unique_ptr", "weak_ptr",
        "size_t", "int8_t", "int16_t", "int32_t", "int64_t", "uint8_t", "uint16_t",
        "uint32_t", "uint64_t", "intptr_t", "uintptr_t", "ptrdiff_t", "nullptr_t"
    };

    // C++预定义函数（标准库函数）
    m_predefinedFunctions = {
        "main", "printf", "scanf", "cout", "cin", "endl", "malloc", "free",
        "new", "delete", "strlen", "strcpy", "strcat", "strcmp", "memset",
        "memcpy", "qsort", "bsearch", "abs", "fabs", "sqrt", "pow", "sin",
        "cos", "tan", "log", "exp", "time", "rand", "srand", "exit", "atoi",
        "atof", "itoa", "getline", "find", "push_back", "pop_back", "begin",
        "end", "size", "empty", "clear", "insert", "erase", "reserve", "capacity",
        "front", "back", "at", "operator[]", "c_str", "data", "length", "compare",
        "append", "substr", "find", "rfind", "replace", "copy", "swap"
    };

    // 预处理器关键字
    m_preprocessorKeywords = {
        "include", "define", "undef", "if", "ifdef", "ifndef", "else", "elif",
        "endif", "line", "error", "pragma", "import", "using", "warning", "region", "endregion"
    };

    // ========== 设置高亮规则（按优先级从高到低） ==========

    // 1. 多行注释（由highlightBlock处理）

    // 2. 单行注释
    {
        HighlightRule rule;
        rule.pattern = QRegularExpression("//[^\n]*");
        rule.format = m_commentFormat;
        m_highlightRules.append(rule);
    }

    // 3. 字符串（双引号）
    {
        HighlightRule rule;
        rule.pattern = QRegularExpression(R"("(?:\\.|[^"\\])*")");
        rule.format = m_stringFormat;
        m_highlightRules.append(rule);
    }

    // 4. 字符（单引号）
    {
        HighlightRule rule;
        rule.pattern = QRegularExpression(R"('(?:\\.|[^'\\])')");
        rule.format = m_stringFormat;
        m_highlightRules.append(rule);
    }

    // 5. 数字规则
    {
        // 整数、浮点数、科学计数法
        HighlightRule rule;
        rule.pattern = QRegularExpression("\\b\\d+(\\.\\d+)?([eE][+-]?\\d+)?\\b");
        rule.format = m_numberFormat;
        m_highlightRules.append(rule);

        // 十六进制
        HighlightRule hexRule;
        hexRule.pattern = QRegularExpression("\\b0[xX][0-9a-fA-F]+\\b");
        hexRule.format = m_numberFormat;
        m_highlightRules.append(hexRule);

        // 二进制
        HighlightRule binRule;
        binRule.pattern = QRegularExpression("\\b0[bB][01]+\\b");
        binRule.format = m_numberFormat;
        m_highlightRules.append(binRule);
    }

    // 6. 预处理器规则
    {
        // 先匹配所有以#开头的行
        HighlightRule rule;
        rule.pattern = QRegularExpression("#.*");
        rule.format = m_preprocessorFormat;
        m_highlightRules.append(rule);

        // 然后为特定的预处理器指令设置更具体的格式
        for (const QString &keyword : m_preprocessorKeywords) {
            HighlightRule specificRule;
            specificRule.pattern = QRegularExpression("#\\s*" + keyword + "\\b");
            specificRule.format = m_preprocessorFormat;
            specificRule.format.setFontWeight(QFont::Bold);
            m_highlightRules.append(specificRule);
        }
    }

    // 7. 标准库类型（斜体）
    for (const QString &type : m_stdTypes) {
        HighlightRule rule;
        rule.pattern = QRegularExpression("\\b" + type + "\\b");
        rule.format = m_stdTypeFormat;
        m_highlightRules.append(rule);
    }

    // 8. 基本数据类型
    for (const QString &type : m_types) {
        HighlightRule rule;
        rule.pattern = QRegularExpression("\\b" + type + "\\b");
        rule.format = m_typeFormat;
        m_highlightRules.append(rule);
    }

    // 9. 存储类关键字
    for (const QString &keyword : m_storageClassKeywords) {
        HighlightRule rule;
        rule.pattern = QRegularExpression("\\b" + keyword + "\\b");
        rule.format = m_storageClassFormat;
        m_highlightRules.append(rule);
    }

    // 10. 控制流关键字
    for (const QString &keyword : m_controlFlowKeywords) {
        HighlightRule rule;
        rule.pattern = QRegularExpression("\\b" + keyword + "\\b");
        rule.format = m_controlFlowFormat;
        m_highlightRules.append(rule);
    }

    // 11. 一般关键字（包括using, namespace等）
    for (const QString &keyword : m_keywords) {
        HighlightRule rule;
        rule.pattern = QRegularExpression("\\b" + keyword + "\\b");
        rule.format = m_keywordFormat;
        m_highlightRules.append(rule);
    }

    // 12. 预定义函数（标准库函数）
    for (const QString &func : m_predefinedFunctions) {
        HighlightRule rule;
        rule.pattern = QRegularExpression("\\b" + func + "\\b");
        rule.format = m_predefinedFunctionFormat;
        m_highlightRules.append(rule);
    }

    // 13. 类/结构体/枚举定义
    {
        HighlightRule rule;
        // 匹配 class MyClass 或 struct MyStruct
        rule.pattern = QRegularExpression("\\b(class|struct|enum)\\s+[A-Za-z_][A-Za-z0-9_]*");
        rule.format = m_typeFormat;
        rule.format.setFontWeight(QFont::Bold);
        m_highlightRules.append(rule);
    }

    // 14. 函数定义（用户定义的函数名）
    {
        HighlightRule rule;
        // 匹配返回类型 函数名(参数)
        // 这个规则用来高亮用户定义的函数名
        rule.pattern = QRegularExpression("\\b[a-zA-Z_][a-zA-Z0-9_]*\\s+[a-zA-Z_][a-zA-Z0-9_]*\\s*\\(");
        rule.format = m_functionFormat;
        m_highlightRules.append(rule);
    }

    // 15. 宏定义
    {
        HighlightRule rule;
        rule.pattern = QRegularExpression("\\b[A-Z_][A-Z0-9_]*\\b");
        rule.format = m_preprocessorFormat;
        rule.format.setFontWeight(QFont::Normal);
        m_highlightRules.append(rule);
    }

    qDebug() << "语法高亮规则初始化完成，共" << m_highlightRules.size() << "条规则";
}

void SyntaxHighlighter::initMultiLineCommentRules()
{
    // 多行注释开始标记：/*
    m_commentStartExpression = QRegularExpression("/\\*");

    // 多行注释结束标记：*/
    m_commentEndExpression = QRegularExpression("\\*/");

    // 设置多行注释格式
    m_multiLineCommentFormat.setForeground(QColor(0, 128, 0)); // 绿色
}
}
