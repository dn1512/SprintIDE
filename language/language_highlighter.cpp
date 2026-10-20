#include "language/language_highlighter.h"

#include <QDebug>
#include <QTextDocument>
#include <QRegularExpression>
#include <QVector>


namespace SprintIDE {

// ============================================================
// 构造函数
// ============================================================
LanguageHighlighter::LanguageHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
    , m_isDark(true)
{
    // 初始化默认颜色方案（深色）
    initDefaultColors(true);

    // 设置默认语言（Plain Text）
    setLanguage("text");

    qDebug() << "[LanguageHighlighter] 已创建，默认语言: text";
}

// ============================================================
// 析构函数
// ============================================================
LanguageHighlighter::~LanguageHighlighter()
{
    qDebug() << "[LanguageHighlighter] 已销毁";
}

// ============================================================
// 根据文件路径设置语言
// ============================================================
void LanguageHighlighter::setLanguageByFilePath(const QString& filePath)
{
    LanguageInfo lang = LanguageDetector::instance().detectLanguage(filePath);
    setLanguage(lang.id);
}

// ============================================================
// 根据语言 ID 设置语言
// ============================================================
void LanguageHighlighter::setLanguage(const QString& languageId)
{
    LanguageInfo lang = LanguageDetector::instance().getLanguage(languageId);

    if (lang.id == m_currentLanguage.id && !m_currentLanguage.id.isEmpty()) {
        // 语言未变，无需更新
        return;
    }

    m_currentLanguage = lang;

    // 更新高亮规则
    updateRules(lang);

    // 重新高亮整个文档
    QSyntaxHighlighter::rehighlight();

    // 发送信号
    emit languageChanged(lang.id, lang.name);

    qDebug() << "[LanguageHighlighter] 切换到语言:" << lang.id << "(" << lang.name << ")";
}

// ============================================================
// 获取当前语言信息
// ============================================================
LanguageInfo LanguageHighlighter::currentLanguage() const
{
    return m_currentLanguage;
}

// ============================================================
// 获取当前语言 ID
// ============================================================
QString LanguageHighlighter::currentLanguageId() const
{
    return m_currentLanguage.id;
}

// ============================================================
// 设置主题
// ============================================================
void LanguageHighlighter::setTheme(bool isDark)
{
    if (m_isDark == isDark) {
        return;
    }

    m_isDark = isDark;

    // 更新颜色方案
    initDefaultColors(isDark);

    // 重新生成规则（应用新颜色）
    updateRules(m_currentLanguage);

    // 重新高亮整个文档
    rehighlightDocument();

    qDebug() << "[LanguageHighlighter] 切换到" << (isDark ? "深色" : "浅色") << "主题";
}

// ============================================================
// 检查是否为深色主题
// ============================================================
bool LanguageHighlighter::isDarkTheme() const
{
    return m_isDark;
}

// ============================================================
// 获取语言显示名称
// ============================================================
QString LanguageHighlighter::languageDisplayName() const
{
    return m_currentLanguage.name;
}

void LanguageHighlighter::rehighlightDocument()
{
    QSyntaxHighlighter::rehighlight();
}

// ============================================================
// 初始化默认颜色方案
// ============================================================
void LanguageHighlighter::initDefaultColors(bool isDark)
{
    if (isDark) {
        // Polaris Dark Nexus —— 去紫
        m_keywordColor        = QColor("#4AB0E6");  // 存储/类型
        m_controlFlowColor    = QColor("#EF7B7B");  // 控制流
        m_storageTypeColor    = QColor("#4AB0E6");
        m_dataTypeColor       = QColor("#4AB0E6");
        m_builtinFuncColor    = QColor("#F0C57A");
        m_commentColor        = QColor("#5E6D82");
        m_stringColor         = QColor("#86C28C");
        m_charColor           = QColor("#86C28C");
        m_numberColor         = QColor("#D4A06A");
        m_preprocessorColor   = QColor("#9BB5D4");
        m_constantColor       = QColor("#66D4E0");
        m_functionColor       = QColor("#F0C57A");
        m_classColor          = QColor("#4AB0E6");
    } else {
        // Polaris Solar Flare
        m_keywordColor        = QColor("#0D6EFD");
        m_controlFlowColor    = QColor("#C0392B");
        m_storageTypeColor    = QColor("#0D6EFD");
        m_dataTypeColor       = QColor("#0D6EFD");
        m_builtinFuncColor    = QColor("#B8770E");
        m_commentColor        = QColor("#9AAEC2");
        m_stringColor         = QColor("#3E8E4E");
        m_charColor           = QColor("#3E8E4E");
        m_numberColor         = QColor("#A8721E");
        m_preprocessorColor   = QColor("#5E7A96");
        m_constantColor       = QColor("#0E8B8B");
        m_functionColor       = QColor("#B8770E");
        m_classColor          = QColor("#0D6EFD");
    }

    // 其余 createFormat 调用不变，只是颜色变了
    m_keywordFormat           = createFormat(m_keywordColor);
    m_controlFlowFormat       = createFormat(m_controlFlowColor, true);
    m_storageTypeFormat       = createFormat(m_storageTypeColor);
    m_dataTypeFormat          = createFormat(m_dataTypeColor, true);
    m_builtinFuncFormat       = createFormat(m_builtinFuncColor);
    m_commentFormat           = createFormat(m_commentColor, false, true);
    m_multiLineCommentFormat  = createFormat(m_commentColor, false, true);
    m_stringFormat            = createFormat(m_stringColor);
    m_charFormat              = createFormat(m_charColor);
    m_numberFormat            = createFormat(m_numberColor);
    m_preprocessorFormat      = createFormat(m_preprocessorColor);
    m_constantFormat          = createFormat(m_constantColor, true);
    m_functionFormat          = createFormat(m_functionColor);
    m_classFormat             = createFormat(m_classColor, true);
}

// ============================================================
// 创建文本格式
// ============================================================
QTextCharFormat LanguageHighlighter::createFormat(const QColor& color,
                                                  bool bold,
                                                  bool italic) const
{
    QTextCharFormat format;
    format.setForeground(color);
    if (bold) {
        format.setFontWeight(QFont::Bold);
    }
    if (italic) {
        format.setFontItalic(true);
    }
    return format;
}

// ============================================================
// 根据语言信息更新高亮规则
// ============================================================
void LanguageHighlighter::updateRules(const LanguageInfo& info)
{
    // 清空旧规则
    m_rules.clear();

    // 清空多行注释规则
    m_commentStartPattern = QRegularExpression();
    m_commentEndPattern = QRegularExpression();

    // ============================================================
    // 1. 多行注释（必须最先处理，防止被其他规则覆盖）
    // ============================================================
    if (!info.blockCommentStart.isEmpty() && !info.blockCommentEnd.isEmpty()) {
        m_commentStartPattern = QRegularExpression(QRegularExpression::escape(info.blockCommentStart));
        m_commentEndPattern = QRegularExpression(QRegularExpression::escape(info.blockCommentEnd));
    }

    // ============================================================
    // 2. 单行注释
    // ============================================================
    if (!info.lineComment.isEmpty()) {
        QString escaped = QRegularExpression::escape(info.lineComment);
        HighlightRule rule;
        rule.pattern = QRegularExpression(escaped + "[^\n]*");
        rule.format = m_commentFormat;
        rule.priority = 20;  // 高优先级
        m_rules.append(rule);
    }

    // ============================================================
    // 3. 字符串（双引号）
    // ============================================================
    if (!info.stringDelimiter.isEmpty()) {
        QString escaped = QRegularExpression::escape(info.stringDelimiter);
        // 匹配转义字符和字符串内容
        QString pattern = escaped + "(?:\\\\.|[^" + escaped + "\\\\])*" + escaped;
        HighlightRule rule;
        rule.pattern = QRegularExpression(pattern);
        rule.format = m_stringFormat;
        rule.priority = 15;
        m_rules.append(rule);
    }

    // ============================================================
    // 4. 字符（单引号，如果有）
    // ============================================================
    if (!info.charDelimiter.isEmpty() && info.charDelimiter != info.stringDelimiter) {
        QString escaped = QRegularExpression::escape(info.charDelimiter);
        QString pattern = escaped + "(?:\\\\.|[^" + escaped + "\\\\])" + escaped;
        HighlightRule rule;
        rule.pattern = QRegularExpression(pattern);
        rule.format = m_charFormat;
        rule.priority = 15;
        m_rules.append(rule);
    }

    // ============================================================
    // 5. 数字
    // ============================================================
    {
        // 整数、浮点数、科学计数法
        HighlightRule rule;
        rule.pattern = QRegularExpression("\\b\\d+(\\.\\d+)?([eE][+-]?\\d+)?\\b");
        rule.format = m_numberFormat;
        rule.priority = 10;
        m_rules.append(rule);

        // 十六进制
        HighlightRule hexRule;
        hexRule.pattern = QRegularExpression("\\b0[xX][0-9a-fA-F]+\\b");
        hexRule.format = m_numberFormat;
        hexRule.priority = 10;
        m_rules.append(hexRule);

        // 二进制（C++/Python 风格）
        HighlightRule binRule;
        binRule.pattern = QRegularExpression("\\b0[bB][01]+\\b");
        binRule.format = m_numberFormat;
        binRule.priority = 10;
        m_rules.append(binRule);
    }

    // ============================================================
    // 6. 预处理器（如果有）
    // ============================================================
    if (info.hasPreprocessor) {
        HighlightRule rule;
        rule.pattern = QRegularExpression("#[a-zA-Z_][a-zA-Z0-9_]*\\b");
        rule.format = m_preprocessorFormat;
        rule.priority = 12;
        m_rules.append(rule);
    }

    // ============================================================
    // 7. 关键字（按类型分组，不同颜色）
    // ============================================================
    // 7.1 控制流关键字
    if (!info.controlFlow.isEmpty()) {
        QVector<HighlightRule> rules = createKeywordRules(
            info.controlFlow, m_controlFlowFormat, 5);
        m_rules.append(rules);
    }

    // 7.2 存储类型关键字
    if (!info.storageTypes.isEmpty()) {
        QVector<HighlightRule> rules = createKeywordRules(
            info.storageTypes, m_storageTypeFormat, 5);
        m_rules.append(rules);
    }

    // 7.3 数据类型关键字
    if (!info.dataTypes.isEmpty()) {
        QVector<HighlightRule> rules = createKeywordRules(
            info.dataTypes, m_dataTypeFormat, 5);
        m_rules.append(rules);
    }

    // 7.4 内置函数
    if (!info.builtinFuncs.isEmpty()) {
        QVector<HighlightRule> rules = createKeywordRules(
            info.builtinFuncs, m_builtinFuncFormat, 4);
        m_rules.append(rules);
    }

    // 7.5 常量
    if (!info.constants.isEmpty()) {
        QVector<HighlightRule> rules = createKeywordRules(
            info.constants, m_constantFormat, 5);
        m_rules.append(rules);
    }

    // 7.6 一般关键字（最后匹配，优先级最低）
    if (!info.keywords.isEmpty()) {
        // 排除已在其他分类中的关键字
        QStringList excluded;
        excluded << info.controlFlow << info.storageTypes << info.dataTypes
                 << info.builtinFuncs << info.constants;

        QStringList generalKeywords;
        for (const QString& kw : info.keywords) {
            if (!excluded.contains(kw)) {
                generalKeywords.append(kw);
            }
        }

        if (!generalKeywords.isEmpty()) {
            QVector<HighlightRule> rules = createKeywordRules(
                generalKeywords, m_keywordFormat, 3);
            m_rules.append(rules);
        }
    }

    // ============================================================
    // 8. 函数名和类名（通过正则匹配，非关键字）
    // ============================================================
    // 8.1 函数调用: 标识符后跟左括号
    {
        HighlightRule rule;
        rule.pattern = QRegularExpression("\\b([a-zA-Z_][a-zA-Z0-9_]*)(?=\\s*\\()");
        rule.format = m_functionFormat;
        rule.priority = 2;
        m_rules.append(rule);
    }

    // 8.2 类/类型名: 大写字母开头的标识符（启发式）
    {
        HighlightRule rule;
        rule.pattern = QRegularExpression("\\b([A-Z][a-zA-Z0-9_]*)\\b");
        rule.format = m_classFormat;
        rule.priority = 1;
        m_rules.append(rule);
    }

    // 按优先级排序（高优先级在前）
    std::sort(m_rules.begin(), m_rules.end(),
              [](const HighlightRule& a, const HighlightRule& b) {
                  return a.priority > b.priority;
              });

    qDebug() << "[LanguageHighlighter] 已生成" << m_rules.size() << "条高亮规则";
}

// ============================================================
// 创建关键字高亮规则
// ============================================================
QVector<HighlightRule> LanguageHighlighter::createKeywordRules(
    const QStringList& keywords,
    const QTextCharFormat& format,
    int priority) const
{
    QVector<HighlightRule> rules;

    if (keywords.isEmpty()) {
        return rules;
    }

    // 将关键字列表转换为正则表达式：\b(keyword1|keyword2|...)\b
    QStringList escapedKeywords;
    for (const QString& kw : keywords) {
        escapedKeywords.append(QRegularExpression::escape(kw));
    }

    QString pattern = "\\b(" + escapedKeywords.join("|") + ")\\b";

    HighlightRule rule;
    rule.pattern = QRegularExpression(pattern);
    rule.format = format;
    rule.priority = priority;
    rules.append(rule);

    return rules;
}

// ============================================================
// 处理多行注释
// ============================================================
void LanguageHighlighter::handleMultiLineComment(
    const QString& text,
    const QRegularExpression& startPattern,
    const QRegularExpression& endPattern,
    const QTextCharFormat& format)
{
    // 如果开始或结束模式为空，跳过
    if (startPattern.pattern().isEmpty() || endPattern.pattern().isEmpty()) {
        return;
    }

    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1) {
        startIndex = text.indexOf(startPattern);
    }

    while (startIndex >= 0) {
        QRegularExpressionMatch match = endPattern.match(text, startIndex);
        int endIndex = match.capturedStart();
        int commentLength;

        if (endIndex == -1) {
            // 注释跨越多行
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            // 注释在本行结束
            commentLength = endIndex - startIndex + match.capturedLength();
        }

        setFormat(startIndex, commentLength, format);

        // 查找下一个注释
        startIndex = text.indexOf(startPattern, startIndex + commentLength);
    }
}

// ============================================================
// 高亮文本块（核心方法）
// ============================================================
void LanguageHighlighter::highlightBlock(const QString& text)
{
    // ============================================================
    // 1. 处理多行注释（优先级最高）
    // ============================================================
    if (!m_commentStartPattern.pattern().isEmpty() &&
        !m_commentEndPattern.pattern().isEmpty()) {
        handleMultiLineComment(text,
                               m_commentStartPattern,
                               m_commentEndPattern,
                               m_multiLineCommentFormat);
    }


    // 根据语言 ID 调用对应的专门高亮函数
    if (m_currentLanguage.id == "yaml") {
        highlightYaml(text);
        return; // 避免通用规则覆盖
    } else if (m_currentLanguage.id == "properties") {
        highlightProperties(text);
        return;
    }
    // ============================================================
    // 2. 应用所有高亮规则
    // ============================================================
    // 对于每个规则，在文本中查找匹配并应用格式
    // 但需要跳过已经格式化的区域（如多行注释）
    for (const HighlightRule& rule : m_rules) {
        // 获取当前已有的格式信息
        // 使用 QSyntaxHighlighter 的 format() 方法检查是否已格式化
        // 但由于无法直接获取任意位置的格式，我们采用"先到先得"策略：
        // 如果某个位置已被高亮，则跳过后续匹配

        // 更简单的实现：直接应用规则（允许覆盖）
        // 但对于多行注释，由于我们使用了状态机制，注释区域不会被其他规则覆盖
        // 但字符串和数字可能被关键字覆盖，所以关键字规则需要设置较低的优先级

        // 创建匹配迭代器
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            int start = match.capturedStart();
            int length = match.capturedLength();

            // 检查该位置是否已被多行注释覆盖（通过检查字符格式）
            // 简单实现：直接应用格式（允许覆盖）
            // 更精确的方式：检查是否在注释块中
            setFormat(start, length, rule.format);
        }
    }

    // ============================================================
    // 3. 特殊情况：高亮函数名（确保函数名不被关键字覆盖）
    // ============================================================
    // 函数名格式：标识符后跟左括号
    // 但我们已经在规则中包含了，这里不需要额外处理

    // ============================================================
    // 4. 特殊情况：高亮类/类型名
    // ============================================================
    // 同样已在规则中处理
}

void LanguageHighlighter::highlightYaml(const QString& text)
{
    // 1. 注释: # 到行尾
    QRegularExpression commentRegex("#.*$");
    QRegularExpressionMatchIterator commentIt = commentRegex.globalMatch(text);
    while (commentIt.hasNext()) {
        QRegularExpressionMatch match = commentIt.next();
        setFormat(match.capturedStart(), match.capturedLength(), m_commentFormat);
    }

    // 2. 键: 冒号前的内容（包括缩进）
    // 匹配行首的 key:（忽略前导空格）
    QRegularExpression keyRegex("^\\s*([a-zA-Z_][a-zA-Z0-9_\\-]*)\\s*:");
    QRegularExpressionMatch keyMatch = keyRegex.match(text);
    if (keyMatch.hasMatch()) {
        setFormat(keyMatch.capturedStart(1), keyMatch.capturedLength(1), m_keywordColor);
    }

    // 3. 字符串值: 双引号或单引号内的内容
    QRegularExpression stringRegex("\"([^\"]*)\"|'([^']*)'");
    QRegularExpressionMatchIterator stringIt = stringRegex.globalMatch(text);
    while (stringIt.hasNext()) {
        QRegularExpressionMatch match = stringIt.next();
        setFormat(match.capturedStart(), match.capturedLength(), m_stringFormat);
    }

    // 4. 数字: 整数或浮点数
    QRegularExpression numberRegex("\\b(\\d+(\\.\\d+)?)\\b");
    QRegularExpressionMatchIterator numIt = numberRegex.globalMatch(text);
    while (numIt.hasNext()) {
        QRegularExpressionMatch match = numIt.next();
        setFormat(match.capturedStart(), match.capturedLength(), m_numberFormat);
    }

    // 5. 布尔值和 null
    QRegularExpression boolRegex("\\b(true|false|on|off|yes|no|null|~)\\b");
    QRegularExpressionMatchIterator boolIt = boolRegex.globalMatch(text);
    while (boolIt.hasNext()) {
        QRegularExpressionMatch match = boolIt.next();
        setFormat(match.capturedStart(), match.capturedLength(), m_constantFormat);
    }
}

void LanguageHighlighter::highlightProperties(const QString& text)
{
    // 1. 注释: # 到行尾
    QRegularExpression commentRegex("#.*$");
    QRegularExpressionMatchIterator commentIt = commentRegex.globalMatch(text);
    while (commentIt.hasNext()) {
        QRegularExpressionMatch match = commentIt.next();
        setFormat(match.capturedStart(), match.capturedLength(), m_commentFormat);
    }

    // 2. 键: 等号或冒号前的部分（包括点分隔符）
    // 匹配 key=value 或 key:value
    QRegularExpression keyRegex("^\\s*([a-zA-Z0-9_\\.]+)\\s*[=:]");
    QRegularExpressionMatch keyMatch = keyRegex.match(text);
    if (keyMatch.hasMatch()) {
        setFormat(keyMatch.capturedStart(1), keyMatch.capturedLength(1), m_keywordColor);
    }

    // 3. 值: 等号或冒号后的部分
    QRegularExpression valueRegex("[=:]\\s*(.+)$");
    QRegularExpressionMatch valueMatch = valueRegex.match(text);
    if (valueMatch.hasMatch()) {
        // 检查值是否包含引号
        QString value = valueMatch.captured(1).trimmed();
        if (value.startsWith('"') && value.endsWith('"')) {
            setFormat(valueMatch.capturedStart(1), valueMatch.capturedLength(1), m_stringFormat);
        } else {
            // 可以是数字或普通字符串，简单赋予一个颜色
            setFormat(valueMatch.capturedStart(1), valueMatch.capturedLength(1), m_dataTypeColor);
        }
    }

    // 4. 额外的字符串（包含引号）
    QRegularExpression stringRegex("\"([^\"]*)\"");
    QRegularExpressionMatchIterator stringIt = stringRegex.globalMatch(text);
    while (stringIt.hasNext()) {
        QRegularExpressionMatch match = stringIt.next();
        setFormat(match.capturedStart(), match.capturedLength(), m_stringFormat);
    }
}

} // namespace SprintIDE
