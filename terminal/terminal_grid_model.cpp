#include "terminal_grid_model.h"

#include <vterm.h>

#include <QDebug>

#include <cstring>

namespace SprintIDE {

// ============================================================================
// Impl：所有 libvterm 相关状态 + 2D grid 数据
//
//   用 PIMPL 隐藏 vterm.h，头文件不依赖 libvterm。
// ============================================================================
struct TerminalGridModel::Impl
{
    // ---- libvterm ----
    VTerm*        vterm  = nullptr;
    VTermScreen*  screen = nullptr;
    VTermState*   state  = nullptr;

    // ---- 2D grid ----
    QVector<QVector<TerminalCell>> cells;
    QVector<QVector<TerminalCell>> scrollback;

    static constexpr int kScrollbackLimit = 1000;

    int rows = 24;
    int cols = 80;

    // ---- 光标 ----
    int cursorRow = 0;
    int cursorCol = 0;
    bool cursorVisible = true;
    TerminalCursorShape cursorShape = TerminalCursorShape::Block;

    // ---- 状态 ----
    bool altScreenActive = false;
    bool bracketedPaste  = false;
    QString title;
    QString lastEmittedTitle;   // 上次 emit 过的 title（供 feed 检测变化）

    // 鼠标协议
    int mouseProtocol = 0;         // 0=关 1=X10 2=VT200按钮 3=VT200任意
    int lastEmittedMouseProtocol = 0;

    // ---- Dirty ----
    QRect dirtyRect;   // 空 = 无变化

    // ---- 主题（默认色，SGR 39/49 时回退到这里） ----
    QColor defaultFg;
    QColor defaultBg;

    // ---- ANSI 16 色调色板 ----
    //   未注入时元素全为 invalid QColor，convertCell() 对 idx<16 回退 defaultFg/defaultBg。
    //   色相 / 顺序仍严格遵循 ANSI SGR 30-37 / 90-97（外部注入时必须按此顺序）。
    std::array<QColor, 16> ansiPalette{};

    // ---- OSC 52 跨 chunk 状态 ----
    //   OSC 52 序列可能跨 chunk 到达，必须累积到终结符（BEL 或 ST）才处理
    bool        inOsc52     = false;
    QByteArray  osc52Buffer;

    // 把 libvterm 的 cell 转成我们的 TerminalCell
    TerminalCell convertCell(const VTermScreenCell& vc) const;
};

// ----------------------------------------------------------------------------
// 转换 libvterm 单元格 → TerminalCell
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// 转换 libvterm 单元格 → TerminalCell
// ----------------------------------------------------------------------------
TerminalCell TerminalGridModel::Impl::convertCell(const VTermScreenCell& vc) const
{
    TerminalCell out;

    uint count = 0;
    while (count < VTERM_MAX_CHARS_PER_CELL && vc.chars[count] != 0) {
        ++count;
    }
    if (count > 0) {
        out.text = QString::fromUcs4(
            reinterpret_cast<const char32_t*>(vc.chars), int(count));
    } else {
        out.text.clear();
    }
    out.width = vc.width > 0 ? quint8(vc.width) : quint8(1);

    // 前景色
    const VTermColor& fg = vc.fg;
    if (VTERM_COLOR_IS_DEFAULT_FG(&fg)) {
        out.fg = defaultFg;                  // 可能 invalid，paintEvent 兜底
    } else if (VTERM_COLOR_IS_INDEXED(&fg)) {
        int idx = fg.indexed.idx;
        if (idx < 16) {
            out.fg = ansiPalette[idx];
        } else if (idx < 232) {
            int r = (idx - 16) / 36;
            int g = ((idx - 16) % 36) / 6;
            int b = (idx - 16) % 6;
            out.fg = QColor(r * 51, g * 51, b * 51);   // xterm 6×6×6 标准算法
        } else {
            int v = (idx - 232) * 11;
            out.fg = QColor(v, v, v);                   // xterm 灰阶标准算法
        }
    } else {
        out.fg = QColor(fg.rgb.red, fg.rgb.green, fg.rgb.blue);   // TrueColor：程序指定
    }

    // 背景色
    const VTermColor& bg = vc.bg;
    if (VTERM_COLOR_IS_DEFAULT_BG(&bg)) {
        out.bg = defaultBg;
    } else if (VTERM_COLOR_IS_INDEXED(&bg)) {
        int idx = bg.indexed.idx;
        if (idx < 16) {
            out.bg = ansiPalette[idx];
        } else if (idx < 232) {
            int r = (idx - 16) / 36;
            int g = ((idx - 16) % 36) / 6;
            int b = (idx - 16) % 6;
            out.bg = QColor(r * 51, g * 51, b * 51);
        } else {
            int v = (idx - 232) * 11;
            out.bg = QColor(v, v, v);
        }
    } else {
        out.bg = QColor(bg.rgb.red, bg.rgb.green, bg.rgb.blue);
    }

    // 属性
    quint16 attrs = 0;
    if (vc.attrs.bold)      attrs |= AttrBold;
    if (vc.attrs.underline) attrs |= AttrUnderline;
    if (vc.attrs.italic)    attrs |= AttrItalic;
    if (vc.attrs.reverse)   attrs |= AttrReverse;
    if (vc.attrs.blink)     attrs |= AttrBlink;
    if (vc.attrs.strike)    attrs |= AttrStrike;
    if (vc.attrs.font == 1) attrs |= AttrFaint;
    out.attrs = attrs;

    return out;
}

// ============================================================================
// libvterm 静态回调
//
//   通过 vterm_screen_get_user_data() 拿回 Impl*，再分发。
// ============================================================================
namespace {

// 把 VTermRect 转成 QRect（row/col 坐标系）
QRect fromVTermRect(VTermRect r)
{
    return QRect(r.start_col, r.start_row,
                 r.end_col - r.start_col,
                 r.end_row - r.start_row);
}

// 统一从 libvterm 的 user pointer 拿 Impl*
TerminalGridModel::Impl* implFromUser(void* user)
{
    return static_cast<TerminalGridModel::Impl*>(user);
}

int cbDamage(VTermRect rect, void* user)
{
    auto* impl = implFromUser(user);
    if (!impl) return 0;

    // 防御：cells 数组尺寸不匹配时忽略本次 damage
    if (impl->cells.size() != impl->rows) return 1;

    QRect dr = fromVTermRect(rect);

    // 更新单元格数组
    for (int row = dr.top(); row <= dr.bottom(); ++row) {
        if (row < 0 || row >= impl->rows) continue;
        if (row >= impl->cells.size()) continue;
        if (impl->cells[row].size() != impl->cols) continue;

        for (int col = dr.left(); col <= dr.right(); ++col) {
            if (col < 0 || col >= impl->cols) continue;
            VTermScreenCell vc;
            vterm_screen_get_cell(impl->screen, { row, col }, &vc);
            impl->cells[row][col] = impl->convertCell(vc);
        }
    }

    // 累积 dirty
    if (impl->dirtyRect.isEmpty()) {
        impl->dirtyRect = dr;
    } else {
        impl->dirtyRect = impl->dirtyRect.united(dr);
    }

    return 1;
}

int cbMoveCursor(VTermPos pos, VTermPos /*oldpos*/, int visible, void* user)
{
    auto* impl = implFromUser(user);
    if (!impl) return 0;
    impl->cursorRow     = pos.row;
    impl->cursorCol     = pos.col;
    impl->cursorVisible = visible != 0;
    return 1;
}

int cbSetTermProp(VTermProp prop, VTermValue* val, void* user)
{
    auto* impl = implFromUser(user);
    if (!impl) return 0;

    switch (prop) {
    case VTERM_PROP_CURSORVISIBLE:
        impl->cursorVisible = val->boolean != 0;
        break;
    case VTERM_PROP_CURSORSHAPE:
        switch (val->number) {
        case VTERM_PROP_CURSORSHAPE_BLOCK:     impl->cursorShape = TerminalCursorShape::Block;     break;
        case VTERM_PROP_CURSORSHAPE_UNDERLINE: impl->cursorShape = TerminalCursorShape::Underline; break;
        case VTERM_PROP_CURSORSHAPE_BAR_LEFT:  impl->cursorShape = TerminalCursorShape::Bar;       break;
        }
        break;
    case VTERM_PROP_ALTSCREEN:
        impl->altScreenActive = val->boolean != 0;
        break;
    case VTERM_PROP_TITLE: {
        // libvterm 0.3.x：val->string 是 VTermStringFragment
        const VTermStringFragment& frag = val->string;
        if (frag.len > 0) {
            impl->title = QString::fromUtf8(frag.str, int(frag.len));
        } else {
            impl->title.clear();
        }
        break;
    }
    case VTERM_PROP_MOUSE: {
        //   val->number 是 VTermMouseProtocol 枚举
        //   VTERM_PROP_MOUSE_NONE = 0
        //   VTERM_PROP_MOUSE_CLICK = 1
        //   VTERM_PROP_MOUSE_DRAG = 2
        //   VTERM_PROP_MOUSE_MOVE = 3
        impl->mouseProtocol = val->number;
        break;
    }
    default:
        break;
    }
    return 1;
}

int cbBell(void* /*user*/)
{
    // 通知由外层去 emit（这里只返回 true 表示已处理）
    return 1;
}

int cbResize(int rows, int cols, void* /*user*/)
{
    // 实际 resize 逻辑在 TerminalGridModel::setSize 里做
    Q_UNUSED(rows); Q_UNUSED(cols);
    return 1;
}

int cbSbPushLine(int cols, const VTermScreenCell* cells, void* user)
{
    auto* impl = implFromUser(user);
    if (!impl) return 0;

    QVector<TerminalCell> line;
    line.reserve(cols);
    for (int i = 0; i < cols; ++i) {
        line.append(impl->convertCell(cells[i]));
    }
    impl->scrollback.append(line);

    // 超过上限时丢弃最旧
    while (impl->scrollback.size() > impl->kScrollbackLimit) {
        impl->scrollback.removeFirst();
    }
    return 1;
}

int cbSbPopLine(int /*cols*/, VTermScreenCell* /*cells*/, void* /*user*/)
{
    return 1;   // 向上滚动恢复历史：暂不支持
}

int cbSbClear(void* user)
{
    auto* impl = implFromUser(user);
    if (impl) impl->scrollback.clear();
    return 1;
}

} // namespace

// ============================================================================
// TerminalGridModel 实现
// ============================================================================
TerminalGridModel::TerminalGridModel(QObject* parent)
    : QObject(parent)
    , m_impl(std::make_unique<Impl>())
{
    // 初始尺寸 80x24
    m_impl->rows = 24;
    m_impl->cols = 80;

    // cells 必须在 vterm_screen_reset() 之前初始化
    m_impl->cells.resize(m_impl->rows);
    for (int r = 0; r < m_impl->rows; ++r) {
        m_impl->cells[r].resize(m_impl->cols);
        for (int c = 0; c < m_impl->cols; ++c) {
            m_impl->cells[r][c].fg = m_impl->defaultFg;
            m_impl->cells[r][c].bg = m_impl->defaultBg;
        }
    }

    initVTerm();
}

// ★ 初始化 / 重建 libvterm
//   构造时调用一次；setSize() 每次重建时也调用
void TerminalGridModel::initVTerm()
{
    m_impl->vterm = vterm_new(m_impl->rows, m_impl->cols);
    vterm_set_utf8(m_impl->vterm, 1);

    m_impl->state  = vterm_obtain_state(m_impl->vterm);
    m_impl->screen = vterm_obtain_screen(m_impl->vterm);

    static VTermScreenCallbacks kCallbacks = {
        .damage      = cbDamage,
        .movecursor  = cbMoveCursor,
        .settermprop = cbSetTermProp,
        .bell        = cbBell,
        .resize      = cbResize,
        .sb_pushline = cbSbPushLine,
        .sb_popline  = cbSbPopLine,
        .sb_clear    = cbSbClear,
    };
    vterm_screen_set_callbacks(m_impl->screen, &kCallbacks, m_impl.get());
    vterm_screen_set_damage_merge(m_impl->screen, VTERM_DAMAGE_SCROLL);

    // 此时 cells 已就绪，reset 触发的 damage 回调安全
    vterm_screen_reset(m_impl->screen, 1);
    vterm_screen_enable_altscreen(m_impl->screen, 1);
}

TerminalGridModel::~TerminalGridModel()
{
    if (m_impl && m_impl->vterm) {
        vterm_free(m_impl->vterm);
        m_impl->vterm = nullptr;
    }
}

// ----------------------------------------------------------------------------
// 尺寸
// ----------------------------------------------------------------------------
void TerminalGridModel::setSize(int rows, int cols)
{
    rows = qBound(5, rows, 100);
    cols = qBound(20, cols, 500);

    if (rows == m_impl->rows && cols == m_impl->cols) return;

    // 1. 保留最近 1000 行 scrollback
    QVector<QVector<TerminalCell>> savedScrollback;
    if (!m_impl->scrollback.isEmpty()) {
        const int keep = qMin(m_impl->scrollback.size(), 1000);
        savedScrollback = m_impl->scrollback.mid(m_impl->scrollback.size() - keep);
    }

    // 2. 销毁旧 vterm
    if (m_impl->vterm) {
        vterm_free(m_impl->vterm);
        m_impl->vterm  = nullptr;
        m_impl->screen = nullptr;
        m_impl->state  = nullptr;
    }

    // 3. 更新尺寸，重建 cells（全空白）
    m_impl->rows = rows;
    m_impl->cols = cols;
    m_impl->cells.clear();
    m_impl->cells.resize(rows);
    for (int r = 0; r < rows; ++r) {
        m_impl->cells[r].resize(cols);
        for (int c = 0; c < cols; ++c) {
            m_impl->cells[r][c].fg = m_impl->defaultFg;
            m_impl->cells[r][c].bg = m_impl->defaultBg;
        }
    }

    // 4. 重置状态
    m_impl->cursorRow     = 0;
    m_impl->cursorCol     = 0;
    m_impl->cursorVisible = true;
    m_impl->altScreenActive = false;
    m_impl->bracketedPaste  = false;
    m_impl->mouseProtocol   = 0;
    m_impl->title.clear();
    m_impl->inOsc52     = false;
    m_impl->osc52Buffer.clear();

    // 5. 重建 vterm
    initVTerm();

    // 6. 恢复 scrollback
    m_impl->scrollback = savedScrollback;

    // 7. 全屏 dirty
    m_impl->dirtyRect = QRect(0, 0, cols, rows);
    emit dirty(m_impl->dirtyRect);
}

int TerminalGridModel::rows() const { return m_impl->rows; }
int TerminalGridModel::cols() const { return m_impl->cols; }

// ----------------------------------------------------------------------------
// 数据输入
// ----------------------------------------------------------------------------
void TerminalGridModel::feed(const QByteArray& data)
{
    if (!m_impl->vterm || data.isEmpty()) return;

    // ============================================================
    // 前置扫描：剥离 OSC 52（剪贴板）序列
    //
    //   OSC 52 格式：
    //     ESC ] 52 ; <targets> ; <base64> BEL          (0x07)
    //     ESC ] 52 ; <targets> ; <base64> ESC \        (ST)
    //
    //   libvterm 0.3.x 不提供 OSC 回调，所以在喂之前自己剥。
    //   剥出去的字节不喂 libvterm，避免影响显示。
    // ============================================================
    QByteArray filtered;
    filtered.reserve(data.size());

    for (int i = 0; i < data.size(); ++i) {
        const char c = data.at(i);

        // ---- 进入 OSC 52 ----
        if (!m_impl->inOsc52) {
            // 检测 ESC ] 5 2 ;
            if (c == '\x1b' && i + 4 < data.size()
                && data.at(i + 1) == ']'
                && data.at(i + 2) == '5'
                && data.at(i + 3) == '2'
                && data.at(i + 4) == ';') {
                m_impl->inOsc52 = true;
                m_impl->osc52Buffer.clear();
                i += 4;   // 跳过 "\x1b]52;"
                continue;
            }
            filtered.append(c);
            continue;
        }

        // ---- 在 OSC 52 中 ----
        // BEL (0x07) 终止
        if (c == '\x07') {
            const QString text = QString::fromUtf8(
                QByteArray::fromBase64(m_impl->osc52Buffer));
            if (!text.isEmpty()) {
                emit oscClipboardRequested(text);
            }
            m_impl->inOsc52 = false;
            m_impl->osc52Buffer.clear();
            continue;
        }

        // ESC \ (ST = String Terminator) 终止
        if (c == '\x1b' && i + 1 < data.size() && data.at(i + 1) == '\\') {
            const QString text = QString::fromUtf8(
                QByteArray::fromBase64(m_impl->osc52Buffer));
            if (!text.isEmpty()) {
                emit oscClipboardRequested(text);
            }
            m_impl->inOsc52 = false;
            m_impl->osc52Buffer.clear();
            i += 1;   // 跳过 '\\'
            continue;
        }

        // 其他字符：累积到 osc52Buffer
        m_impl->osc52Buffer.append(c);
    }

    {
        const int hIdx = filtered.lastIndexOf("\x1b[?2004h");
        const int lIdx = filtered.lastIndexOf("\x1b[?2004l");
        if (hIdx >= 0 || lIdx >= 0) {
            m_impl->bracketedPaste = (hIdx > lIdx);
        }
    }

    // 剩余的字节才喂 libvterm
    if (!filtered.isEmpty()) {
        vterm_input_write(m_impl->vterm,
                          filtered.constData(),
                          size_t(filtered.size()));
        vterm_screen_flush_damage(m_impl->screen);
    }

    // 光标变化 → 通知
    emit cursorChanged();

    // dirty 变化 → 通知
    if (!m_impl->dirtyRect.isEmpty()) {
        QRect r = m_impl->dirtyRect;
        emit dirty(r);
    }

    // title 变化检测
    if (m_impl->title != m_impl->lastEmittedTitle) {
        m_impl->lastEmittedTitle = m_impl->title;
        emit titleChanged(m_impl->title);
    }

    // 鼠标协议变化检测
    if (m_impl->mouseProtocol != m_impl->lastEmittedMouseProtocol) {
        m_impl->lastEmittedMouseProtocol = m_impl->mouseProtocol;
        emit mouseProtocolChanged(m_impl->mouseProtocol);
    }
}

// ----------------------------------------------------------------------------
// 单元格访问
// ----------------------------------------------------------------------------
const TerminalCell& TerminalGridModel::cellAt(int row, int col) const
{
    static const TerminalCell empty;
    if (row < 0 || row >= m_impl->rows || col < 0 || col >= m_impl->cols) {
        return empty;
    }
    return m_impl->cells[row][col];
}

// ----------------------------------------------------------------------------
// Scrollback
// ----------------------------------------------------------------------------
int TerminalGridModel::scrollbackCount() const
{
    return m_impl->scrollback.size();
}

QVector<TerminalCell> TerminalGridModel::scrollbackLine(int index) const
{
    if (index < 0 || index >= m_impl->scrollback.size()) {
        return {};
    }
    // index 0 = 最近一条
    return m_impl->scrollback[m_impl->scrollback.size() - 1 - index];
}

// ----------------------------------------------------------------------------
// 光标 / 状态
// ----------------------------------------------------------------------------
int  TerminalGridModel::cursorRow() const { return m_impl->cursorRow; }
int  TerminalGridModel::cursorCol() const { return m_impl->cursorCol; }
bool TerminalGridModel::cursorVisible() const { return m_impl->cursorVisible; }
TerminalCursorShape TerminalGridModel::cursorShape() const { return m_impl->cursorShape; }
bool TerminalGridModel::isAltScreenActive() const { return m_impl->altScreenActive; }
bool TerminalGridModel::bracketedPasteEnabled() const { return m_impl->bracketedPaste; }
QString TerminalGridModel::title() const { return m_impl->title; }

int TerminalGridModel::mouseProtocol() const
{
    return m_impl->mouseProtocol;
}

// ----------------------------------------------------------------------------
// 选区文本
// ----------------------------------------------------------------------------
QString TerminalGridModel::selectedText(int sr, int sc, int er, int ec) const
{
    if (m_impl->rows == 0) return {};

    // 确保 sr <= er
    if (sr > er) { std::swap(sr, er); std::swap(sc, ec); }

    sr = qBound(0, sr, m_impl->rows - 1);
    er = qBound(0, er, m_impl->rows - 1);

    QString out;
    for (int r = sr; r <= er; ++r) {
        int startCol = (r == sr) ? qBound(0, sc, m_impl->cols - 1) : 0;
        int endCol   = (r == er) ? qBound(0, ec, m_impl->cols - 1) : m_impl->cols - 1;

        for (int c = startCol; c <= endCol; ++c) {
            const TerminalCell& cell = m_impl->cells[r][c];
            if (cell.text.isEmpty()) {
                out.append(QStringLiteral(" "));
            } else {
                out.append(cell.text);
            }
        }
        if (r < er) out.append(QLatin1Char('\n'));
    }
    return out;
}

// ----------------------------------------------------------------------------
// 全文
// ----------------------------------------------------------------------------
QString TerminalGridModel::plainText() const
{
    QString out;
    for (int r = 0; r < m_impl->rows; ++r) {
        for (int c = 0; c < m_impl->cols; ++c) {
            const auto& cell = m_impl->cells[r][c];
            out.append(cell.text.isEmpty() ? QStringLiteral(" ") : cell.text);
        }
        out.append(QLatin1Char('\n'));
    }
    return out;
}

// ----------------------------------------------------------------------------
// Dirty
// ----------------------------------------------------------------------------
QRect TerminalGridModel::takeDirtyRect()
{
    QRect r = m_impl->dirtyRect;
    m_impl->dirtyRect = QRect();
    return r;
}

// ----------------------------------------------------------------------------
// 默认配色
// ----------------------------------------------------------------------------
void TerminalGridModel::setDefaultColors(const QColor& fg, const QColor& bg)
{
    if (m_impl->defaultFg == fg && m_impl->defaultBg == bg) return;

    m_impl->defaultFg = fg;
    m_impl->defaultBg = bg;

    // 关键：TerminalCell 里存的是转换时的 fg/bg 快照，
    // 改 defaultFg/Bg 不会自动更新已存在的 cell。
    // 必须重新从 libvterm 拉一遍，把 SGR 39/49 的 cell 换成新色。
    refreshAllVisibleCells();

    // 通知 View 全屏重绘
    emit dirty(QRect(0, 0, m_impl->cols, m_impl->rows));
    emit defaultColorsChanged();
}

void TerminalGridModel::setAnsiPalette(const std::array<QColor, 16>& palette)
{
    if (m_impl->ansiPalette == palette) return;
    m_impl->ansiPalette = palette;
    refreshAllVisibleCells();
    emit dirty(QRect(0, 0, m_impl->cols, m_impl->rows));
}

QColor TerminalGridModel::defaultFg() const { return m_impl->defaultFg; }
QColor TerminalGridModel::defaultBg() const { return m_impl->defaultBg; }

// ----------------------------------------------------------------------------
// 从 libvterm 重新拉取所有可视区 cell
//
// 只刷可视区（rows × cols），不刷 scrollback：
//   - scrollback 可能有 10000 行，全刷有成本
//   - 用户看历史内容的频率低，配色不跟随主题可以接受
// ----------------------------------------------------------------------------
void TerminalGridModel::refreshAllVisibleCells()
{
    if (!m_impl->screen) return;

    for (int r = 0; r < m_impl->rows; ++r) {
        for (int c = 0; c < m_impl->cols; ++c) {
            VTermScreenCell vc;
            vterm_screen_get_cell(m_impl->screen, { r, c }, &vc);
            m_impl->cells[r][c] = m_impl->convertCell(vc);
        }
    }
}

} // namespace SprintIDE