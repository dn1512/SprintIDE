#include "lsp/diagnostic_filter.h"
#include <QDebug>

namespace SprintIDE {

DiagnosticFilter& DiagnosticFilter::instance()
{
    static DiagnosticFilter inst;
    return inst;
}

DiagnosticFilter::DiagnosticFilter()
{
    static const QStringList raw = {
        // ---- Python: pyflakes ----
        "imported but unused",
        "is assigned to but never used",
        "is assigned but never used",
        "unable to detect undefined names",

        // ---- Python: pycodestyle ----
        "line too long",
        "trailing whitespace",
        "blank line contains whitespace",
        "expected \\d+ blank lines",
        "too many blank lines",
        "whitespace before",
        "whitespace after",
        "missing whitespace after",
        "multiple spaces after",
        "tab after",
        "continuation line",

        // ---- Python: pydocstyle ----
        "missing docstring",
        "missing module docstring",
        "missing function docstring",
        "missing class docstring",
        "missing method docstring",
        "docstring is",
        "first line should",

        // ---- Python: pylint ----
        "missing-module-docstring",
        "missing-function-docstring",
        "missing-class-docstring",
        "invalid-name",
        "too-few-public-methods",
        "too-many-arguments",
        "too-many-instance-attributes",
        "consider-using-f-string",
        "f-string is missing placeholders",
        "unnecessary-",
        "simplifiable",
        "redefined-outer-name",
        "unused-argument",
        "unused-wildcard-import",
        "wildcard-import",
        "broad-exception-caught",
        "broad-except",
        "raise-missing-from",
        "consider-using-with",
        "superfluous-parens",

        // ---- Python: mypy ----
        "missing type annotation",
        "need type annotation",

        // ---- C++: clangd ----
        "unused variable",
        "unused parameter",
        "unused private field",
        "no newline at end of file",
        "is not used directly",
        "declaration uses placeholder type",
        "narrowing conversion",
    };

    for (const QString& p : raw) {
        Rule r;
        r.pattern = p;
        r.regex = QRegularExpression(p, QRegularExpression::CaseInsensitiveOption);
        if (r.regex.isValid()) {
            m_defaultRules.append(r);
        }
    }
}

void DiagnosticFilter::loadUserRules(const QStringList& rules)
{
    m_userRules.clear();
    for (const QString& p : rules) {
        if (p.trimmed().isEmpty()) continue;
        Rule r;
        r.pattern = p;
        r.regex = QRegularExpression(p, QRegularExpression::CaseInsensitiveOption);
        if (!r.regex.isValid()) {
            qWarning() << "[DiagnosticFilter] 无效的正则:" << p;
            continue;
        }
        m_userRules.append(r);
    }
}

QStringList DiagnosticFilter::defaultRules() const
{
    QStringList result;
    result.reserve(m_defaultRules.size());
    for (const Rule& r : m_defaultRules) {
        result.append(r.pattern);
    }
    return result;
}

bool DiagnosticFilter::shouldKeep(const LSPDiagnostic& diag) const
{
    // ---- error 永远保留（所有模式下） ----
    if (diag.severity.compare("error", Qt::CaseInsensitive) == 0) {
        return true;
    }

    // ---- warning / info：按模式处理 ----
    switch (m_mode) {
    case Mode::ShowAll:
        return true;

    case Mode::ErrorsOnly:
        return false;

    case Mode::Custom: {
        // 先查用户规则（优先级更高）
        for (const Rule& r : m_userRules) {
            if (r.regex.match(diag.message).hasMatch()) return false;
        }
        // 再查默认规则
        for (const Rule& r : m_defaultRules) {
            if (r.regex.match(diag.message).hasMatch()) return false;
        }
        return true;
    }
    }

    return true;   // 兜底
}

} // namespace SprintIDE
