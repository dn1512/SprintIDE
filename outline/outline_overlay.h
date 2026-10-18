#ifndef OUTLINE_OVERLAY_H
#define OUTLINE_OVERLAY_H

#include <QDialog>
#include <QList>
#include "outline_types.h"

class QSplitter;
class QListWidget;
class QListWidgetItem;
class QLabel;

namespace SprintIDE {

    class OutlineItemDelegate;

    /**
    * @brief 大纲浮层：属性 / 方法双栏显示
    *
    * 显示时机：TextEditor 的 Ctrl/Cmd + ` 按住时调用 showAt()
    * 关闭时机：TextEditor 的释放事件调用 hideOverlay()
    *
    * 设计：
    *   - Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus
    *     —— 显示时不抢编辑器焦点，键盘事件仍由编辑器转发
    *   - 4:6 分栏，两列各自独立纵向滚动，无横向滚动
    *   - 文字超出自动折行，项间距 24px > 换行行距 ~15px
    *   - 颜色全部从 ThemeManager 取
    */
    class OutlineOverlay : public QDialog
    {
        Q_OBJECT
        public:
        explicit OutlineOverlay(QWidget* parent = nullptr);
        ~OutlineOverlay() override;

        /// 设置当前文件的符号树（每次显示前调用）
        void setSymbols(const QList<OutlineSymbol>& symbols);

        /// 显示在 globalPos（通常是光标位置）下方
        void showAt(const QPoint& globalPos);

        /// 关闭
        void hideOverlay();

        /// 鼠标是否在浮层上（用于松开快捷键时的延迟关闭判断）
        bool isMouseInside() const;

        // ---- 键盘导航（由 TextEditor 转发）----
        void selectNext();
        void selectPrev();
        void focusLeft();
        void focusRight();
        void activateCurrent();

        signals:
        /// 用户确认某一项（Enter 或 鼠标点击）→ 跳转到该行
        void symbolActivated(int line);

        private:
        void applyTheme();
        void rebuildLists();
        void appendSymbol(QListWidget* list,
            const OutlineSymbol& symbol,
            const QString& prefix);
        QListWidget* activeList() const { return m_activeList; }

        QList<OutlineSymbol> m_symbols;

        QLabel*      m_propTitle    = nullptr;
        QLabel*      m_methodTitle  = nullptr;
        QListWidget* m_propList     = nullptr;
        QListWidget* m_methodList   = nullptr;
        QSplitter*   m_splitter     = nullptr;
        QListWidget* m_activeList   = nullptr;

        OutlineItemDelegate* m_delegate = nullptr;
    };

} // namespace SprintIDE

#endif