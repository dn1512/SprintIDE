/*
* SprintIDE - SprintIDE is a cross-platform IDE built from scratch with Qt 6 and C++20. Its core capability is to provide AI Agents with an executable, verifiable code execution environment.
* Copyright (C) 2026  Liu Tengjun (刘腾骏)
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef TAB_WIDGET_H
#define TAB_WIDGET_H

#include <QWidget>
#include <QTabBar>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMenu>
#include <QMouseEvent>

namespace SprintIDE {

/**
 * @class TabWidget
 * @brief 自定义标签栏组件，模仿PyCharm风格的标签页
 *
 * 特点：
 * 1. 水平排列的标签，可以拖动重新排序
 * 2. 标签有关闭按钮
 * 3. 支持右键菜单（关闭、关闭其他、关闭所有等）
 * 4. 支持双击标签重命名
 * 5. 标签颜色跟随主题变化
 */
class TabWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TabWidget(QWidget *parent = nullptr);
    ~TabWidget();

    // 标签操作
    int addTab(QWidget *widget, const QString &label, const QString &toolTip = QString());
    void insertTab(int index, QWidget *widget, const QString &label);
    void removeTab(int index);
    void setTabText(int index, const QString &text);
    QString tabText(int index) const;
    void setTabToolTip(int index, const QString &toolTip);
    QString tabToolTip(int index) const;

    // 当前标签
    int currentIndex() const;
    void setCurrentIndex(int index);
    QWidget* currentWidget() const;
    void setCurrentWidget(QWidget *widget);

    // 标签数量
    int count() const;
    QWidget* widget(int index) const;
    int indexOf(QWidget *widget) const;

    // 标签状态
    void setTabEnabled(int index, bool enabled);
    bool isTabEnabled(int index) const;
    void setTabVisible(int index, bool visible);
    bool isTabVisible(int index) const;

    // 标签图标
    void setTabIcon(int index, const QIcon &icon);
    QIcon tabIcon(int index) const;

    // 标签数据
    void setTabData(int index, const QVariant &data);
    QVariant tabData(int index) const;

    // 标签样式
    void setTabsClosable(bool closable);
    bool tabsClosable() const;
    void setMovable(bool movable);
    bool isMovable() const;
    void setTabShape(QTabBar::Shape shape);
    QTabBar::Shape tabShape() const;
    void updateTheme();

signals:
    // 标签相关信号
    void currentChanged(int index);
    void tabCloseRequested(int index);
    void tabMoved(int from, int to);
    void tabBarClicked(int index);
    void tabBarDoubleClicked(int index);

    // 上下文菜单信号
    void contextMenuRequested(int index, const QPoint &pos);

private slots:
    void onTabCloseRequested(int index);
    void onTabMoved(int from, int to);
    void onCurrentChanged(int index);

private:
    // 初始化
    void setupUI();
    void setupConnections();

    // 标签管理
    QList<QWidget*> m_widgets;
    QList<QString> m_labels;
    QList<QString> m_toolTips;
    QList<QVariant> m_datas;
    QList<bool> m_enabled;
    QList<bool> m_visible;
    QList<QIcon> m_icons;

    // UI组件
    QTabBar *m_tabBar;
    QStackedWidget *m_stack;
    // QHBoxLayout *m_layout;

    // 配置
    bool m_tabsClosable;
    bool m_movable;
    QTabBar::Shape m_shape;

    // 当前索引
    int m_currentIndex;
};

} // namespace SprintIDE

#endif // TAB_WIDGET_H
