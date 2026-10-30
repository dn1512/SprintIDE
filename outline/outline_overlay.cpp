#include "outline_overlay.h"
#include "outline_item_delegate.h"
#include "theme_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QPainter>
#include <QMouseEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QApplication>

namespace SprintIDE {

    namespace {

        // 递归扁平化：容器（Class/Namespace）不作为可跳转项，
        // 但它的名字作为前缀加到子项上，让用户看到 "Foo::bar" 这样的归属
        void flattenSymbols(const QList<OutlineSymbol>& symbols,
            const QString& parentPrefix,
            QList<QPair<OutlineSymbol, QString>>& props,
            QList<QPair<OutlineSymbol, QString>>& methods)
        {
            for (const auto& s : symbols) {
                if (s.isContainer()) {
                    flattenSymbols(s.children,
                        parentPrefix + s.name + QStringLiteral("::"),
                        props, methods);
                } else if (s.isCallable()) {
                    methods.append({s, parentPrefix});
                } else {
                    props.append({s, parentPrefix});
                }
            }
        }

    } // anonymous namespace

    OutlineOverlay::OutlineOverlay(QWidget* parent)
    : QDialog(parent, Qt::Tool | Qt::FramelessWindowHint)
    {
        setObjectName("OutlineOverlay");
        setAttribute(Qt::WA_ShowWithoutActivating, true);
        setModal(false);
        setFocusPolicy(Qt::NoFocus);

        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        // ---------- 标题栏（左右各一个） ----------
        auto* titleBar = new QWidget(this);
        titleBar->setObjectName("OutlineTitleBar");
        auto* titleLayout = new QHBoxLayout(titleBar);
        titleLayout->setContentsMargins(8, 6, 8, 6);
        titleLayout->setSpacing(0);

        m_propTitle = new QLabel(tr("属性"), titleBar);
        m_propTitle->setObjectName("OutlineSectionTitle");
        titleLayout->addWidget(m_propTitle, 4);

        m_methodTitle = new QLabel(tr("方法"), titleBar);
        m_methodTitle->setObjectName("OutlineSectionTitle");
        titleLayout->addWidget(m_methodTitle, 6);

        mainLayout->addWidget(titleBar);

        // ---------- 分割区域 ----------
        m_splitter = new QSplitter(Qt::Horizontal, this);
        m_splitter->setHandleWidth(1);
        m_splitter->setChildrenCollapsible(false);

        m_propList = new QListWidget(m_splitter);
        m_propList->setObjectName("OutlineList");
        m_propList->setFrameShape(QFrame::NoFrame);
        m_propList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_propList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_propList->setWordWrap(true);
        m_propList->setUniformItemSizes(false);
        m_propList->setResizeMode(QListView::Adjust);
        m_propList->setSelectionMode(QAbstractItemView::SingleSelection);
        m_propList->setMouseTracking(true);

        m_methodList = new QListWidget(m_splitter);
        m_methodList->setObjectName("OutlineList");
        m_methodList->setFrameShape(QFrame::NoFrame);
        m_methodList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_methodList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_methodList->setWordWrap(true);
        m_methodList->setUniformItemSizes(false);
        m_methodList->setResizeMode(QListView::Adjust);
        m_methodList->setSelectionMode(QAbstractItemView::SingleSelection);
        m_methodList->setMouseTracking(true);

        m_splitter->addWidget(m_propList);
        m_splitter->addWidget(m_methodList);
        m_splitter->setStretchFactor(0, 4);
        m_splitter->setStretchFactor(1, 6);
        m_splitter->setSizes({400, 600});

        mainLayout->addWidget(m_splitter, 1);

        m_delegate = new OutlineItemDelegate(m_propList, this);
        m_propList->setItemDelegate(m_delegate);
        m_methodList->setItemDelegate(m_delegate);

        // ---------- 鼠标点击 跳转 ----------
        auto onItemClicked = [this](QListWidgetItem* item) {
            if (!item) return;
            const int line = item->data(Qt::UserRole + 3).toInt();
            if (line > 0) emit symbolActivated(line);
            hideOverlay();
        };
        // itemClicked：正常路径（松开鼠标时触发）
        connect(m_propList, &QListWidget::itemClicked, this, onItemClicked);
        connect(m_methodList, &QListWidget::itemClicked, this, onItemClicked);

        // itemPressed：兜底路径（按下时触发）
        //   万一某个平台上 itemClicked 不触发（焦点/事件机制差异），
        //   itemPressed 会在按下瞬间触发。
        //   因为 hideOverlay() 执行后浮层不可见，不会出现"两次跳转"。
        connect(m_propList, &QListWidget::itemPressed, this, onItemClicked);
        connect(m_methodList, &QListWidget::itemPressed, this, onItemClicked);

        // ---------- 主题 ----------
        applyTheme();
        connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &OutlineOverlay::applyTheme);

        // ---------- 焦点监听 ----------
        //   目的：用"焦点真正离开编辑器+浮层"作为关闭条件
        //   避免 TextEditor::focusOutEvent 直接关浮层导致的
        //   "点击浮层 item 时浮层先消失、itemClicked 不触发"问题
        //
        //   规则：
        //     焦点 nullptr（切到其他应用）         关闭
        //     焦点 浮层自身或浮层内部控件          保持
        //     焦点 父编辑器（点击编辑器回到编辑）  保持（由 TextEditor::mousePressEvent 关闭）
        //     焦点 其他控件（文件树、终端等）      关闭
        connect(qApp, &QApplication::focusChanged, this,
            [this](QWidget* /*old*/, QWidget* now) {
                if (!isVisible()) return;

                if (now == nullptr) {
                    hideOverlay();
                    return;
                }

                // 焦点在浮层内部 保持
                if (now == this || isAncestorOf(now)) return;

                // 焦点在父编辑器 保持（后续由 TextEditor::mousePressEvent 处理）
                if (parentWidget()) {
                    if (now == parentWidget() || parentWidget()->isAncestorOf(now)) {
                        return;
                    }
                }

                // 其他 关闭
                hideOverlay();
            });
    }

    OutlineOverlay::~OutlineOverlay() = default;

    void OutlineOverlay::applyTheme()
    {
        ThemeManager& tm = ThemeManager::instance();

        const QString bg       = tm.colorString(ThemeManager::UIElement::OutlineOverlayBackground);
        const QString border   = tm.colorString(ThemeManager::UIElement::OutlineOverlayBorder);
        const QString divider  = tm.colorString(ThemeManager::UIElement::OutlineOverlayDivider);
        const QString secTitle = tm.colorString(ThemeManager::UIElement::OutlineOverlaySectionTitle);
        const QString secText  = tm.colorString(ThemeManager::UIElement::SecondaryTextColor);
        const QString scroll   = tm.rgba(ThemeManager::UIElement::ScrollHandleColor, 90);
        const QString scrollHv = tm.rgba(ThemeManager::UIElement::ScrollHandleHoverColor, 180);

        setStyleSheet(QString(
                "QDialog#OutlineOverlay {"
                "  background-color: %1;"
                "  border: 1px solid %2;"
                "}"
                "QWidget#OutlineTitleBar {"
                "  background-color: %1;"
                "  border-bottom: 1px solid %3;"
                "}"
                "QLabel#OutlineSectionTitle {"
                "  color: %4;"
                "  font-size: 11px;"
                "  font-weight: 600;"
                "  padding: 2px 4px;"
                "}"
                "QListWidget#OutlineList {"
                "  background-color: %1;"
                "  border: none;"
                "  outline: none;"
                "  padding: 2px 4px;"
                "}"
                "QListWidget#OutlineList::item {"
                "  border: none;"
                "}"
                "QSplitter::handle {"
                "  background-color: %3;"
                "}"
                "QScrollBar:vertical {"
                "  background: transparent; width: 6px; margin: 0;"
                "}"
                "QScrollBar::handle:vertical {"
                "  background: %5; min-height: 30px; border-radius: 3px;"
                "}"
                "QScrollBar::handle:vertical:hover { background: %6; }"
                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
                "QScrollBar:horizontal { height: 0; }"
            ).arg(bg, border, divider, secTitle, scroll, scrollHv));

        // 副标题颜色语义：无属性/方法时用 SecondaryTextColor
        Q_UNUSED(secText);

        // 更新 delegate 的字体（保留 option.font 即可，无需干预）
        update();
        if (m_propList)   m_propList->viewport()->update();
        if (m_methodList) m_methodList->viewport()->update();
    }

    void OutlineOverlay::setSymbols(const QList<OutlineSymbol>& symbols)
    {
        m_symbols = symbols;
    }

    void OutlineOverlay::showAt(const QPoint& /*globalPos*/)
    {
        rebuildLists();

        // v8.5：以编辑器的"全局几何"为锚点，浮层居中于编辑区域
        //   之前用 parentWidget()->geometry() 是相对主窗口的坐标，
        //   和 globalPos / 全局坐标混用，导致强制拉回屏幕左侧。
        //   这里统一转成全局坐标再计算。
        QRect editorGlobal;
        if (parentWidget()) {
            editorGlobal = QRect(
                parentWidget()->mapToGlobal(QPoint(0, 0)),
                parentWidget()->size()
            );
        } else if (QScreen* sc = QGuiApplication::primaryScreen()) {
            editorGlobal = sc->availableGeometry();
        }

        // 尺寸：编辑区域的 60% × 50%，有上下限
        int w = qBound(640, int(editorGlobal.width() * 0.60), 1100);
        int h = qBound(400, int(editorGlobal.height() * 0.50), 720);

        // 居中于编辑区域
        int x = editorGlobal.center().x() - w / 2;
        int y = editorGlobal.center().y() - h / 2;

        // 兜底：不超出所在屏幕的可用区域
        QScreen* targetScreen = QGuiApplication::screenAt(editorGlobal.center());
        if (!targetScreen) targetScreen = QGuiApplication::primaryScreen();
        if (targetScreen) {
            const QRect avail = targetScreen->availableGeometry();
            x = qBound(avail.left() + 10, x, avail.right()  - w - 10);
            y = qBound(avail.top()  + 10, y, avail.bottom() - h - 10);
        }

        setGeometry(x, y, w, h);

        // 默认选中：左列有项 左列第一行；否则右列第一行
        m_activeList = m_propList;
        if (m_propList->count() > 0) {
            m_propList->setCurrentRow(0);
            m_methodList->setCurrentItem(nullptr);
        } else if (m_methodList->count() > 0) {
            m_activeList = m_methodList;
            m_methodList->setCurrentRow(0);
        }

        show();
        raise();
    }

    void OutlineOverlay::hideOverlay()
    {
        hide();
    }

    bool OutlineOverlay::isMouseInside() const
    {
        if (!isVisible()) return false;
        return geometry().contains(QCursor::pos());
    }

    void OutlineOverlay::rebuildLists()
    {
        m_propList->clear();
        m_methodList->clear();

        QList<QPair<OutlineSymbol, QString>> props, methods;
        flattenSymbols(m_symbols, QString(), props, methods);

        for (const auto& p : props) {
            appendSymbol(m_propList, p.first, p.second);
        }
        for (const auto& m : methods) {
            appendSymbol(m_methodList, m.first, m.second);
        }

        m_propTitle->setText(tr("属性") + QString("(%1)").arg(m_propList->count()));
        m_methodTitle->setText(tr("方法") + QString("(%1)").arg(m_methodList->count()));
    }

    void OutlineOverlay::appendSymbol(QListWidget* list,
        const OutlineSymbol& symbol,
        const QString& prefix)
    {
        auto* item = new QListWidgetItem();
        const QString displayName = prefix.isEmpty()
        ? symbol.name
        : prefix + symbol.name;

        item->setData(Qt::DisplayRole,     displayName);
        item->setData(Qt::UserRole + 1,    symbol.typeName);
        item->setData(Qt::UserRole + 2,    static_cast<int>(symbol.kind));
        item->setData(Qt::UserRole + 3,    symbol.line);

        list->addItem(item);
    }

    void OutlineOverlay::selectNext()
    {
        if (!m_activeList || m_activeList->count() == 0) return;
        const int row = m_activeList->currentRow();
        if (row + 1 < m_activeList->count()) {
            m_activeList->setCurrentRow(row + 1);
        }
    }

    void OutlineOverlay::selectPrev()
    {
        if (!m_activeList || m_activeList->count() == 0) return;
        const int row = m_activeList->currentRow();
        if (row > 0) {
            m_activeList->setCurrentRow(row - 1);
        }
    }

    void OutlineOverlay::focusLeft()
    {
        if (m_propList->count() == 0) return;
        m_activeList = m_propList;
        m_methodList->setCurrentItem(nullptr);
        if (!m_propList->currentItem()) {
            m_propList->setCurrentRow(0);
        }
    }

    void OutlineOverlay::focusRight()
    {
        if (m_methodList->count() == 0) return;
        m_activeList = m_methodList;
        m_propList->setCurrentItem(nullptr);
        if (!m_methodList->currentItem()) {
            m_methodList->setCurrentRow(0);
        }
    }

    void OutlineOverlay::activateCurrent()
    {
        if (!m_activeList) return;
        QListWidgetItem* item = m_activeList->currentItem();
        if (!item) return;
        const int line = item->data(Qt::UserRole + 3).toInt();
        if (line > 0) emit symbolActivated(line);
        hideOverlay();
    }

} // namespace SprintIDE