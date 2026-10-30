#include "rust_outline_provider.h"
#include <QRegularExpression>
#include <QStringList>
#include <functional>

namespace SprintIDE {

    // 与 CppOutlineProvider 相同的"净括号变化"逻辑
    int RustOutlineProvider::braceDelta(const QString& line)
    {
        int d = 0;
        bool inStr = false, inChar = false, inLine = false, inBlock = false, esc = false;
        for (int i = 0; i < line.size(); ++i) {
            const QChar c = line[i];
            if (inLine) break;
            if (inBlock) {
                if (c == '*' && i + 1 < line.size() && line[i + 1] == '/') {
                    inBlock = false;
                    ++i;
                }
                continue;
            }
            if (esc) { esc = false; continue; }
            if (c == '\\') { esc = true; continue; }
            if (c == '"'  && !inChar) { inStr  = !inStr;  continue; }
            if (c == '\'' && !inStr ) { inChar = !inChar; continue; }
            if (inStr || inChar) continue;
            if (c == '/' && i + 1 < line.size()) {
                if (line[i + 1] == '/') { inLine = true; break; }
                if (line[i + 1] == '*') { inBlock = true; ++i; continue; }
            }
            if (c == '{') ++d;
            else if (c == '}') --d;
        }
        return d;
    }

    QList<OutlineSymbol> RustOutlineProvider::scan(const QString& text,
        const QString& /*filePath*/)
    {
        struct HeapNode {
            OutlineSymbol      sym;
            QList<HeapNode*>   children;
            int                startDepth = 0;
        };

        QList<HeapNode*> roots;
        QList<HeapNode*> containerStack;   // mod / struct / enum / trait / impl
        QList<HeapNode*> functionStack;
        HeapNode*        pendingContainer = nullptr;
        int              depth = 0;

        // ============================================================
        //  Rust 符号正则（按优先级从上到下匹配，命中即 break）
        // ============================================================

        // fn name(...) / async fn / pub fn / pub(crate) fn
        static const QRegularExpression reFn(
            R"(^\s*(?:pub(?:\([^)]*\))?\s+)?(?:async\s+)?(?:unsafe\s+)?fn\s+([A-Za-z_]\w*)\s*[(<])");

        // struct Name / enum Name / trait Name / union Name
        static const QRegularExpression reType(
            R"(^\s*(?:pub(?:\([^)]*\))?\s+)?(?:struct|enum|trait|union)\s+([A-Za-z_]\w*))");

        // impl [<T>] [Trait for] Name
        static const QRegularExpression reImpl(
            R"(^\s*impl(?:<[^>]*>)?\s+(?:[A-Za-z_][\w:<>,\s]*?\s+for\s+)?([A-Za-z_]\w*))");

        // mod name / mod name { / mod name;
        static const QRegularExpression reMod(
            R"(^\s*(?:pub(?:\([^)]*\))?\s+)?mod\s+([A-Za-z_]\w*))");

        // const NAME / static NAME
        static const QRegularExpression reConst(
            R"(^\s*(?:pub(?:\([^)]*\))?\s+)?(?:const|static)\s+([A-Za-z_]\w*)\s*:)");

        // macro_rules! name
        static const QRegularExpression reMacro(
            R"(^\s*macro_rules!\s+([A-Za-z_]\w*))");

        // type Name = ... / type Name<T> = ...
        static const QRegularExpression reAlias(
            R"(^\s*(?:pub(?:\([^)]*\))?\s+)?type\s+([A-Za-z_]\w*))");

        // ============================================================
        //  helper：把新节点挂到当前容器
        // ============================================================
        auto attachNode = [&](HeapNode* node) {
            if (containerStack.isEmpty()) roots.append(node);
            else                          containerStack.last()->children.append(node);
        };

        // helper：在指定深度处关闭符号
        auto closeChecks = [&](int lineNo1Based) {
            if (pendingContainer && depth > pendingContainer->startDepth) {
                containerStack.append(pendingContainer);
                pendingContainer = nullptr;
            }
            while (!functionStack.isEmpty()
                && depth <= functionStack.last()->startDepth) {
                if (functionStack.last()->sym.endLine <= 0) {
                    functionStack.last()->sym.endLine = lineNo1Based;
                }
                functionStack.removeLast();
            }
            while (!containerStack.isEmpty()
                && depth <= containerStack.last()->startDepth) {
                if (containerStack.last()->sym.endLine <= 0) {
                    containerStack.last()->sym.endLine = lineNo1Based;
                }
                containerStack.removeLast();
            }
        };

        const QStringList lines = text.split('\n');

        for (int i = 0; i < lines.size(); ++i) {
            const int lineNo = i + 1;
            const QString& line = lines[i];
            const QString trimmed = line.trimmed();

            // 空行 / 行注释：只更新深度
            if (trimmed.isEmpty() || trimmed.startsWith("//")) {
                depth += braceDelta(line);
                closeChecks(lineNo);
                continue;
            }

            // ---------- macro_rules! ----------
            {
                auto m = reMacro.match(line);
                if (m.hasMatch()) {
                    auto* node = new HeapNode();
                    node->sym.name    = m.captured(1);
                    node->sym.kind    = OutlineSymbol::Kind::Define;
                    node->sym.line    = lineNo;
                    node->sym.endLine = lineNo;
                    attachNode(node);
                }
            }

            // ---------- 容器类型：mod / struct / enum / trait / impl ----------
            if (!pendingContainer) {
                // 优先级：mod > type/impl（因为 impl Foo 里的 `Foo` 也可能是 struct）
                QRegularExpressionMatch mMod = reMod.match(line);
                QRegularExpressionMatch mImpl = reImpl.match(line);
                QRegularExpressionMatch mType = reType.match(line);

                QString containerName;
                OutlineSymbol::Kind kind = OutlineSymbol::Kind::Class;

                if (mMod.hasMatch()) {
                    containerName = mMod.captured(1);
                    kind = OutlineSymbol::Kind::Namespace;
                } else if (mImpl.hasMatch()) {
                    containerName = QStringLiteral("impl ") + mImpl.captured(1);
                    kind = OutlineSymbol::Kind::Class;
                } else if (mType.hasMatch()) {
                    containerName = mType.captured(1);
                    kind = OutlineSymbol::Kind::Class;
                }

                if (!containerName.isEmpty()) {
                    auto* node = new HeapNode();
                    node->sym.name = containerName;
                    node->sym.kind = kind;
                    node->sym.line = lineNo;
                    attachNode(node);
                    pendingContainer = node;
                }
            }

            // ---------- 深度更新 ----------
            const int delta  = braceDelta(line);
            const int before = depth;
            depth += delta;

            if (pendingContainer && depth > before) {
                pendingContainer->startDepth = before;
                containerStack.append(pendingContainer);
                pendingContainer = nullptr;
            }

            // ---------- fn 函数 ----------
            {
                auto mf = reFn.match(line);
                if (mf.hasMatch()) {
                    auto* node = new HeapNode();
                    node->sym.name = mf.captured(1);
                    node->sym.kind = containerStack.isEmpty()
                    ? OutlineSymbol::Kind::Function
                    : OutlineSymbol::Kind::Method;
                    node->sym.line = lineNo;

                    // 返回类型：fn 前面的部分
                    int namePos = line.indexOf(mf.captured(1));
                    if (namePos > 0) {
                        QString prefix = line.left(namePos).trimmed();
                        // 去掉修饰词
                        static const QRegularExpression reModifier(
                            R"(^(?:pub(?:\([^)]*\))?\s+)?(?:async\s+)?(?:unsafe\s+)?(?:const\s+)?)");
                        prefix.remove(reModifier);
                        // 去掉结尾的 "fn "
                        if (prefix.endsWith(QStringLiteral("fn"))) {
                            prefix.chop(2);
                        }
                        node->sym.typeName = prefix.trimmed();
                    }

                    attachNode(node);

                    // 填 endLine
                    if (trimmed.endsWith(';') && !line.contains('{')) {
                        node->sym.endLine = lineNo;   // trait 方法声明
                    } else {
                        const bool singleLine = (delta == 0
                            && line.contains('{')
                            && line.contains('}'));
                        if (singleLine) {
                            node->sym.endLine = lineNo;
                        } else if (depth <= before) {
                            node->startDepth = before;
                            functionStack.append(node);
                        } else {
                            node->startDepth = before;
                            functionStack.append(node);
                        }
                    }
                }
            }

            // ---------- const / static ----------
            {
                auto mc = reConst.match(line);
                if (mc.hasMatch()) {
                    auto* node = new HeapNode();
                    node->sym.name    = mc.captured(1);
                    node->sym.kind    = OutlineSymbol::Kind::Variable;
                    node->sym.line    = lineNo;
                    node->sym.endLine = lineNo;
                    attachNode(node);
                }
            }

            // ---------- type alias ----------
            {
                auto mt = reAlias.match(line);
                if (mt.hasMatch()) {
                    auto* node = new HeapNode();
                    node->sym.name    = mt.captured(1);
                    node->sym.kind    = OutlineSymbol::Kind::Define;
                    node->sym.line    = lineNo;
                    node->sym.endLine = lineNo;
                    attachNode(node);
                }
            }

            closeChecks(lineNo);
        }

        // 兜底：未关闭节点用末行作为 endLine
        const int lastLine = lines.size();
        if (pendingContainer) {
            if (pendingContainer->sym.endLine <= 0) {
                pendingContainer->sym.endLine = lastLine;
            }
            pendingContainer = nullptr;
        }
        for (auto* n : functionStack) {
            if (n->sym.endLine <= 0) n->sym.endLine = lastLine;
        }
        for (auto* n : containerStack) {
            if (n->sym.endLine <= 0) n->sym.endLine = lastLine;
        }

        // 深拷贝成值树
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

        // 清理
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