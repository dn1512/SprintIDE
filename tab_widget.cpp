#include "tab_widget.h"
#include "theme_manager.h"

#include <QApplication>
#include <QStyle>
#include <QPainter>
#include <QStyleOptionTab>
#include <QMouseEvent>
#include <QMenu>
#include <QDebug>
#include <QRect>

namespace SprintIDE {

// ==================== 自定义TabBar类 ====================

class CustomTabBar : public QTabBar
{
    Q_OBJECT

public:
    explicit CustomTabBar(QWidget *parent = nullptr)
        : QTabBar(parent)
        , m_hoveredTab(-1)
        , m_pressedTab(-1)
        , m_hoveredCloseTab(-1)
        , m_maxTabWidth(200)
        , m_showCloseButton(true)
    {
        setMouseTracking(true);
        setFocusPolicy(Qt::NoFocus);
        setDrawBase(false);
        setElideMode(Qt::ElideRight);
        setUsesScrollButtons(true);
        setExpanding(false);

        // 重要：禁用系统的关闭按钮
        setTabsClosable(false);

        // 设置布局策略，确保左对齐
        QSizePolicy policy = sizePolicy();
        policy.setHorizontalPolicy(QSizePolicy::Preferred);
        policy.setHorizontalStretch(0);
        setSizePolicy(policy);
    }

    void setShowCloseButton(bool show) {
        if (m_showCloseButton == show) return;
        m_showCloseButton = show;
        update();
    }
    bool showCloseButton() const { return m_showCloseButton; }
    // 设置标签最大宽度
    void setMaxTabWidth(int width) { m_maxTabWidth = width; }
    int maxTabWidth() const { return m_maxTabWidth; }
    void updateAppearance() {
        update();
    }

protected:
    QRect closeButtonRect(int tabIndex) const
    {
        const QRect r = tabRect(tabIndex);
        if (!r.isValid()) return QRect();

        constexpr int stripeWidth   = 2;   // 左侧活动竖条宽度
        constexpr int closeBtnSize  = 14;  // 关闭按钮尺寸
        constexpr int closeBtnMargin = 6;  // 按钮与竖条/文字的间距

        const int x = r.left() + stripeWidth + closeBtnMargin;
        const int y = r.top() + (r.height() - closeBtnSize) / 2;

        return QRect(x, y, closeBtnSize, closeBtnSize);
    }

    // 重写tabSizeHint以限制标签宽度
    QSize tabSizeHint(int index) const override
    {
        QSize size = QTabBar::tabSizeHint(index);
        QString text = tabText(index);
        QFontMetrics fm = fontMetrics();
        int textWidth = fm.horizontalAdvance(text) + 60;

        if (textWidth > m_maxTabWidth) textWidth = m_maxTabWidth;
        if (textWidth < 100)            textWidth = 100;

        size.setWidth(textWidth);
        return size;
    }

    // 重写sizeHint，让标签栏根据内容确定宽度
    QSize sizeHint() const override
    {
        // 不再累加所有标签宽度，也不硬限制成某个数字。
        // 标签栏宽度交给布局（Expanding）决定，实际会等于编辑区宽度。
        // 这里只提供一个"基准宽度"，不会成为最小约束（minimumSizeHint 才是）。
        return QSize(400, QTabBar::sizeHint().height());
    }

    // 重写minimumSizeHint
    QSize minimumSizeHint() const override
    {
        //   标签放不下时会自动出现左右滚动按钮。
        QSize size = QTabBar::minimumSizeHint();
        size.setWidth(100);
        return size;
    }

    // 重写resizeEvent，调整布局
    void resizeEvent(QResizeEvent *event) override
    {
        QTabBar::resizeEvent(event);

        // 强制标签栏根据内容宽度调整
        updateGeometry();
        update();
    }

    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event)

        QPainter painter(this);
        ThemeManager& tm = ThemeManager::instance();

        // Polaris 令牌
        const QColor bgColor     = tm.color(ThemeManager::UIElement::MainWindowBackground);  // #212730
        const QColor borderColor = tm.color(ThemeManager::UIElement::BorderColor);           // #2F3848

        // ---------- 1. 整体背景 ----------
        painter.fillRect(rect(), bgColor);

        // ---------- 2. 底部 1px 极弱分割线 ----------
        painter.setPen(borderColor);
        painter.drawLine(0, height() - 1, width(), height() - 1);

        // ---------- 3. 逐个绘制标签 ----------
        for (int i = 0; i < count(); ++i) {
            paintTab(&painter, i);
        }

        // ---------- 4. 左侧 1px 竖线（让标签栏视觉更完整） ----------
        painter.setPen(borderColor);
        painter.drawLine(0, 0, 0, height());

        // ---------- 5. 标签栏右侧空白区域填充背景 ----------
        // if (width() > sizeHint().width()) {
        //     QRect rightRect(sizeHint().width(), 0,
        //                     width() - sizeHint().width(), height());
        //     painter.fillRect(rightRect, bgColor);
        // }
    }

    void paintTab(QPainter *painter, int tabIndex)
    {
        QRect rect = tabRect(tabIndex);
        if (!rect.isValid()) return;

        ThemeManager& tm = ThemeManager::instance();

        // ---------- Polaris 令牌 ----------
        const QColor activeBg     = tm.color(ThemeManager::UIElement::ActiveTabBackground);
        const QColor inactiveBg   = tm.color(ThemeManager::UIElement::MainWindowBackground);
        const QColor hoverBg      = tm.color(ThemeManager::UIElement::HoverBackground);
        const QColor activeText   = tm.color(ThemeManager::UIElement::TextColor);
        const QColor inactiveText = tm.color(ThemeManager::UIElement::SecondaryTextColor);
        const QColor borderColor  = tm.color(ThemeManager::UIElement::BorderColor);
        const QColor primaryColor = tm.color(ThemeManager::UIElement::PrimaryColor);
        const QColor errorColor   = tm.color(ThemeManager::UIElement::ErrorColor);

        const bool isCurrent = (currentIndex() == tabIndex);
        const bool isHovered = (m_hoveredTab == tabIndex);

        // ---------- 1. 标签背景 ----------
        QColor tabBg;
        QColor tabText;
        if (isCurrent) {
            tabBg   = activeBg;
            tabText = activeText;
        } else if (isHovered) {
            tabBg   = hoverBg;
            tabText = activeText;
        } else {
            tabBg   = inactiveBg;
            tabText = inactiveText;
        }
        painter->fillRect(rect, tabBg);

        // ---------- 2. 左侧活动竖条（Polaris 特征） ----------
        constexpr int stripeWidth   = 2;
        constexpr int stripePadding = 4;
        if (isCurrent) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(primaryColor);
            painter->drawRect(rect.left(),
                              rect.top() + stripePadding,
                              stripeWidth,
                              rect.height() - 2 * stripePadding);
            painter->setBrush(Qt::NoBrush);
        }

        // ---------- 3. 右侧极弱分隔线 ----------
        painter->setPen(QPen(borderColor, 1));
        painter->drawLine(rect.right(), rect.top() + 3,
                          rect.right(), rect.bottom() - 3);

        // ---------- 4. 关闭按钮（ 画在左侧） ----------
        const QRect closeRect = closeButtonRect(tabIndex);
        constexpr int closeBtnMargin = 6;
        constexpr int closeBtnSize   = 14;

        // 悬停 / 活动标签才显示关闭按钮（PyCharm 风格）
        const bool showClose = m_showCloseButton && (isCurrent || isHovered);

        if (showClose && closeRect.isValid()) {
            const QPoint center = closeRect.center();
            const bool hoveringClose = (m_hoveredCloseTab == tabIndex);

            // 悬停时的圆形背景
            if (hoveringClose) {
                painter->setPen(Qt::NoPen);
                painter->setBrush(errorColor);
                painter->drawEllipse(center, closeBtnSize / 2, closeBtnSize / 2);
                painter->setBrush(Qt::NoBrush);
            }

            // X 符号
            const int lineLen = closeBtnSize / 2 - 3;
            const QColor xColor = hoveringClose
                                      ? QColor(Qt::white)
                                      : (isCurrent ? activeText : inactiveText);

            painter->setPen(QPen(xColor, 1.6));
            painter->drawLine(center.x() - lineLen, center.y() - lineLen,
                              center.x() + lineLen, center.y() + lineLen);
            painter->drawLine(center.x() + lineLen, center.y() - lineLen,
                              center.x() - lineLen, center.y() + lineLen);
        }

        // ---------- 5. 文件名（ 从关闭按钮右侧开始） ----------
        painter->setPen(tabText);

        QString displayText = QTabBar::tabText(tabIndex);
        const bool isModified = displayText.endsWith("*");
        if (isModified) {
            displayText.chop(1);   // 去掉末尾 *，稍后画成红点
        }

        // 文字区域：
        //   left  = 竖条 + 间距 + 关闭按钮 + 间距
        //   right = 8px 右边距
        const int textLeft = stripeWidth + closeBtnMargin
                             + closeBtnSize + closeBtnMargin;
        const int textRight = 8;

        QRect textRect = rect.adjusted(textLeft, 0, -textRight, 0);

        QFont font = painter->font();
        font.setBold(isCurrent);
        painter->setFont(font);

        QString elided = painter->fontMetrics().elidedText(
            displayText, Qt::ElideRight, textRect.width());

        painter->drawText(textRect,
                          Qt::AlignVCenter | Qt::AlignLeft,
                          elided);

        // ---------- 6. 已修改标记：小圆点 ----------
        if (isModified) {
            const int dotX = textRect.left()
            + painter->fontMetrics().horizontalAdvance(elided)
                + 6;
            const int dotY = rect.center().y();

            painter->setPen(Qt::NoPen);
            painter->setBrush(errorColor);
            painter->drawEllipse(QPoint(dotX, dotY), 3, 3);
            painter->setBrush(Qt::NoBrush);
        }
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        //  关键：用 m_showCloseButton 判断，不是 tabsClosable()
        //   因为 tabsClosable() 被我们强制设成 false（避免 Qt 造内置按钮）
        if (event->button() == Qt::LeftButton && m_showCloseButton) {
            const int tabIndex = tabAt(event->pos());
            if (tabIndex >= 0) {
                const QRect closeRect = closeButtonRect(tabIndex);
                if (closeRect.isValid() && closeRect.contains(event->pos())) {
                    m_closeButtonPressed = true;
                    update();                       // 视觉反馈（可选）
                    return;                         // 消费 press，不切换标签
                }
            }
        }

        m_closeButtonPressed = false;
        QTabBar::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (m_closeButtonPressed) {
            m_closeButtonPressed = false;

            // 确认 release 位置仍落在同一个关闭按钮内
            const int tabIndex = tabAt(event->pos());
            if (event->button() == Qt::LeftButton && tabIndex >= 0) {
                const QRect closeRect = closeButtonRect(tabIndex);
                if (closeRect.isValid() && closeRect.contains(event->pos())) {
                    emit tabCloseRequested(tabIndex);   //  触发关闭
                }
            }
            update();
            return;   // 不传给 QTabBar
        }

        m_pressedTab = -1;
        QTabBar::mouseReleaseEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        const int oldHoveredTab      = m_hoveredTab;
        const int oldHoveredCloseTab = m_hoveredCloseTab;

        m_hoveredTab      = tabAt(event->pos());
        m_hoveredCloseTab = -1;

        //  同样用 m_showCloseButton，不用 tabsClosable()
        if (m_hoveredTab >= 0 && m_showCloseButton) {
            const QRect closeRect = closeButtonRect(m_hoveredTab);
            if (closeRect.isValid() && closeRect.contains(event->pos())) {
                m_hoveredCloseTab = m_hoveredTab;
            }
        }

        if (oldHoveredTab != m_hoveredTab ||
            oldHoveredCloseTab != m_hoveredCloseTab) {
            update();
        }

        QTabBar::mouseMoveEvent(event);
    }

    void leaveEvent(QEvent *event) override
    {
        m_hoveredTab = -1;
        // m_hoveredCloseTab = -1;
        update();
        QTabBar::leaveEvent(event);
    }

    void contextMenuEvent(QContextMenuEvent *event) override
    {
        int tabIndex = tabAt(event->pos());
        if (tabIndex >= 0) {
            emit contextMenuRequested(tabIndex, event->globalPos());
        }
    }

signals:
    void contextMenuRequested(int index, const QPoint &globalPos);

private:
    int m_hoveredTab;
    int m_pressedTab;
    int m_hoveredCloseTab;
    int m_maxTabWidth;
    bool m_showCloseButton;
    bool m_closeButtonPressed = false;
};

// ==================== TabWidget实现 ====================

TabWidget::TabWidget(QWidget *parent)
    : QWidget(parent)
    , m_tabsClosable(true)  // 设置为false，因为我们自己绘制关闭按钮
    , m_movable(true)
    , m_shape(QTabBar::RoundedNorth)
    , m_currentIndex(-1)
    , m_tabBar(nullptr)
    , m_stack(nullptr)
    // , m_layout(nullptr)
{
    setupUI();
    setupConnections();
}

TabWidget::~TabWidget()
{
}

void TabWidget::setupUI()
{
    // 创建自定义标签栏
    m_tabBar = new CustomTabBar(this);
    m_tabBar->setShape(m_shape);
    m_tabBar->setMovable(m_movable);
    // m_tabBar->setTabsClosable(m_tabsClosable);

    // 设置标签最大宽度
    CustomTabBar *customBar = qobject_cast<CustomTabBar*>(m_tabBar);
    if (customBar) {
        customBar->setMaxTabWidth(400); // 设置最大宽度
    }

    // 创建堆栈窗口
    m_stack = new QStackedWidget(this);
    m_stack->setFrameShape(QFrame::NoFrame);

    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 创建标签栏容器，使用水平布局
    QWidget *tabBarContainer = new QWidget(this);
    QHBoxLayout *tabBarLayout = new QHBoxLayout(tabBarContainer);
    tabBarLayout->setContentsMargins(0, 0, 0, 0);
    tabBarLayout->setSpacing(0);
    tabBarLayout->addWidget(m_tabBar);

    // 添加一个拉伸弹簧，确保标签左对齐
    // QSpacerItem *spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    // tabBarLayout->addSpacerItem(spacer);

    // 将容器添加到主布局
    mainLayout->addWidget(tabBarContainer);
    mainLayout->addWidget(m_stack, 1);
}

void TabWidget::setupConnections()
{
    connect(m_tabBar, &QTabBar::currentChanged, this, &TabWidget::onCurrentChanged);
    connect(m_tabBar, &QTabBar::tabCloseRequested, this, &TabWidget::onTabCloseRequested);
    connect(m_tabBar, &QTabBar::tabMoved, this, &TabWidget::onTabMoved);
    connect(m_tabBar, &QTabBar::tabBarClicked, this, &TabWidget::tabBarClicked);
    connect(m_tabBar, &QTabBar::tabBarDoubleClicked, this, &TabWidget::tabBarDoubleClicked);

    // 连接自定义标签栏的上下文菜单信号
    CustomTabBar *customBar = qobject_cast<CustomTabBar*>(m_tabBar);
    if (customBar) {
        connect(customBar, &CustomTabBar::contextMenuRequested,
                this, &TabWidget::contextMenuRequested);
    }

    // 确保tabCloseRequested信号能正确转发
    // qDebug() << "TabWidget connections set up";
}

int TabWidget::addTab(QWidget *widget, const QString &label, const QString &toolTip)
{
    m_widgets.append(widget);
    m_labels.append(label);
    m_toolTips.append(toolTip);
    m_datas.append(QVariant());
    m_enabled.append(true);
    m_visible.append(true);
    m_icons.append(QIcon());

    m_stack->addWidget(widget);
    int index = m_tabBar->addTab(label);
    if (!toolTip.isEmpty()) {
        m_tabBar->setTabToolTip(index, toolTip);
    }

    // 如果是第一个标签，设置为当前
    if (m_currentIndex < 0 && m_widgets.size() == 1) {
        setCurrentIndex(0);
    }

    return index;
}

void TabWidget::insertTab(int index, QWidget *widget, const QString &label)
{
    if (index < 0 || index > m_widgets.size()) {
        index = m_widgets.size();
    }

    m_widgets.insert(index, widget);
    m_labels.insert(index, label);
    m_toolTips.insert(index, QString());
    m_datas.insert(index, QVariant());
    m_enabled.insert(index, true);
    m_visible.insert(index, true);
    m_icons.insert(index, QIcon());

    m_stack->insertWidget(index, widget);
    m_tabBar->insertTab(index, label);

    if (m_currentIndex < 0) {
        setCurrentIndex(0);
    }
}

void TabWidget::removeTab(int index)
{
    if (index < 0 || index >= m_widgets.size()) {
        return;
    }

    // 移除widget
    QWidget *widget = m_widgets.at(index);
    m_stack->removeWidget(widget);

    // 从列表中移除
    m_widgets.removeAt(index);
    m_labels.removeAt(index);
    m_toolTips.removeAt(index);
    m_datas.removeAt(index);
    m_enabled.removeAt(index);
    m_visible.removeAt(index);
    m_icons.removeAt(index);

    // 移除标签
    m_tabBar->removeTab(index);

    // 更新当前索引
    if (m_currentIndex == index) {
        if (m_widgets.isEmpty()) {
            m_currentIndex = -1;
        } else {
            setCurrentIndex(qMin(index, m_widgets.size() - 1));
        }
    } else if (m_currentIndex > index) {
        m_currentIndex--;
    }
}

void TabWidget::setTabText(int index, const QString &text)
{
    if (index >= 0 && index < m_labels.size()) {
        m_labels[index] = text;
        m_tabBar->setTabText(index, text);
    }
}

QString TabWidget::tabText(int index) const
{
    if (index >= 0 && index < m_labels.size()) {
        return m_labels.at(index);
    }
    return QString();
}

void TabWidget::setTabToolTip(int index, const QString &toolTip)
{
    if (index >= 0 && index < m_toolTips.size()) {
        m_toolTips[index] = toolTip;
        m_tabBar->setTabToolTip(index, toolTip);
    }
}

QString TabWidget::tabToolTip(int index) const
{
    if (index >= 0 && index < m_toolTips.size()) {
        return m_toolTips.at(index);
    }
    return QString();
}

int TabWidget::currentIndex() const
{
    return m_currentIndex;
}

void TabWidget::setCurrentIndex(int index)
{
    if (index >= 0 && index < m_widgets.size() && index != m_currentIndex) {
        m_currentIndex = index;
        m_stack->setCurrentIndex(index);
        m_tabBar->setCurrentIndex(index);
        emit currentChanged(index);
    }
}

QWidget* TabWidget::currentWidget() const
{
    if (m_currentIndex >= 0 && m_currentIndex < m_widgets.size()) {
        return m_widgets.at(m_currentIndex);
    }
    return nullptr;
}

void TabWidget::setCurrentWidget(QWidget *widget)
{
    int index = indexOf(widget);
    if (index >= 0) {
        setCurrentIndex(index);
    }
}

int TabWidget::count() const
{
    return m_widgets.size();
}

QWidget* TabWidget::widget(int index) const
{
    if (index >= 0 && index < m_widgets.size()) {
        return m_widgets.at(index);
    }
    return nullptr;
}

int TabWidget::indexOf(QWidget *widget) const
{
    return m_widgets.indexOf(widget);
}

void TabWidget::setTabEnabled(int index, bool enabled)
{
    if (index >= 0 && index < m_enabled.size()) {
        m_enabled[index] = enabled;
        m_tabBar->setTabEnabled(index, enabled);
    }
}

bool TabWidget::isTabEnabled(int index) const
{
    if (index >= 0 && index < m_enabled.size()) {
        return m_enabled.at(index);
    }
    return false;
}

void TabWidget::setTabVisible(int index, bool visible)
{
    if (index >= 0 && index < m_visible.size()) {
        m_visible[index] = visible;
        m_tabBar->setTabVisible(index, visible);
    }
}

bool TabWidget::isTabVisible(int index) const
{
    if (index >= 0 && index < m_visible.size()) {
        return m_visible.at(index);
    }
    return false;
}

void TabWidget::setTabIcon(int index, const QIcon &icon)
{
    if (index >= 0 && index < m_icons.size()) {
        m_icons[index] = icon;
        m_tabBar->setTabIcon(index, icon);
    }
}

QIcon TabWidget::tabIcon(int index) const
{
    if (index >= 0 && index < m_icons.size()) {
        return m_icons.at(index);
    }
    return QIcon();
}

void TabWidget::setTabData(int index, const QVariant &data)
{
    if (index >= 0 && index < m_datas.size()) {
        m_datas[index] = data;
    }
}

QVariant TabWidget::tabData(int index) const
{
    if (index >= 0 && index < m_datas.size()) {
        return m_datas.at(index);
    }
    return QVariant();
}

void TabWidget::setTabsClosable(bool closable)
{
    m_tabsClosable = closable;
    if (auto* customBar = qobject_cast<CustomTabBar*>(m_tabBar)) {
        customBar->setShowCloseButton(closable);
    }
}

bool TabWidget::tabsClosable() const
{
    return m_tabsClosable;
}

void TabWidget::setMovable(bool movable)
{
    m_movable = movable;
    m_tabBar->setMovable(movable);
}

bool TabWidget::isMovable() const
{
    return m_movable;
}

void TabWidget::setTabShape(QTabBar::Shape shape)
{
    m_shape = shape;
    m_tabBar->setShape(shape);
}

QTabBar::Shape TabWidget::tabShape() const
{
    return m_shape;
}

void TabWidget::updateTheme()
{
    if (m_tabBar) {
        m_tabBar->update();
    }
    // 更新当前widget的主题
    if (QWidget* widget = currentWidget()) {
        // 如果当前widget是TextEditor，它会通过MainWindow的applyThemeToTextEditors处理
        // 这里我们只需要确保widget本身更新
        widget->update();
    }

    // 强制重绘整个TabWidget
    update();
}

void TabWidget::onTabCloseRequested(int index)
{
    emit tabCloseRequested(index);
}

void TabWidget::onTabMoved(int from, int to)
{
    // 重新排列内部列表
    m_widgets.move(from, to);
    m_labels.move(from, to);
    m_toolTips.move(from, to);
    m_datas.move(from, to);
    m_enabled.move(from, to);
    m_visible.move(from, to);
    m_icons.move(from, to);

    // 更新堆栈顺序
    QWidget *widget = m_stack->widget(from);
    m_stack->removeWidget(widget);
    m_stack->insertWidget(to, widget);

    // 更新当前索引
    if (m_currentIndex == from) {
        m_currentIndex = to;
    } else if (m_currentIndex == to) {
        m_currentIndex = from;
    } else if (m_currentIndex > from && m_currentIndex <= to) {
        m_currentIndex--;
    } else if (m_currentIndex < from && m_currentIndex >= to) {
        m_currentIndex++;
    }

    emit tabMoved(from, to);
}

void TabWidget::onCurrentChanged(int index)
{
    if (index >= 0 && index < m_widgets.size()) {
        m_currentIndex = index;
        m_stack->setCurrentIndex(index);
        emit currentChanged(index);
    }
}

} // namespace SprintIDE

#include "tab_widget.moc"
