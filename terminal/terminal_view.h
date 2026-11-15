// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QAbstractScrollArea>
#include <QFont>
#include <QList>
#include <QPair>
#include <QString>
#include <QTimer>

class QDragEnterEvent;
class QDropEvent;

namespace SprintIDE {

class TerminalGridModel;
struct TerminalCell;

// ---------------------------------------------------------------------------
// TerminalView
//
//   终端渲染层。负责：
//     - 把 TerminalGridModel 的 2D grid 画到屏幕上
//     - 处理鼠标拖选、双击选词、三击选行、右键菜单
//     - 处理滚轮/键盘滚动 scrollback
//     - 光标闪烁
//     - 键盘按键转换为 PTY 字节序列（通过 inputRequested 信号发出）
//     - 搜索高亮（外部通过 setSearchQuery 传入）
//     - 超链接识别（URL / path:line:col），悬停下划线 + 手型
//
//   不负责：
//     - PTY 生命周期（由 TerminalPanel 管）
//     - 搜索栏 UI（由 TerminalSearchBar 管）
// ---------------------------------------------------------------------------
class TerminalView : public QAbstractScrollArea
{
    Q_OBJECT
public:
    explicit TerminalView(QWidget* parent = nullptr);
    ~TerminalView() override;

    void setModel(TerminalGridModel* model);
    TerminalGridModel* model() const;

    QString selectedText() const;
    bool    isScrolledBack() const;
    void    scrollToBottom();

    // 把当前选区复制到系统剪贴板（无选区时不做任何事）
    // 供 MainWindow::onCopy 在焦点在终端时调用
    void copySelectionToClipboard();

    // 把系统剪贴板内容粘贴到终端（换行自动转 CR）
    void pasteFromClipboard();

    // 跟随编辑器字体设置（family 为空时保留当前字体族）
    //   - 内部会重算 charW/charH，并同步 model 尺寸
    //   - TerminalPanel 负责在这个调用之后再 syncPtySize() 把尺寸传给 PTY
    void applyEditorFont(const QString& family, int pointSize);

    // 允许/禁止 resize（TerminalPanel 在 startShell 后才允许）
    void setResizeAllowed(bool allowed);

    // 供 TerminalPanel 计算初始 PTY 尺寸用
    int visibleRowCount() const;
    int visibleColCount() const;

    // 主题色（由 TerminalPanel 在主题切换时调用）
    // bg 由 model->defaultBg() 提供，不在这里设
    void setThemeColors(const QColor& selectionBg,
                        const QColor& matchBg,
                        const QColor& activeMatchBg,
                        const QColor& linkFg,
                        const QColor& cursorFg);

    // ---- 搜索 ----
    // 设置搜索词，自动重建匹配列表并跳到第一个匹配
    void setSearchQuery(const QString& query, bool caseSensitive = false);
    // 清除所有搜索高亮
    void clearSearch();
    void findNext();
    void findPrev();
    int  searchMatchCount() const;
    int  searchCurrentIndex() const;
    QString searchQuery() const;

signals:
    void inputRequested(const QByteArray& bytes);
    void selectionChanged();

    // 用户按回车提交了一条疑似 shell 命令（用于命令历史记录）
    //   - 全屏程序（vim/htop）活跃时不发出
    //   - 以 `:` `.` `~` `/` `$` 或字母开头的才视为命令
    void commandSubmitted(const QString& command);

    // 用户按下 Ctrl+F / Cmd+F 请求打开搜索栏
    void searchRequested();

    // 搜索状态变化（当前匹配 / 总数），供搜索栏更新显示
    void searchStateChanged(int current, int total);

    // 用户 Cmd/Ctrl+点击超链接
    //   link 可能是 URL、可能是 path:line:col
    void linkClicked(const QString& link);

    // 右键菜单里的"清屏"被点击
    void clearRequested();

    // 型尺寸变化（resize 完成后）
    // TerminalPanel 监听此信号，调 syncPtySize 通知 PTY 发 SIGWINCH
    void modelSizeChanged(int rows, int cols);

protected:
    void paintEvent(QPaintEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void focusInEvent(QFocusEvent* e) override;
    void focusOutEvent(QFocusEvent* e) override;
    void contextMenuEvent(QContextMenuEvent* e) override;
    void leaveEvent(QEvent* e) override;
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dropEvent(QDropEvent* e) override;

private slots:
    void onModelDirty(const QRect& cellRect);
    void onCursorChanged();
    void onBlinkTick();

private:
    struct CellPos {
        int row = -1;
        int col = -1;
        bool valid() const { return row >= 0 && col >= 0; }
        bool operator==(const CellPos& o) const { return row == o.row && col == o.col; }
        bool operator!=(const CellPos& o) const { return !(*this == o); }
    };

    // 匹配记录（contentRow + 起始 col + 长度）
    struct SearchMatch {
        int contentRow = 0;
        int col = 0;
        int len = 0;
    };

    // ---- 模型 ----
    TerminalGridModel* m_model = nullptr;

    // ---- 字体 ----
    QFont m_font;
    qreal m_charW = 8.0;
    qreal m_charH = 16.0;

    // resize debounce（拖动中合并，避免反复 setSize）
    QTimer* m_sizeDebounceTimer = nullptr;
    int     m_pendingRows       = 0;
    int     m_pendingCols       = 0;

    // 是否允许 resize（startShell 之后才允许）
    bool m_resizeAllowed = false;

    bool m_inResize = false;
    bool m_inUpdate = false;

    // 预创建的字体变体（避免每 cell 都创建 QFont，否则 paintEvent 慢几百倍）
    QFont m_fontNormal;
    QFont m_fontBold;
    QFont m_fontItalic;
    QFont m_fontBoldItalic;

    // ---- 主题色（由 TerminalPanel 设置） ----
    QColor m_selectionBg;
    QColor m_matchBg;
    QColor m_activeMatchBg;
    QColor m_linkFg;
    QColor m_cursorFg;

    // ---- 滚动 ----
    int m_scrollOffset = 0;

    // ---- 选区 ----
    CellPos m_selStart;         // 视觉起点（≤ m_selEnd）
    CellPos m_selEnd;           // 视觉终点（≥ m_selStart）
    CellPos m_selAnchor;        // 拖选锚点（mousePress 位置，整个拖选期间不变）
    bool    m_selecting = false;

    // ---- 光标闪烁 ----
    QTimer* m_blinkTimer = nullptr;
    bool    m_cursorBlinkOn = true;

    // ---- 命令历史捕获 ----
    // 累积从"上次回车"到"这次回车"之间的可打印输入，用于命令历史
    QString m_commandBuffer;

    // ---- 搜索 ----
    QString     m_searchQuery;
    bool        m_searchCaseSensitive = false;
    QList<SearchMatch> m_searchMatches;
    int         m_searchCurrentIndex = -1;

    // ---- 鼠标事件透传状态 ----
    bool m_mouseButtonPressed = false;    // 左键是否按下（区分拖动/单击）
    int  m_lastMouseButton    = -1;       // 上次按下的按钮（0/1/2）

    // ---- 超链接悬停 ----
    QString     m_hoveredLink;
    CellPos     m_hoveredLinkStart;
    CellPos     m_hoveredLinkEnd;

    // ---- 几何 ----
    void updateCharMetrics();
    QPoint cellPosToPixel(int row, int col) const;
    CellPos pixelToCellPos(const QPoint& p) const;

    int    visibleRowToContentRow(int visibleRow) const;
    int    contentRowToVisibleRow(int contentRow) const;
    QVector<TerminalCell> contentRowCells(int contentRow) const;

    // ---- 绘制 ----
    void paintCell(QPainter& p, int visibleRow, int col,
                   const TerminalCell& cell, bool selected);
    void paintCursor(QPainter& p);

    // 鼠标事件透传
    //   把 Qt 鼠标事件编码成 X10 / VT200 CSI 序列发给 PTY
    //
    //   button：
    //     0 = 左键, 1 = 中键, 2 = 右键
    //     3 = 释放
    //     4 = 滚轮上, 5 = 滚轮下（仅滚轮）
    //     +32 表示拖动（按住移动）
    //   修饰位：Shift = +4, Alt = +8, Ctrl = +16
    //
    //   col / row 是 0-based 单元格坐标，内部会 +1 +32 转换成终端协议格式
    void sendMouseToPty(int button, int col, int row);
    bool shouldForwardMouse(QMouseEvent* e) const;

    // ---- 选区 ----
    bool isCellSelected(int visibleRow, int col) const;
    bool isSelectionActive() const { return m_selStart.valid() && m_selEnd.valid()
                                         && m_selStart != m_selEnd; }
    void normalizeSelection();

    // ---- 搜索高亮 ----
    bool isCellInMatch(int visibleRow, int col) const;
    bool isCellInActiveMatch(int visibleRow, int col) const;
    void rebuildSearchMatches();
    void scrollToActiveMatch();

    // ---- 超链接 ----
    void updateHoveredLink(const QPoint& pos);
    QString detectLinkAt(int contentRow, int col) const;
    void openLink(const QString& link, bool altModifier) const;

    // ---- 命令历史捕获 ----
    void captureInputForHistory(const QByteArray& bytes);

    // ---- 滚动条 ----
    void updateScrollBarRange();
};

} // namespace SprintIDE