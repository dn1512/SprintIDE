#ifndef BREADCRUMB_BAR_H
#define BREADCRUMB_BAR_H

#include <QWidget>
#include <QList>

class QHBoxLayout;
class QToolButton;

namespace SprintIDE {

    struct OutlineSymbol;

    /**
    * @brief 编辑器上方的面包屑栏
    *
    *   [图标] main.cpp › class Foo › def bar()
    *
    * 特性：
    *  - 每级可点，弹出同级候选菜单
    *  - 光标移动时自动更新
    *  - 高度固定 24px，颜色全部走 ThemeManager
    */
    class BreadcrumbBar : public QWidget
    {
        Q_OBJECT
    public:
        explicit BreadcrumbBar(QWidget* parent = nullptr);

        /// 设置当前文件的符号树（来自 OutlineController）
        void setSymbols(const QList<OutlineSymbol>& symbols);

        /// 设置文件显示名（第一级面包屑）
        void setFileDisplayName(const QString& name);

        /// 光标移动时调用：line 是 1-based 行号
        void updateCurrentLine(int line);

        QSize sizeHint() const override;

    signals:
        /// 用户点击某一级 breadcrumb 或菜单中的某一项
        void symbolJumpRequested(int line);

    private:
        void rebuild();

        /// 计算给定行所在的"符号路径"（从根到最深命中）
        QList<const OutlineSymbol*> pathAtLine(int line) const;

        void addSeparator();
        void addBreadcrumb(const QString& text, const QString& tooltip,
            const QList<QPair<QString, int>>& menuItems,
            bool isActive);

        QHBoxLayout* m_layout = nullptr;
        QString      m_fileName;
        QList<OutlineSymbol> m_symbols;
        int          m_currentLine = 1;
    };

} // namespace SprintIDE

#endif