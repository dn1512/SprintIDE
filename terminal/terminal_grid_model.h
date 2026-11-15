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

enum TerminalCellAttr : quint16 {
    AttrBold      = 1 << 0,
    AttrUnderline = 1 << 1,
    AttrItalic    = 1 << 2,
    AttrReverse   = 1 << 3,
    AttrBlink     = 1 << 4,
    AttrStrike    = 1 << 5,
    AttrFaint     = 1 << 6,
};

struct TerminalCell {
    QString text;
    QColor  fg;
    QColor  bg;
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

class TerminalGridModel : public QObject
{
    Q_OBJECT
public:
    explicit TerminalGridModel(QObject* parent = nullptr);
    ~TerminalGridModel() override;

    struct Impl;

    // ---- 尺寸 ----
    // ★ setSize 通过重建 vterm 实现，不调用 vterm_set_size（规避 libvterm 0.3.3 的 resize 死循环 bug）
    void setSize(int rows, int cols);
    int  rows() const;
    int  cols() const;

    // ---- 数据输入 ----
    void feed(const QByteArray& data);

    // ---- 默认配色 ----
    void   setDefaultColors(const QColor& fg, const QColor& bg);
    void   setAnsiPalette(const std::array<QColor, 16>& palette);
    QColor defaultFg() const;
    QColor defaultBg() const;

    // ---- 单元格访问 ----
    const TerminalCell& cellAt(int row, int col) const;

    // ---- Scrollback ----
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
    int mouseProtocol() const;

    // ---- 选区 / 文本 ----
    QString selectedText(int sr, int sc, int er, int ec) const;
    QString plainText() const;

    // ---- Dirty ----
    QRect takeDirtyRect();

signals:
    void cursorChanged();
    void bellRequested();
    void titleChanged(const QString& title);
    void altScreenChanged(bool active);
    void dirty(const QRect& cellRect);
    void defaultColorsChanged();
    void oscClipboardRequested(const QString& text);
    void mouseProtocolChanged(int protocol);

private:
    std::unique_ptr<Impl> m_impl;

    void refreshAllVisibleCells();

    void initVTerm();
};

} // namespace SprintIDE