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
        , m_maxTabWidth(400)
    {
        setMouseTracking(true);
        setFocusPolicy(Qt::NoFocus);
        setDrawBase(false);
        setElideMode(Qt::ElideRight);
        setUsesScrollButtons(true);
        setExpanding(false);

        // 重要：禁用系统的关闭按钮
        setTabsClosable(true);

        // 设置布局策略，确保左对齐
        QSizePolicy policy = sizePolicy();
        policy.setHorizontalPolicy(QSizePolicy::Preferred);
        policy.setHorizontalStretch(0);
        setSizePolicy(policy);
    }
    // 设置标签最大宽度
    void setMaxTabWidth(int width) { m_maxTabWidth = width; }
    int maxTabWidth() const { return m_maxTabWidth; }
    void updateAppearance() {
        update();
    }

protected:
    // 重写tabSizeHint以限制标签宽度
    QSize tabSizeHint(int index) const override
    {
        QSize size = QTabBar::tabSizeHint(index);

        // 根据内容计算宽度
        QString text = tabText(index);
        QFontMetrics fm = fontMetrics();
        int textWidth = fm.horizontalAdvance(text) + 60; // 文字宽度 + 边距和关闭按钮

        // 限制宽度
        if (textWidth > m_maxTabWidth) {
            textWidth = m_maxTabWidth;
        }

        // 确保最小宽度
        if (textWidth < 100) {
            textWidth = 100;
        }

        size.setWidth(textWidth);
        return size;
    }

    // 重写sizeHint，让标签栏根据内容确定宽度
    QSize sizeHint() const override
    {
        int totalWidth = 0;
        for (int i = 0; i < count(); ++i) {
            totalWidth += tabSizeHint(i).width();
        }

        // 加上一些边距
        totalWidth += 5;

        QSize size = QTabBar::sizeHint();
        size.setWidth(totalWidth);
        return size;
    }

    // 重写minimumSizeHint
    QSize minimumSizeHint() const override
    {
        int totalWidth = 0;
        for (int i = 0; i < count(); ++i) {
            totalWidth += tabSizeHint(i).width();
        }

        // 加上一些边距
        totalWidth += 5;

        QSize size = QTabBar::minimumSizeHint();
        size.setWidth(totalWidth);
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

        // 绘制背景
        QStyleOptionTabBarBase option;
        option.initFrom(this);
        option.shape = shape();
        option.documentMode = documentMode();

        // 获取主题颜色
        ThemeManager& tm = ThemeManager::instance();
        QColor bgColor = tm.color(ThemeManager::UIElement::MainWindowBackground);
        // 使用ThemeManager的颜色而不是硬编码颜色
        QColor backgroundColor = tm.color(ThemeManager::UIElement::MainWindowBackground);
        QColor textColor = tm.color(ThemeManager::UIElement::TextColor);

        // 填充背景
        painter.fillRect(rect(), bgColor);

        // 绘制底部边框线（PyCharm风格）
        painter.setPen(bgColor.darker(120));
        painter.drawLine(0, height() - 1, width(), height() - 1);

        // 绘制每个标签
        for (int i = 0; i < count(); ++i) {
            paintTab(&painter, i);
        }

        // 绘制左侧起始位置的边框（让标签栏看起来更完整）
        painter.setPen(bgColor.darker(120));
        painter.drawLine(0, 0, 0, height());

        // 如果标签栏宽度小于窗口宽度，绘制右侧背景
        if (width() > sizeHint().width()) {
            QRect rightRect(sizeHint().width(), 0, width() - sizeHint().width(), height());
            painter.fillRect(rightRect, bgColor);
        }
    }

    void paintTab(QPainter *painter, int tabIndex)
    {
        QRect rect = tabRect(tabIndex);
        if (!rect.isValid()) return;

        // ========== PyCharm风格颜色方案 ==========
        ThemeManager& tm = ThemeManager::instance();

        // 基础颜色 - 确保有足够的对比度
        QColor bgColor = tm.color(ThemeManager::UIElement::MainWindowBackground);
        QColor baseTextColor = tm.color(ThemeManager::UIElement::TextColor);

        // 如果背景和文字颜色太接近，自动调整
        if (bgColor.lightness() > 128) {
            // 浅色背景：确保文字是深色的
            if (baseTextColor.lightness() > 128) {
                baseTextColor = QColor(30, 30, 30); // 深灰色
            }
        } else {
            // 深色背景：确保文字是浅色的
            if (baseTextColor.lightness() < 128) {
                baseTextColor = QColor(220, 220, 220); // 浅灰色
            }
        }

        // PyCharm风格的标签颜色
        bool isCurrent = (currentIndex() == tabIndex);
        bool isHovered = (m_hoveredTab == tabIndex);

        // 活动标签颜色（PyCharm风格）
        QColor activeTabBg;
        if (bgColor.lightness() > 128) {
            activeTabBg = QColor(255, 255, 255); // 浅色主题：白色背景
        } else {
            activeTabBg = QColor(50, 50, 60); // 深色主题：深灰色背景
        }

        QColor activeTabTextClr = baseTextColor;
        // 活动标签边框颜色
        QColor activeTabBorder;
        if (bgColor.lightness() > 128) {
            // 浅色主题：使用较亮的蓝色
            activeTabBorder = QColor(74, 144, 226); // #4A90E2
        } else {
            // 深色主题：使用较暗的蓝色
            activeTabBorder = QColor(75, 110, 175); // #4B6EAF
        }

        // 非活动标签颜色
        QColor inactiveTabBg;
        if (bgColor.lightness() > 128) {
            inactiveTabBg = bgColor.darker(110); // 浅色主题：稍微深一点
        } else {
            inactiveTabBg = bgColor.lighter(110); // 深色主题：稍微浅一点
        }

        QColor inactiveTabTextClr = baseTextColor;
        QColor inactiveTabBorder;
        if (bgColor.lightness() > 128) {
            inactiveTabBorder = QColor(200, 200, 200); // 浅色主题：浅灰色边框
        } else {
            inactiveTabBorder = QColor(80, 80, 80); // 深色主题：深灰色边框
        }

        // 悬停标签颜色
        QColor hoverTabBg;
        if (isCurrent) {
            hoverTabBg = activeTabBg;
        } else if (inactiveTabBg.lightness() > 128) {
            hoverTabBg = inactiveTabBg.darker(105); // 浅色主题：悬停时稍微深一点
        } else {
            hoverTabBg = inactiveTabBg.lighter(105); // 深色主题：悬停时稍微浅一点
        }

        // 根据状态选择颜色 - 使用不同的变量名避免冲突
        QColor currentTabBg, currentTextClr, currentTabBorder;
        if (isCurrent) {
            currentTabBg = activeTabBg;
            currentTextClr = activeTabTextClr;
            currentTabBorder = activeTabBorder;
        } else if (isHovered) {
            currentTabBg = hoverTabBg;
            currentTextClr = inactiveTabTextClr;
            currentTabBorder = inactiveTabBorder;
        } else {
            currentTabBg = inactiveTabBg;
            currentTextClr = inactiveTabTextClr;
            currentTabBorder = inactiveTabBorder;
        }

        // 绘制标签背景
        painter->fillRect(rect, currentTabBg);

        // 绘制边框 - PyCharm风格：活动标签底部蓝色边框，非活动标签有轻微边框
        painter->setPen(QPen(currentTabBorder, 1));

        if (isCurrent) {
            // 活动标签：底部蓝色边框，左右轻微边框
            painter->drawLine(rect.left(), rect.top(), rect.right(), rect.top());
            painter->drawLine(rect.left(), rect.top(), rect.left(), rect.bottom());
            painter->drawLine(rect.right(), rect.top(), rect.right(), rect.bottom());

            // 底部蓝色粗边框
            painter->setPen(QPen(activeTabBorder, 2));
            painter->drawLine(rect.left(), rect.bottom(), rect.right(), rect.bottom());
        } else {
            // 非活动标签：只有右侧边框
            painter->drawLine(rect.right(), rect.top() + 3, rect.right(), rect.bottom() - 3);

            // 顶部轻微边框（与标签栏背景分隔）
            painter->setPen(QPen(inactiveTabBorder.lighter(120), 1));
            painter->drawLine(rect.left(), rect.top(), rect.right(), rect.top());
        }

        // 绘制标签文本 - 获取标签文本，使用父类的tabText函数
        painter->setPen(currentTextClr);
        QString tabTextStr = QTabBar::tabText(tabIndex); // 改名并明确调用父类函数

        // 如果标签文本包含*号（表示修改），用不同颜色
        if (tabTextStr.endsWith("*")) {
            painter->setPen(QColor(255, 100, 100)); // 红色表示修改
            tabTextStr = tabTextStr.left(tabTextStr.length() - 1); // 移除*号，因为我们会特殊绘制
        }

        QRect textRect = rect;
        // textRect.adjust(12, 0, -30, 0); // 左边距12，右边留出关闭按钮位置
        textRect.adjust(20, 0, -20, 0);

        QFont font = painter->font();
        if (isCurrent) {
            font.setBold(true);
        }
        painter->setFont(font);

        // 绘制文字
        QString displayText = painter->fontMetrics().elidedText(tabTextStr, Qt::ElideRight, textRect.width());
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, displayText);

        // 如果文件已修改，在文本后面添加红色*
        if (QTabBar::tabText(tabIndex).endsWith("*")) { // 明确调用父类函数
            QRect starRect = textRect;
            starRect.setLeft(textRect.left() + painter->fontMetrics().horizontalAdvance(displayText) + 2);
            painter->setPen(QColor(255, 100, 100));
            painter->drawText(starRect, Qt::AlignVCenter | Qt::AlignLeft, "*");
        }

        // ========== 绘制关闭按钮 ==========
        // if (tabsClosable() || isHovered || isCurrent) {
        //     // PyCharm风格：只有悬停或活动标签才显示关闭按钮
        //     // 修改前：rect.top() + (rect.height() - 14) / 2
        //     // 修改后：使用更精确的垂直居中计算

        //     int closeButtonSize = 12; // 关闭按钮大小
        //     int closeButtonTop = rect.top() + (rect.height() - closeButtonSize) / 2; // 垂直居中

        //     // 关闭按钮矩形
        //     QRect closeRect(rect.right() - 22, closeButtonTop, closeButtonSize, closeButtonSize);

        //     // 绘制关闭按钮背景（圆形）
        //     QColor closeBg;
        //     if (m_hoveredCloseTab == tabIndex) {
        //         closeBg = QColor(200, 80, 80); // 悬停时红色背景
        //     } else {
        //         closeBg = Qt::transparent; // 非悬停时透明
        //     }

        //     painter->setBrush(closeBg);
        //     painter->setPen(Qt::NoPen);

        //     // 使用圆形绘制，确保圆心在矩形的中心
        //     QPoint center = closeRect.center();
        //     painter->drawEllipse(center, closeButtonSize/2, closeButtonSize/2); // 以中心点为圆心

        //     // 绘制X符号 - 确保关闭按钮可见
        //     QColor closeSymbolColor;
        //     if (m_hoveredCloseTab == tabIndex) {
        //         closeSymbolColor = Qt::white; // 悬停时白色X
        //     } else if (isCurrent) {
        //         closeSymbolColor = currentTextClr; // 活动标签使用文字颜色
        //     } else {
        //         // 根据背景亮度决定X的颜色
        //         if (currentTabBg.lightness() > 128) {
        //             closeSymbolColor = QColor(120, 120, 120); // 浅色背景：深灰色X
        //         } else {
        //             closeSymbolColor = QColor(180, 180, 180); // 深色背景：浅灰色X
        //         }
        //     }

        //     painter->setPen(QPen(closeSymbolColor, 2));
        //     painter->setBrush(Qt::NoBrush);
        //     int lineLength = closeButtonSize / 2 - 2; // 线的长度

        //     // 从中心点向左上、右下画线
        //     painter->drawLine(center - QPoint(lineLength, lineLength),
        //                       center + QPoint(lineLength, lineLength));
        //     // 从中心点向右上、左下画线
        //     painter->drawLine(center + QPoint(lineLength, -lineLength),
        //                       center - QPoint(lineLength, -lineLength));
        // }
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        // if (event->button() == Qt::LeftButton && tabsClosable()) {
        //     int tabIndex = tabAt(event->pos());
        //     if (tabIndex >= 0) {
        //         QRect tabRect = this->tabRect(tabIndex);

        //         // 计算关闭按钮区域（与paintTab中保持一致）
        //         int closeButtonSize = 12;
        //         int closeButtonTop = tabRect.top() + (tabRect.height() - closeButtonSize) / 2;

        //         // 计算圆心位置
        //         QPoint center(tabRect.right() - 22 + closeButtonSize/2,
        //                       closeButtonTop + closeButtonSize/2);

        //         // 计算点到圆心的距离，判断是否在圆形区域内
        //         int distanceX = event->pos().x() - center.x();
        //         int distanceY = event->pos().y() - center.y();
        //         int distanceSquared = distanceX*distanceX + distanceY*distanceY;

        //         // 检查是否点击了圆形区域内（半径平方）
        //         int radiusSquared = (closeButtonSize/2) * (closeButtonSize/2);

        //         if (distanceSquared <= radiusSquared) {
        //             emit tabCloseRequested(tabIndex);
        //             return; // 直接返回，不进行标签切换
        //         }
        //     }
        // }

        // 如果不是点击关闭按钮，调用父类处理（用于标签切换等）
        QTabBar::mousePressEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent *event) override
    {
        m_pressedTab = -1;
        QTabBar::mouseReleaseEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        int oldHoveredTab = m_hoveredTab;
        int oldHoveredCloseTab = m_hoveredCloseTab;

        m_hoveredTab = tabAt(event->pos());
        m_hoveredCloseTab = -1;

        // 检查是否悬停在关闭按钮上
        // if (m_hoveredTab >= 0) {
        //     QRect tabRect = this->tabRect(m_hoveredTab);

        //     // 计算关闭按钮区域（与paintTab中保持一致）
        //     int closeButtonSize = 12;
        //     int closeButtonTop = tabRect.top() + (tabRect.height() - closeButtonSize) / 2;

        //     // 计算圆心位置
        //     QPoint center(tabRect.right() - 22 + closeButtonSize/2,
        //                   closeButtonTop + closeButtonSize/2);

        //     // 计算点到圆心的距离，判断是否在圆形区域内
        //     int distanceX = event->pos().x() - center.x();
        //     int distanceY = event->pos().y() - center.y();
        //     int distanceSquared = distanceX*distanceX + distanceY*distanceY;

        //     // 检查是否在圆形区域内（半径平方）
        //     int radiusSquared = (closeButtonSize/2) * (closeButtonSize/2);

        //     if (distanceSquared <= radiusSquared) {
        //         m_hoveredCloseTab = m_hoveredTab;
        //     }
        // }

        // 只有当悬停状态改变时才更新
        // if (oldHoveredTab != m_hoveredTab || oldHoveredCloseTab != m_hoveredCloseTab) {
        //     update();
        // }
        if (oldHoveredTab != m_hoveredTab) {
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
    m_tabBar->setTabsClosable(m_tabsClosable);

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
    QSpacerItem *spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    tabBarLayout->addSpacerItem(spacer);

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
    qDebug() << "TabWidget connections set up";
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
    m_tabBar->setTabsClosable(closable);
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
