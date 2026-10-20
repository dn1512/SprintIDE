#include "breadcrumb_bar.h"
#include "outline_types.h"
#include "theme_manager.h"

#include <QHBoxLayout>
#include <QToolButton>
#include <QLabel>
#include <QMenu>
#include <QFont>

namespace SprintIDE {

    BreadcrumbBar::BreadcrumbBar(QWidget* parent)
    : QWidget(parent)
    {
        setObjectName("BreadcrumbBar");
        setFixedHeight(24);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        m_layout = new QHBoxLayout(this);
        m_layout->setContentsMargins(8, 0, 8, 0);
        m_layout->setSpacing(0);

        // 背景跟随主题（每帧变化时由主程序调 applyTheme）
        ThemeManager& tm = ThemeManager::instance();
        setAutoFillBackground(true);
        QPalette pal = palette();
        pal.setColor(QPalette::Window,
            tm.color(ThemeManager::UIElement::BreadcrumbBarBackground));
        setPalette(pal);

        connect(&tm, &ThemeManager::themeChanged, this, [this]() {
                ThemeManager& t = ThemeManager::instance();
                QPalette p = palette();
                p.setColor(QPalette::Window,
                    t.color(ThemeManager::UIElement::BreadcrumbBarBackground));
                setPalette(p);
                rebuild();   // 重新渲染按钮颜色
            });
    }

    void BreadcrumbBar::setFileDisplayName(const QString& name)
    {
        if (m_fileName == name) return;
        m_fileName = name;
        rebuild();
    }

    void BreadcrumbBar::setSymbols(const QList<OutlineSymbol>& symbols)
    {
        m_symbols = symbols;
        rebuild();
    }

    void BreadcrumbBar::updateCurrentLine(int line)
    {
        if (m_currentLine == line) return;
        m_currentLine = line;
        rebuild();
    }

    QSize BreadcrumbBar::sizeHint() const
    {
        // v8.5：固定高度 24，宽度由布局决定
        //   重写 sizeHint 后，QWidget::height() 在布局前也能正确返回
        return QSize(0, 24);
    }

    void BreadcrumbBar::rebuild()
    {
        // 清空布局
        QLayoutItem* item = nullptr;
        while ((item = m_layout->takeAt(0)) != nullptr) {
            if (QWidget* w = item->widget()) w->deleteLater();
            delete item;
        }

        // 第 1 级：文件名（不可点击）
        addBreadcrumb(m_fileName, m_fileName, {}, false);
        addSeparator();

        // 后续级：从根到最深命中
        auto path = pathAtLine(m_currentLine);
        for (int i = 0; i < path.size(); ++i) {
            const OutlineSymbol* s = path[i];
            // 同级候选（用于菜单）
            const QList<OutlineSymbol>* siblings;
            if (i == 0) {
                siblings = &m_symbols;
            } else {
                siblings = &path[i - 1]->children;
            }
            QList<QPair<QString, int>> menuItems;
            for (const auto& sib : *siblings) {
                menuItems.append({sib.name, sib.line});
            }

            // 最后一级加粗（当前所在）
            const bool isActive = (i == path.size() - 1);
            const QString text = s->name;
            QString tooltip = s->name;
            if (!s->typeName.isEmpty()) {
                tooltip = QString("%1 : %2").arg(s->typeName, s->name);
            }
            addBreadcrumb(text, tooltip, menuItems, isActive);
            if (i < path.size() - 1) addSeparator();
        }

        m_layout->addStretch(1);
    }

    QList<const OutlineSymbol*>
    BreadcrumbBar::pathAtLine(int line) const
    {
        QList<const OutlineSymbol*> path;

        // 深搜：沿 children 逐步走，找 line 落在 [s.line, s.endLine] 或 "最近的 s.line <= line"
        // 简化：没有 endLine 时，退化为"名字出现在当前行或之前的最深符号"
        std::function<void(const QList<OutlineSymbol>&,
            QList<const OutlineSymbol*>&)> dfs =
        [&](const QList<OutlineSymbol>& symbols,
            QList<const OutlineSymbol*>& cur) {
            for (int i = 0; i < symbols.size(); ++i) {
                const OutlineSymbol& s = symbols[i];

                // 计算本符号的"有效结束行"：
                // 最后一个符号的兜底
                //   - Class / Namespace 是容器，用 INT_MAX 可以（内部递归会自然收窄）
                //   - Function / Method / Variable 无 endLine 时只匹配自身一行，
                //     避免"误识别的符号"（比如被错误捕获的 QString）吞掉后面全部内容
                int effectiveEnd = s.endLine;
                if (effectiveEnd <= 0) {
                    if (i + 1 < symbols.size()) {
                        effectiveEnd = symbols[i + 1].line - 1;
                    } else if (s.kind == OutlineSymbol::Kind::Class
                        || s.kind == OutlineSymbol::Kind::Namespace) {
                        effectiveEnd = INT_MAX;   // 容器：允许覆盖到文件尾
                    } else {
                        effectiveEnd = s.line;    // 叶子：只匹配自身行
                    }
                }

                if (line >= s.line && line <= effectiveEnd) {
                    cur.append(&s);
                    // 更深层递归
                    if (!s.children.isEmpty()) {
                        dfs(s.children, cur);
                    }
                    if (cur.size() > path.size()) {
                        path = cur;
                    }
                    cur.removeLast();
                    return;
                } else if (line < s.line) {
                    return;  // 后面的兄弟不可能包含 line
                }
            }
        };

        QList<const OutlineSymbol*> cur;
        dfs(m_symbols, cur);
        return path;
    }

    void BreadcrumbBar::addSeparator()
    {
        auto* sep = new QLabel("›", this);
        ThemeManager& tm = ThemeManager::instance();
        sep->setStyleSheet(QString("color: %1; padding: 0 6px;")
            .arg(tm.colorString(ThemeManager::UIElement::SecondaryTextColor)));
        sep->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_layout->addWidget(sep);
    }

    void BreadcrumbBar::addBreadcrumb(const QString& text,
        const QString& tooltip,
        const QList<QPair<QString, int>>& menuItems,
        bool isActive)
    {
        ThemeManager& tm = ThemeManager::instance();

        auto* btn = new QToolButton(this);
        btn->setObjectName("BreadcrumbButton");
        btn->setAutoRaise(true);
        btn->setToolTip(tooltip);
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setCursor(menuItems.isEmpty() ? Qt::ArrowCursor : Qt::PointingHandCursor);
        btn->setText(text);

        QFont f = btn->font();
        f.setPointSizeF(f.pointSizeF() * 0.95);
        if (isActive) f.setBold(true);
        btn->setFont(f);

        const QString color = isActive
        ? tm.colorString(ThemeManager::UIElement::TextColor)
        : tm.colorString(ThemeManager::UIElement::SecondaryTextColor);
        btn->setStyleSheet(QString(
                "QToolButton#BreadcrumbButton {"
                "  color: %1;"
                "  background: transparent;"
                "  border: none;"
                "  padding: 2px 6px;"
                "  border-radius: 4px;"
                "}"
                "QToolButton#BreadcrumbButton:hover {"
                "  background-color: %2;"
                "}"
            ).arg(color,
                tm.colorString(ThemeManager::UIElement::HoverBackground)));

        // 单级菜单
        if (!menuItems.isEmpty()) {
            connect(btn, &QToolButton::clicked, this, [this, btn, menuItems]() {
                    QMenu menu(this);
                    for (const auto& item : menuItems) {
                        QAction* a = menu.addAction(item.first);
                        connect(a, &QAction::triggered, this, [this, line = item.second]() {
                                emit symbolJumpRequested(line);
                            });
                    }
                    menu.exec(btn->mapToGlobal(QPoint(0, btn->height())));
                });
        }

        m_layout->addWidget(btn);
    }

} // namespace SprintIDE