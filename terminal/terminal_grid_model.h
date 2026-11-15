// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QColor>
#include <QRect>
#include <QVector>

#include <array>
#include <memory>

namespace SprintIDE {

// ---------------------------------------------------------------------------
// 终端单元格属性位（对应 ANSI SGR 的常见属性）
// ---------------------------------------------------------------------------
enum TerminalCellAttr : quint16 {
    AttrBold      = 1 << 0,
    AttrUnderline = 1 << 1,
    AttrItalic    = 1 << 2,
    AttrReverse   = 1 << 3,   // 前景背景互换
    AttrBlink     = 1 << 4,
    AttrStrike    = 1 << 5,
    AttrFaint     = 1 << 6,   // 半透明/灰色
};

// ---------------------------------------------------------------------------
// 一个终端单元格
//   - text  保存 1~2 个 codepoint（宽字符 / Emoji 组合可能占 2 格）
//   - width 1 = 窄字符，2 = 宽字符（占 2 列）
// ---------------------------------------------------------------------------
struct TerminalCell {
    QString text;
    QColor  fg;      // 运行时由 convertCell() 从 defaultFg / ansiPalette 填充
    QColor  bg;      // 同上
    quint16 attrs = 0;
    quint8  width = 1;

    bool operator==(const TerminalCell& o) const {
        return text == o.text && fg == o.fg && bg == o.bg
            && attrs == o.attrs && width == o.width;
    }
    bool operator!=(const TerminalCell& o) const { return !(*this == o); }
};

enum class TerminalCursorShape {
    Block,
    Underline,
    Bar,
};

// ---------------------------------------------------------------------------
// TerminalGridModel
//
//   终端 2D 网格数据模型。职责：
//     1. 内部持有一个 libvterm 实例，负责解析 ANSI 字节流
//     2. 把 VTermScreen 的状态映射到 TerminalCell 数组
//     3. 提供滚动缓冲（scrollback）
//     4. 提供光标位置、选区文本、dirty 矩形
//     5. 不依赖 Qt Widgets（只依赖 QtCore / QtGui 的颜色）
//
//   使用方式：
//     - feed(data) 喂原始字节
//     - cellAt(row, col) 读单元格
//     - takeDirtyRect() 拿到需要重绘的区域（供 TerminalView）
//     - 监听 dirty() 信号，最小化重绘
//
//   生命周期：
//     - 一个 TerminalGridModel 对应一个 PTY 会话
//     - 线程安全：只能在 GUI 线程使用（libvterm 不是线程安全的）
// ---------------------------------------------------------------------------
class TerminalGridModel : public QObject
{
    Q_OBJECT
public:
    explicit TerminalGridModel(QObject* parent = nullptr);
    ~TerminalGridModel() override;

    struct Impl;

    // ---- 尺寸 ----
    void setSize(int rows, int cols);
    int  rows() const;
    int  cols() const;

    // ---- 数据输入 ----
    void feed(const QByteArray& data);

    // ---- 默认配色（SGR 39/49 回退用） ----
    // 主题切换时由 TerminalPanel 调用。
    // 调用后会自动重新拉取 libvterm 里所有 cell（因为 TerminalCell 里
    // 存的是转换时的快照，改 defaultFg/Bg 不会自动同步已存在的 cell）。
    void   setDefaultColors(const QColor& fg, const QColor& bg);
    void   setAnsiPalette(const std::array<QColor, 16>& palette);
    QColor defaultFg() const;
    QColor defaultBg() const;

    // ---- 单元格访问（row/col 是可视区坐标，不含 scrollback） ----
    const TerminalCell& cellAt(int row, int col) const;

    // ---- Scrollback ----
    // count > 0 时，scrollbackLine(0) 是最近一条历史行（即屏幕最上方之上那一行）
    int scrollbackCount() const;
    QVector<TerminalCell> scrollbackLine(int index) const;

    // ---- 光标 ----
    int cursorRow() const;
    int cursorCol() const;
    bool cursorVisible() const;
    TerminalCursorShape cursorShape() const;

    // ---- 状态 ----
    bool isAltScreenActive() const;
    bool bracketedPasteEnabled() const;
    QString title() const;

    // 当前鼠标协议
    //   0 = 关闭（不转发）
    //   1 = X10（只转发按下，对应 \x1b[?9h）
    //   2 = VT200 按钮事件（按下+释放+拖动，对应 \x1b[?1000h / \x1b[?1002h）
    //   3 = VT200 任意事件（所有移动，对应 \x1b[?1003h）
    //   由 libvterm 的 VTERM_PROP_MOUSE 属性驱动
    int mouseProtocol() const;

    // ---- 选区 / 文本 ----
    // 从 (sr, sc) 到 (er, ec) 的文本（坐标是可视区坐标）
    // 如果 sr > er，会自动交换
    QString selectedText(int sr, int sc, int er, int ec) const;

    // 可视区全文（每行末尾追加 \n，供搜索用）
    QString plainText() const;

    // ---- Dirty ----
    // 返回自上次 take 之后累积的 dirty 矩形（单元格坐标系）
    // 返回后自动清空
    QRect takeDirtyRect();

signals:
    void cursorChanged();
    void bellRequested();
    void titleChanged(const QString& title);
    void altScreenChanged(bool active);
    void dirty(const QRect& cellRect);
    void defaultColorsChanged();

    // OSC 52：远端程序请求写本地剪贴板（base64 解码后的文本）
    // 安全提示：默认需要用户确认，由 TerminalPanel 弹框
    void oscClipboardRequested(const QString& text);

    // 鼠标协议变化（0/1/2/3）
    void mouseProtocolChanged(int protocol);

private:
    std::unique_ptr<Impl> m_impl;

    // 从 libvterm 重新拉取所有可视区的 cell（用于配色变化后刷新）
    void refreshAllVisibleCells();
};

} // namespace SprintIDE