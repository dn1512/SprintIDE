// SPDX-License-Identifier: GPL-3.0-or-later
#include "terminal_view.h"
#include "terminal_grid_model.h"

#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFontMetricsF>
#include <QKeyEvent>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTimer>
#include <QUrl>
#include <QWheelEvent>
#include <QtCore/qelapsedtimer.h>

namespace SprintIDE {

// ============================================================================
// 键盘 -> PTY 字节
// ============================================================================
static QByteArray keyToPtyBytes(QKeyEvent* e)
{
    if ((e->modifiers() & Qt::ControlModifier) && !e->text().isEmpty()) {
        const int k = e->key();
        if (k >= Qt::Key_A && k <= Qt::Key_Z) {
            return QByteArray(1, char(k - Qt::Key_A + 1));
        }
        if (k == Qt::Key_Space)         return QByteArray(1, '\0');
        if (k == Qt::Key_BracketLeft)   return QByteArray(1, '\x1b');
        if (k == Qt::Key_Backslash)     return QByteArray(1, '\x1c');
        if (k == Qt::Key_BracketRight)  return QByteArray(1, '\x1d');
        if (k == Qt::Key_AsciiCircum)   return QByteArray(1, '\x1e');
        if (k == Qt::Key_Underscore)    return QByteArray(1, '\x1f');
    }
    if ((e->modifiers() & Qt::AltModifier) && !e->text().isEmpty()) {
        return QByteArray(1, '\x1b') + e->text().toUtf8();
    }

    switch (e->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:     return "\r";
    case Qt::Key_Backspace: return "\x7F";
    case Qt::Key_Tab:       return "\t";
    case Qt::Key_Escape:    return "\x1b";
    case Qt::Key_Up:        return "\x1b[A";
    case Qt::Key_Down:      return "\x1b[B";
    case Qt::Key_Right:     return "\x1b[C";
    case Qt::Key_Left:      return "\x1b[D";
    case Qt::Key_Home:      return "\x1b[H";
    case Qt::Key_End:       return "\x1b[F";
    case Qt::Key_PageUp:    return "\x1b[5~";
    case Qt::Key_PageDown:  return "\x1b[6~";
    case Qt::Key_Delete:    return "\x1b[3~";
    case Qt::Key_Insert:    return "\x1b[2~";
    case Qt::Key_F1:        return "\x1bOP";
    case Qt::Key_F2:        return "\x1bOQ";
    case Qt::Key_F3:        return "\x1bOR";
    case Qt::Key_F4:        return "\x1bOS";
    case Qt::Key_F5:        return "\x1b[15~";
    case Qt::Key_F6:        return "\x1b[17~";
    case Qt::Key_F7:        return "\x1b[18~";
    case Qt::Key_F8:        return "\x1b[19~";
    case Qt::Key_F9:        return "\x1b[20~";
    case Qt::Key_F10:       return "\x1b[21~";
    case Qt::Key_F11:       return "\x1b[23~";
    case Qt::Key_F12:       return "\x1b[24~";
    default: break;
    }

    const QString text = e->text();
    if (!text.isEmpty()) {
        QString filtered;
        for (QChar ch : text) {
            if (ch.unicode() >= 0x20 || ch == QLatin1Char('\t')) filtered.append(ch);
        }
        if (!filtered.isEmpty()) return filtered.toUtf8();
    }
    return {};
}

// ============================================================================
// 超链接识别正则
// ============================================================================
static const QRegularExpression& urlRegex()
{
    static const QRegularExpression re(
        QStringLiteral(R"(https?://[^\s<>"'`]+)"),
        QRegularExpression::CaseInsensitiveOption);
    return re;
}

static const QRegularExpression& fileLineColRegex()
{
    // path/to/file.ext:LINE[:COL]
    // 路径至少包含一个 / 或 \，或者含扩展名，避免把裸数字误判
    static const QRegularExpression re(
        QStringLiteral(R"(([A-Za-z]:[\\/][^\s:]+|[./~]?[\w\-./\\]*[\w\-]\.[A-Za-z0-9]{1,8}):(\d+)(?::(\d+))?)"));
    return re;
}

// ============================================================================
// 构造 / 析构
// ============================================================================
TerminalView::TerminalView(QWidget* parent)
    : QAbstractScrollArea(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    viewport()->setAutoFillBackground(false);

    // 启用拖放
    setAcceptDrops(true);
    viewport()->setMouseTracking(true);

    m_font = QFont(QStringLiteral("Menlo"), 12);
    m_font.setStyleHint(QFont::Monospace);
    m_font.setFixedPitch(true);
    updateCharMetrics();

    m_blinkTimer = new QTimer(this);
    m_blinkTimer->setInterval(530);
    connect(m_blinkTimer, &QTimer::timeout, this, &TerminalView::onBlinkTick);

    connect(verticalScrollBar(), &QScrollBar::valueChanged,
            this, [this](int value) {
                const int maxValue = verticalScrollBar()->maximum();
                m_scrollOffset = maxValue - value;
                viewport()->update();
            });
    // ============================================================
    // resize debounce
    //
    //   resizeEvent -> 记录 m_pendingRows/Cols -> 启动 500ms debounce
    //   拖动中 debounce 反复重启，永不触发 setSize
    //   鼠标松开 500ms 后 debounce 触发 -> setSize（重建 vterm < 10ms）
    //   -> emit modelSizeChanged -> TerminalPanel 发 SIGWINCH -> shell 重绘
    // ============================================================
    m_sizeDebounceTimer = new QTimer(this);
    m_sizeDebounceTimer->setSingleShot(true);
    m_sizeDebounceTimer->setInterval(500);
    connect(m_sizeDebounceTimer, &QTimer::timeout, this, [this]() {
        if (!m_model) return;
        if (!m_resizeAllowed) return;
        if (m_pendingRows <= 0 || m_pendingCols <= 0) return;

        // 鼠标还按着 -> 继续等
        if (QApplication::mouseButtons() & Qt::LeftButton) {
            m_sizeDebounceTimer->start();
            return;
        }

        // 尺寸没变 -> 跳过
        if (m_pendingRows == m_model->rows()
            && m_pendingCols == m_model->cols()) {
            m_pendingRows = 0;
            m_pendingCols = 0;
            return;
        }

        const int newRows = m_pendingRows;
        const int newCols = m_pendingCols;
        m_pendingRows = 0;
        m_pendingCols = 0;

        // 重建 vterm（< 10ms）
        m_model->setSize(newRows, newCols);

        // 通知 TerminalPanel 同步 PTY（发 SIGWINCH）
        emit modelSizeChanged(newRows, newCols);

        // 重排滚动条
        QSignalBlocker blocker(verticalScrollBar());
        updateScrollBarRange();
    });
}

TerminalView::~TerminalView() = default;

// ============================================================================
// 模型
// ============================================================================
void TerminalView::setModel(TerminalGridModel* model)
{
    if (m_model == model) return;
    if (m_model) disconnect(m_model, nullptr, this, nullptr);

    m_model = model;
    if (m_model) {
        connect(m_model, &TerminalGridModel::dirty,
                this, &TerminalView::onModelDirty);
        connect(m_model, &TerminalGridModel::cursorChanged,
                this, &TerminalView::onCursorChanged);

        //  鼠标协议变化时清空选中 + 重置鼠标状态
        // （防止协议从"关"切换到"开"时残留选中状态）
        connect(m_model, &TerminalGridModel::mouseProtocolChanged,
                this, [this](int protocol) {
                    if (protocol != 0) {
                        m_selStart = {};
                        m_selEnd   = {};
                        m_selecting = false;
                        m_mouseButtonPressed = false;
                        m_lastMouseButton    = -1;
                        viewport()->update();
                    }
                });
    }
    m_scrollOffset = 0;
    m_selStart = {};
    m_selEnd   = {};
    clearSearch();
    updateScrollBarRange();
    updateCharMetrics();
    m_blinkTimer->start();
    viewport()->update();
}

void TerminalView::applyEditorFont(const QString& family, int pointSize)
{
    pointSize = qBound(8, pointSize, 48);

    //   严格比较 family + pointSize（不用 QFont::operator==，
    //   因为它还会比较 hint/style 等派生属性，可能导致误判）
    if (m_font.pointSize() == pointSize
        && (family.isEmpty() || m_font.family() == family)) {
        return;
    }

    QFont newFont = m_font;
    if (!family.isEmpty()) {
        newFont.setFamily(family);
    }
    newFont.setPointSize(pointSize);
    newFont.setStyleHint(QFont::Monospace);
    newFont.setFixedPitch(true);

    m_font = newFont;
    updateCharMetrics();

    if (m_model && m_resizeAllowed && m_sizeDebounceTimer) {
        const int cols = visibleColCount();
        const int rows = visibleRowCount();
        if (rows > 0 && rows <= 100 && cols > 0 && cols <= 500) {
            m_pendingRows = rows;
            m_pendingCols = cols;
        }
        m_sizeDebounceTimer->start();
    }

    updateScrollBarRange();
    viewport()->update();
}

void TerminalView::setThemeColors(const QColor& selectionBg,
                                  const QColor& matchBg,
                                  const QColor& activeMatchBg,
                                  const QColor& linkFg,
                                  const QColor& cursorFg)
{
    m_selectionBg   = selectionBg;
    m_matchBg       = matchBg;
    m_activeMatchBg = activeMatchBg;
    m_linkFg        = linkFg;
    m_cursorFg      = cursorFg;
    viewport()->update();
}

TerminalGridModel* TerminalView::model() const { return m_model; }

// ============================================================================
// 字符度量
// ============================================================================
void TerminalView::updateCharMetrics()
{
    m_fontNormal     = m_font;
    m_fontBold       = m_font;
    m_fontItalic     = m_font;
    m_fontBoldItalic = m_font;

    m_fontBold.setBold(true);
    m_fontItalic.setItalic(true);
    m_fontBoldItalic.setBold(true);
    m_fontBoldItalic.setItalic(true);

    QFontMetricsF fm(m_fontNormal);
    //   qMax(1.0)：如果字体加载失败，horizontalAdvance/height 返回 0，
    //   会导致 visibleColCount/visibleRowCount 算成 1 或 0，
    //   进而 updateScrollBarRange/setSize 反复触发形成死循环
    m_charW = qMax<qreal>(1.0, fm.horizontalAdvance(QLatin1Char('M')));
    //   height() -> lineSpacing()：
    //   height() 只含 ascent + descent；lineSpacing() 还包含 leading（行间空隙）
    //   用 lineSpacing 视觉上更接近 QTextEdit 的行距，不会显得"行挤在一起"
    m_charH = qMax<qreal>(1.0, fm.lineSpacing());

    setViewportMargins(0, 0, 0, 0);
    updateScrollBarRange();
}

int TerminalView::visibleRowCount() const
{
    if (m_charH <= 0) return 24;
    const int h = viewport()->height();
    // 防御：viewport 尺寸 < 100px 或 > 10000px 时返回默认值
    //   布局过程中 viewport 可能瞬时变成 24px（算出 1 行）
    if (h < 100 || h > 10000) return 24;
    return qBound(5, int(h / m_charH), 100);
}

int TerminalView::visibleColCount() const
{
    if (m_charW <= 0) return 80;
    const int w = viewport()->width();
    if (w < 100 || w > 10000) return 80;
    return qBound(20, int(w / m_charW), 500);
}

// ============================================================================
// 滚动条
// ============================================================================
void TerminalView::updateScrollBarRange()
{
    if (!m_model) {
        QSignalBlocker b(verticalScrollBar());
        verticalScrollBar()->setRange(0, 0);
        return;
    }

    // 防递归
    if (m_inUpdate) return;
    m_inUpdate = true;

    const int sb = m_model->scrollbackCount();
    const int totalContent = sb + m_model->rows();
    const int scrollRange  = qMax(0, totalContent - visibleRowCount());

    //  QSignalBlocker 屏蔽 setRange/setValue 触发的 rangeChanged/valueChanged
    //   否则 valueChanged 里的 lambda 改 m_scrollOffset -> update -> paint -> 回调
    {
        QSignalBlocker b(verticalScrollBar());
        if (verticalScrollBar()->minimum() != 0
            || verticalScrollBar()->maximum() != scrollRange) {
            verticalScrollBar()->setRange(0, scrollRange);
        }
        const int desiredValue = scrollRange - m_scrollOffset;
        if (verticalScrollBar()->value() != desiredValue) {
            verticalScrollBar()->setValue(desiredValue);
        }
    }

    m_inUpdate = false;
}

// ============================================================================
// 可视行 ↔ contentRow
// ============================================================================
int TerminalView::visibleRowToContentRow(int visibleRow) const
{
    if (!m_model) return visibleRow;
    const int sb = m_model->scrollbackCount();
    const int totalContent = sb + m_model->rows();
    const int firstContentRow = totalContent - visibleRowCount() - m_scrollOffset;
    return firstContentRow + visibleRow;
}

int TerminalView::contentRowToVisibleRow(int contentRow) const
{
    if (!m_model) return -1;
    const int sb = m_model->scrollbackCount();
    const int totalContent = sb + m_model->rows();
    const int firstContentRow = totalContent - visibleRowCount() - m_scrollOffset;
    const int vr = contentRow - firstContentRow;
    return (vr >= 0 && vr < visibleRowCount()) ? vr : -1;
}

QVector<TerminalCell> TerminalView::contentRowCells(int contentRow) const
{
    if (!m_model) return {};
    const int sb = m_model->scrollbackCount();
    if (contentRow < 0 || contentRow >= sb + m_model->rows()) return {};

    if (contentRow < sb) return m_model->scrollbackLine(sb - 1 - contentRow);

    const int screenRow = contentRow - sb;
    QVector<TerminalCell> line;
    line.reserve(m_model->cols());
    for (int c = 0; c < m_model->cols(); ++c) line.append(m_model->cellAt(screenRow, c));
    return line;
}

// ============================================================================
// 像素 ↔ 单元格
// ============================================================================
QPoint TerminalView::cellPosToPixel(int row, int col) const
{
    return QPoint(int(col * m_charW), int(row * m_charH));
}

TerminalView::CellPos TerminalView::pixelToCellPos(const QPoint& p) const
{
    if (m_charW <= 0 || m_charH <= 0) return {};
    CellPos pos;
    pos.col = qMax(0, int(p.x() / m_charW));
    pos.row = qMax(0, int(p.y() / m_charH));
    return pos;
}

// ============================================================================
// 模型信号
// ============================================================================
void TerminalView::onModelDirty(const QRect& cellRect)
{
    Q_UNUSED(cellRect);
    updateScrollBarRange();
    // 如果搜索在进行，输出变化可能新增匹配
    if (!m_searchQuery.isEmpty()) {
        rebuildSearchMatches();
    }
    viewport()->update();
}

void TerminalView::onCursorChanged()
{
    viewport()->update();
}

void TerminalView::onBlinkTick()
{
    m_cursorBlinkOn = !m_cursorBlinkOn;
    if (m_model && m_model->cursorVisible() && m_scrollOffset == 0) {
        const int row = m_model->cursorRow();
        QRect r(0, int(row * m_charH), viewport()->width(), int(m_charH + 1));
        viewport()->update(r);
    }
}

// ============================================================================
// resize
// ============================================================================
void TerminalView::resizeEvent(QResizeEvent* e)
{
    QAbstractScrollArea::resizeEvent(e);
    if (!m_model) return;

    // startShell 之前完全不动 model
    //   构造期 viewport 尺寸不稳定（可能返回 1×8），会触发 libvterm bug
    if (!m_resizeAllowed) return;

    const int cols = visibleColCount();
    const int rows = visibleRowCount();
    if (rows > 0 && rows <= 100 && cols > 0 && cols <= 500) {
        m_pendingRows = rows;
        m_pendingCols = cols;
    }

    // 启动 debounce（拖动中反复重启，松开 500ms 后才触发 setSize）
    if (m_sizeDebounceTimer) {
        m_sizeDebounceTimer->start();
    }
}

// 允许/禁止 resize
void TerminalView::setResizeAllowed(bool allowed)
{
    m_resizeAllowed = allowed;

    // 允许后如果有 pending 尺寸，启动 debounce
    if (allowed && m_sizeDebounceTimer
        && m_pendingRows > 0 && m_pendingCols > 0) {
        m_sizeDebounceTimer->start();
    }
}

// ============================================================================
// 绘制
// ============================================================================
void TerminalView::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);

    QPainter painter(viewport());

    // 背景兜底：m_model 未 set 或 defaultBg 未注入 -> 用 QPalette::Base
    QColor bgColor = m_model ? m_model->defaultBg() : QColor();
    if (!bgColor.isValid()) {
        bgColor = viewport()->palette().color(QPalette::Base);
    }
    painter.fillRect(viewport()->rect(), bgColor);
    if (!m_model) return;

    painter.setFont(m_font);

    const QPalette& pal = viewport()->palette();
    const QColor fallbackFg = pal.color(QPalette::Text);
    const QColor fallbackHighlight = pal.color(QPalette::Highlight);

    const int visRows = visibleRowCount();
    const int visCols = visibleColCount();
    const int modelCols = m_model->cols();
    const int sb        = m_model->scrollbackCount();

    for (int vr = 0; vr < visRows; ++vr) {
        const int cr = visibleRowToContentRow(vr);

        const TerminalCell* rowData = nullptr;
        QVector<TerminalCell> rowCopy;
        int drawCols = 0;

        if (cr < sb) {
            rowCopy = m_model->scrollbackLine(sb - 1 - cr);
            rowData = rowCopy.constData();
            drawCols = qMin(visCols, rowCopy.size());
        } else {
            const int screenRow = cr - sb;
            if (screenRow < 0 || screenRow >= m_model->rows()) continue;

            rowCopy.reserve(modelCols);
            for (int c = 0; c < modelCols; ++c) {
                rowCopy.append(m_model->cellAt(screenRow, c));
            }
            rowData = rowCopy.constData();
            drawCols = qMin(visCols, modelCols);
        }

        int c = 0;
        while (c < drawCols) {
            const TerminalCell& firstCell = rowData[c];

            // 跳过延续 cell（width == 0），它已被前面的宽字符覆盖
            if (firstCell.width == 0) {
                ++c;
                continue;
            }

            const bool firstSel = isCellSelected(vr, c);
            const bool firstMatch = isCellInMatch(vr, c);
            const bool firstActiveMatch = isCellInActiveMatch(vr, c);

            int runEnd = c + 1;
            while (runEnd < drawCols) {
                const TerminalCell& next = rowData[runEnd];

                // 延续 cell 属于当前 run，跳过但让 runEnd 前进
                if (next.width == 0) {
                    ++runEnd;
                    continue;
                }

                if (next.fg != firstCell.fg || next.bg != firstCell.bg
                    || next.attrs != firstCell.attrs)
                    break;
                if (isCellSelected(vr, runEnd) != firstSel) break;
                if (isCellInMatch(vr, runEnd) != firstMatch) break;
                if (isCellInActiveMatch(vr, runEnd) != firstActiveMatch) break;
                ++runEnd;
            }

            QString text;
            text.reserve(runEnd - c);
            int widthCells = 0;
            for (int k = c; k < runEnd; ++k) {
                const auto& cell = rowData[k];
                // 跳过延续 cell，不追加到 text
                if (cell.width == 0) continue;
                if (cell.text.isEmpty()) {
                    text.append(QLatin1Char(' '));
                } else {
                    text.append(cell.text);
                }
                widthCells += cell.width;   // 直接加 width（0/1/2）
            }

            // ---- 背景 ----
            QColor bg = firstCell.bg;
            if (firstCell.attrs & AttrReverse) bg = firstCell.fg;
            if (firstSel) {
                bg = m_selectionBg.isValid() ? m_selectionBg : fallbackHighlight;
            } else if (firstActiveMatch) {
                bg = m_activeMatchBg.isValid() ? m_activeMatchBg : fallbackHighlight;
            } else if (firstMatch) {
                bg = m_matchBg.isValid() ? m_matchBg : fallbackHighlight;
            }
            // invalid 时跳过：背景已经 fillRect 过，不画黑块
            if (bg.isValid()) {
                painter.fillRect(QRectF(c * m_charW, vr * m_charH,
                                        widthCells * m_charW, m_charH),
                                 bg);
            }

            // ---- 前景 ----
            QColor fg = firstCell.fg;
            if (firstCell.attrs & AttrReverse) fg = firstCell.bg;
            if (firstCell.attrs & AttrFaint) fg.setAlpha(160);
            if (!fg.isValid()) fg = fallbackFg;    // invalid 回退 QPalette::Text

            const bool bold   = (firstCell.attrs & AttrBold)   != 0;
            const bool italic = (firstCell.attrs & AttrItalic) != 0;

            const QFont* fontPtr = &m_fontNormal;
            if (bold && italic)      fontPtr = &m_fontBoldItalic;
            else if (bold)           fontPtr = &m_fontBold;
            else if (italic)         fontPtr = &m_fontItalic;

            if ((firstCell.attrs & (AttrUnderline | AttrStrike)) != 0) {
                QFont decorated = *fontPtr;
                if (firstCell.attrs & AttrUnderline) decorated.setUnderline(true);
                if (firstCell.attrs & AttrStrike)    decorated.setStrikeOut(true);
                painter.setFont(decorated);
            } else {
                painter.setFont(*fontPtr);
            }
            painter.setPen(fg);

            const QRectF textRect(c * m_charW, vr * m_charH,
                                  widthCells * m_charW, m_charH);
            painter.drawText(textRect,
                             Qt::AlignLeft | Qt::AlignVCenter | Qt::TextDontClip,
                             text);

            c = runEnd;
        }
    }

    if (m_scrollOffset == 0 && m_model->cursorVisible()
        && m_cursorBlinkOn && hasFocus()) {
        paintCursor(painter);
    }
}

void TerminalView::paintCell(QPainter& p, int vr, int col,
                             const TerminalCell& cell, bool selected)
{
    // 背景
    QColor bg = cell.bg;
    if (cell.attrs & AttrReverse) bg = cell.fg;
    if (selected) {
        bg = m_selectionBg;
    } else if (isCellInActiveMatch(vr, col)) {
        bg = m_activeMatchBg;
    } else if (isCellInMatch(vr, col)) {
        bg = m_matchBg;
    }

    // 超链接悬停下划线
    const bool isHoveredLink = m_hoveredLinkStart.valid() && m_hoveredLinkEnd.valid()
        && vr >= m_hoveredLinkStart.row && vr <= m_hoveredLinkEnd.row
        && (vr != m_hoveredLinkStart.row || col >= m_hoveredLinkStart.col)
        && (vr != m_hoveredLinkEnd.row   || col <= m_hoveredLinkEnd.col);

    const QRectF cellRect(col * m_charW, vr * m_charH, m_charW, m_charH);
    p.fillRect(cellRect, bg);

    if (cell.text.isEmpty()) {
        if (isHoveredLink) {
            p.setPen(m_linkFg);
            p.drawLine(QPointF(cellRect.left(), cellRect.bottom() - 1),
                       QPointF(cellRect.right(), cellRect.bottom() - 1));
        }
        return;
    }

    QColor fg = cell.fg;
    if (cell.attrs & AttrReverse) fg = cell.bg;
    if (cell.attrs & AttrFaint) fg.setAlpha(160);

    // 从 4 个预创建变体里按需选（O(1)，不创建新 QFont）
    const bool bold   = (cell.attrs & AttrBold)   != 0;
    const bool italic = (cell.attrs & AttrItalic) != 0;

    const QFont* fontPtr = &m_fontNormal;
    if (bold && italic)      fontPtr = &m_fontBoldItalic;
    else if (bold)           fontPtr = &m_fontBold;
    else if (italic)         fontPtr = &m_fontItalic;

    // 下划线 / 删除线需要单独的 QFont 变体（相对少见，可以临时复制）
    // 99% 的 cell 没有下划线 / 删除线，所以走上面快路径
    if ((cell.attrs & (AttrUnderline | AttrStrike)) != 0) {
        QFont decorated = *fontPtr;
        if (cell.attrs & AttrUnderline) decorated.setUnderline(true);
        if (cell.attrs & AttrStrike)    decorated.setStrikeOut(true);
        p.setFont(decorated);
    } else {
        p.setFont(*fontPtr);
    }
    p.setPen(fg);

    const qreal width = (cell.width == 2) ? m_charW * 2 : m_charW;
    const QRectF textRect(col * m_charW, vr * m_charH, width, m_charH);
    p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextDontClip, cell.text);

    if (isHoveredLink) {
        p.setPen(m_linkFg);
        p.drawLine(QPointF(cellRect.left(), cellRect.bottom() - 1),
                   QPointF(cellRect.right(), cellRect.bottom() - 1));
    }
}

void TerminalView::paintCursor(QPainter& p)
{
    if (!m_model) return;
    const QRectF r(m_model->cursorCol() * m_charW,
                   m_model->cursorRow() * m_charH,
                   m_charW, m_charH);

    const QColor cursorColor = m_cursorFg.isValid()
        ? m_cursorFg
        : viewport()->palette().color(QPalette::Text);   // 兜底

    p.setCompositionMode(QPainter::CompositionMode_Difference);
    switch (m_model->cursorShape()) {
    case TerminalCursorShape::Block:
        p.fillRect(r, cursorColor);
        break;
    case TerminalCursorShape::Underline:
        p.fillRect(QRectF(r.left(), r.bottom() - 2, r.width(), 2), cursorColor);
        break;
    case TerminalCursorShape::Bar:
        p.fillRect(QRectF(r.left(), r.top(), 2, r.height()), cursorColor);
        break;
    }
}

// ============================================================================
//  把鼠标事件编码成 X10 / VT200 CSI 序列发给 PTY
//
//   协议格式（X10 / VT200 相同）：
//     \x1b[M <button+32> <col+33> <row+33>
//
//   button 是 8 位值：
//     bit 0~1：0=左 1=中 2=右 3=释放
//     bit 2  ：Shift
//     bit 3  ：Alt
//     bit 4  ：Ctrl
//     拖动时 +32
//     滚轮：64=上 65=下
//
//   col / row 是 1-based 的单元格坐标，+32 偏移后写入一个字节。
//   超出 223 的坐标会溢出（VT200 协议限制），宽屏下最右 30 列可能丢事件。
//   这是协议限制，SGR 扩展能解决，但 libvterm 0.3.x 不支持 SGR。
// ============================================================================
void TerminalView::sendMouseToPty(int button, int col, int row)
{
    if (!m_model) return;

    // 修饰位
    //   注意：Qt::ControlModifier 在 macOS 对应 Cmd 而不是 Ctrl。
    //   终端里 Ctrl+点击是终端惯例，但 macOS 用户习惯用 Cmd。这里按终端惯例走，
    //   用 ControlModifier（macOS 上相当于物理 Ctrl 键）。
    //   如果用户按 Cmd，在 macOS 上会被 Qt 解释成 MetaModifier，不参与修饰位。
    const Qt::KeyboardModifiers mods = QApplication::keyboardModifiers();
    if (mods & Qt::ShiftModifier)   button |= 4;
    if (mods & Qt::AltModifier)     button |= 8;
    if (mods & Qt::ControlModifier) button |= 16;

    // 坐标 0-based -> 1-based -> +32 偏移
    //   限制到 223 以内避免溢出（255 - 32 = 223，但 X10 协议更保守，用 223 上限）
    const int cx = qBound(1, col + 1, 223);
    const int cy = qBound(1, row + 1, 223);

    QByteArray seq;
    seq.reserve(6);
    seq.append('\x1b');
    seq.append('[');
    seq.append('M');
    seq.append(char(button + 32));
    seq.append(char(cx + 32));
    seq.append(char(cy + 32));

    emit inputRequested(seq);
}

// ============================================================================
//   判断当前鼠标事件是否应该转发给 PTY
//
//   规则：
//     - 模型鼠标协议为 0（关闭）-> 不转发
//     - Shift 按下 -> 不转发（Shift+点击强制走选中，终端惯例）
// ============================================================================
bool TerminalView::shouldForwardMouse(QMouseEvent* e) const
{
    if (!m_model) return false;
    if (m_model->mouseProtocol() == 0) return false;
    if (e->modifiers() & Qt::ShiftModifier) return false;
    return true;
}

// ============================================================================
// 选区
// ============================================================================
void TerminalView::normalizeSelection()
{
    if (!m_selAnchor.valid() || !m_selEnd.valid()) return;

    const bool anchorBeforeCursor =
        (m_selAnchor.row < m_selEnd.row)
        || (m_selAnchor.row == m_selEnd.row && m_selAnchor.col <= m_selEnd.col);

    if (anchorBeforeCursor) {
        m_selStart = m_selAnchor;
        // m_selEnd 保持当前鼠标位置
    } else {
        m_selStart = m_selEnd;
        m_selEnd = m_selAnchor;
    }
}

bool TerminalView::isCellSelected(int vr, int col) const
{
    if (!isSelectionActive()) return false;
    if (vr < m_selStart.row || vr > m_selEnd.row) return false;
    if (vr == m_selStart.row && col < m_selStart.col) return false;
    if (vr == m_selEnd.row   && col > m_selEnd.col)   return false;
    return true;
}

QString TerminalView::selectedText() const
{
    if (!isSelectionActive() || !m_model) return {};

    QString out;
    for (int vr = m_selStart.row; vr <= m_selEnd.row; ++vr) {
        const int cr = visibleRowToContentRow(vr);
        const QVector<TerminalCell> line = contentRowCells(cr);
        int startCol = (vr == m_selStart.row) ? m_selStart.col : 0;
        int endCol   = (vr == m_selEnd.row)   ? m_selEnd.col   : m_model->cols() - 1;
        while (endCol > startCol && endCol < line.size()
               && line[endCol].text.trimmed().isEmpty()) {
            --endCol;
        }
        for (int c = startCol; c <= endCol && c < line.size(); ++c) {
            const auto& cell = line[c];
            out.append(cell.text.isEmpty() ? QStringLiteral(" ") : cell.text);
        }
        if (vr < m_selEnd.row) out.append(QLatin1Char('\n'));
    }
    return out;
}

void TerminalView::copySelectionToClipboard()
{
    if (!isSelectionActive()) return;
    QApplication::clipboard()->setText(selectedText());
}

void TerminalView::pasteFromClipboard()
{
    QString text = QApplication::clipboard()->text();
    if (text.isEmpty()) return;

    text.replace(QLatin1String("\r\n"), QLatin1String("\r"));
    text.replace(QLatin1Char('\n'), QLatin1Char('\r'));

    // 单行粘贴进命令 buffer（多行粘贴不记录）
    if (!text.contains(QLatin1Char('\r'))) {
        m_commandBuffer.append(text);
    }

    // Bracketed Paste
    if (m_model && m_model->bracketedPasteEnabled()) {
        QByteArray bytes = "\x1b[200~";
        bytes += text.toUtf8();
        bytes += "\x1b[201~";
        emit inputRequested(bytes);
    } else {
        emit inputRequested(text.toUtf8());
    }
}

// ============================================================================
// 滚动
// ============================================================================
bool TerminalView::isScrolledBack() const { return m_scrollOffset > 0; }

void TerminalView::scrollToBottom()
{
    m_scrollOffset = 0;
    updateScrollBarRange();
    viewport()->update();
}

void TerminalView::wheelEvent(QWheelEvent* e)
{
    if (!m_model) return;

    // 鼠标模式开启时，滚轮也转发给 PTY（vim 滚动 / tmux 翻历史等）
    //   滚轮编码：64 = 上滚，65 = 下滚
    if (m_model->mouseProtocol() != 0
        && !(e->modifiers() & Qt::ShiftModifier)) {
        const CellPos pos = pixelToCellPos(e->position().toPoint());
        const int delta = e->angleDelta().y();
        if (delta != 0) {
            const int btn = (delta > 0) ? 64 : 65;
            sendMouseToPty(btn, pos.col, pos.row);
        }
        e->accept();
        return;
    }

    // 原有逻辑：滚动 scrollback
    const int lines = e->angleDelta().y() / 120 * 3;
    const int oldOffset = m_scrollOffset;
    m_scrollOffset = qBound(0, m_scrollOffset + lines,
                            qMax(0, m_model->scrollbackCount()));
    if (m_scrollOffset != oldOffset) {
        updateScrollBarRange();
        viewport()->update();
    }
    e->accept();
}

// ============================================================================
// 焦点
// ============================================================================
void TerminalView::focusInEvent(QFocusEvent* e)
{
    QAbstractScrollArea::focusInEvent(e);
    m_blinkTimer->start();
    viewport()->update();
}

void TerminalView::focusOutEvent(QFocusEvent* e)
{
    QAbstractScrollArea::focusOutEvent(e);
    m_blinkTimer->stop();
    m_cursorBlinkOn = false;
    viewport()->update();
}

// ============================================================================
// 搜索
// ============================================================================
void TerminalView::setSearchQuery(const QString& query, bool caseSensitive)
{
    m_searchQuery         = query;
    m_searchCaseSensitive = caseSensitive;
    rebuildSearchMatches();

    if (!m_searchMatches.isEmpty()) {
        m_searchCurrentIndex = 0;
        scrollToActiveMatch();
    } else {
        m_searchCurrentIndex = -1;
    }
    emit searchStateChanged(m_searchCurrentIndex, m_searchMatches.size());
    viewport()->update();
}

void TerminalView::clearSearch()
{
    m_searchQuery.clear();
    m_searchMatches.clear();
    m_searchCurrentIndex = -1;
    emit searchStateChanged(-1, 0);
    viewport()->update();
}

void TerminalView::findNext()
{
    if (m_searchMatches.isEmpty()) return;
    m_searchCurrentIndex = (m_searchCurrentIndex + 1) % m_searchMatches.size();
    scrollToActiveMatch();
    emit searchStateChanged(m_searchCurrentIndex, m_searchMatches.size());
    viewport()->update();
}

void TerminalView::findPrev()
{
    if (m_searchMatches.isEmpty()) return;
    m_searchCurrentIndex = (m_searchCurrentIndex - 1 + m_searchMatches.size())
                           % m_searchMatches.size();
    scrollToActiveMatch();
    emit searchStateChanged(m_searchCurrentIndex, m_searchMatches.size());
    viewport()->update();
}

int TerminalView::searchMatchCount() const { return m_searchMatches.size(); }
int TerminalView::searchCurrentIndex() const { return m_searchCurrentIndex; }
QString TerminalView::searchQuery() const { return m_searchQuery; }

void TerminalView::rebuildSearchMatches()
{
    m_searchMatches.clear();
    if (m_searchQuery.isEmpty() || !m_model) return;

    const int sb = m_model->scrollbackCount();
    const int totalRows = sb + m_model->rows();
    const Qt::CaseSensitivity cs = m_searchCaseSensitive ? Qt::CaseSensitive
                                                         : Qt::CaseInsensitive;

    for (int cr = 0; cr < totalRows; ++cr) {
        const QVector<TerminalCell> line = contentRowCells(cr);
        QString text;
        text.reserve(line.size());
        for (const auto& cell : line) {
            text.append(cell.text.isEmpty() ? QStringLiteral(" ") : cell.text);
        }
        int from = 0;
        while (from <= text.size() - m_searchQuery.size()) {
            const int pos = text.indexOf(m_searchQuery, from, cs);
            if (pos < 0) break;
            m_searchMatches.append({ cr, pos, int(m_searchQuery.size()) });
            from = pos + qMax(1, int(m_searchQuery.size()));
        }
    }

    if (m_searchCurrentIndex >= m_searchMatches.size()) {
        m_searchCurrentIndex = m_searchMatches.isEmpty() ? -1 : 0;
    }
}

void TerminalView::scrollToActiveMatch()
{
    if (m_searchCurrentIndex < 0 || m_searchCurrentIndex >= m_searchMatches.size()) return;
    const auto& m = m_searchMatches[m_searchCurrentIndex];

    // 计算该行在当前可视区内的位置
    const int totalContent = m_model->scrollbackCount() + m_model->rows();
    const int visRows = visibleRowCount();
    // 让匹配行出现在可视区中间附近
    const int desiredScrollOffset = qBound(0,
        totalContent - visRows - m.contentRow + visRows / 2,
        qMax(0, m_model->scrollbackCount()));
    m_scrollOffset = desiredScrollOffset;
    updateScrollBarRange();
}

bool TerminalView::isCellInMatch(int vr, int col) const
{
    if (m_searchMatches.isEmpty()) return false;
    const int cr = visibleRowToContentRow(vr);
    for (const auto& m : m_searchMatches) {
        if (m.contentRow == cr && col >= m.col && col < m.col + m.len) return true;
    }
    return false;
}

bool TerminalView::isCellInActiveMatch(int vr, int col) const
{
    if (m_searchCurrentIndex < 0 || m_searchCurrentIndex >= m_searchMatches.size()) {
        return false;
    }
    const auto& m = m_searchMatches[m_searchCurrentIndex];
    const int cr = visibleRowToContentRow(vr);
    return m.contentRow == cr && col >= m.col && col < m.col + m.len;
}

// ============================================================================
// 超链接
// ============================================================================
QString TerminalView::detectLinkAt(int contentRow, int col) const
{
    const QVector<TerminalCell> line = contentRowCells(contentRow);
    if (line.isEmpty() || col < 0 || col >= line.size()) return {};

    QString text;
    text.reserve(line.size());
    for (const auto& cell : line) {
        text.append(cell.text.isEmpty() ? QStringLiteral(" ") : cell.text);
    }

    auto it = urlRegex().globalMatch(text);
    while (it.hasNext()) {
        const auto m = it.next();
        if (col >= m.capturedStart() && col < m.capturedEnd()) return m.captured(0);
    }

    auto it2 = fileLineColRegex().globalMatch(text);
    while (it2.hasNext()) {
        const auto m = it2.next();
        if (col >= m.capturedStart() && col < m.capturedEnd()) return m.captured(0);
    }
    return {};
}

void TerminalView::updateHoveredLink(const QPoint& pos)
{
    if (!m_model) return;

    const CellPos cell = pixelToCellPos(pos);
    const int cr = visibleRowToContentRow(cell.row);
    const QString link = detectLinkAt(cr, cell.col);

    if (link != m_hoveredLink) {
        m_hoveredLink = link;
        if (link.isEmpty()) {
            m_hoveredLinkStart = {};
            m_hoveredLinkEnd   = {};
            viewport()->setCursor(Qt::IBeamCursor);
        } else {
            // 简单起见：整个匹配范围从 cell.col 左右各扩展 1 直到空格
            const QVector<TerminalCell> line = contentRowCells(cr);
            int startCol = cell.col;
            int endCol = cell.col;
            while (startCol > 0 && !line[startCol - 1].text.trimmed().isEmpty()) --startCol;
            while (endCol + 1 < line.size() && !line[endCol + 1].text.trimmed().isEmpty()) ++endCol;
            m_hoveredLinkStart = { cell.row, startCol };
            m_hoveredLinkEnd   = { cell.row, endCol };
            viewport()->setCursor(Qt::PointingHandCursor);
        }
        viewport()->update();
    }
}

void TerminalView::openLink(const QString& link, bool altModifier) const
{
    Q_UNUSED(altModifier);
    if (link.startsWith(QLatin1String("http://"))
        || link.startsWith(QLatin1String("https://"))) {
        QDesktopServices::openUrl(QUrl(link));
        return;
    }
    // path:line:col -> 交给外部（mainwindow 处理跳转）
    emit const_cast<TerminalView*>(this)->linkClicked(link);
}

// ============================================================================
// 键盘
// ============================================================================
void TerminalView::keyPressEvent(QKeyEvent* e)
{
    const bool hasSel = isSelectionActive();
    const bool hasCtrlOrMeta =
        (e->modifiers() & (Qt::ControlModifier | Qt::MetaModifier)) != 0;
    const bool hasShift = (e->modifiers() & Qt::ShiftModifier) != 0;

    // ============================================================
    // 1. 复制
    //
    //   支持：Cmd+C / Ctrl+C / Cmd+Shift+C / Ctrl+Shift+C
    //
    //   有选区 -> 复制
    //   无选区 + Ctrl/Cmd+Shift+C -> 吞掉（不发给 PTY）
    //   无选区 + Ctrl+C（无 Shift）-> 落到下面走 PTY（发 SIGINT）
    // ============================================================
    if ((e->key() == Qt::Key_C) && hasCtrlOrMeta) {
        if (hasSel) {
            QApplication::clipboard()->setText(selectedText());
            e->accept();
            return;
        }
        // 无选区：Shift 组合吞掉，非 Shift 落下去走 PTY
        if (hasShift) {
            e->accept();
            return;
        }
    }

    // ============================================================
    // 2. 粘贴：Cmd+V / Ctrl+V / Cmd+Shift+V / Ctrl+Shift+V
    // ============================================================
    if ((e->key() == Qt::Key_V) && hasCtrlOrMeta) {
        QString text = QApplication::clipboard()->text();
        text.replace(QLatin1String("\r\n"), QLatin1String("\r"));
        text.replace(QLatin1Char('\n'), QLatin1Char('\r'));
        if (!text.isEmpty()) {
            // 单行粘贴进命令 buffer（多行不记录）
            if (!text.contains(QLatin1Char('\r'))) {
                m_commandBuffer.append(text);
            }
            if (m_model && m_model->bracketedPasteEnabled()) {
                QByteArray bytes = "\x1b[200~";
                bytes += text.toUtf8();
                bytes += "\x1b[201~";
                emit inputRequested(bytes);
            } else {
                emit inputRequested(text.toUtf8());
            }
        }
        e->accept();
        return;
    }

    // ============================================================
    // 3. Ctrl+F / Cmd+F 搜索
    // ============================================================
    if ((e->key() == Qt::Key_F) && hasCtrlOrMeta) {
        emit searchRequested();
        e->accept();
        return;
    }

    // ============================================================
    // 4. Shift+PgUp/PgDn 滚动 scrollback
    // ============================================================
    if (e->key() == Qt::Key_PageUp && hasShift) {
        m_scrollOffset = qMin(m_scrollOffset + visibleRowCount(),
                              m_model ? m_model->scrollbackCount() : 0);
        updateScrollBarRange();
        viewport()->update();
        e->accept();
        return;
    }
    if (e->key() == Qt::Key_PageDown && hasShift) {
        m_scrollOffset = qMax(0, m_scrollOffset - visibleRowCount());
        updateScrollBarRange();
        viewport()->update();
        e->accept();
        return;
    }

    // ============================================================
    // 5. 任意按键回到最新
    // ============================================================
    if (m_scrollOffset > 0) {
        m_scrollOffset = 0;
        updateScrollBarRange();
        viewport()->update();
    }

    // ============================================================
    // 6. 其他按键 -> PTY
    // ============================================================
    const QByteArray bytes = keyToPtyBytes(e);
    if (!bytes.isEmpty()) {
        captureInputForHistory(bytes);
        emit inputRequested(bytes);
        e->accept();
        return;
    }
    QAbstractScrollArea::keyPressEvent(e);
}

// ============================================================================
// 命令历史捕获
//
//   策略：
//     - 全屏程序（alt-screen）活跃时不捕获（vim / htop 里的输入不是 shell 命令）
//     - 回车时提交 m_commandBuffer 作为一条命令，然后清空
//     - Backspace 从 buffer 去掉一个字符
//     - 方向键 / F1-F12 / Ctrl+字母 -> 清空 buffer（可能是历史回溯，buffer 已不可靠）
//     - Ctrl+C -> 清空 buffer
//     - 可打印字符 -> 累加到 buffer
// ============================================================================
void TerminalView::captureInputForHistory(const QByteArray& bytes)
{
    // 全屏程序活跃：不捕获
    if (m_model && m_model->isAltScreenActive()) {
        m_commandBuffer.clear();
        return;
    }

    if (bytes == "\r") {
        const QString cmd = m_commandBuffer.trimmed();
        m_commandBuffer.clear();

        if (!cmd.isEmpty() && cmd.length() <= 2000) {
            const QChar first = cmd.at(0);
            // 首字符是字母 / '.' / '~' / '/' / '$' 才视为 shell 命令
            // 这样能过滤掉一些误输入的片段
            if (first.isLetter()
                || first == QLatin1Char('.')
                || first == QLatin1Char('~')
                || first == QLatin1Char('/')
                || first == QLatin1Char('$')) {
                emit commandSubmitted(cmd);
            }
        }
        return;
    }

    if (bytes == "\x7F") {   // Backspace
        if (!m_commandBuffer.isEmpty()) m_commandBuffer.chop(1);
        return;
    }

    if (bytes.startsWith('\x1b')) {
        // 方向键 / F1-F12 / 其他 CSI 序列 —— 清空
        m_commandBuffer.clear();
        return;
    }

    if (bytes.size() == 1) {
        const char c = bytes.at(0);
        if (c == 0x03) {                     // Ctrl+C
            m_commandBuffer.clear();
            return;
        }
        if (c >= 0x01 && c <= 0x1A) {        // Ctrl+字母（其他）
            m_commandBuffer.clear();
            return;
        }
        if (c == '\t') {
            m_commandBuffer.append(QLatin1Char('\t'));
            return;
        }
        if (c >= 0x20) {                     // 可打印 ASCII
            m_commandBuffer.append(QChar(c));
            return;
        }
        return;
    }

    // 多字节 UTF-8：用 QStringDecoder 流式解码，正确处理跨 chunk 的多字节字符
    //   （虽然键盘输入一般不会跨 chunk，但 IM 输入法上屏时可能一次给多个 codepoint）
    static QStringDecoder s_inputDecoder(QStringDecoder::Utf8);
    m_commandBuffer.append(s_inputDecoder(bytes));
}

// ============================================================================
// E8：Shell 路径转义
//
//   拖文件到终端时，自动转义：
//     - 纯字母数字/下划线/点/斜杠/连字符 -> 原样
//     - 其他 -> 单引号包裹，路径中的单引号替换为 '\''
// ============================================================================
static QString shellEscapePath(const QString& path)
{
    if (path.isEmpty()) return QStringLiteral("''");

    static const QRegularExpression safeRe(
        QStringLiteral(R"(^[A-Za-z0-9_./-]+$)"));

    if (safeRe.match(path).hasMatch()) {
        return path;
    }

    QString escaped = path;
    escaped.replace(QLatin1Char('\''), QLatin1String("'\\''"));
    return QLatin1Char('\'') + escaped + QLatin1Char('\'');
}

// ============================================================================
//   拖入
// ============================================================================
void TerminalView::dragEnterEvent(QDragEnterEvent* e)
{
    if (!e || !e->mimeData()) return;

    // 接受：文件 URL（从 Finder / 项目树拖过来的）+ 纯文本（从编辑器拖代码）
    if (e->mimeData()->hasUrls() || e->mimeData()->hasText()) {
        e->acceptProposedAction();
    }
}

// ============================================================================
//   放下
//   - 用户按回车才会执行
// ============================================================================
void TerminalView::dropEvent(QDropEvent* e)
{
    if (!e || !e->mimeData()) return;

    QString text;

    // 优先处理文件 URI
    if (e->mimeData()->hasUrls()) {
        const QList<QUrl> urls = e->mimeData()->urls();
        QStringList escaped;
        for (const QUrl& url : urls) {
            if (!url.isLocalFile()) continue;
            escaped.append(shellEscapePath(url.toLocalFile()));
        }
        text = escaped.join(QLatin1Char(' '));
    } else if (e->mimeData()->hasText()) {
        text = e->mimeData()->text();
    }

    if (text.isEmpty()) return;

    // 写入 PTY（不自动回车，用户自己按回车确认）
    emit inputRequested(text.toUtf8());

    setFocus();
    e->acceptProposedAction();
}

// ============================================================================
// 鼠标
// ============================================================================
void TerminalView::mousePressEvent(QMouseEvent* e)
{
    setFocus();

    // 鼠标模式开启时，把鼠标事件转发给 PTY（Shift 例外走选中）
    if (shouldForwardMouse(e)) {
        const CellPos pos = pixelToCellPos(e->pos());

        int btn = -1;
        if (e->button() == Qt::LeftButton)   btn = 0;
        else if (e->button() == Qt::MiddleButton) btn = 1;
        else if (e->button() == Qt::RightButton)  btn = 2;

        if (btn >= 0) {
            m_mouseButtonPressed = true;
            m_lastMouseButton    = btn;
            sendMouseToPty(btn, pos.col, pos.row);
            e->accept();
            return;
        }
    }

    if (e->button() != Qt::LeftButton) {
        QAbstractScrollArea::mousePressEvent(e);
        return;
    }

    // Cmd / Ctrl+点击 -> 打开超链接
    if ((e->modifiers() & (Qt::ControlModifier | Qt::MetaModifier))
        && !m_hoveredLink.isEmpty()) {
        openLink(m_hoveredLink, e->modifiers() & Qt::AltModifier);
        e->accept();
        return;
    }

    // 初始化锚点
    const CellPos pos = pixelToCellPos(e->pos());
    m_selAnchor = pos;
    m_selStart  = pos;
    m_selEnd    = pos;
    m_selecting = true;
    emit selectionChanged();
    viewport()->update();
    e->accept();
}

void TerminalView::mouseMoveEvent(QMouseEvent* e)
{
    // 鼠标模式开启时，转发拖动 / 移动
    if (m_model && shouldForwardMouse(e)) {
        const int proto = m_model->mouseProtocol();
        const CellPos pos = pixelToCellPos(e->pos());

        if (proto == 3) {
            const int btn = m_mouseButtonPressed ? m_lastMouseButton : 3;
            sendMouseToPty(btn + 32, pos.col, pos.row);
            e->accept();
            return;
        }

        if (proto == 2 && m_mouseButtonPressed) {
            sendMouseToPty(m_lastMouseButton + 32, pos.col, pos.row);
            e->accept();
            return;
        }
    }

    if (!m_selecting) {
        updateHoveredLink(e->pos());
        QAbstractScrollArea::mouseMoveEvent(e);
        return;
    }

    CellPos pos = pixelToCellPos(e->pos());

    // 边缘自动滚动
    if (e->pos().y() < 0 && m_scrollOffset < (m_model ? m_model->scrollbackCount() : 0)) {
        m_scrollOffset = qMin(m_scrollOffset + 1, m_model ? m_model->scrollbackCount() : 0);
        updateScrollBarRange();
        pos.row = 0;
    } else if (e->pos().y() > viewport()->height() && m_scrollOffset > 0) {
        m_scrollOffset = qMax(0, m_scrollOffset - 1);
        updateScrollBarRange();
        pos.row = visibleRowCount() - 1;
    }

    // 更新终点，从锚点重新计算 start / end
    m_selEnd = pos;
    normalizeSelection();
    emit selectionChanged();
    viewport()->update();
    e->accept();
}

void TerminalView::mouseReleaseEvent(QMouseEvent* e)
{
    // 鼠标模式开启时，转发释放
    if (m_model && shouldForwardMouse(e) && m_mouseButtonPressed) {
        const CellPos pos = pixelToCellPos(e->pos());
        sendMouseToPty(3, pos.col, pos.row);
        m_mouseButtonPressed = false;
        m_lastMouseButton    = -1;
        m_selecting = false;
        e->accept();
        return;
    }

    if (e->button() == Qt::LeftButton) {
        m_selecting = false;
        m_mouseButtonPressed = false;
        m_lastMouseButton    = -1;
        //  保留 m_selStart / m_selEnd（选区保留），清掉 anchor（下次 mousePress 重设）
        //   anchor 不清也没关系，因为下次 mousePress 会覆盖。这里清是为了逻辑清晰。
        m_selAnchor = {};
        e->accept();
        return;
    }
    QAbstractScrollArea::mouseReleaseEvent(e);
}

void TerminalView::mouseDoubleClickEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton) {
        QAbstractScrollArea::mouseDoubleClickEvent(e);
        return;
    }

    setFocus();
    const CellPos pos = pixelToCellPos(e->pos());
    if (!pos.valid()) return;

    const int cr = visibleRowToContentRow(pos.row);
    const QVector<TerminalCell> line = contentRowCells(cr);

    auto isWordChar = [](const QString& s) {
        if (s.isEmpty()) return false;
        const QChar c = s.at(0);
        return c.isLetterOrNumber() || c == QLatin1Char('_')
            || c == QLatin1Char('.') || c == QLatin1Char('-')
            || c == QLatin1Char('/') || c == QLatin1Char(':');
    };

    int startCol = pos.col;
    int endCol   = pos.col;
    while (startCol > 0 && isWordChar(line.value(startCol - 1).text)) --startCol;
    while (endCol + 1 < line.size() && isWordChar(line.value(endCol + 1).text)) ++endCol;

    // 双击后如果用户接着拖选，锚点应该是词起点
    m_selAnchor = { pos.row, startCol };
    m_selStart  = { pos.row, startCol };
    m_selEnd    = { pos.row, endCol };
    m_selecting = true;    // 允许双击后继续拖选扩展选区
    emit selectionChanged();
    viewport()->update();
    e->accept();
}

void TerminalView::leaveEvent(QEvent* e)
{
    if (!m_hoveredLink.isEmpty()) {
        m_hoveredLink.clear();
        m_hoveredLinkStart = {};
        m_hoveredLinkEnd   = {};
        viewport()->setCursor(Qt::IBeamCursor);
        viewport()->update();
    }
    QAbstractScrollArea::leaveEvent(e);
}

// ============================================================================
// 右键菜单
// ============================================================================
void TerminalView::contextMenuEvent(QContextMenuEvent* e)
{
    //  鼠标模式开启时，右键已经通过 mousePressEvent 转发给 PTY 了，
    //   这里不再弹菜单。但 Shift+右键例外——用户想复制时按住 Shift 即可。
    if (m_model && m_model->mouseProtocol() != 0
        && !(e->modifiers() & Qt::ShiftModifier)) {
        e->accept();
        return;
    }

    QMenu menu(this);

    const bool hasSelection = isSelectionActive();
    const bool canPaste     = !QApplication::clipboard()->text().isEmpty();

    QAction* copyAct  = menu.addAction(tr("复制"));
    copyAct->setShortcut(QKeySequence::Copy);
    copyAct->setEnabled(hasSelection);

    QAction* pasteAct = menu.addAction(tr("粘贴"));
    pasteAct->setShortcut(QKeySequence::Paste);
    pasteAct->setEnabled(canPaste);

    menu.addSeparator();

    QAction* selectAllAct = menu.addAction(tr("全选"));
    QAction* clearAct     = menu.addAction(tr("清屏"));

    QAction* chosen = menu.exec(e->globalPos());
    if (chosen == copyAct) {
        QApplication::clipboard()->setText(selectedText());
    } else if (chosen == pasteAct) {
        QString text = QApplication::clipboard()->text();
        text.replace(QLatin1String("\r\n"), QLatin1String("\r"));
        text.replace(QLatin1Char('\n'), QLatin1Char('\r'));
        if (!text.isEmpty()) emit inputRequested(text.toUtf8());
    } else if (chosen == selectAllAct) {
        if (m_model) {
            m_selStart = { 0, 0 };
            m_selEnd   = { visibleRowCount() - 1, visibleColCount() - 1 };
            normalizeSelection();
            emit selectionChanged();
            viewport()->update();
        }
    } else if (chosen == clearAct) {
        emit clearRequested();
    }
}

} // namespace SprintIDE