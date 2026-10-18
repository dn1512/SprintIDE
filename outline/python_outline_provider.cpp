#include "python_outline_provider.h"
#include <QRegularExpression>
#include <QStringList>

namespace SprintIDE {

    QList<OutlineSymbol> PythonOutlineProvider::scan(const QString& text,
        const QString& /*filePath*/)
    {
        QList<OutlineSymbol> roots;

        // 栈：每个元素 = (缩进, children 指针, 该层 symbol 的 kind)
        struct Frame { int indent; QList<OutlineSymbol>* children; OutlineSymbol::Kind kind; };
        QList<Frame> stack;

        static const QRegularExpression reClass(
            R"(^(\s*)class\s+([A-Za-z_]\w*)\s*(?:\(([^)]*)\))?\s*:)");
        static const QRegularExpression reDef(
            R"(^(\s*)(?:async\s+)?def\s+([A-Za-z_]\w*)\s*\(([^)]*)\)\s*(?:->\s*([^:]+?))?\s*:)");
        static const QRegularExpression reVar(
            R"(^(\s*)([A-Za-z_]\w*)\s*:\s*([^=]+?)\s*=)");

        const QStringList lines = text.split('\n');
        for (int i = 0; i < lines.size(); ++i) {
            const QString& line = lines[i];
            const QString trimmed = line.trimmed();
            if (trimmed.isEmpty()) continue;
            if (trimmed.startsWith('#')) continue;
            // 过滤掉显然不是顶层的语句（if/for/while/return/...）
            if (trimmed.startsWith("return ") || trimmed.startsWith("pass")
                || trimmed.startsWith("raise ") || trimmed.startsWith("yield ")
                || trimmed.startsWith("assert ") || trimmed.startsWith("del ")
                || trimmed.startsWith("import ") || trimmed.startsWith("from ")
                || trimmed.startsWith("global ") || trimmed.startsWith("nonlocal ")
                || trimmed.startsWith("print(")) {
                continue;
            }

            const int indent = line.size() - line.trimmed().size();
            // 弹出缩进不足的层
            while (!stack.isEmpty() && stack.last().indent >= indent) {
                stack.removeLast();
            }
            QList<OutlineSymbol>* target = stack.isEmpty()
            ? &roots
            : stack.last().children;

            QRegularExpressionMatch m;
            if ((m = reClass.match(line)).hasMatch()) {
                OutlineSymbol s;
                s.name     = m.captured(2);
                s.typeName = m.captured(3).trimmed();   // 基类
                s.kind     = OutlineSymbol::Kind::Class;
                s.line     = i + 1;
                target->append(s);
                stack.append({indent, &target->last().children, OutlineSymbol::Kind::Class});
            } else if ((m = reDef.match(line)).hasMatch()) {
                OutlineSymbol s;
                s.name     = m.captured(2);
                s.typeName = m.captured(4).trimmed();   // 返回类型注解
                s.kind     = stack.isEmpty()
                ? OutlineSymbol::Kind::Function
                : OutlineSymbol::Kind::Method;
                s.line     = i + 1;
                target->append(s);
                stack.append({indent, &target->last().children, s.kind});
            } else if ((m = reVar.match(line)).hasMatch()) {
                // 只在 class 层识别为属性；顶层识别为变量
                bool insideClass = !stack.isEmpty()
                && stack.last().kind == OutlineSymbol::Kind::Class;
                OutlineSymbol s;
                s.name     = m.captured(2);
                s.typeName = m.captured(3).trimmed();
                s.kind     = insideClass ? OutlineSymbol::Kind::Property
                : OutlineSymbol::Kind::Variable;
                s.line     = i + 1;
                target->append(s);
            }
        }
        return roots;
    }

} // namespace SprintIDE