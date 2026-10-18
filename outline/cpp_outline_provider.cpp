#include "cpp_outline_provider.h"
#include <QRegularExpression>
#include <QStringList>
#include <functional>

namespace SprintIDE {

    int CppOutlineProvider::braceDelta(const QString& line)
    {
        int d = 0;
        bool inStr = false, inChar = false, inLine = false, esc = false;
        for (int i = 0; i < line.size(); ++i) {
            const QChar c = line[i];
            if (inLine) break;
            if (esc) { esc = false; continue; }
            if (c == '\\') { esc = true; continue; }
            if (c == '"'  && !inChar) { inStr  = !inStr;  continue; }
            if (c == '\'' && !inStr ) { inChar = !inChar; continue; }
            if (inStr || inChar) continue;
            if (c == '/' && i + 1 < line.size() && line[i + 1] == '/') { inLine = true; break; }
            if (c == '{') ++d;
            else if (c == '}') --d;
        }
        return d;
    }

    QList<OutlineSymbol> CppOutlineProvider::scan(const QString& text,
        const QString& /*filePath*/)
    {
        // ============================================================
        //  内部堆节点：保证指针稳定
        //    QList<OutlineSymbol>::append 会让之前保存的指针失效，
        //    所以扫描阶段用堆对象管理，扫描完深拷贝成值树返回。
        // ============================================================
        struct HeapNode {
            OutlineSymbol    sym;
            QList<HeapNode*> children;
            int              startDepth = 0;   // 进入这个节点时的括号深度
        };

        QList<HeapNode*> roots;               // 顶层节点
        QList<HeapNode*> classStack;          // 活跃容器（Class/Namespace）
        QList<HeapNode*> functionStack;       // 活跃函数/方法（用于填 endLine）
        HeapNode*        pendingClass    = nullptr;   // 等待 { 出现的类
        HeapNode*        pendingFunction = nullptr;   // 等待 { 出现的函数
        int              depth = 0;

        static const QRegularExpression reClass(
            R"(^\s*(?:class|struct|union|enum(?:\s+class)?)\s+([A-Za-z_]\w*))");
        static const QRegularExpression reDefine(
            R"(^\s*#define\s+([A-Za-z_]\w*))");
        static const QRegularExpression reFunc(
            R"(^\s*(?:template\s*<[^>]+>\s*)?(?:static\s+|inline\s+|virtual\s+|constexpr\s+|explicit\s+|friend\s+)*(?:[A-Za-z_][\w:<>,\s\*&]*?[\s\*&]+)(?:([A-Za-z_]\w*)::)?(~?[A-Za-z_]\w*)\s*\([^;{]*\)\s*(?:const\s*)?(?:override\s*)?(?:noexcept\s*)?(?:=\s*0)?\s*;?\s*$)");
        static const QRegularExpression reMember(
            R"(^\s*([A-Za-z_][\w:<>,\s\*&]*?[\s\*&]+)([A-Za-z_]\w*)\s*(?:=|;))");

        const QStringList lines = text.split('\n');

        // 统一的"深度回落 → 关闭符号"检查
        auto closeChecks = [&](int lineNo1Based) {
            // pendingFunction 何时进入 functionStack：后续行出现 { 时
            if (pendingFunction && depth > pendingFunction->startDepth) {
                functionStack.append(pendingFunction);
                pendingFunction = nullptr;
            }

            while (!functionStack.isEmpty()
                && depth <= functionStack.last()->startDepth) {
                if (functionStack.last()->sym.endLine <= 0) {
                    functionStack.last()->sym.endLine = lineNo1Based;
                }
                functionStack.removeLast();
            }
            while (!classStack.isEmpty()
                && depth <= classStack.last()->startDepth) {
                if (classStack.last()->sym.endLine <= 0) {
                    classStack.last()->sym.endLine = lineNo1Based;
                }
                classStack.removeLast();
            }
        };

        for (int i = 0; i < lines.size(); ++i) {
            const int lineNo = i + 1;
            const QString& line = lines[i];
            const QString trimmed = line.trimmed();

            // ---------- 空行 / 纯注释行：只更新深度 ----------
            if (trimmed.isEmpty() || trimmed.startsWith("//")) {
                depth += braceDelta(line);
                closeChecks(lineNo);
                continue;
            }

            // ---------- 类声明识别 ----------
            if (!pendingClass) {
                QRegularExpressionMatch m = reClass.match(line);
                if (m.hasMatch()) {
                    auto* node = new HeapNode();
                    node->sym.name = m.captured(1);
                    node->sym.kind = OutlineSymbol::Kind::Class;
                    node->sym.line = lineNo;

                    if (classStack.isEmpty()) roots.append(node);
                    else                      classStack.last()->children.append(node);

                    pendingClass = node;
                }
            }

            // ---------- 深度更新 + 进入 Class ----------
            const int delta  = braceDelta(line);
            const int before = depth;
            depth += delta;

            if (pendingClass && depth > before) {
                pendingClass->startDepth = before;
                classStack.append(pendingClass);
                pendingClass = nullptr;
            }

            // ---------- 函数 / 方法识别 ----------
            {
                QRegularExpressionMatch mf = reFunc.match(line);
                if (mf.hasMatch()) {
                    const QString owner = mf.captured(1);
                    const QString fname = mf.captured(2);

                    auto* node = new HeapNode();
                    node->sym.name = owner.isEmpty()
                    ? fname
                    : (owner + QStringLiteral("::") + fname);
                    node->sym.kind = (classStack.isEmpty() && owner.isEmpty())
                    ? OutlineSymbol::Kind::Function
                    : OutlineSymbol::Kind::Method;
                    node->sym.line = lineNo;

                    // 返回类型：name 之前的部分，剥掉修饰词和 owner 前缀
                    int namePos = line.indexOf(fname);
                    QString prefix = (namePos > 0) ? line.left(namePos).trimmed()
                    : QString();
                    prefix.remove(QRegularExpression(
                            R"(^(?:template\s*<[^>]*>\s*)?(?:static\s+|inline\s+|virtual\s+|constexpr\s+|explicit\s+|friend\s+)*)"));
                    if (!owner.isEmpty()) {
                        prefix.remove(owner + QStringLiteral("::"));
                    }
                    node->sym.typeName = prefix.trimmed();

                    if (classStack.isEmpty()) roots.append(node);
                    else                      classStack.last()->children.append(node);

                    // ---- 填 endLine ----
                    if (trimmed.endsWith(';') && !line.contains('{')) {
                        // 纯声明，无函数体
                        node->sym.endLine = lineNo;
                    } else {
                        const bool singleLine = (delta == 0
                            && line.contains('{')
                            && line.contains('}'));
                        if (singleLine) {
                            // 单行函数体：void f() { body; }
                            node->sym.endLine = lineNo;
                        } else if (depth <= before) {
                            // 本行没有 {，等后面的行出现
                            node->startDepth = before;
                            pendingFunction = node;
                        } else {
                            // 本行有 {，进入函数体
                            node->startDepth = before;
                            functionStack.append(node);
                        }
                    }
                }
            }

            // ---------- 顶层 #define ----------
            if (classStack.isEmpty() && !pendingClass) {
                QRegularExpressionMatch md = reDefine.match(line);
                if (md.hasMatch()) {
                    auto* node = new HeapNode();
                    node->sym.name    = md.captured(1);
                    node->sym.kind    = OutlineSymbol::Kind::Define;
                    node->sym.line    = lineNo;
                    node->sym.endLine = lineNo;   // 单行
                    roots.append(node);
                }
            }

            // ---------- 类内成员变量（不在任何函数体内） ----------
            if (!classStack.isEmpty()
                && functionStack.isEmpty()
                && !pendingFunction
                && !pendingClass
                && !line.contains('(')
                && !trimmed.startsWith('#')) {

                QRegularExpressionMatch mm = reMember.match(line);
                if (mm.hasMatch()) {
                    auto* node = new HeapNode();
                    node->sym.typeName = mm.captured(1).trimmed();
                    node->sym.name     = mm.captured(2);
                    node->sym.kind     = OutlineSymbol::Kind::Property;
                    node->sym.line     = lineNo;
                    if (trimmed.endsWith(';')) {
                        node->sym.endLine = lineNo;
                    }
                    classStack.last()->children.append(node);
                }
            }

            closeChecks(lineNo);
        }

        // ---------- 兜底：未正常关闭的节点，用文件末行作为 endLine ----------
        const int lastLine = lines.size();
        if (pendingFunction) {
            if (pendingFunction->sym.endLine <= 0) {
                pendingFunction->sym.endLine = lastLine;
            }
            pendingFunction = nullptr;
        }
        for (auto* n : functionStack) {
            if (n->sym.endLine <= 0) n->sym.endLine = lastLine;
        }
        for (auto* n : classStack) {
            if (n->sym.endLine <= 0) n->sym.endLine = lastLine;
        }

        // ---------- 深拷贝成值树 ----------
        std::function<QList<OutlineSymbol>(const QList<HeapNode*>&)> copyTree;
        copyTree = [&](const QList<HeapNode*>& nodes) -> QList<OutlineSymbol> {
            QList<OutlineSymbol> out;
            for (auto* n : nodes) {
                OutlineSymbol s = n->sym;
                s.children = copyTree(n->children);
                out.append(s);
            }
            return out;
        };
        QList<OutlineSymbol> result = copyTree(roots);

        // ---------- 清理堆节点 ----------
        std::function<void(QList<HeapNode*>&)> cleanup;
        cleanup = [&](QList<HeapNode*>& nodes) {
            for (auto* n : nodes) {
                cleanup(n->children);
                delete n;
            }
            nodes.clear();
        };
        cleanup(roots);

        return result;
    }

} // namespace SprintIDE