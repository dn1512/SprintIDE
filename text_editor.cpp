#include "text_editor.h"
// #include "syntax_highlighter.h"
#include "language/language_highlighter.h"
#include "font_manager.h"
#include "theme_manager.h"
#include "language/editor_rules.h"
#include "editor/completion_delegate.h"

#include <QPainter>
#include <QTextBlock>
#include <QSet>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextLayout>
#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QContextMenuEvent>
#include <QToolTip>
#include <QDebug>
#include <QInputDialog>
#include <QStringConverter>  // Qt6中用于编码
#include <QFontDatabase>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QTimer>
#include <QHelpEvent>
#include <QPainterPath>
#include <QLabel>
#include <QRect>
#include <QHBoxLayout>
#include <QGuiApplication>
#include <QSvgRenderer>
#include <QPixmap>
#include <QHash>
#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>


namespace SprintIDE {

// ========== LineNumberArea类实现 ==========

LineNumberArea::LineNumberArea(TextEditor *editor)
    : QWidget(editor), m_editor(editor)
{
    setAutoFillBackground(true);
    // 让鼠标事件穿透到父控件（TextEditor）
    // setAttribute(Qt::WA_TransparentForMouseEvents);
}

QSize LineNumberArea::sizeHint() const
{
    int digits = 1;
    int max = qMax(1, m_editor->lineCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int width = 8 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    // qDebug() << "LineNumberArea sizeHint width:" << width;  // 添加调试
    return QSize(width, 0);
}

void LineNumberArea::contextMenuEvent(QContextMenuEvent* event)
{
    int y = event->y();
    // 与 mousePressEvent 相同的行号计算方式
    const int line = m_editor->cursorForPosition(event->pos()).blockNumber() + 1;
    if (line < 1 || line > m_editor->lineCount()) {
        QWidget::contextMenuEvent(event);
        return;
    }

    // 只有已有断点才提供"编辑条件"选项
    if (!m_editor->hasBreakpoint(line)) {
        QWidget::contextMenuEvent(event);
        return;
    }

    QMenu menu;
    QAction* editCond = menu.addAction(tr("编辑断点条件..."));
    QAction* removeBp = menu.addAction(tr("移除断点"));

    QAction* chosen = menu.exec(event->globalPos());
    if (chosen == editCond) {
        QString oldCond;
        // 从 MainWindow 侧拿不到，简单起见弹空输入框，用户重填
        bool ok;
        QString cond = QInputDialog::getText(
            this, tr("断点条件"),
            tr("第 %1 行的断点条件（空 = 无条件）:").arg(line),
            QLineEdit::Normal, oldCond, &ok);
        if (ok) {
            emit m_editor->breakpointConditionChanged(
                m_editor->filePath(), line, cond);
        }
    } else if (chosen == removeBp) {
        emit m_editor->breakpointToggled(m_editor->filePath(), line);
    }
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    // ---------- 背景 ----------
    const QColor bg = ThemeManager::instance().color(
        ThemeManager::UIElement::LineNumberAreaBackground);
    painter.fillRect(event->rect(), bg);

    QTextBlock block = m_editor->firstVisibleBlock();
    int blockNumber = block.blockNumber();
    QPointF contentOffset = m_editor->contentOffset();
    qreal top = m_editor->blockBoundingGeometry(block).translated(contentOffset).top();
    qreal bottom = top + m_editor->blockBoundingRect(block).height();
    int currentLine = m_editor->currentLine();

    // 尺寸常量
    constexpr int clickAreaWidth = 22;
    constexpr int dotX           = 6;      // 断点图标左上角 x
    constexpr int dotSize        = 12;     // 断点图标尺寸
    constexpr int foldAreaWidth  = 16;     // 折叠按钮区域宽度
    constexpr int foldX          = 2;      // 折叠图标左上角 x
    constexpr int foldSize       = 10;     // 折叠图标尺寸

    //  主题相关路径 + 是否深色主题
    const bool isDark = ThemeManager::instance().isDarkTheme();
    const QString foldClosedPath = isDark
                                       ? ":/icons/icons/branch-closed-dark.svg"
                                       : ":/icons/icons/branch-closed-light.svg";
    const QString foldOpenPath   = isDark
                                     ? ":/icons/icons/branch-open-dark.svg"
                                     : ":/icons/icons/branch-open-light.svg";
    const QString bpNormalPath   = ":/icons/icons/breakpoint.svg";
    const QString bpCondPath     = ":/icons/icons/breakpoint_conditional.svg";
    const QString bpDisabledPath = ":/icons/icons/breakpoint_disabled.svg";

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const int lineNumber = blockNumber + 1;
            const QString number = QString::number(lineNumber);

            // ========== 1. 折叠三角（SVG） ==========
            if (m_editor->isFoldableBlock(lineNumber)) {
                const bool isFolded = m_editor->isBlockFolded(lineNumber);
                const QString path = isFolded ? foldClosedPath : foldOpenPath;

                const QPixmap pm = m_editor->cachedIcon(path,
                                                        QSize(foldSize, foldSize));
                const int btnY = static_cast<int>(top + (bottom - top) / 2
                                                  - foldSize / 2);
                painter.drawPixmap(foldX, btnY, pm);
            }

            // ========== 2. 断点图标（SVG） ==========
            if (m_editor->hasBreakpoint(lineNumber)) {
                const bool hasCond  = m_editor->m_conditionalBreakpoints
                                         .contains(lineNumber);
                const bool enabled  = !m_editor->m_disabledBreakpoints
                                          .contains(lineNumber);  // ← 见下方说明

                const QString path = !enabled
                                         ? bpDisabledPath
                                         : (hasCond ? bpCondPath : bpNormalPath);

                const QPixmap pm = m_editor->cachedIcon(path,
                                                        QSize(dotSize, dotSize));
                const int dotY = static_cast<int>(top + (bottom - top) / 2
                                                  - dotSize / 2);
                painter.drawPixmap(dotX, dotY, pm);
            }

            // ========== 3. 行号（保留原逻辑） ==========
            if (lineNumber == currentLine) {
                painter.setPen(ThemeManager::instance().color(
                    ThemeManager::UIElement::CurrentLineNumberColor));
                painter.setFont(QFont(font().family(), font().pointSize(),
                                      QFont::Bold));
            } else {
                painter.setPen(ThemeManager::instance().color(
                    ThemeManager::UIElement::LineNumberColor));
                painter.setFont(font());
            }
            painter.drawText(clickAreaWidth, static_cast<int>(top),
                             width() - clickAreaWidth - 16,
                             static_cast<int>(bottom - top),
                             Qt::AlignRight | Qt::AlignVCenter, number);

            // ========== 4. 错误/警告标记（保留手绘，形状简单更快） ==========
            if (m_editor->hasErrorMarker(lineNumber)) {
                const int iconSize = 12;
                const int iconX = width() - 30;
                const int iconY = static_cast<int>(top + (bottom - top) / 2
                                                   - iconSize / 2);
                painter.setBrush(QColor(255, 50, 50));
                painter.setPen(Qt::NoPen);
                painter.drawEllipse(iconX, iconY, iconSize, iconSize);
                painter.setPen(QPen(Qt::white, 2));
                const int pad = 3;
                painter.drawLine(iconX + pad, iconY + pad,
                                 iconX + iconSize - pad, iconY + iconSize - pad);
                painter.drawLine(iconX + iconSize - pad, iconY + pad,
                                 iconX + pad, iconY + iconSize - pad);
            }

            if (m_editor->hasWarningMarker(lineNumber)) {
                const int iconSize = 12;
                const int iconX = width() - 30;
                const int iconY = static_cast<int>(top + (bottom - top) / 2
                                                   - iconSize / 2);
                painter.setBrush(QColor(255, 200, 0));
                painter.setPen(Qt::NoPen);
                QPolygon triangle;
                triangle << QPoint(iconX + iconSize / 2, iconY)
                         << QPoint(iconX, iconY + iconSize)
                         << QPoint(iconX + iconSize, iconY + iconSize);
                painter.drawPolygon(triangle);
                painter.setPen(QPen(Qt::black, 1));
                painter.setFont(QFont("Arial", 9, QFont::Bold));
                painter.drawText(iconX + 3, iconY + iconSize - 2, "!");
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + m_editor->blockBoundingRect(block).height();
        ++blockNumber;
    }
}
// ========== TextEditor类实现 ==========

void LineNumberArea::mousePressEvent(QMouseEvent *event)
{
    // 获取点击位置相对于 LineNumberArea 的 x 坐标
    int x = event->x();
    int clickAreaWidth = 26;  // 左侧断点点击区宽度
    int foldAreaWidth = 16;  // 折叠按钮区域宽度

    // 先算出点击的是哪一行
    const int line = m_editor->cursorForPosition(event->pos()).blockNumber() + 1;
    if (line < 1 || line > m_editor->lineCount()) {
        m_editor->mousePressEvent(event);
        return;
    }

    //    不可折叠行的折叠区不拦截，交给下方断点逻辑
    if (x < foldAreaWidth && m_editor->isFoldableBlock(line)) {
        m_editor->toggleFold(line);
        return;
    }

    // 断点点击：整个左侧点击区（含不可折叠行的折叠区）
    if (x < clickAreaWidth) {
        emit m_editor->breakpointToggled(m_editor->filePath(), line);
        return;
    }

    // 4. 落在行号数字区 → 转发给编辑器（选中行、拖选等）
    m_editor->mousePressEvent(event);
}

TextEditor::TextEditor(QWidget *parent)
    : QPlainTextEdit(parent),
    m_filePath(""),
    m_isModified(false),
    m_showLineNumbers(true),
    m_highlightCurrentLine(false),
    m_useSpacesForTab(true),
    m_tabWidth(4),
    m_currentDebugLine(-1),
    m_lineNumberArea(nullptr),
    m_highlighter(nullptr),
    m_contextMenu(nullptr),
    m_findBar(nullptr),
    m_findLineEdit(nullptr),
    m_replaceLineEdit(nullptr),
    m_caseSensitiveCheck(nullptr),
    m_wholeWordCheck(nullptr),
    m_findNextButton(nullptr),
    m_findPreviousButton(nullptr),
    m_replaceButton(nullptr),
    m_replaceAllButton(nullptr),
    m_closeButton(nullptr),
    m_toggleReplaceButton(nullptr),
    m_matchCountLabel(nullptr),
    m_currentMatchIndex(-1),
    m_findBarVisible(false),
    m_replaceMode(false)
{
    initEditor();

    // 错误标记定时器（用于延迟重绘）
    m_errorMarkerTimer = new QTimer(this);
    m_errorMarkerTimer->setSingleShot(true);
    m_errorMarkerTimer->setInterval(50);
    connect(m_errorMarkerTimer, &QTimer::timeout, this, &TextEditor::onErrorMarkerTimer);

    // 启用鼠标追踪（用于悬停提示）
    setMouseTracking(true);
    viewport()->setMouseTracking(true);

    setupFindBar();
}

TextEditor::~TextEditor()
{
    delete m_lineNumberArea;
    delete m_highlighter;
    delete m_contextMenu;
    delete m_errorMarkerTimer;
}

void TextEditor::initEditor()
{
    setLineWrapMode(QPlainTextEdit::NoWrap);


    // ============================================================
    //    从 ThemeManager 取字体
    //   - ThemeManager 在 Application::initialize() 里已经从
    //     ConfigManager 读入了用户设置（family + size）
    //   - 如果 ThemeManager 还没初始化（极端情况），回退到 FontManager 默认值
    // ============================================================
    QFont editorFont = ThemeManager::instance().editorFont();

    // 创建行号区域
    m_lineNumberArea = new LineNumberArea(this);
    m_lineNumberArea->setFont(editorFont);

    if (editorFont.family().isEmpty() || editorFont.pointSize() <= 0) {
        // 回退：用 FontManager 的默认等宽字体
        FontManager::loadEmbeddedFonts();
        editorFont = FontManager::getDefaultMonoFont(12);
    }

    // 确保等宽属性
    editorFont.setFixedPitch(true);
    editorFont.setStyleHint(QFont::Monospace);

    setEditorFont(editorFont);

    // 文档默认字体（影响粘贴的文本、新插入的文本）
    document()->setDefaultFont(editorFont);

    // Tab 键宽度跟随字体
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * m_tabWidth);

    // 语法高亮器
    m_highlighter = new LanguageHighlighter(document());
    m_highlighter->setTheme(ThemeManager::instance().isDarkTheme());

    // 上下文菜单
    createContextMenu();

    // 信号连接
    connect(this, &TextEditor::blockCountChanged,
            this, &TextEditor::updateLineNumberAreaWidth);
    connect(this, &TextEditor::updateRequest,
            this, &TextEditor::updateLineNumberArea);
    connect(this, &TextEditor::cursorPositionChanged,
            this, &TextEditor::onCursorPositionChanged);
    connect(this, &TextEditor::textChanged,
            this, &TextEditor::onTextChanged);

    updateLineNumberAreaWidth();
    applyDefaultPalette();
    setupCompletion();

    // ---- LSP didChange 防抖：停止输入 180ms 后才发 ----
    m_lspChangeTimer = new QTimer(this);
    m_lspChangeTimer->setSingleShot(true);
    m_lspChangeTimer->setInterval(180);
    connect(m_lspChangeTimer, &QTimer::timeout, this, [this]() {
        if (!m_pendingLspChange) return;
        m_pendingLspChange = false;
        if (!m_filePath.isEmpty()) {
            emit documentChangedForLsp(m_filePath, toPlainText());
        }
    });

    // 滚动条错误标记
    if (verticalScrollBar()) {
        verticalScrollBar()->installEventFilter(this);
    }
}

void TextEditor::setupFindBar()
{
    m_findBar = new QWidget(this);
    m_findBar->setVisible(false);
    m_findBar->setStyleSheet(
        "QWidget { background-color: #2D2D2D; border-top: 1px solid #555555; }"
        "QLineEdit { background-color: #1E1E1E; color: #E0E0E0; border: 1px solid #555555; padding: 2px; }"
        "QCheckBox { color: #E0E0E0; }"
        "QPushButton { background-color: #3C3C3C; color: #E0E0E0; border: 1px solid #555555; padding: 4px 8px; }"
        "QPushButton:hover { background-color: #4A4A4A; }"
        );

    QHBoxLayout* layout = new QHBoxLayout(m_findBar);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(6);

    QLabel* findLabel = new QLabel(tr("查找:"), m_findBar);
    findLabel->setStyleSheet("color: #E0E0E0;");
    layout->addWidget(findLabel);

    m_findLineEdit = new QLineEdit(m_findBar);
    m_findLineEdit->setPlaceholderText(tr("输入查找内容..."));
    m_findLineEdit->setMinimumWidth(200);
    connect(m_findLineEdit, &QLineEdit::textChanged, this, &TextEditor::onFindTextChanged);
    connect(m_findLineEdit, &QLineEdit::returnPressed, this, &TextEditor::onFindNextClicked);
    layout->addWidget(m_findLineEdit);

    m_caseSensitiveCheck = new QCheckBox("Aa", m_findBar);
    m_caseSensitiveCheck->setToolTip(tr("大小写敏感"));
    m_caseSensitiveCheck->setStyleSheet("color: #E0E0E0;");
    connect(m_caseSensitiveCheck, &QCheckBox::toggled, this, &TextEditor::onToggleCaseSensitive);
    layout->addWidget(m_caseSensitiveCheck);

    m_wholeWordCheck = new QCheckBox("\"\"", m_findBar);
    m_wholeWordCheck->setToolTip(tr("全字匹配"));
    m_wholeWordCheck->setStyleSheet("color: #E0E0E0;");
    connect(m_wholeWordCheck, &QCheckBox::toggled, this, &TextEditor::onToggleWholeWord);
    layout->addWidget(m_wholeWordCheck);

    m_findPreviousButton = new QPushButton("▲", m_findBar);
    m_findPreviousButton->setToolTip(tr("上一个 (Shift+F3)"));
    m_findPreviousButton->setFixedSize(30, 24);
    connect(m_findPreviousButton, &QPushButton::clicked, this, &TextEditor::onFindPreviousClicked);
    layout->addWidget(m_findPreviousButton);

    m_findNextButton = new QPushButton("▼", m_findBar);
    m_findNextButton->setToolTip(tr("下一个 (F3)"));
    m_findNextButton->setFixedSize(30, 24);
    connect(m_findNextButton, &QPushButton::clicked, this, &TextEditor::onFindNextClicked);
    layout->addWidget(m_findNextButton);

    m_matchCountLabel = new QLabel("0/0", m_findBar);
    m_matchCountLabel->setStyleSheet("color: #888888;");
    m_matchCountLabel->setMinimumWidth(40);
    layout->addWidget(m_matchCountLabel);

    m_toggleReplaceButton = new QPushButton("▼", m_findBar);
    m_toggleReplaceButton->setToolTip(tr("显示替换"));
    m_toggleReplaceButton->setFixedSize(30, 24);
    connect(m_toggleReplaceButton, &QPushButton::clicked, this, &TextEditor::onToggleReplaceMode);
    layout->addWidget(m_toggleReplaceButton);

    m_replaceLineEdit = new QLineEdit(m_findBar);
    m_replaceLineEdit->setPlaceholderText(tr("替换为..."));
    m_replaceLineEdit->setMinimumWidth(150);
    m_replaceLineEdit->setVisible(false);
    connect(m_replaceLineEdit, &QLineEdit::textChanged, this, &TextEditor::onReplaceTextChanged);
    connect(m_replaceLineEdit, &QLineEdit::returnPressed, this, &TextEditor::onReplaceClicked);
    layout->addWidget(m_replaceLineEdit);

    m_replaceButton = new QPushButton(tr("替换"), m_findBar);
    m_replaceButton->setVisible(false);
    m_replaceButton->setFixedWidth(50);
    connect(m_replaceButton, &QPushButton::clicked, this, &TextEditor::onReplaceClicked);
    layout->addWidget(m_replaceButton);

    m_replaceAllButton = new QPushButton(tr("全部替换"), m_findBar);
    m_replaceAllButton->setVisible(false);
    m_replaceAllButton->setFixedWidth(70);
    connect(m_replaceAllButton, &QPushButton::clicked, this, &TextEditor::onReplaceAllClicked);
    layout->addWidget(m_replaceAllButton);

    layout->addStretch();

    m_closeButton = new QPushButton("×", m_findBar);
    m_closeButton->setToolTip(tr("关闭 (Esc)"));
    m_closeButton->setFixedSize(30, 24);
    m_closeButton->setStyleSheet("QPushButton { background-color: #555555; color: #E0E0E0; border: none; }");
    connect(m_closeButton, &QPushButton::clicked, this, &TextEditor::onCloseFindBar);
    layout->addWidget(m_closeButton);

    m_findBar->setParent(this);
    m_findBar->setGeometry(0, height() - 40, width(), 40);
}

void TextEditor::fixParenthesisHeight()
{
    // 创建一个字体列表，确保括号字符有统一的来源
    QStringList fontFamilies;

// 添加首选字体
#if defined(Q_OS_WIN)
    fontFamilies << "Consolas" << "Courier New" << "Lucida Console";
#elif defined(Q_OS_MAC)
    fontFamilies << "Menlo" << "Monaco" << "Courier New";
#else
    fontFamilies << "DejaVu Sans Mono" << "Liberation Mono" << "Courier New";
#endif

    // 设置字体族
    QFont font;
    font.setFamilies(fontFamilies);
    font.setPointSize(12);
    font.setFixedPitch(true);

    // 应用字体
    setEditorFont(font);

    // 使用QTextDocument的默认字体，确保所有字符使用同一字体
    document()->setDefaultFont(font);
}

void TextEditor::applyDefaultPalette()
{
    // 设置一个基本的调色板，确保文本可读
    QPalette palette = this->palette();

    // 默认情况下，让Qt使用系统主题

    // 使用CSS样式确保文本显示一致
    QString styleSheet = QString(
                             "QPlainTextEdit {"
                             "  font-family: '%1', monospace;"
                             "  font-size: %2pt;"
                             "  line-height: 1.2;"
                             "  font-weight: normal;"
                             "}"
                             "QPlainTextEdit QTextBlock {"
                             "  vertical-align: middle;"
                             "}"
                             ).arg(font().family()).arg(font().pointSize());

    setStyleSheet(styleSheet);

    // 更新当前行高亮设置
    if (!m_highlightCurrentLine) {
        // 清空所有额外选择（包括当前行高亮）
        QList<QTextEdit::ExtraSelection> empty;
        setExtraSelections(empty);
    }

    // 强制更新一次
    update();
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }
}

void TextEditor::initFontMetrics()
{
    // 确保使用等宽字体
    QFont font = this->font();

// 设置字体属性 - 使用可靠的等宽字体
#if defined(Q_OS_WIN)
    font.setFamily("Consolas");
#elif defined(Q_OS_MAC)
    font.setFamily("Menlo");
#else
    font.setFamily("DejaVu Sans Mono");
#endif

    font.setFixedPitch(true);
    font.setKerning(false); // 禁用字距调整

    // 使用单一的风格策略
    font.setStyleStrategy(QFont::PreferAntialias);

    // 确保字体大小一致
    font.setPointSize(12);

    // 应用字体
    setEditorFont(font);

    // 检查字体度量
    QFontMetrics fm(font);
    int charHeight = fm.height();

    // qDebug() << "Font metrics - Height:" << charHeight
             // << "Family:" << font.family()
             // << "Fixed pitch:" << font.fixedPitch();
}

void TextEditor::fixFontRendering()
{
    // 首先，检查系统可用的等宽字体
    QFontDatabase fontDatabase;
    QStringList fontFamilies = fontDatabase.families();

    // 寻找最优的等宽字体
    QString bestFont;

    // 字体优先级列表
    QStringList preferredFonts;

#if defined(Q_OS_WIN)
    preferredFonts << "Consolas" << "Courier New" << "Lucida Console";
#elif defined(Q_OS_MAC)
    preferredFonts << "Menlo" << "Monaco" << "Courier New" << "SF Mono";
#else
    preferredFonts << "DejaVu Sans Mono" << "Liberation Mono" << "Ubuntu Mono" << "Courier New";
#endif

    // 寻找可用的字体
    for (const QString& font : preferredFonts) {
        if (fontFamilies.contains(font)) {
            bestFont = font;
            break;
        }
    }

    if (bestFont.isEmpty()) {
        // 使用默认的等宽字体
        bestFont = "Monospace";
    }

    // 设置字体
    QFont font(bestFont, 12);

    // 关键：强制等宽字体特性
    font.setFixedPitch(true);

// 对于Qt6，使用setFamilies确保括号字符来自同一字体族
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QStringList fontFamilyList;
    fontFamilyList << bestFont << "Monospace";
    font.setFamilies(fontFamilyList);
#endif

    // 设置字体渲染选项
    font.setStyleHint(QFont::Monospace);
    font.setHintingPreference(QFont::PreferFullHinting); // 完整提示，确保字符对齐

    // 应用字体
    setEditorFont(font);

    // 设置文档的默认字体，确保所有字符使用同一字体
    document()->setDefaultFont(font);

    // 打印调试信息
    // qDebug() << "使用字体:" << bestFont
    //          << "Point size:" << font.pointSize()
    //          << "Fixed pitch:" << font.fixedPitch();
}

void TextEditor::testFontMetrics()
{
    QFontMetrics fm(font());

    // qDebug() << "=== 字体度量测试 ===";
    // qDebug() << "字体家族:" << font().family();
    // qDebug() << "是否为等宽:" << font().fixedPitch();

    // 测试括号对齐
    QString testText = "()[]{}";
    for (QChar ch : testText) {
        QRect rect = fm.boundingRect(ch);
        // qDebug() << "字符" << ch
        //          << ": 宽度=" << fm.horizontalAdvance(ch)
        //          << ", 高度=" << rect.height()
        //          << ", Y偏移=" << rect.y()
        //          << ", 上升=" << fm.ascent()
        //          << ", 下降=" << fm.descent();
    }

    // 测试编程常用字符
    QString programmingChars = "iIl1|";
    for (QChar ch : programmingChars) {
        int width = fm.horizontalAdvance(ch);
        // qDebug() << "字符" << ch << "宽度:" << width;
    }
}

void TextEditor::createContextMenu()
{
    m_contextMenu = new QMenu(this);

    // 创建菜单动作
    m_undoAction = m_contextMenu->addAction(tr("撤销 (Ctrl+Z)"), this, &TextEditor::undo);
    m_redoAction = m_contextMenu->addAction(tr("重做 (Ctrl+Y)"), this, &TextEditor::redo);
    m_contextMenu->addSeparator();

    m_cutAction = m_contextMenu->addAction(tr("剪切 (Ctrl+X)"), this, &TextEditor::cut);
    m_copyAction = m_contextMenu->addAction(tr("复制 (Ctrl+C)"), this, &TextEditor::copy);
    m_pasteAction = m_contextMenu->addAction(tr("粘贴 (Ctrl+V)"), this, &TextEditor::paste);
    m_contextMenu->addSeparator();

    m_selectAllAction = m_contextMenu->addAction(tr("全选 (Ctrl+A)"), this, &TextEditor::selectAll);
    m_contextMenu->addSeparator();

    m_commentAction = m_contextMenu->addAction(tr("注释选中行 (Ctrl+/)"), this, &TextEditor::commentSelection);
    m_uncommentAction = m_contextMenu->addAction(tr("取消注释 (Ctrl+/)"), this, &TextEditor::uncommentSelection);
    m_contextMenu->addSeparator();

    m_duplicateLineAction = m_contextMenu->addAction(tr("复制当前行 (Ctrl+D)"), this, &TextEditor::duplicateLine);
    m_deleteLineAction = m_contextMenu->addAction(tr("删除当前行 (Ctrl+Y)"), this, &TextEditor::deleteLine);
    m_contextMenu->addSeparator();

    // 修复：连接到一个实际的槽函数，而不是不存在的信号
    m_gotoLineAction = m_contextMenu->addAction(tr("跳转到行 (Ctrl+G)..."), this, &TextEditor::onGotoLineAction);
}

bool TextEditor::loadFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("文件打开失败"),
                             QString(tr("无法打开文件：%1\n错误：%2"))
                                 .arg(filePath).arg(file.errorString()));
        return false;
    }

    QTextStream stream(&file);
    stream.setAutoDetectUnicode(true);
    QString content = stream.readAll();
    file.close();

    // 进入静默期：此后所有程序内部的文档改动都不算"用户修改"
    beginSilentModification();


    // 加载文本
    setPlainText(content);
    m_filePath = filePath;

    // 应用行距
    applyLineHeight();

    // 4. 应用语法高亮
    setLanguageByFilePath(filePath);

    // 5. 计算折叠
    m_foldedLines.clear();

    // 结束静默期，重置为"干净"状态
    endSilentModification();
    m_isModified = false;
    document()->setModified(false);

    // 发出加载完成信号
    emit fileLoaded(filePath);

    return true;
}

bool TextEditor::reloadFile()
{
    if (m_filePath.isEmpty()) {
        return false;
    }
    // 记住当前光标位置
    int line = currentLine();
    int col = currentColumn();
    bool success = loadFile(m_filePath);
    if (success) {
        // 恢复光标到近似位置
        gotoLine(line);
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, col - 1);
        setTextCursor(cursor);
    }
    return success;
}

bool TextEditor::saveFile()
{
    if (m_filePath.isEmpty()) {
        QString filePath = QFileDialog::getSaveFileName(
            this, tr("保存文件"), "", tr("C++文件 (*.cpp *.h *.hpp);;所有文件 (*.*)"));

        if (filePath.isEmpty()) {
            return false;
        }

        m_filePath = filePath;
    }

    QFile file(m_filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("文件保存失败"),
                             QString(tr("无法保存文件：%1\n错误：%2"))
                                 .arg(m_filePath).arg(file.errorString()));
        return false;
    }

    QTextStream stream(&file);
    // Qt6中使用setEncoding而不是setCodec
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf8);
#else
    stream.setCodec("UTF-8");
#endif

    if (m_formatOnSave) {
        QString err;
        if (!runExternalFormatter(&err)) {
            if (!err.isEmpty()) {
                qWarning() << "[TextEditor] 格式化失败:" << err;
            }
            // 格式化失败不阻断保存
        }
    }

    stream << toPlainText();
    file.close();

    m_isModified = false;
    document()->setModified(false);

    // 保存成功后，记录本地历史
    if (m_localHistory && !m_filePath.isEmpty()) {
        int version = m_localHistory->saveVersion(m_filePath, toPlainText());
        if (version > 0) {
            qDebug() << "[TextEditor] 本地历史已保存，版本:" << version;
        }
    }

    emit fileSaved(m_filePath);
    emit modificationChanged(false);


    return true;
}

bool TextEditor::saveAs(const QString &filePath)
{
    QString oldPath = m_filePath;
    m_filePath = filePath;

    bool success = saveFile();

    if (!success) {
        m_filePath = oldPath;
    }

    return success;
}

bool TextEditor::isModified() const
{
    return m_isModified || document()->isModified();
}

// ========== 设置本地历史管理器 ==========
void TextEditor::setLocalHistory(LocalHistory *history)
{
    m_localHistory = history;
}

QString TextEditor::filePath() const
{
    return m_filePath;
}

void TextEditor::setFilePath(const QString& path)
{
    if (m_filePath == path) return;

    // 保存当前 modified 状态（语言切换可能污染它）
    const bool wasModified = m_isModified || document()->isModified();
    beginSilentModification();

    const QString oldExt = QFileInfo(m_filePath).suffix().toLower();
    const QString newExt = QFileInfo(path).suffix().toLower();

    m_filePath = path;

    // 扩展名变了 → 语言可能变（例如 .txt 改成 .py）
    if (oldExt != newExt && m_highlighter) {
        setLanguageByFilePath(path);
    }

    endSilentModification();
    m_isModified = wasModified;
    document()->setModified(wasModified);
}

QString TextEditor::fileName() const
{
    if (m_filePath.isEmpty()) {
        return tr("未命名");
    }

    QFileInfo fileInfo(m_filePath);
    return fileInfo.fileName();
}

void TextEditor::setShowLineNumbers(bool show)
{
    m_showLineNumbers = show;
    m_lineNumberArea->setVisible(show);
    updateLineNumberAreaWidth();
}

void TextEditor::setHighlightCurrentLine(bool highlight)
{
    m_highlightCurrentLine = highlight;

    if (highlight) {
        highlightCurrentLine();
    } else {
        QList<QTextEdit::ExtraSelection> extraSelections;
        setExtraSelections(extraSelections);
    }
}

void TextEditor::setUseSpacesForTab(bool useSpaces, int tabWidth)
{
    m_useSpacesForTab = useSpaces;
    m_tabWidth = tabWidth;

    if (useSpaces) {
        setTabStopDistance(fontMetrics().horizontalAdvance(' ') * tabWidth);
    } else {
        setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 8);
    }
}

void TextEditor::setEditorFont(const QFont &font)
{
    // 只有真正需要改字体时才走静默逻辑
    const bool wasModified = m_isModified || document()->isModified();
    beginSilentModification();

    QPlainTextEdit::setFont(font);
    document()->setDefaultFont(font);       // 确保新插入的文本用同一字体

    // Tab 宽度跟随字体尺寸
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * m_tabWidth);

    // 应用行距（每次字体变化都重设）
    m_lastAppliedLineHeight = -1.0;   // 使缓存失效，强制重新应用
    applyLineHeight();

    // 行号区字体也同步
    if (m_lineNumberArea) {
        m_lineNumberArea->setFont(font);
        m_lineNumberArea->update();
    }

    applyDefaultPalette();

    if (m_lineNumberArea) {
        updateLineNumberAreaWidth();
    }

    endSilentModification();
    m_isModified = wasModified;
    document()->setModified(wasModified);
}

// ========== 主题相关方法实现 ==========

void TextEditor::applyTheme(const QString &themeName)
{
    // 保存当前真实状态，因为主题应用不应改变"是否被用户修改"的语义
    const bool wasModified = m_isModified || document()->isModified();

    beginSilentModification();

    const bool isDark = (themeName == "dark");
    ThemeManager& tm = ThemeManager::instance();
    QFont tmFont = tm.editorFont();
    tmFont.setFixedPitch(true);
    tmFont.setStyleHint(QFont::Monospace);

    if (font().family() != tmFont.family() ||
        font().pointSize() != tmFont.pointSize()) {
        setEditorFont(tmFont);
        document()->setDefaultFont(tmFont);
    }

    if (m_highlighter) {
        m_highlighter->setTheme(isDark);
        m_highlighter->rehighlight();          // ← 这里会触发 textChanged
    }

    if (isDark) {
        applyDarkTheme();
    } else {
        applyLightTheme();
    }

    if (m_lineNumberArea) {
        m_lineNumberArea->setFont(tmFont);
        updateLineNumberAreaPalette(isDark);
    }

    // 结束静默期，并把 modified 恢复到之前的真实值
    endSilentModification();
    m_isModified = wasModified;
    document()->setModified(wasModified);

    // 主题切换后，缓存里的旧色图标作废
    invalidateIconCache();

    update();
    viewport()->update();
}

void TextEditor::updateLineNumberAreaPalette(bool isDark)
{
    QPalette palette = m_lineNumberArea->palette();

    if (isDark) {
        palette.setColor(QPalette::Window,
                         ThemeManager::instance().color(
                             ThemeManager::UIElement::LineNumberAreaBackground));
        palette.setColor(QPalette::WindowText, QColor(150, 150, 150));
    } else {
        palette.setColor(QPalette::Window, QColor(240, 240, 240));
        palette.setColor(QPalette::WindowText, QColor(100, 100, 100));
    }

    m_lineNumberArea->setPalette(palette);
    m_lineNumberArea->update();
}

void TextEditor::applyDarkTheme()
{
    ThemeManager& tm = ThemeManager::instance();

    QPalette palette = this->palette();
    palette.setColor(QPalette::Base,   tm.color(ThemeManager::UIElement::EditorBackground));
    palette.setColor(QPalette::Text,   tm.color(ThemeManager::UIElement::TextColor));
    palette.setColor(QPalette::Highlight,
                     tm.color(ThemeManager::UIElement::SelectionBackground));
    palette.setColor(QPalette::HighlightedText,
                     tm.color(ThemeManager::UIElement::TextColor));
    palette.setColor(QPalette::Window,
                     tm.color(ThemeManager::UIElement::MainWindowBackground));
    palette.setColor(QPalette::WindowText,
                     tm.color(ThemeManager::UIElement::TextColor));
    palette.setColor(QPalette::Button,
                     tm.color(ThemeManager::UIElement::ButtonBackground));
    palette.setColor(QPalette::ButtonText,
                     tm.color(ThemeManager::UIElement::TextColor));

    setPalette(palette);

    if (m_lineNumberArea) {
        QPalette lnPal = m_lineNumberArea->palette();
        lnPal.setColor(QPalette::Window,
                       tm.color(ThemeManager::UIElement::LineNumberAreaBackground));
        lnPal.setColor(QPalette::WindowText,
                       tm.color(ThemeManager::UIElement::LineNumberColor));
        m_lineNumberArea->setPalette(lnPal);
        m_lineNumberArea->setAutoFillBackground(true);
        m_lineNumberArea->update();
    }

    // 查找栏同理，只是取色一样，其实可以用同一个 lambda 复用
    if (m_findBar) {
        m_findBar->setStyleSheet(QString(
                                     "QWidget { background-color: %1; border-top: 1px solid %2; }"
                                     "QLineEdit { background-color: %3; color: %4; "
                                     "  border: 1px solid %2; border-radius: 3px; padding: 2px 6px; }"
                                     "QLineEdit:focus { border: 1px solid %5; }"
                                     "QCheckBox { color: %4; }"
                                     "QPushButton { background-color: %6; color: %4; "
                                     "  border: 1px solid %2; border-radius: 3px; padding: 3px 8px; }"
                                     "QPushButton:hover { background-color: %7; }"
                                     "QLabel { color: %8; }"
                                     )
                                     .arg(tm.colorString(ThemeManager::UIElement::MainWindowBackground))
                                     .arg(tm.colorString(ThemeManager::UIElement::BorderColor))
                                     .arg(tm.colorString(ThemeManager::UIElement::EditorBackground))
                                     .arg(tm.colorString(ThemeManager::UIElement::TextColor))
                                     .arg(tm.colorString(ThemeManager::UIElement::PrimaryColor))
                                     .arg(tm.colorString(ThemeManager::UIElement::ButtonBackground))
                                     .arg(tm.colorString(ThemeManager::UIElement::HoverBackground))
                                     .arg(tm.colorString(ThemeManager::UIElement::SecondaryTextColor)));
    }

    if (m_highlightCurrentLine) {
        highlightCurrentLine();
    }

    update();
    viewport()->update();
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }
}

void TextEditor::applyLightTheme()
{
    ThemeManager& tm = ThemeManager::instance();

    // ============================================================
    // 1. 编辑器本体调色板（从 Polaris Solar Flare 取令牌）
    // ============================================================
    QPalette palette = this->palette();

    // 编辑区背景 + 文字
    palette.setColor(QPalette::Base,
                     tm.color(ThemeManager::UIElement::EditorBackground));      // #FFFFFF
    palette.setColor(QPalette::Text,
                     tm.color(ThemeManager::UIElement::TextColor));             // #2C3848

    // 选区
    palette.setColor(QPalette::Highlight,
                     tm.color(ThemeManager::UIElement::SelectionBackground));   // #D7E6FF
    palette.setColor(QPalette::HighlightedText,
                     tm.color(ThemeManager::UIElement::TextColor));

    // 整个控件的窗口色（比如滚动条所在区域），也走侧边栏色
    palette.setColor(QPalette::Window,
                     tm.color(ThemeManager::UIElement::MainWindowBackground));  // #F4F7FA
    palette.setColor(QPalette::WindowText,
                     tm.color(ThemeManager::UIElement::TextColor));

    // 按钮态（极少数场景会用到，比如内联查找栏）
    palette.setColor(QPalette::Button,
                     tm.color(ThemeManager::UIElement::ButtonBackground));      // #E8ECF0
    palette.setColor(QPalette::ButtonText,
                     tm.color(ThemeManager::UIElement::TextColor));

    setPalette(palette);

    // ============================================================
    // 2. 行号区调色板（自绘控件，必须单独设）
    // ============================================================
    if (m_lineNumberArea) {
        QPalette lnPal = m_lineNumberArea->palette();

        // 行号区背景与编辑器一致（Polaris 要求"行号区背景透明/与编辑器一致"）
        lnPal.setColor(QPalette::Window,
                       tm.color(ThemeManager::UIElement::LineNumberAreaBackground));
        // 行号默认文字色 #9AAEC2（浅色主题下用的是 SecondaryTextColor）
        lnPal.setColor(QPalette::WindowText,
                       tm.color(ThemeManager::UIElement::SecondaryTextColor));

        m_lineNumberArea->setPalette(lnPal);
        m_lineNumberArea->setAutoFillBackground(true);
        m_lineNumberArea->update();
    }

    // ============================================================
    // 3. 内联查找栏（如果已创建）重新套色
    // ============================================================
    if (m_findBar) {
        m_findBar->setStyleSheet(QString(
                                     "QWidget {"
                                     "  background-color: %1;"
                                     "  border-top: 1px solid %2;"
                                     "}"
                                     "QLineEdit {"
                                     "  background-color: %3;"
                                     "  color: %4;"
                                     "  border: 1px solid %2;"
                                     "  border-radius: 3px;"
                                     "  padding: 2px 6px;"
                                     "}"
                                     "QLineEdit:focus {"
                                     "  border: 1px solid %5;"
                                     "}"
                                     "QCheckBox { color: %4; }"
                                     "QPushButton {"
                                     "  background-color: %6;"
                                     "  color: %4;"
                                     "  border: 1px solid %2;"
                                     "  border-radius: 3px;"
                                     "  padding: 3px 8px;"
                                     "}"
                                     "QPushButton:hover {"
                                     "  background-color: %7;"
                                     "}"
                                     "QLabel { color: %8; }"
                                     )
                                     .arg(tm.colorString(ThemeManager::UIElement::MainWindowBackground))
                                     .arg(tm.colorString(ThemeManager::UIElement::BorderColor))
                                     .arg(tm.colorString(ThemeManager::UIElement::EditorBackground))
                                     .arg(tm.colorString(ThemeManager::UIElement::TextColor))
                                     .arg(tm.colorString(ThemeManager::UIElement::PrimaryColor))
                                     .arg(tm.colorString(ThemeManager::UIElement::ButtonBackground))
                                     .arg(tm.colorString(ThemeManager::UIElement::HoverBackground))
                                     .arg(tm.colorString(ThemeManager::UIElement::SecondaryTextColor)));
    }

    // ============================================================
    // 4. 关键修复：不要在这里清空 ExtraSelection
    //    原来代码：
    //        QList<QTextEdit::ExtraSelection> extraSelections;
    //        setExtraSelections(extraSelections);
    //    这样会把【查找高亮】【括号匹配】【当前调试行】全部清掉，
    //    所以这次改为只调用 highlightCurrentLine()（内部自己处理）
    // ============================================================
    if (m_highlightCurrentLine) {
        highlightCurrentLine();
    }

    // ============================================================
    // 5. 强制重绘
    // ============================================================
    update();
    viewport()->update();
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }
}

// ========== 代码编辑相关方法 ==========
void TextEditor::commentSelection()
{
    const QString cmt = m_editorRules.lineComment;
    if (cmt.isEmpty()) return;   // 该语言不支持行注释

    QTextCursor cursor = textCursor();

    // ---- 收集选区覆盖的所有块 ----
    int start = cursor.selectionStart();
    int end   = cursor.selectionEnd();
    cursor.setPosition(start);
    QTextBlock startBlock = cursor.block();
    cursor.setPosition(end);
    QTextBlock endBlock = cursor.block();

    // 选区尾部正好落在行首 最后一行不算
    if (cursor.positionInBlock() == 0 && endBlock != startBlock) {
        endBlock = endBlock.previous();
    }

    // ---- 全已注释 反注释 ----
    bool allCommented = true;
    for (QTextBlock b = startBlock;
         b.isValid() && b.blockNumber() <= endBlock.blockNumber();
         b = b.next())
    {
        const QString t = b.text().trimmed();
        if (!t.isEmpty() && !t.startsWith(cmt)) {
            allCommented = false;
            break;
        }
    }
    if (allCommented) {
        uncommentSelection();
        return;
    }

    // ---- 逐行加注释（保留原有缩进）----
    cursor.beginEditBlock();
    const QString cmtPrefix = cmt + ' ';
    for (QTextBlock b = startBlock;
         b.isValid() && b.blockNumber() <= endBlock.blockNumber();
         b = b.next())
    {
        // 空行跳过（或强制注释？VSCode 会插入，我们跳过更干净）
        if (b.text().trimmed().isEmpty()) continue;

        QTextCursor c(b);
        c.movePosition(QTextCursor::StartOfBlock);
        // 保留缩进：把注释符插在缩进之后
        const QString line = b.text();
        int i = 0;
        while (i < line.length() && (line[i] == ' ' || line[i] == '\t')) ++i;
        c.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, i);
        c.insertText(cmtPrefix);
    }
    cursor.endEditBlock();
}

void TextEditor::uncommentSelection()
{
    const QString cmt = m_editorRules.lineComment;
    if (cmt.isEmpty()) return;

    QTextCursor cursor = textCursor();
    int start = cursor.selectionStart();
    int end   = cursor.selectionEnd();
    cursor.setPosition(start);
    QTextBlock startBlock = cursor.block();
    cursor.setPosition(end);
    QTextBlock endBlock = cursor.block();
    if (cursor.positionInBlock() == 0 && endBlock != startBlock) {
        endBlock = endBlock.previous();
    }

    cursor.beginEditBlock();
    for (QTextBlock b = startBlock;
         b.isValid() && b.blockNumber() <= endBlock.blockNumber();
         b = b.next())
    {
        const QString line = b.text();
        // 找到缩进结束位置
        int i = 0;
        while (i < line.length() && (line[i] == ' ' || line[i] == '\t')) ++i;

        if (!line.mid(i).startsWith(cmt)) continue;

        int removeLen = cmt.length();
        // 注释符后如果跟一个空格，也一起删掉
        if (i + removeLen < line.length() &&
            line[i + removeLen] == ' ') {
            removeLen += 1;
        }

        QTextCursor c(b);
        c.movePosition(QTextCursor::StartOfBlock);
        c.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, i);
        c.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, removeLen);
        c.removeSelectedText();
    }
    cursor.endEditBlock();
}

void TextEditor::duplicateLine()
{
    QTextCursor cursor = textCursor();
    const int col = cursor.positionInBlock();

    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    const QString lineText = cursor.selectedText();
    cursor.setPosition(cursor.selectionEnd());
    cursor.insertText("\n" + lineText);
    cursor.endEditBlock();

    // 光标落到新行，保持列
    QTextCursor nc(document());
    nc.setPosition(cursor.position() - lineText.length());
    nc.movePosition(QTextCursor::StartOfBlock);
    nc.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, col);
    setTextCursor(nc);
}

void TextEditor::deleteLine()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    cursor.select(QTextCursor::LineUnderCursor);

    // 若后面还有内容，把后一个换行也删掉；否则删前一个换行
    if (!cursor.atEnd()) {
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
    } else if (cursor.selectionStart() > 0) {
        QTextCursor tmp = cursor;
        tmp.setPosition(cursor.selectionStart());
        tmp.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);
        tmp.removeSelectedText();
    }
    cursor.removeSelectedText();

    cursor.endEditBlock();
}

void TextEditor::gotoLine(int lineNumber)
{
    if (lineNumber < 1) {
        lineNumber = 1;
    }

    if (lineNumber > lineCount()) {
        lineNumber = lineCount();
    }

    QTextCursor cursor = textCursor();
    int position = document()->findBlockByNumber(lineNumber - 1).position();
    cursor.setPosition(position);
    setTextCursor(cursor);

    centerCursor();
}

// ========== 断点管理 ==========

void TextEditor::setBreakpoint(int line, bool enabled)
{
    if (enabled) {
        m_breakpoints.insert(line);
    } else {
        m_breakpoints.remove(line);
    }
    // 只更新行号区域，不重绘整个编辑器
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }

    if (verticalScrollBar()) verticalScrollBar()->update();
}

bool TextEditor::hasBreakpoint(int line) const
{
    return m_breakpoints.contains(line);
}

void TextEditor::clearAllBreakpoints()
{
    m_breakpoints.clear();
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }

    if (verticalScrollBar()) verticalScrollBar()->update();
}

void TextEditor::setCurrentDebugLine(int line)
{
    // 切换停止行时，旧的变量值立即作废（它们属于上一行）
    m_inlineValues.clear();
    m_currentDebugLine = line;
    // 滚动到该行
    if (line > 0) {
        gotoLine(line);
    }
    update();  // 重绘编辑器
}

void TextEditor::clearCurrentDebugLine()
{
    m_currentDebugLine = -1;
    m_inlineValues.clear();
    update();
}

void TextEditor::setInlineValues(int line, const QString& text)
{
    if (line <= 0) return;
    if (text.isEmpty()) {
        m_inlineValues.remove(line);
    } else {
        m_inlineValues[line] = text;
    }
    // 简单起见：整块 viewport 重绘。绘制代价很小（只多画一行文本）
    viewport()->update();
}

void TextEditor::clearInlineValues()
{
    if (m_inlineValues.isEmpty()) return;
    m_inlineValues.clear();
    viewport()->update();
}

// ========== 错误/警告标记 ==========

void TextEditor::setErrorMarker(const QString& filePath, int line, int column, const QString& message)
{
    // 只处理当前编辑器的文件
    if (filePath != m_filePath) return;

    // 移除该行已有的错误标记
    for (int i = m_errorMarkers.size() - 1; i >= 0; --i) {
        if (m_errorMarkers[i].line == line) {
            m_errorMarkers.removeAt(i);
        }
    }

    m_errorMarkers.append(ErrorMarker(filePath, line, column, message, "error"));

    // ========== 强制刷新 ==========
    viewport()->update();
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }

    if (verticalScrollBar()) verticalScrollBar()->update();
}

void TextEditor::setWarningMarker(const QString& filePath, int line, int column, const QString& message)
{
    if (filePath != m_filePath) return;

    for (int i = m_warningMarkers.size() - 1; i >= 0; --i) {
        if (m_warningMarkers[i].line == line) {
            m_warningMarkers.removeAt(i);
        }
    }

    m_warningMarkers.append(ErrorMarker(filePath, line, column, message, "warning"));
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }

    if (verticalScrollBar()) verticalScrollBar()->update();
}

void TextEditor::clearErrorMarkers()
{
    m_errorMarkers.clear();
    viewport()->update();
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }

    if (verticalScrollBar()) verticalScrollBar()->update();
}

void TextEditor::clearWarningMarkers()
{
    m_warningMarkers.clear();
    viewport()->update();
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }

    if (verticalScrollBar()) verticalScrollBar()->update();
}

void TextEditor::clearAllMarkers()
{
    m_errorMarkers.clear();
    m_warningMarkers.clear();
    viewport()->update();
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }
}

bool TextEditor::hasErrorMarker(int line) const
{
    for (const ErrorMarker& marker : m_errorMarkers) {
        if (marker.line == line) return true;
    }
    return false;
}

bool TextEditor::hasWarningMarker(int line) const
{
    for (const ErrorMarker& marker : m_warningMarkers) {
        if (marker.line == line) return true;
    }
    return false;
}

QList<ErrorMarker> TextEditor::getErrorMarkers() const
{
    return m_errorMarkers;
}

void TextEditor::onErrorMarkerTimer()
{
    update();
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }
}

void TextEditor::showFindBar()
{
    if (!m_findBar) return;
    m_findBarVisible = true;
    m_findBar->setVisible(true);
    m_findBar->setGeometry(0, height() - 40, width(), 40);
    m_findLineEdit->setFocus();
    m_findLineEdit->selectAll();
    emit findBarVisibilityChanged(true);
}

void TextEditor::showFindBarWithReplace()
{
    showFindBar();
    onToggleReplaceMode();  // 展开替换模式
}

void TextEditor::hideFindBar()
{
    if (!m_findBar) return;
    m_findBarVisible = false;
    m_findBar->setVisible(false);
    clearHighlights();

    // ========== 重置替换模式 ==========
    if (m_replaceMode) {
        m_replaceMode = false;
        m_replaceLineEdit->setVisible(false);
        m_replaceButton->setVisible(false);
        m_replaceAllButton->setVisible(false);
        m_toggleReplaceButton->setText("▼");
    }
    emit findBarVisibilityChanged(false);
}

bool TextEditor::isFindBarVisible() const
{
    return m_findBarVisible;
}

void TextEditor::findNext()
{
    if (m_findBarVisible && !m_matchCursors.isEmpty()) {
        onFindNextClicked();
    }
}

void TextEditor::findPrevious()
{
    if (m_findBarVisible && !m_matchCursors.isEmpty()) {
        onFindPreviousClicked();
    }
}

// ============================================================
// 语言相关方法实现
// ============================================================

void TextEditor::setLanguageByFilePath(const QString& filePath)
{
    if (m_highlighter) {
        m_highlighter->setLanguageByFilePath(filePath);
    }

    if (m_highlighter) {
        applyEditorRulesForLanguage(m_highlighter->currentLanguageId());
    }
}

void TextEditor::setLanguage(const QString& languageId)
{
    if (m_highlighter) {
        m_highlighter->setLanguage(languageId);
    }
    applyEditorRulesForLanguage(languageId);
}

QString TextEditor::currentLanguageId() const
{
    if (m_highlighter) {
        return m_highlighter->currentLanguageId();
    }
    return "text";
}

QString TextEditor::currentLanguageName() const
{
    if (m_highlighter) {
        return m_highlighter->languageDisplayName();
    }
    return "Plain Text";
}

void TextEditor::onFindTextChanged(const QString& text)
{
    m_lastFindText = text;
    m_currentMatchIndex = -1;

    if (text.isEmpty()) {
        clearHighlights();
        m_matchCountLabel->setText("0/0");
        return;
    }

    highlightAllMatches(text, m_caseSensitiveCheck->isChecked(), m_wholeWordCheck->isChecked());

    if (!m_matchCursors.isEmpty()) {
        m_currentMatchIndex = 0;
        scrollToMatch(m_currentMatchIndex);
        m_matchCountLabel->setText(QString("%1/%2").arg(m_currentMatchIndex + 1).arg(m_matchCursors.size()));
    }
}

void TextEditor::onFindNextClicked()
{
    if (m_matchCursors.isEmpty()) return;
    m_currentMatchIndex = (m_currentMatchIndex + 1) % m_matchCursors.size();
    scrollToMatch(m_currentMatchIndex);
    m_matchCountLabel->setText(QString("%1/%2").arg(m_currentMatchIndex + 1).arg(m_matchCursors.size()));
}

void TextEditor::onFindPreviousClicked()
{
    if (m_matchCursors.isEmpty()) return;
    m_currentMatchIndex = (m_currentMatchIndex - 1 + m_matchCursors.size()) % m_matchCursors.size();
    scrollToMatch(m_currentMatchIndex);
    m_matchCountLabel->setText(QString("%1/%2").arg(m_currentMatchIndex + 1).arg(m_matchCursors.size()));
}

void TextEditor::highlightAllMatches(const QString& text, bool caseSensitive, bool wholeWord)
{
    clearHighlights();

    QTextDocument::FindFlags flags;
    if (caseSensitive) flags |= QTextDocument::FindCaseSensitively;
    if (wholeWord) flags |= QTextDocument::FindWholeWords;

    QTextCursor cursor(document());
    cursor.movePosition(QTextCursor::Start);

    while (!cursor.isNull()) {
        QTextCursor found = document()->find(text, cursor, flags);
        if (found.isNull()) break;

        m_matchCursors.append(found);
        cursor = found;
    }

    QList<QTextEdit::ExtraSelection> selections;
    for (const QTextCursor& match : m_matchCursors) {
        QTextEdit::ExtraSelection sel;
        sel.cursor = match;
        sel.format.setBackground(QColor(255, 255, 100, 80));
        selections.append(sel);
    }

    if (!m_matchCursors.isEmpty() && m_currentMatchIndex >= 0 && m_currentMatchIndex < m_matchCursors.size()) {
        QTextEdit::ExtraSelection currentSel;
        currentSel.cursor = m_matchCursors[m_currentMatchIndex];
        currentSel.format.setBackground(QColor(255, 200, 50, 150));
        selections.append(currentSel);
    }

    setExtraSelections(selections);
}

void TextEditor::scrollToMatch(int index)
{
    if (index < 0 || index >= m_matchCursors.size()) return;
    setTextCursor(m_matchCursors[index]);
    centerCursor();
    highlightAllMatches(m_lastFindText, m_caseSensitiveCheck->isChecked(), m_wholeWordCheck->isChecked());
}

void TextEditor::clearHighlights()
{
    m_matchCursors.clear();
    setExtraSelections(QList<QTextEdit::ExtraSelection>());
}

void TextEditor::onToggleReplaceMode()
{
    m_replaceMode = !m_replaceMode;
    m_replaceLineEdit->setVisible(m_replaceMode);
    m_replaceButton->setVisible(m_replaceMode);
    m_replaceAllButton->setVisible(m_replaceMode);
    m_toggleReplaceButton->setText(m_replaceMode ? "▲" : "▼");

    if (m_replaceMode) {
        m_replaceLineEdit->setFocus();
    } else {
        m_findLineEdit->setFocus();
    }
}

void TextEditor::onReplaceTextChanged(const QString& text)
{
    // 预留：未来可添加替换预览功能
    Q_UNUSED(text);
}

void TextEditor::onReplaceClicked()
{
    if (m_matchCursors.isEmpty() || m_currentMatchIndex < 0) return;

    QTextCursor cursor = m_matchCursors[m_currentMatchIndex];
    cursor.beginEditBlock();
    cursor.removeSelectedText();
    cursor.insertText(m_replaceLineEdit->text());
    cursor.endEditBlock();

    onFindTextChanged(m_findLineEdit->text());
    onFindNextClicked();
}

void TextEditor::onReplaceAllClicked()
{
    if (m_matchCursors.isEmpty()) return;

    int count = 0;
    for (int i = m_matchCursors.size() - 1; i >= 0; --i) {
        QTextCursor cursor = m_matchCursors[i];
        cursor.beginEditBlock();
        cursor.removeSelectedText();
        cursor.insertText(m_replaceLineEdit->text());
        cursor.endEditBlock();
        count++;
    }

    clearHighlights();
    m_matchCountLabel->setText("0/0");
    m_findLineEdit->setFocus();
    m_findLineEdit->selectAll();
    emit replaceCompleted(count);
}

void TextEditor::onCloseFindBar()
{
    hideFindBar();
}

void TextEditor::onToggleCaseSensitive(bool checked)
{
    QString text = m_findLineEdit->text();
    if (!text.isEmpty()) {
        onFindTextChanged(text);
    }
}

void TextEditor::onToggleWholeWord(bool checked)
{
    QString text = m_findLineEdit->text();
    if (!text.isEmpty()) {
        onFindTextChanged(text);
    }
}

// ========== 鼠标事件 ==========

void TextEditor::mousePressEvent(QMouseEvent* event)
{
    // 处理 Ctrl+左键（macOS 上为 Cmd+左键）
    if (event->button() == Qt::LeftButton &&
        (event->modifiers() & (Qt::ControlModifier | Qt::MetaModifier))) {

        // 获取光标位置
        QTextCursor cursor = cursorForPosition(event->pos());
        int pos = cursor.position();
        QString text = toPlainText();

        // 提取标识符（字母、数字、下划线）
        int start = pos;
        while (start > 0 && (text[start-1].isLetterOrNumber() || text[start-1] == '_')) {
            start--;
        }
        int end = pos;
        while (end < text.length() && (text[end].isLetterOrNumber() || text[end] == '_')) {
            end++;
        }

        if (end > start) {
            qDebug() << "[TextEditor] Identifier found:" << text.mid(start, end-start);
            // 有有效的标识符，发送请求
            int line = cursor.blockNumber();          // 0-based
            int column = cursor.positionInBlock();    // 0-based
            emit definitionRequested(m_filePath, line, column);
            // 显示一个小反馈（可选）
            setCursor(Qt::PointingHandCursor);
        } else {
            qDebug() << "[TextEditor] No identifier at click position";
            // 没有标识符，按普通点击处理
            QPlainTextEdit::mousePressEvent(event);
        }
        return;
    }

    // 其他情况调用基类
    QPlainTextEdit::mousePressEvent(event);
}

void TextEditor::mouseMoveEvent(QMouseEvent *event)
{
    m_lastMousePos = event->pos();
    m_hasValidMousePos = true;

    //  实时读取修饰键状态（Cmd/Ctrl）—— 不依赖 keyPress 事件
    const bool ctrlHeld = (QGuiApplication::keyboardModifiers()
                           & (Qt::ControlModifier | Qt::MetaModifier));

    if (ctrlHeld) {
        QTextCursor ident = getIdentifierAt(event->pos());

        if (!ident.isNull()) {
            // ---- 命中标识符：手型 + 高亮 ----
            if (cursor().shape() != Qt::PointingHandCursor) {
                setCursor(Qt::PointingHandCursor);
            }

            // 只在标识符变化时才更新高亮，避免重复 setExtraSelections
            const bool changed =
                m_definitionCursor.isNull()
                || m_definitionCursor.selectionStart() != ident.selectionStart()
                || m_definitionCursor.selectionEnd()   != ident.selectionEnd();

            if (changed) {
                m_definitionCursor = ident;
                m_ctrlHeldForDefinition = true;
                updateDefinitionHighlight();
            }
        } else {
            // ---- 未命中：恢复光标 + 清高亮 ----
            if (cursor().shape() == Qt::PointingHandCursor) {
                unsetCursor();
            }
            if (!m_definitionCursor.isNull()) {
                clearDefinitionHighlight();
            }
        }
    } else {
        // ---- 未按 Cmd：恢复一切 ----
        if (cursor().shape() == Qt::PointingHandCursor) {
            unsetCursor();
        }
        if (!m_definitionCursor.isNull()) {
            clearDefinitionHighlight();
        }
    }

    QPlainTextEdit::mouseMoveEvent(event);
}

void TextEditor::keyReleaseEvent(QKeyEvent *event)
{
    //  Cmd/Ctrl 松开 → 立即撤销视觉反馈
    if (event->key() == Qt::Key_Meta || event->key() == Qt::Key_Control) {
        m_ctrlHeldForDefinition = false;
        if (cursor().shape() == Qt::PointingHandCursor) {
            unsetCursor();
        }
        clearDefinitionHighlight();
    }

    QPlainTextEdit::keyReleaseEvent(event);
}

void TextEditor::leaveEvent(QEvent *event)
{
    //  鼠标离开编辑器 → 撤销一切
    if (cursor().shape() == Qt::PointingHandCursor) {
        unsetCursor();
    }
    clearDefinitionHighlight();

    QPlainTextEdit::leaveEvent(event);
}

// ============================================================================
//  获取鼠标位置下的标识符（字母/数字/下划线组成）
//  - 返回一个选中该标识符的 QTextCursor
//  - 若鼠标不在标识符上，返回空 QTextCursor
// ============================================================================
QTextCursor TextEditor::getIdentifierAt(const QPoint& pos) const
{
    QTextCursor cursor = cursorForPosition(pos);
    const int position = cursor.position();
    const QString text = toPlainText();

    if (position < 0 || position > text.length()) {
        return QTextCursor();
    }

    // 向前找标识符起始
    int start = position;
    while (start > 0
           && (text[start-1].isLetterOrNumber() || text[start-1] == '_')) {
        --start;
    }

    // 向后找标识符结束
    int end = position;
    while (end < text.length()
           && (text[end].isLetterOrNumber() || text[end] == '_')) {
        ++end;
    }

    if (end == start) return QTextCursor();   // 无标识符

    QTextCursor sel(document());
    sel.setPosition(start);
    sel.setPosition(end, QTextCursor::KeepAnchor);
    return sel;
}

// ============================================================================
//  应用/更新"可跳转标识符"高亮
//  - 使用 QTextEdit::ExtraSelection 机制，避免修改文档内容
//  - 用 UserProperty 打标记，方便下一次清理
// ============================================================================
void TextEditor::updateDefinitionHighlight()
{
    QList<QTextEdit::ExtraSelection> all = extraSelections();

    // 移除所有旧的"可跳转"高亮
    static const QString kDefLinkTag = QStringLiteral("definitionLink");
    for (int i = all.size() - 1; i >= 0; --i) {
        if (all[i].format.property(QTextFormat::UserProperty).toString() == kDefLinkTag) {
            all.removeAt(i);
        }
    }

    // 添加新高亮
    if (m_ctrlHeldForDefinition && !m_definitionCursor.isNull()) {
        ThemeManager& tm = ThemeManager::instance();

        QTextEdit::ExtraSelection sel;
        sel.cursor = m_definitionCursor;
        sel.format.setFontUnderline(true);
        sel.format.setUnderlineColor(tm.color(ThemeManager::UIElement::PrimaryColor));
        sel.format.setForeground(tm.color(ThemeManager::UIElement::PrimaryColor));
        sel.format.setProperty(QTextFormat::UserProperty, kDefLinkTag);
        all.append(sel);
    }

    setExtraSelections(all);
}

void TextEditor::clearDefinitionHighlight()
{
    if (m_definitionCursor.isNull()) return;
    m_definitionCursor = QTextCursor();
    updateDefinitionHighlight();
}

// ========== 悬停提示 ==========
bool TextEditor::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent*>(event);
        QPoint pos = helpEvent->pos();
        QString tooltip = getMarkerTooltip(pos);
        if (!tooltip.isEmpty()) {
            QToolTip::showText(helpEvent->globalPos(), tooltip, this);
        } else {
            QToolTip::hideText();
        }
        return true;
    }
    return QPlainTextEdit::event(event);
}

bool TextEditor::viewportEvent(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent*>(event);
        QPoint pos = helpEvent->pos();
        QString tooltip = getMarkerTooltip(pos);
        if (!tooltip.isEmpty()) {
            QToolTip::showText(helpEvent->globalPos(), tooltip, this);
        } else {
            QToolTip::hideText();
        }
        return true;
    }
    return QPlainTextEdit::viewportEvent(event);
}

void TextEditor::focusOutEvent(QFocusEvent* event)
{
    closeCompletion();
    // 调用基类处理
    QPlainTextEdit::focusOutEvent(event);
}

// ============================================================================
//  在滚动条上叠加错误/警告/断点标记
//  通过 eventFilter 监听 scrollbar 的 paint，绘制完后我们追加
// ============================================================================
bool TextEditor::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == verticalScrollBar() && event->type() == QEvent::Paint) {
        // 先让基类绘制完滚动条
        const bool ret = QPlainTextEdit::eventFilter(obj, event);

        // 再叠加我们的标记
        paintScrollBarMarkers();
        return ret;
    }
    return QPlainTextEdit::eventFilter(obj, event);
}

QString TextEditor::getMarkerTooltip(const QPoint& pos) const
{
    // 获取鼠标所在行
    QTextCursor cursor = cursorForPosition(pos);
    int line = cursor.blockNumber() + 1;

    QStringList messages;

    // 查找错误
    for (const ErrorMarker& marker : m_errorMarkers) {
        if (marker.line == line) {
            messages.append("❌ " + marker.message);
        }
    }

    // 查找警告
    for (const ErrorMarker& marker : m_warningMarkers) {
        if (marker.line == line) {
            messages.append("⚠️ " + marker.message);
        }
    }

    return messages.join("\n");
}

// ========== 自定义绘制 ==========

void TextEditor::paintEvent(QPaintEvent *event)
{
    // 先调用基类绘制文本
    QPlainTextEdit::paintEvent(event);

    QPainter painter(viewport());

    // ========== 绘制错误行背景 ==========
    for (const ErrorMarker& marker : m_errorMarkers) {
        int line = marker.line;
        if (line < 1 || line > lineCount()) continue;

        QTextBlock block = document()->findBlockByNumber(line - 1);
        if (!block.isValid()) continue;

        QPointF offset = contentOffset();
        QRectF blockRect = blockBoundingGeometry(block).translated(offset);
        QColor errorBg = QColor(255, 50, 50, 60);
        painter.fillRect(blockRect, errorBg);
    }

    // ========== 绘制警告行背景 ==========
    for (const ErrorMarker& marker : m_warningMarkers) {
        int line = marker.line;
        if (line < 1 || line > lineCount()) continue;

        QTextBlock block = document()->findBlockByNumber(line - 1);
        if (!block.isValid()) continue;

        QPointF offset = contentOffset();
        QRectF blockRect = blockBoundingGeometry(block).translated(offset);
        QColor warningBg = QColor(255, 200, 0, 60);
        painter.fillRect(blockRect, warningBg);
    }

    // ========== 绘制错误红色波浪线（精确指示列） ==========
    QFontMetrics fm(font());
    QTextCursor cursor(document());

    for (const ErrorMarker& marker : m_errorMarkers) {
        int line = marker.line;
        int column = marker.column;
        if (line < 1 || line > lineCount()) continue;

        QTextBlock block = document()->findBlockByNumber(line - 1);
        if (!block.isValid()) continue;

        // 计算起始位置
        cursor.setPosition(block.position());
        if (column > 0) {
            // 移动到指定列（注意：column 是从1开始的，需减1）
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, column - 1);
        } else {
            // 如果 column 为 -1 或 0，从行首开始
            cursor.movePosition(QTextCursor::StartOfLine);
        }
        QRect cursorRect = this->cursorRect(cursor);
        int startX = cursorRect.x();
        int startY = cursorRect.y() + cursorRect.height() - 3; // 文字底部稍上

        // 计算结束位置（行尾）
        cursor.movePosition(QTextCursor::EndOfLine);
        QRect endRect = this->cursorRect(cursor);
        int endX = endRect.x() + endRect.width();
        int width = endX - startX;
        if (width <= 0) width = 50; // 如果列号无效，给个默认宽度

        // 绘制波浪线
        QPainterPath path;
        path.moveTo(startX, startY);
        int waveLength = 3;
        int amplitude = 2;
        for (int x = startX; x < startX + width; x += waveLength) {
            int y = startY + ((x / waveLength) % 2 == 0 ? -amplitude : amplitude);
            path.lineTo(x + waveLength/2, y);
        }
        painter.setPen(QPen(QColor(200, 160, 0), 1));
        painter.drawPath(path);
    }

    // 绘制当前执行行标记
    if (m_currentDebugLine > 0) {
        QPainter painter(viewport());
        QTextBlock block = document()->findBlockByNumber(m_currentDebugLine - 1);
        if (block.isValid()) {
            QPointF offset = contentOffset();
            QRectF blockRect = blockBoundingGeometry(block).translated(offset);
            // 半透明黄色背景
            QColor highlightColor = QColor(255, 255, 100, 60);
            painter.fillRect(blockRect, highlightColor);

            // 在行号区域左侧画一个绿色箭头指示器
            if (m_lineNumberArea) {
                int lineAreaWidth = m_lineNumberArea->width();
                int arrowX = 2;
                int arrowY = (int)(blockRect.top() + blockRect.height() / 2);
                painter.setBrush(QColor(50, 200, 50));
                painter.setPen(Qt::NoPen);
                QPolygon arrow;
                arrow << QPoint(arrowX, arrowY - 6)
                      << QPoint(arrowX + 8, arrowY)
                      << QPoint(arrowX, arrowY + 6);
                painter.drawPolygon(arrow);
            }

            // 内联变量显示（在当前停止行的行尾绘制）
            if (m_inlineValues.contains(m_currentDebugLine)) {
                const QString text = m_inlineValues.value(m_currentDebugLine);
                if (!text.isEmpty()) {
                    painter.save();

                    // 用行末字符位置算出绘制起点
                    QTextCursor endCursor(block);
                    endCursor.movePosition(QTextCursor::EndOfBlock);
                    QRect endRect = cursorRect(endCursor);   // viewport 坐标

                    int x = endRect.right() + 20;   // 行尾留 20px 空白
                    int y = endRect.top();
                    int h = endRect.height();
                    int availableWidth = viewport()->width() - x - 10;

                    // 只在可见范围内、且宽度足够时绘制
                    if (availableWidth > 50
                        && y + h >= 0
                        && y < viewport()->height()) {

                        QFont inlineFont = font();
                        inlineFont.setPointSize(qMax(8, font().pointSize() - 1));
                        inlineFont.setItalic(true);
                        inlineFont.setBold(false);

                        painter.setFont(inlineFont);
                        QColor inlineColor = ThemeManager::instance().color(
                            ThemeManager::UIElement::InlineValueColor);
                        inlineColor.setAlpha(220);
                        painter.setPen(inlineColor);

                        painter.drawText(
                            QRect(x, y, availableWidth, h),
                            Qt::AlignLeft | Qt::AlignVCenter,
                            text);
                    }

                    painter.restore();
                }
            }
        }
    }
}

// ============================================================
// 代码折叠实现
// ============================================================

/**
 * @brief 在指定块中查找大括号的位置
 */
int TextEditor::findBraceInBlock(const QTextBlock &block, bool opening) const
{
    if (!block.isValid()) {
        return -1;
    }

    QString text = block.text();
    QChar target = opening ? '{' : '}';

    for (int i = 0; i < text.length(); ++i) {
        if (text[i] == target) {
            return i;
        }
    }
    return -1;
}

bool TextEditor::tryExpandPairOnReturn()
{
    QTextCursor cursor = textCursor();
    if (cursor.hasSelection()) return false;

    const int pos = cursor.position();
    if (pos <= 0 || pos >= document()->characterCount()) return false;

    const QChar prev = document()->characterAt(pos - 1);
    const QChar next = document()->characterAt(pos);

    // 检查是否配对
    QChar expectedClose = QChar::Null;
    if (prev == '(')      expectedClose = ')';
    else if (prev == '[') expectedClose = ']';
    else if (prev == '{') expectedClose = '}';
    else return false;

    if (next != expectedClose) return false;

    // 上下文保护：字符串/注释里不展开
    if (m_editorRules.needsContextGuard() && isInsideStringOrComment(pos)) {
        
        return false;
    }

    // 计算缩进
    const QTextBlock block = cursor.block();
    const QString baseIndent  = leadingWhitespace(block.text());
    const QString innerIndent = baseIndent + indentUnit();

    // 布局：
    //   pos-1: prev   （不动）
    //   pos  : '\n'
    //   pos+1..pos+innerLen : innerIndent
    //   pos+innerLen+1      : '\n'
    //   pos+innerLen+2...   : baseIndent（接原 next）
    cursor.beginEditBlock();
    cursor.insertText("\n" + innerIndent);
    cursor.insertText("\n" + baseIndent);
    cursor.endEditBlock();

    // 光标绝对定位到中间行缩进之后
    QTextCursor finalCursor(document());
    finalCursor.setPosition(pos + 1 + innerIndent.length());
    setTextCursor(finalCursor);

    return true;
}

void TextEditor::applyLineHeight()
{
    const qreal want = ThemeManager::kEditorLineHeightPercent;

    // 同一比例、且已经应用过 → 直接返回
    if (m_lastAppliedLineHeight >= 0.0
        && qFuzzyCompare(m_lastAppliedLineHeight, want)) {
        return;
    }

    QTextBlockFormat fmt;
    fmt.setLineHeight(want, QTextBlockFormat::ProportionalHeight);

    const bool wasModified = document()->isModified();

    QTextCursor cursor(document());
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.mergeBlockFormat(fmt);
    cursor.endEditBlock();

    document()->setModified(wasModified);

    m_lastAppliedLineHeight = want;
}

/**
 * @brief 查找匹配的大括号所在行号
 */
int TextEditor::findMatchingBraceLine(int line) const
{
    if (line < 1 || line > document()->blockCount()) {
        return -1;
    }

    QTextBlock block = document()->findBlockByLineNumber(line - 1);
    if (!block.isValid()) {
        return -1;
    }

    // 查找 '{' 或 '}'
    int bracePos = findBraceInBlock(block, true);
    bool isOpening = true;

    if (bracePos == -1) {
        bracePos = findBraceInBlock(block, false);
        isOpening = false;
    }

    if (bracePos == -1) {
        return -1;
    }

    QTextDocument *doc = document();
    int position = block.position() + bracePos;

    QChar braceChar = isOpening ? '{' : '}';
    QChar otherBrace = isOpening ? '}' : '{';

    int depth = 1;
    int step = isOpening ? 1 : -1;
    int pos = position + step;

    while (pos >= 0 && pos < doc->characterCount()) {
        QChar ch = doc->characterAt(pos);

        if (ch == braceChar) {
            depth++;
        } else if (ch == otherBrace) {
            depth--;
            if (depth == 0) {
                QTextBlock matchedBlock = doc->findBlock(pos);
                if (matchedBlock.isValid()) {
                    int matchedLine = matchedBlock.blockNumber() + 1;
                    // 确保返回的行号在有效范围内
                    if (matchedLine <= document()->blockCount()) {
                        return matchedLine;
                    }
                    return -1;
                }
                return -1;
            }
        }
        pos += step;
    }

    return -1;
}

/**
 * @brief 判断某行是否可折叠（包含 '{' 且匹配的 '}' 不在同一行）
 */
bool TextEditor::isFoldableBlock(int line) const
{
    if (line < 1 || line > document()->blockCount()) {
        return false;
    }

    QTextBlock block = document()->findBlockByLineNumber(line - 1);
    if (!block.isValid()) {
        return false;
    }

    // 检查是否包含 '{'
    int bracePos = findBraceInBlock(block, true);
    if (bracePos == -1) {
        return false;
    }

    // 检查匹配的 '}' 是否在同一行
    int matchLine = findMatchingBraceLine(line);
    if (matchLine == -1 || matchLine == line) {
        return false;
    }

    return true;
}

/**
 * @brief 判断某行是否已被折叠
 */
bool TextEditor::isBlockFolded(int line) const
{
    return m_foldedLines.contains(line);
}

/**
 * @brief 切换指定行的折叠状态
 */
void TextEditor::toggleFold(int line)
{
    // qDebug() << "=== toggleFold 开始 ===";
    // qDebug() << "行号:" << line << "文档总行数:" << document()->blockCount();

    if (line < 1 || line > document()->blockCount()) {
        // qDebug() << "行号超出范围";
        return;
    }

    if (!isFoldableBlock(line)) {
        // qDebug() << "行" << line << "不可折叠";
        return;
    }

    int matchLine = findMatchingBraceLine(line);
    // qDebug() << "匹配行:" << matchLine;
    if (matchLine == -1 || matchLine <= line) {
        // qDebug() << "无效匹配行";
        return;
    }

    // 边界检查
    int totalLines = document()->blockCount();
    if (matchLine > totalLines) {
        // qDebug() << "匹配行超出文档范围，修正为" << totalLines;
        matchLine = totalLines;
    }

    bool isFolded = m_foldedLines.contains(line);
    // qDebug() << "当前折叠状态:" << isFolded;

    if (isFolded) {
        // ========== 展开 ==========
        // qDebug() << "--- 开始展开（遍历块） ---";
        QTextBlock block = document()->findBlockByLineNumber(line - 1);
        if (!block.isValid()) {
            // qDebug() << "起始块无效";
            return;
        }
        // 遍历从起始行到匹配行之间的所有块
        while (block.isValid() && block.blockNumber() + 1 <= matchLine) {
            block.setVisible(true);
            // qDebug() << "展开块 行号:" << block.blockNumber() + 1 << "内容:" << block.text().left(30);
            block = block.next();
        }
        m_foldedLines.remove(line);
        // qDebug() << "展开完成，移除折叠记录";
    } else {
        // ========== 折叠 ==========
        // qDebug() << "--- 开始折叠（遍历块） ---";
        QTextBlock block = document()->findBlockByLineNumber(line + 1 - 1); // line+1 行的块
        if (!block.isValid()) {
            // qDebug() << "起始块无效";
            return;
        }
        // 遍历从 line+1 到 matchLine-1
        while (block.isValid() && block.blockNumber() + 1 < matchLine) {
            block.setVisible(false);
            // qDebug() << "折叠块 行号:" << block.blockNumber() + 1 << "内容:" << block.text().left(30);
            block = block.next();
        }
        m_foldedLines.insert(line);
        // qDebug() << "折叠完成，添加折叠记录";
    }

    // 强制文档重新布局
    // qDebug() << "强制文档重排...";
    document()->markContentsDirty(0, document()->characterCount());
    document()->adjustSize();

    viewport()->update();
    m_lineNumberArea->update();

    // qDebug() << "折叠列表现在:" << m_foldedLines.values();
    // qDebug() << "=== toggleFold 结束 ===";
}

/**
 * @brief 重新计算所有行的折叠状态（文件加载或内容变更后调用）
 */
void TextEditor::updateFolding()
{
    // 没有任何折叠记录 → 块本身全部可见，无需遍历整个文档
    if (m_foldedLines.isEmpty()) {
        return;
    }

    // 清除所有折叠状态
    m_foldedLines.clear();

    // 显示所有行
    QTextBlock block = document()->firstBlock();
    while (block.isValid()) {
        block.setVisible(true);
        block = block.next();
    }

    document()->markContentsDirty(0, document()->characterCount());
    document()->adjustSize();
    viewport()->update();
    m_lineNumberArea->update();
}

// ========== 状态查询方法 ==========
int TextEditor::currentLine() const
{
    return textCursor().blockNumber() + 1;
}

int TextEditor::currentColumn() const
{
    QTextCursor cursor = textCursor();
    return cursor.positionInBlock() + 1;
}

int TextEditor::lineCount() const
{
    return document()->blockCount();
}

void TextEditor::keyPressEvent(QKeyEvent *event)
{
    // ---- 0. Cmd/Ctrl 高亮（原样保留） ----
    if ((event->key() == Qt::Key_Meta || event->key() == Qt::Key_Control)
        && !m_ctrlHeldForDefinition) {
        m_ctrlHeldForDefinition = true;
        if (m_hasValidMousePos) {
            m_definitionCursor = getIdentifierAt(m_lastMousePos);
            if (!m_definitionCursor.isNull()) {
                setCursor(Qt::PointingHandCursor);
            }
            updateDefinitionHighlight();
        }
    }

    // ---- 1. 补全弹窗导航（原样保留） ----
    if (m_completionActive) {
        switch (event->key()) {
        case Qt::Key_Escape:    closeCompletion(); return;
        case Qt::Key_Down:      m_completionPopup->selectNext(); return;
        case Qt::Key_Up:        m_completionPopup->selectPrev(); return;
        case Qt::Key_PageDown:
            for (int i = 0; i < 5; ++i) m_completionPopup->selectNext();
            return;
        case Qt::Key_PageUp:
            for (int i = 0; i < 5; ++i) m_completionPopup->selectPrev();
            return;
        case Qt::Key_Tab:
        case Qt::Key_Return:
        case Qt::Key_Enter: {
            if (m_completionPopup->isVisible() && m_completionModel->rowCount() > 0) {
                int row = m_completionPopup->currentIndex();
                if (row < 0) row = 0;
                commitCompletion(row);
                return;
            }
            closeCompletion();
            if (event->key() == Qt::Key_Tab
                && (event->modifiers() & ~Qt::KeypadModifier) == Qt::NoModifier) {
                handleTabKey(true);   // ← 改为新入口
            }
            return;
        }
        case Qt::Key_Backspace: {
            QPlainTextEdit::keyPressEvent(event);
            if (m_completionPrefix.isEmpty()) {
                closeCompletion();
            } else {
                m_completionPrefix = currentWordPrefix();
                m_completionAnchor = textCursor().position()
                                     - m_completionPrefix.length();
                rebuildCompletionList();
            }
            return;
        }
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Home:
        case Qt::Key_End:
            closeCompletion();
            break;
        default:
            break;
        }
    }

    // ========== 2. Esc / F3 / Ctrl+F（原样保留） ==========
    if (event->key() == Qt::Key_Escape && m_findBarVisible) {
        hideFindBar();
        return;
    }
    if (event->key() == Qt::Key_F3) {
        if (m_findBarVisible && !m_matchCursors.isEmpty()) {
            if (event->modifiers() & Qt::ShiftModifier) findPrevious();
            else                                        findNext();
            return;
        }
    }
    if (event->key() == Qt::Key_F && (event->modifiers() & Qt::ControlModifier)) {
        showFindBar();
        return;
    }

    // ========== 3. 编辑器快捷键（Ctrl+D / Ctrl+Shift+K） ==========
    if (handleEditorShortcuts(event)) return;

    // ========== 4. Alt+↑ / Alt+↓ 移动行 ==========
    if (event->modifiers() == Qt::AltModifier) {
        if (event->key() == Qt::Key_Up)   { moveLinesUp();   return; }
        if (event->key() == Qt::Key_Down) { moveLinesDown(); return; }
    }

    // ========== 5. 智能 Home / End ==========
    if (handleSmartHome(event)) return;
    if (handleSmartEnd(event))  return;

    // ========== 6. Backspace 删除空配对 ==========
    if (event->key() == Qt::Key_Backspace
        && (event->modifiers() & (Qt::ControlModifier
                                  | Qt::ShiftModifier
                                  | Qt::AltModifier)) == Qt::NoModifier) {
        if (tryDeletePairOnBackspace()) return;
    }

    // ========== 7. Tab / Shift+Tab（改为新入口） ==========
    if (event->key() == Qt::Key_Tab
        && (event->modifiers() & ~Qt::KeypadModifier) == Qt::NoModifier) {
        // 优先片段展开
        if (tryExpandSnippet()) return;
        handleTabKey(true);
        return;
    }
    if (event->key() == Qt::Key_Backtab
        || (event->key() == Qt::Key_Tab
            && (event->modifiers() & Qt::ShiftModifier))) {
        handleTabKey(false);
        return;
    }

    // ========== 8. 回车（在 autoIndent 前插块注释续行） ==========
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        // 8.1 块注释内续 " * "
        if (tryContinueBlockComment()) {
            closeCompletion();
            return;
        }
        // 8.2 配对符中间展开三行
        if (tryExpandPairOnReturn()) {
            closeCompletion();
            return;
        }
        // 8.3 常规：换行 + autoIndent
        QPlainTextEdit::keyPressEvent(event);
        autoIndentAfterReturn();
        closeCompletion();
        return;
    }

    // ========== 9. 可打印字符 ==========
    const QString typed = event->text();
    if (!typed.isEmpty() && typed.at(0).isPrint()) {
        // 9.1 选中文本被包裹
        if (trySurroundSelection(typed)) return;
        // 9.2 大括号换行
        if (tryBraceNewline(typed)) return;
        // 9.3 自动配对
        if (tryAutoPair(typed)) return;
        // 9.4 常规输入 + 补全触发
        QPlainTextEdit::keyPressEvent(event);
        const QChar c = typed.at(0);
        if (isCompletionTriggerChar(c)) {
            // 优先识别 include 上下文
            if (isInIncludeContext(nullptr)) {
                requestIncludeCompletions();
            } else {
                triggerCompletion();
            }
        } else if (m_completionActive) {
            closeCompletion();
        }
        if (isInIncludeContext(nullptr)) {
            requestIncludeCompletions();
        }
        return;
    }

    // ========== 10. 其它键（Home/方向键等） ==========
    closeCompletion();
    QPlainTextEdit::keyPressEvent(event);
}

bool TextEditor::shouldTriggerCompletion(const QString& text, int cursorPos) const
{
    if (cursorPos <= 0) return false;

    // 检查光标前的字符
    QChar ch = text.at(cursorPos - 1);

    // 单字符触发：.
    if (ch == '.') {
        return true;
    }

    // 双字符触发：-> 或 ::
    if (cursorPos >= 2) {
        QString prev2 = text.mid(cursorPos - 2, 2);
        if (prev2 == "->" || prev2 == "::") {
            return true;
        }
    }

    return false;
}

// ============================================================
// 判断字符是否是"补全触发字符"
//   - 字母、下划线 → 触发
//   - . -> ::       → 触发（且触发后带上下文前缀）
//   - 其它          → 不触发
// ============================================================
bool TextEditor::isCompletionTriggerChar(QChar c) const
{
    if (c.isLetter() || c == '_') return true;
    if (c == '.') return true;
    return false;
}

bool TextEditor::isWordChar(QChar c) const
{
    return c.isLetterOrNumber() || c == '_';
}

// 取光标前的连续 word 字符
QString TextEditor::currentWordPrefix() const
{
    QTextCursor cursor = textCursor();
    const QString line = cursor.block().text();
    const int posInBlock = cursor.positionInBlock();

    int start = posInBlock;
    while (start > 0 && isWordChar(line[start - 1])) {
        --start;
    }
    return line.mid(start, posInBlock - start);
}

// ============================================================
// 主入口：输入字符后调用
// ============================================================
void TextEditor::triggerCompletion()
{
    QTextCursor cursor = textCursor();
    const int pos = cursor.position();

    // 前缀从哪里开始：光标前连续 word 字符
    const QString prefix = currentWordPrefix();

    // 如果前缀为空，并且最后一个字符不是 . ，就不弹
    const QChar lastChar = (pos > 0) ? document()->characterAt(pos - 1) : QChar();
    if (prefix.isEmpty() && lastChar != '.') {
        closeCompletion();
        return;
    }

    // 记录 anchor
    m_completionAnchor = pos - prefix.length();
    m_completionPrefix = prefix;
    m_completionActive = true;

    // 立即刷新本地补全（不做防抖，本地很快）
    // rebuildCompletionList();

    // LSP 请求走防抖
    m_pendingLspItems.clear();
    m_lspResponseWaiting = true;
    m_lspDebounceTimer->start();

    // 显示弹窗
    // if (m_completionModel->rowCount() > 0) {
    //     QRect rect = cursorRect();
    //     QPoint globalPos = viewport()->mapToGlobal(rect.bottomLeft());
    //     m_completionPopup->showWithModel(m_completionModel, globalPos, 320);
    // } else {
    //     m_completionPopup->hideIfVisible();
    // }

    rebuildCompletionList();
}

// ============================================================
// 本地符号：收集当前作用域内可见的变量名
//
// 轻量实现：扫描当前文档，找所有 "identifier =" 或 "def identifier" 等模式
// 后续可换成使用 LSP 的 documentSymbol，但当前实现已经能覆盖
// "当前文件里定义过的变量和函数"这一最常见场景。
// ============================================================
void TextEditor::requestLocalCompletions(const QString& prefix)
{
    Q_UNUSED(prefix);   // 过滤交给 model

    QList<CompletionEntry> entries;
    QSet<QString> seen;

    const QString text = toPlainText();
    const QStringList lines = text.split('\n');

    // 语言相关：拿到本语言的关键字列表（用于兜底）
    const QString langId = currentLanguageId();
    const LanguageInfo langInfo =
        LanguageDetector::instance().getLanguage(langId);
    const QStringList keywords = langInfo.keywords
                                 + langInfo.controlFlow
                                 + langInfo.dataTypes
                                 + langInfo.storageTypes
                                 + langInfo.constants;

    // ---- 1. 关键字（优先级最低） ----
    for (const QString& kw : keywords) {
        if (kw.isEmpty() || seen.contains(kw)) continue;
        seen.insert(kw);

        CompletionEntry e;
        e.label = kw;
        e.insertText = kw;
        e.detail = QStringLiteral("keyword");
        e.kind = "keyword";
        e.sortPriority = 80;
        entries.append(e);
    }

    // ---- 2. 本地符号（变量、函数、类） ----
    // 简单正则：
    //   C++/Java/JS:  type name = ...   /   name(   / class Name
    //   Python:       name = ...  /  def name(  /  class Name
    static const QRegularExpression reDef(
        R"((?:^|\s)(?:def|class|function|struct|enum|interface)\s+([A-Za-z_][A-Za-z0-9_]*))"
        R"(|(?:^|\s)([A-Za-z_][A-Za-z0-9_]*)\s*(?:=|\(|:))"
        );

    for (const QString& raw : lines) {
        const QString line = raw.trimmed();
        if (line.isEmpty() || line.startsWith("//") || line.startsWith("#")) {
            continue;
        }

        QRegularExpressionMatchIterator it = reDef.globalMatch(line);
        while (it.hasNext()) {
            const auto m = it.next();
            QString name = m.captured(1);
            if (name.isEmpty()) name = m.captured(2);
            if (name.isEmpty()) continue;
            if (name.length() < 2) continue;    // 排除单字符
            if (seen.contains(name)) continue;
            seen.insert(name);

            CompletionEntry e;
            e.label = name;
            e.insertText = name;
            e.detail = QStringLiteral("local");
            e.kind = "var";
            e.sortPriority = 10;   // 本地符号最优先
            entries.append(e);
        }
    }

    // 合并到 model：本地 + 已到的 LSP（如果 LSP 已回）
    QList<CompletionEntry> all = entries;
    all.append(m_pendingLspItems.isEmpty()
                   ? QList<CompletionEntry>()
                   : QList<CompletionEntry>());   // LSP 结果在 rebuildCompletionList 里合并
    m_completionModel->setEntries(all);
    m_completionModel->setFilterPrefix(prefix);
}

// ============================================================
// 合并本地 + LSP 并刷新 UI
// ============================================================
void TextEditor::rebuildCompletionList()
{
    QList<CompletionEntry> all;
    QSet<QString> seen;

    // 生成去重 key：label + kind
    // 同一 label 同一 kind 只保留一个；
    // 同一 label 不同 kind（如 print 是 keyword 又是 function）保留优先级最高的
    auto makeKey = [](const CompletionEntry& e) {
        return e.label + QChar(0x1F) + e.kind;
    };

    // ---------- 1. 本地符号（优先级最高） ----------
    {
        const QString text = toPlainText();
        const QStringList lines = text.split('\n');

        static const QRegularExpression reDef(
            R"((?:^|\s)(?:def|class|function|struct|enum|interface)\s+([A-Za-z_][A-Za-z0-9_]*))"
            R"(|(?:^|\s)([A-Za-z_][A-Za-z0-9_]*)\s*(?:=|\(|:))"
            );

        for (const QString& raw : lines) {
            const QString line = raw.trimmed();
            if (line.isEmpty() || line.startsWith("//") || line.startsWith("#")) {
                continue;
            }

            QRegularExpressionMatchIterator it = reDef.globalMatch(line);
            while (it.hasNext()) {
                const auto m = it.next();
                QString name = m.captured(1);
                if (name.isEmpty()) name = m.captured(2);
                if (name.isEmpty()) continue;
                if (name.length() < 2) continue;

                CompletionEntry e;
                e.label = name;
                e.insertText = name;
                e.detail = QStringLiteral("local");
                e.kind = "var";
                e.sortPriority = 10;

                const QString key = makeKey(e);
                if (seen.contains(key)) continue;
                seen.insert(key);
                all.append(e);
            }
        }
    }

    // ---------- 2. 关键字（优先级最低） ----------
    {
        const QString langId = currentLanguageId();
        const LanguageInfo langInfo =
            LanguageDetector::instance().getLanguage(langId);
        const QStringList keywords = langInfo.keywords
                                     + langInfo.controlFlow
                                     + langInfo.dataTypes
                                     + langInfo.storageTypes
                                     + langInfo.constants;

        for (const QString& kw : keywords) {
            if (kw.isEmpty()) continue;

            CompletionEntry e;
            e.label = kw;
            e.insertText = kw;
            e.detail = QStringLiteral("keyword");
            e.kind = "keyword";
            e.sortPriority = 80;

            const QString key = makeKey(e);
            if (seen.contains(key)) continue;
            seen.insert(key);
            all.append(e);
        }
    }

    // ---------- 3. LSP 结果（参与去重，但也要尊重 LSP 自己的分类） ----------
    {
        for (const LSPCompletionItem& it : m_pendingLspItems) {
            const QString label = it.label;
            if (label.isEmpty()) continue;

            CompletionEntry e;
            e.label      = label;
            e.detail     = it.detail;
            e.kind       = "lsp";
            e.sortPriority = 30;

            QString insertText = it.insertText;
            if (insertText.isEmpty()) {
                insertText = label;
            }

            const int parenIdx = label.indexOf('(');
            if (parenIdx > 0
                && !insertText.contains('(')
                && !insertText.contains(')')) {
                const QString fnName = label.left(parenIdx);
                if (insertText == fnName) {
                    insertText = fnName + "()";
                }
            }

            e.insertText = insertText;

            const QString key = e.label + QChar(0x1F) + e.kind;
            if (seen.contains(key)) continue;
            seen.insert(key);
            all.append(e);
        }
    }

    // ---------- 4. 剔除"用户已完整输入"的补全项 ----------
    //   规则：若某项的 insertText 与当前前缀完全相等 → 已经没有可补的内容，
    //        直接移除，避免"输入 pass 还提示 pass"这种无意义项。
    //
    //   为什么用 insertText 而不是 label：
    //     - 函数项 label="print" / insertText="print()" → 用户输入 "print" 时，
    //       我们仍希望保留（按 Tab 能补出括号），所以不能被误删。
    //     - 纯标识符 label="pass"  / insertText="pass"    → 输入 "pass" 时删掉。
    if (!m_completionPrefix.isEmpty()) {
        const QString prefix = m_completionPrefix;
        for (int i = all.size() - 1; i >= 0; --i) {
            if (all[i].insertText == prefix) {
                all.removeAt(i);
            }
        }
    }

    m_completionModel->setEntries(all);
    m_completionModel->setFilterPrefix(m_completionPrefix);

    if (!m_completionActive) return;

    // 根据过滤结果决定弹窗
    if (m_completionModel->rowCount() > 0) {
        QRect rect = cursorRect();
        QPoint globalPos = viewport()->mapToGlobal(rect.bottomLeft());

        if (!m_completionPopup->isVisible()) {
            m_completionPopup->showWithModel(m_completionModel, globalPos, 320);
        } else {
            // 已可见 → 只调整尺寸和位置，不 show
            const int rowHeight = m_completionPopup->sizeHintForRow(0) > 0
                                      ? m_completionPopup->sizeHintForRow(0) : 22;
            const int rows = qMin(m_completionModel->rowCount(), 10);
            m_completionPopup->resize(320, rows * rowHeight + 4);
            m_completionPopup->move(globalPos);       // ← 光标可能移动了，位置也要更新
        }
    } else {
        m_completionPopup->hideIfVisible();
    }
}

// ============================================================
// LSP 请求（防抖 200ms 后由定时器触发）
// ============================================================
void TextEditor::requestLspCompletions(const QString& prefix)
{
    Q_UNUSED(prefix);
    if (m_filePath.isEmpty()) return;

    QTextCursor cursor = textCursor();
    const int line = cursor.blockNumber();        // 0-based
    const int column = cursor.positionInBlock();  // 0-based

    // 复用旧的信号——MainWindow 里已有对 completionRequested 的连接
    emit completionRequested(m_filePath, line, column);
}

// ============================================================
// LSP 响应到达 → 合并 + 刷新
// ============================================================
void TextEditor::updateCompletionItems(const QList<LSPCompletionItem>& items)
{
    m_pendingLspItems = items;
    m_lspResponseWaiting = false;

    if (!m_completionActive) {
        // 用户在等待期间关闭了补全 → 丢弃
        m_pendingLspItems.clear();
        return;
    }

    rebuildCompletionList();
}

void TextEditor::onLSPCompletionReceived(const QString& filePath,
                                         const QList<LSPCompletionItem>& items)
{
    if (filePath != m_filePath) return;
    updateCompletionItems(items);
}

void TextEditor::onLSPDefinitionReceived(const QString& targetFile,
                                         int targetLine, int targetColumn)
{
    emit definitionJumpRequested(targetFile, targetLine, targetColumn);
}

// ============================================================
// 提交选中项
// ============================================================
void TextEditor::commitCompletion(int row)
{
    if (row < 0 || row >= m_completionModel->rowCount()) {
        closeCompletion();
        return;
    }

    const CompletionEntry& e = m_completionModel->entryAt(row);

    // 删除已输入的 anchor~光标 之间的文本
    QTextCursor cursor = textCursor();
    const int cursorPos = cursor.position();
    if (m_completionAnchor >= 0 && cursorPos > m_completionAnchor) {
        cursor.setPosition(m_completionAnchor);
        cursor.setPosition(cursorPos, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
    }

    // 插入文本
    cursor.insertText(e.insertText);

    // 如果插入的文本以 "()" 结尾，把光标移到括号中间
    if (e.insertText.endsWith("()")) {
        QTextCursor c2 = cursor;
        c2.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);
        setTextCursor(c2);
    } else {
        setTextCursor(cursor);
    }

    closeCompletion();
}

void TextEditor::closeCompletion()
{
    m_completionActive = false;
    m_completionAnchor = -1;
    m_completionPrefix.clear();
    m_pendingLspItems.clear();
    m_lspDebounceTimer->stop();

    if (m_completionPopup) {
        m_completionPopup->hideIfVisible();
    }
}

void TextEditor::highlightLine(int line)
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, line - 1);
    cursor.movePosition(QTextCursor::StartOfLine);
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
    // 设置高亮格式（自定义或使用临时格式）
    QTextEdit::ExtraSelection selection;
    selection.format.setBackground(Qt::yellow);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = cursor;
    QList<QTextEdit::ExtraSelection> extraSelections;
    extraSelections.append(selection);
    setExtraSelections(extraSelections);
    // 设置滚动到该行
    ensureCursorVisible();
}

void TextEditor::setBreakpointWithCondition(int line, bool enabled, bool hasCondition)
{
    if (enabled) {
        m_breakpoints.insert(line);
        if (hasCondition) m_conditionalBreakpoints.insert(line);
        else              m_conditionalBreakpoints.remove(line);
    } else {
        m_breakpoints.remove(line);
        m_conditionalBreakpoints.remove(line);
    }
    if (m_lineNumberArea) m_lineNumberArea->update();
}

void TextEditor::setBreakpointEnabled(int line, bool enabled)
{
    if (enabled) {
        m_disabledBreakpoints.remove(line);
    } else {
        m_disabledBreakpoints.insert(line);
    }
    if (m_lineNumberArea) m_lineNumberArea->update();
}

void TextEditor::insertCompletion(const QString& insertText)
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    // 获取当前行文本和光标位置
    QString lineText = cursor.block().text();
    int posInBlock = cursor.positionInBlock();

    // 获取触发前缀（可能包含 . 或 -> 或 ::）
    QString prefix = lineText.left(posInBlock);

    m_completionPopup->hide();
    m_completionActive = false;

    // 删除触发字符（如果有）
    if (prefix.endsWith('.')) {
        cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);
        cursor.removeSelectedText();
    } else if (prefix.endsWith("->")) {
        cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 2);
        cursor.removeSelectedText();
    } else if (prefix.endsWith("::")) {
        cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 2);
        cursor.removeSelectedText();
    }

    // 插入补全文本
    cursor.insertText(insertText);
    cursor.endEditBlock();
    setTextCursor(cursor);
}

void TextEditor::setupCompletion()
{
    m_completionModel = new CompletionModel(this);

    m_completionPopup = new CompletionPopup(this);
    m_completionPopup->setModel(m_completionModel);
    m_completionPopup->setItemDelegate(new CompletionDelegate(m_completionPopup));

    connect(m_completionPopup, &CompletionPopup::itemCommitted,
            this, &TextEditor::commitCompletion);

    m_lspDebounceTimer = new QTimer(this);
    m_lspDebounceTimer->setSingleShot(true);
    m_lspDebounceTimer->setInterval(200);
    connect(m_lspDebounceTimer, &QTimer::timeout, this, [this]() {
        if (m_completionActive) {
            requestLspCompletions(m_completionPrefix);
        }
    });
}

void TextEditor::autoIndentAfterReturn()
{
    QTextCursor cursor = textCursor();
    QTextBlock curBlock = cursor.block();
    QTextBlock prevBlock = curBlock.previous();
    if (!prevBlock.isValid()) return;

    const QString prevText    = prevBlock.text();
    const QString prevTrimmed = prevText.trimmed();
    const QString prevIndent  = leadingWhitespace(prevText);

    //  空行不继承缩进
    if (prevTrimmed.isEmpty()) {
        return;
    }

    // ---- 先做字符串前缀判断（O(前缀长度)），无触发就快速返回 ----
    const bool mayIndent = m_editorRules.endsWithIndentTrigger(prevTrimmed);
    const bool mayDedent = m_editorRules.startsWithDedentTrigger(prevTrimmed);

    if (!mayIndent && !mayDedent) {
        if (!prevIndent.isEmpty()) {
            cursor.insertText(prevIndent);
        }
        return;
    }

    const bool needGuard = m_editorRules.needsContextGuard();
    const bool prevSafe  = !needGuard
                          || !isInsideString(prevBlock.position() + prevText.length());

    QString newIndent = prevIndent;

    if (prevSafe) {
        if (mayIndent) {
            newIndent = prevIndent + indentUnit();
        } else if (mayDedent) {
            const QString unit = indentUnit();
            if (prevIndent.endsWith(unit)) {
                newIndent = prevIndent.left(prevIndent.length() - unit.length());
            }
        }
    }

    if (!newIndent.isEmpty()) {
        cursor.insertText(newIndent);
    }
}

bool TextEditor::tryAutoPair(const QString& input)
{
    if (input.isEmpty()) return false;
    const QChar typed = input.at(0);

    // 1. 闭合字符"跳过"
    {
        bool isClosingChar = false;
        bool isOpeningChar = false;
        for (const auto& p : m_editorRules.autoPairs) {
            if (p.second == typed) isClosingChar = true;
            if (p.first  == typed) isOpeningChar = true;
        }
        // 仅当"纯闭合"（如 ) ] }）时优先跳过；引号这类两侧相同的走下方配对
        if (isClosingChar && !isOpeningChar) {
            QTextCursor cursor = textCursor();
            const int pos = cursor.position();
            const QChar prev = (pos > 0) ? document()->characterAt(pos - 1) : QChar();
            if (prev == '\\') {
                return false;
            }
            if (document()->characterAt(pos) == typed) {
                cursor.setPosition(pos + 1);
                setTextCursor(cursor);
                return true;
            }
        }
    }

    // 2. 上下文保护：字符串/注释内不做"配对插入"
    if (m_editorRules.needsContextGuard()) {
        QTextCursor c = textCursor();
        if (isInsideStringOrComment(c.position())) {
            return false;
        }
    }

    // 3. { 交给 tryBraceNewline
    if (typed == '{' && m_editorRules.autoBraceOnNewline) {
        return false;
    }

    // 4. 配对插入
    QChar closing = QChar::Null;
    for (const auto& p : m_editorRules.autoPairs) {
        if (p.first == typed) { closing = p.second; break; }
    }
    if (closing == QChar::Null) return false;

    //直接用光标插入
    QTextCursor cursor = textCursor();
    const int pos = cursor.position();

    cursor.beginEditBlock();
    cursor.insertText(QString(typed) + QString(closing));
    cursor.endEditBlock();

    QTextCursor finalCursor(document());
    finalCursor.setPosition(pos + 1);
    setTextCursor(finalCursor);
    return true;
}

// ============================================================================
//  输入 else / catch / finally 时，把该行缩进对齐到对应的 if / try
//  调用时机：在常规输入后立即调用（字符已经插入）
// ============================================================================
bool TextEditor::tryAlignDedentKeyword()
{
    QTextCursor cursor = textCursor();
    const QTextBlock block = cursor.block();
    const QString text = block.text();
    const QString trimmed = text.trimmed();

    // 只在光标处于该行末尾时才处理
    if (cursor.positionInBlock() != text.length()) return false;

    static const QStringList kKeywords = { "else", "catch", "finally" };
    bool matched = false;
    for (const QString& kw : kKeywords) {
        if (trimmed == kw || trimmed == kw + " " || trimmed == kw + "{") {
            matched = true; break;
        }
    }
    if (!matched) return false;

    // 向上找最近的同级 { } 块的开头
    int depth = 0;
    QTextBlock b = block.previous();
    int targetIndent = -1;
    while (b.isValid()) {
        const QString t = b.text().trimmed();
        if (t.endsWith('}')) {
            ++depth;
        } else if (t.endsWith('{')) {
            if (depth == 0) {
                targetIndent = leadingWhitespace(b.text()).length();
                break;
            }
            --depth;
        }
        b = b.previous();
    }
    if (targetIndent < 0) return false;

    // 重设当前行缩进
    QTextCursor c(block);
    c.movePosition(QTextCursor::StartOfBlock);
    c.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor,
                   text.length() - trimmed.length());
    c.beginEditBlock();
    c.removeSelectedText();
    c.insertText(QString(targetIndent, ' '));
    c.endEditBlock();

    // 光标跟随
    QTextCursor nc(block);
    nc.movePosition(QTextCursor::EndOfBlock);
    setTextCursor(nc);
    return true;
}

// ============================================================================
//  Ctrl+Shift+\ ：从光标位置向后扫，定位第一个不匹配的括号
// ============================================================================
void TextEditor::jumpToMismatchedBracket()
{
    QTextDocument* doc = document();
    const int total = doc->characterCount();
    QTextCursor cursor = textCursor();
    const int startPos = cursor.position();

    // 用栈记录所有未闭合的开括号位置
    QVector<QPair<QChar, int>> stack;
    auto isOpen  = [](QChar c){ return c=='(' || c=='[' || c=='{'; };
    auto isClose = [](QChar c){ return c==')' || c==']' || c=='}'; };
    auto pairOf  = [](QChar c)->QChar {
        if (c=='(') return ')';
        if (c=='[') return ']';
        if (c=='{') return '}';
        if (c==')') return '(';
        if (c==']') return '[';
        if (c=='}') return '{';
        return QChar();
    };

    // 从文档头扫到文档尾（跳过字符串/注释过于复杂，这里不做）
    for (int i = 0; i < total; ++i) {
        const QChar c = doc->characterAt(i);
        if (isOpen(c)) {
            stack.append({ c, i });
        } else if (isClose(c)) {
            if (stack.isEmpty()) {
                // 孤立的闭合括号
                cursor.setPosition(i);
                setTextCursor(cursor);
                centerCursor();
                return;
            }
            const auto top = stack.last();
            if (pairOf(top.first) == c) {
                stack.removeLast();
            } else {
                // 类型不匹配
                cursor.setPosition(i);
                setTextCursor(cursor);
                centerCursor();
                return;
            }
        }
    }

    // 扫描完仍有未闭合的开括号 → 跳最后一个
    if (!stack.isEmpty()) {
        cursor.setPosition(stack.last().second);
    } else {
        cursor.setPosition(startPos);   // 没有不匹配 → 保持原位
    }
    setTextCursor(cursor);
    centerCursor();
}

// ============================================================================
//  Ctrl+/ ：注释选中行；已全部注释则取消
// ============================================================================
bool TextEditor::handleCommentShortcut(QKeyEvent* event)
{
    if (event->key() != Qt::Key_Slash) return false;
    if (event->modifiers() != Qt::ControlModifier) return false;

    // 复用已有的 commentSelection / uncommentSelection
    // 前者内部会判断"全已注释 → 反注释"
    commentSelection();
    return true;
}

// ============================================================================
//  Ctrl+Enter：在当前行下方插入空行（保留当前行缩进），光标落到新行
// ============================================================================
void TextEditor::insertLineBelow()
{
    QTextCursor cursor = textCursor();
    const QTextBlock block = cursor.block();
    const QString indent = leadingWhitespace(block.text());

    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::EndOfBlock);
    cursor.insertText("\n" + indent);
    cursor.endEditBlock();
    setTextCursor(cursor);
}

// ============================================================================
//  Ctrl+Shift+Enter：在当前行上方插入空行，光标落到新行
// ============================================================================
void TextEditor::insertLineAbove()
{
    QTextCursor cursor = textCursor();
    const QTextBlock block = cursor.block();
    const QString indent = leadingWhitespace(block.text());

    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.insertText(indent + "\n");
    // 光标放到刚插入的行（现在光标在原行行首）
    cursor.movePosition(QTextCursor::Up);
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::EndOfBlock);
    cursor.endEditBlock();
    setTextCursor(cursor);
}

// ============================================================================
//  智能退格：光标左侧全是空白且缩进对齐到 tabWidth 的整数倍时，
//  一次退一整个缩进单元（tabWidth 个空格）
// ============================================================================
bool TextEditor::trySmartBackspace()
{
    if (!m_useSpacesForTab) return false;

    QTextCursor cursor = textCursor();
    if (cursor.hasSelection()) return false;

    const QTextBlock block = cursor.block();
    const QString text = block.text();
    const int col = cursor.positionInBlock();
    if (col == 0) return false;

    // 光标左侧必须全是空白
    for (int i = 0; i < col; ++i) {
        if (!text[i].isSpace()) return false;
    }

    // 只处理 col 在 (0, tabWidth] 内的场景，或刚好是 tabWidth 整数倍
    int removeCount = 0;
    if (col % m_tabWidth == 0) {
        removeCount = m_tabWidth;
    } else {
        // 不是整数倍，就退到最近的上一个 tab stop
        removeCount = col % m_tabWidth;
    }

    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, removeCount);
    cursor.removeSelectedText();
    cursor.endEditBlock();
    return true;
}

void TextEditor::matchParentheses()
{
    QTextCursor cursor = textCursor();
    int position = cursor.position();

    if (position == 0) {
        return;
    }

    QTextDocument *doc = document();
    QChar charBefore = doc->characterAt(position - 1);
    QChar charAfter = doc->characterAt(position);

    QList<QTextEdit::ExtraSelection> extraSelections = this->extraSelections();

    for (int i = 0; i < extraSelections.size(); ++i) {
        if (extraSelections[i].format.background().color() == QColor(255, 255, 0) ||
            extraSelections[i].format.background().color() == QColor(255, 0, 0)) {
            extraSelections.removeAt(i);
            i--;
        }
    }

    QChar matchChar;
    int matchPosition = -1;
    bool isForward = false;

    if (charBefore == '(' || charBefore == '[' || charBefore == '{') {
        matchChar = (charBefore == '(') ? ')' : (charBefore == '[') ? ']' : '}';
        matchPosition = findMatchingParenthesis(position - 1, matchChar, false);
        isForward = true;
    } else if (charAfter == ')' || charAfter == ']' || charAfter == '}') {
        matchChar = (charAfter == ')') ? '(' : (charAfter == ']') ? '[' : '}';
        matchPosition = findMatchingParenthesis(position, matchChar, true);
        isForward = false;
    }

    if (matchPosition != -1) {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(QColor(255, 255, 0));

        selection.cursor = cursor;
        if (isForward) {
            selection.cursor.setPosition(position - 1);
            selection.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 1);
        } else {
            selection.cursor.setPosition(position);
            selection.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 1);
        }
        extraSelections.append(selection);

        selection.cursor = cursor;
        selection.cursor.setPosition(matchPosition);
        selection.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 1);
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void TextEditor::mouseDoubleClickEvent(QMouseEvent* event)
{
    QTextCursor cursor = cursorForPosition(event->pos());
    const QString text = toPlainText();
    int pos = cursor.position();
    if (pos < 0 || pos > text.length()) {
        QPlainTextEdit::mouseDoubleClickEvent(event);
        return;
    }

    auto isWordChar = [](QChar c) {
        return c.isLetterOrNumber() || c == '_';
    };

    int start = pos;
    while (start > 0 && isWordChar(text[start - 1])) --start;
    int end = pos;
    while (end < text.length() && isWordChar(text[end])) ++end;

    if (end > start) {
        QTextCursor sel = textCursor();
        sel.setPosition(start);
        sel.setPosition(end, QTextCursor::KeepAnchor);
        setTextCursor(sel);
        return;
    }
    QPlainTextEdit::mouseDoubleClickEvent(event);
}

bool TextEditor::tryBraceNewline(const QString& input)
{
    qDebug() << "[Editor] tryBraceNewline input=" << input;
    if (!m_editorRules.autoBraceOnNewline) return false;
    if (input != "{") return false;

    QTextCursor cursor = textCursor();

    // ---- 上下文保护 ----
    if (m_editorRules.needsContextGuard()) {
        if (isInsideStringOrComment(cursor.position())) {
            return false;
        }
    }

    const QTextBlock block = cursor.block();
    const QString lineText   = block.text();
    const QString baseIndent  = leadingWhitespace(lineText);
    const QString innerIndent = baseIndent + indentUnit();

    const int pos = cursor.position();

    // ---- 三行展开 ----
    // 布局（从 pos 开始）：
    //   pos       → '{'
    //   pos+1     → '\n'
    //   pos+2     → innerIndent 第一个字符
    //   ...
    //   pos+2+innerLen → '\n'
    //   ...            → baseIndent + '}'
    cursor.beginEditBlock();
    cursor.insertText("{");
    cursor.insertText("\n" + innerIndent);
    cursor.insertText("\n" + baseIndent + "}");
    cursor.endEditBlock();

    // ---- 光标绝对定位到中间那行的缩进之后 ----
    // 目标位置 = pos + 1 (左大括号) + 1 (\n) + innerIndent.length()
    QTextCursor finalCursor(document());
    finalCursor.setPosition(pos + 2 + innerIndent.length());
    setTextCursor(finalCursor);

    return true;
}

QString TextEditor::indentUnit() const
{
    if (m_useSpacesForTab) {
        return QString(m_tabWidth, ' ');
    }
    return QStringLiteral("\t");
}

QString TextEditor::leadingWhitespace(const QString& line) const
{
    QString ws;
    for (QChar c : line) {
        if (c == ' ' || c == '\t') ws += c;
        else break;
    }
    return ws;
}

// ============================================================
// 判断 position 是否在字符串字面量内部
//
// 策略：从当前块起始扫描到 position，对 ", ', ` 做配对翻转，
//       遇到 \ 跳过下一个字符
// 局限：不处理三引号、原始字符串等复杂情况，
//       但对"阻止 { 在 f"{...}" 里被强行展开"已经足够
// ============================================================
bool TextEditor::isInsideString(int position) const
{
    QTextBlock block = document()->findBlock(position);
    if (!block.isValid()) return false;

    const QString line = block.text();
    const int posInBlock = position - block.position();

    bool inDq = false, inSq = false, inBt = false;
    bool escaped = false;

    for (int i = 0; i < posInBlock && i < line.length(); ++i) {
        const QChar c = line[i];
        if (escaped) { escaped = false; continue; }
        if (c == '\\') { escaped = true; continue; }

        if (c == '"'  && !inSq && !inBt) inDq = !inDq;
        else if (c == '\'' && !inDq && !inBt) inSq = !inSq;
        else if (c == '`'  && !inDq && !inSq) inBt = !inBt;
    }

    return inDq || inSq || inBt;
}

bool TextEditor::isInsideLineComment(int position) const
{
    if (m_editorRules.lineComment.isEmpty()) return false;

    QTextBlock block = document()->findBlock(position);
    if (!block.isValid()) return false;

    const QString line = block.text();
    const int posInBlock = position - block.position();

    // 找本行注释符起始位置（跳过字符串内的假注释符号）
    // 简化处理：找到第一个不在字符串内的注释符
    bool inDq = false, inSq = false, inBt = false, esc = false;
    for (int i = 0; i + m_editorRules.lineComment.length() <= line.length(); ++i) {
        const QChar c = line[i];
        if (esc) { esc = false; continue; }
        if (c == '\\') { esc = true; continue; }
        if (c == '"'  && !inSq && !inBt) { inDq = !inDq; continue; }
        if (c == '\'' && !inDq && !inBt) { inSq = !inSq; continue; }
        if (c == '`'  && !inDq && !inSq) { inBt = !inBt; continue; }

        if (!inDq && !inSq && !inBt) {
            if (line.mid(i, m_editorRules.lineComment.length())
                == m_editorRules.lineComment) {
                return i < posInBlock;
            }
        }
    }
    return false;
}

bool TextEditor::isInsideStringOrComment(int position) const
{
    if (m_editorRules.suppressInsideString && isInsideString(position))
        return true;
    if (m_editorRules.suppressInsideLineComment && isInsideLineComment(position))
        return true;
    return false;
}

void TextEditor::applyEditorRulesForLanguage(const QString& languageId)
{
    m_editorRules = EditorRules::forLanguage(languageId);
    // 可选：同步到 highlighter 已有的缩进设置
    // （如果 LanguageInfo 里也有 indentStyle / indentSize，可在此对齐）
}

// ============================================================================
//  Tab / Shift+Tab 处理入口
//  - 有选区：调 indentSelection（多行整体缩进）
//  - 无选区 + increase：对齐到下一个 tab stop
//  - 无选区 + !increase：删除行首一个缩进单元（上轮已修）
// ============================================================================
void TextEditor::handleTabKey(bool increase)
{
    QTextCursor cursor = textCursor();

    // ---- 有选区：多行处理 ----
    if (cursor.hasSelection()) {
        indentSelection(increase);
        return;
    }

    // ---- 无选区：局部处理 ----
    if (!increase) {
        // 反缩进：删除"当前行行首"一个缩进单元
        const QTextBlock block = cursor.block();
        const QString text = block.text();
        if (text.isEmpty()) return;

        int removeCount = 0;
        if (text.at(0) == '\t') {
            removeCount = 1;
        } else {
            int i = 0;
            while (i < text.length() && i < m_tabWidth && text.at(i) == ' ') ++i;
            removeCount = i;
        }
        if (removeCount == 0) return;

        const int oldPos   = cursor.position();
        const int blockStart = block.position();
        const int removeEnd  = blockStart + removeCount;

        QTextCursor bc(block);
        bc.movePosition(QTextCursor::StartOfBlock);
        bc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, removeCount);
        bc.beginEditBlock();
        bc.removeSelectedText();
        bc.endEditBlock();

        QTextCursor nc = textCursor();
        if (oldPos <= removeEnd) nc.setPosition(blockStart);
        else                     nc.setPosition(oldPos - removeCount);
        setTextCursor(nc);
        return;
    }

    // 缩进：对齐到下一个 tab stop
    cursor.beginEditBlock();
    if (m_useSpacesForTab) {
        const int col = cursor.positionInBlock();
        const int spaces = m_tabWidth - (col % m_tabWidth);
        cursor.insertText(QString(spaces, ' '));
    } else {
        cursor.insertText("\t");
    }
    cursor.endEditBlock();
}

// ============================================================================
//  智能 Home：首次跳到首个非空白列，已在首个非空白列则跳列 0
// ============================================================================
bool TextEditor::handleSmartHome(QKeyEvent* event)
{
    if (event->key() != Qt::Key_Home) return false;
    if (event->modifiers() & Qt::ControlModifier) return false;  // Ctrl+Home 保留

    QTextCursor cursor = textCursor();
    const QString line = cursor.block().text();
    int firstNonWs = 0;
    while (firstNonWs < line.length() && line[firstNonWs].isSpace()) ++firstNonWs;

    const int curCol = cursor.positionInBlock();
    int targetCol = (curCol != firstNonWs) ? firstNonWs : 0;

    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, targetCol);
    setTextCursor(cursor);
    return true;
}

// ============================================================================
//  智能 End：默认跳到行尾（Qt 已如此），这里只保留对 Ctrl+End 的处理
//  可选扩展：第一次跳到最后一个非空白字符后，再按到行尾
// ============================================================================
bool TextEditor::handleSmartEnd(QKeyEvent* event)
{
    if (event->key() != Qt::Key_End) return false;
    if (event->modifiers() & Qt::ControlModifier) return false;

    QTextCursor cursor = textCursor();
    const QString line = cursor.block().text();
    int lastNonWs = line.length();
    while (lastNonWs > 0 && line[lastNonWs - 1].isSpace()) --lastNonWs;

    const int curCol = cursor.positionInBlock();
    int targetCol = (curCol != lastNonWs) ? lastNonWs : line.length();

    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, targetCol);
    setTextCursor(cursor);
    return true;
}

// ============================================================================
//  选中文本时输入 opening 字符 → 用一对字符包裹选区
//  例：选中 foo，输入 " → "foo"；输入 ( → (foo)
// ============================================================================
bool TextEditor::trySurroundSelection(const QString& input)
{
    if (input.isEmpty()) return false;
    const QChar typed = input.at(0);

    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) return false;

    // 查配对表
    QChar closing = QChar::Null;
    for (const auto& p : m_editorRules.autoPairs) {
        if (p.first == typed) { closing = p.second; break; }
    }
    // 引号也允许包裹
    if (closing == QChar::Null) {
        if (typed == '"')       closing = '"';
        else if (typed == '\'') closing = '\'';
        else if (typed == '`')  closing = '`';
        else return false;
    }

    // 上下文保护：字符串/注释里不做包裹
    if (m_editorRules.needsContextGuard()
        && isInsideStringOrComment(cursor.selectionStart())) {
        return false;
    }

    const int start = cursor.selectionStart();
    const QString sel = cursor.selectedText();   // 注意：换行是 U+2029

    cursor.beginEditBlock();
    cursor.insertText(QString(typed) + sel + QString(closing));
    cursor.endEditBlock();

    // 选中被包裹的内容
    QTextCursor nc(document());
    nc.setPosition(start + 1);
    nc.setPosition(start + 1 + sel.length(), QTextCursor::KeepAnchor);
    setTextCursor(nc);
    return true;
}

// ============================================================================
//  Backspace 位于空配对中间 → 删除整对
//  例：(|) 按 Backspace → |
// ============================================================================
bool TextEditor::tryDeletePairOnBackspace()
{
    QTextCursor cursor = textCursor();
    if (cursor.hasSelection()) return false;

    const int pos = cursor.position();
    if (pos <= 0 || pos >= document()->characterCount()) return false;

    const QChar prev = document()->characterAt(pos - 1);
    const QChar next = document()->characterAt(pos);

    // 明确的开闭字符对
    for (const auto& p : m_editorRules.autoPairs) {
        if (prev == p.first && next == p.second) {
            cursor.beginEditBlock();
            cursor.deletePreviousChar();
            cursor.deleteChar();
            cursor.endEditBlock();
            return true;
        }
    }
    // 引号（两侧相同）
    if ((prev == '"'  && next == '"')
        || (prev == '\'' && next == '\'')
        || (prev == '`'  && next == '`')) {
        cursor.beginEditBlock();
        cursor.deletePreviousChar();
        cursor.deleteChar();
        cursor.endEditBlock();
        return true;
    }
    return false;
}

// ============================================================================
//  在 C 风格块注释内回车 → 自动插入 " * "
//  触发条件：当前行 trimmed 以 "*" 或 "/*" 开头，且向上能追溯到 "/*"
// ============================================================================
bool TextEditor::tryContinueBlockComment()
{
    QTextCursor cursor = textCursor();
    const QTextBlock block = cursor.block();
    const QString text    = block.text();
    const QString trimmed = text.trimmed();

    // 当前行必须是 " * xxx" / " *" / "/*xxx" / "/**"
    const bool startsWithStar = trimmed.startsWith("*") && !trimmed.startsWith("*/");
    const bool startsWithOpen = trimmed.startsWith("/*");
    if (!startsWithStar && !startsWithOpen) return false;

    // 向上确认处于块注释中
    QTextBlock b = block;
    bool inBlockComment = false;
    while (b.isValid()) {
        const QString t = b.text().trimmed();
        if (t.startsWith("/*")) { inBlockComment = true; break; }
        if (t.startsWith("*/")) break;
        if (t.startsWith("//")) break;
        if (!t.isEmpty() && !t.startsWith("*")) break;
        b = b.previous();
    }
    if (!inBlockComment) return false;

    // 若当前行已以 */ 结尾，直接正常回车（退出注释）
    if (trimmed.endsWith("*/")) return false;

    // 计算插入前缀（保留原缩进 + " * "）
    QString indent = leadingWhitespace(text);

    // 让基类插入换行 + 继承缩进（QPlainTextEdit 默认会保留缩进）
    QPlainTextEdit::keyPressEvent(
        new QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier));

    // 此时光标在新行行首、继承了 indent（若 Qt 没继承则手动补）
    QTextCursor nc = textCursor();
    const QString newLine = nc.block().text();
    if (newLine.isEmpty() || !newLine.startsWith(indent)) {
        nc.insertText(indent);
    }
    nc.insertText(" * ");
    return true;
}

// ============================================================================
//  光标前的标识符前缀（只取字母数字下划线）
// ============================================================================
QString TextEditor::currentIdentPrefix() const
{
    QTextCursor cursor = textCursor();
    const QString line = cursor.block().text();
    const int col = cursor.positionInBlock();
    int i = col;
    while (i > 0 && (line[i-1].isLetterOrNumber() || line[i-1] == '_')) --i;
    return line.mid(i, col - i);
}

// ============================================================================
//  内置片段表（按语言可扩展）
//  - key: 触发前缀；body: 展开内容（$0=最终光标，$1..$9=顺序 Tab 位置）
// ============================================================================
static const QHash<QString, QString>& builtinSnippets()
{
    static const QHash<QString, QString> table = {
                                                  // 通用
                                                  { "for",    "for (int $1 = 0; $1 < $2; ++$1) {\n    $0\n}" },
                                                  { "foreach","for (auto& $1 : $2) {\n    $0\n}" },
                                                  { "while",  "while ($1) {\n    $0\n}" },
                                                  { "if",     "if ($1) {\n    $0\n}" },
                                                  { "ifelse", "if ($1) {\n    $2\n} else {\n    $0\n}" },
                                                  { "switch", "switch ($1) {\n    case $2:\n        $0\n        break;\n}" },
                                                  { "class",  "class $1 {\npublic:\n    $1();\n    ~$1();\n\nprivate:\n    $0\n};" },
                                                  { "struct", "struct $1 {\n    $0\n};" },
                                                  { "func",   "$1 $2($3) {\n    $0\n}" },
                                                  { "try",    "try {\n    $1\n} catch (const std::exception& e) {\n    $0\n}" },
                                                  { "inc",    "#include <$0>" },
                                                  { "main",   "int main(int argc, char** argv) {\n    $0\n    return 0;\n}" },
                                                  // 语言相关：Python
                                                  { "def",    "def $1($2):\n    $0" },
                                                  { "pyif",   "if $1:\n    $0" },
                                                  { "pyfor",  "for $1 in $2:\n    $0" },
                                                  };
    return table;
}

// ============================================================================
//  Tab 按下时优先尝试展开片段
// ============================================================================
bool TextEditor::tryExpandSnippet()
{
    const QString prefix = currentIdentPrefix();
    if (prefix.isEmpty()) return false;

    const auto& table = builtinSnippets();
    if (!table.contains(prefix)) return false;

    QTextCursor cursor = textCursor();
    const int col = cursor.positionInBlock();

    // 删除已输入的前缀
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, prefix.length());
    cursor.removeSelectedText();
    cursor.endEditBlock();

    // 取当前行缩进
    const QTextBlock block = cursor.block();
    const int indent = leadingWhitespace(block.text()).length();

    applySnippet(table.value(prefix), indent);
    return true;
}

// ============================================================================
//  判断光标是否处于 #include " 或 #include < 之后
//  若是，输出已输入前缀（prefixOut）
// ============================================================================
bool TextEditor::isInIncludeContext(QString* prefixOut) const
{
    QTextCursor cursor = textCursor();
    const QTextBlock block = cursor.block();
    const QString line = block.text();
    const int col = cursor.positionInBlock();
    const QString left = line.left(col);

    // 两种形式：#include "xxx  或  #include <xxx
    static const QRegularExpression re(
        R"(^\s*#\s*include\s*[<"]([^">]*)$)");
    const auto m = re.match(left);
    if (!m.hasMatch()) return false;

    if (prefixOut) *prefixOut = m.captured(1);
    return true;
}

// ============================================================================
//  在 include 上下文里刷新补全：列出当前目录 + include 目录下的头文件
// ============================================================================
void TextEditor::requestIncludeCompletions()
{
    QString prefix;
    if (!isInIncludeContext(&prefix)) return;

    QList<CompletionEntry> entries;

    // 搜索目录
    QStringList dirs;
    if (!m_filePath.isEmpty()) {
        const QFileInfo fi(m_filePath);
        dirs << fi.absolutePath();
        dirs << fi.absolutePath() + "/include";
        dirs << fi.absolutePath() + "/../include";
    }

    QSet<QString> seen;
    for (const QString& d : dirs) {
        QDir dir(d);
        if (!dir.exists()) continue;
        const QStringList files = dir.entryList(
            QStringList() << "*.h" << "*.hpp" << "*.hh"
                          << "*.c" << "*.cpp" << "*.cc",
            QDir::Files);

        for (const QString& f : files) {
            if (seen.contains(f)) continue;
            seen.insert(f);

            CompletionEntry e;
            e.label      = f;
            e.insertText = f;
            e.detail     = d;
            e.kind       = "file";
            e.sortPriority = 5;
            entries.append(e);
        }
    }

    m_completionPrefix = prefix;
    m_completionAnchor = textCursor().position() - prefix.length();
    m_completionActive = true;
    m_pendingLspItems.clear();

    m_completionModel->setEntries(entries);
    m_completionModel->setFilterPrefix(prefix);

    if (m_completionModel->rowCount() > 0) {
        QRect rect = cursorRect();
        QPoint globalPos = viewport()->mapToGlobal(rect.bottomLeft());
        m_completionPopup->showWithModel(m_completionModel, globalPos, 360);
    } else {
        m_completionPopup->hideIfVisible();
    }
}

// ============================================================================
//  保存前调用：按语言选择外部格式化器
//  - C/C++: clang-format
//  - Python: black
//  - JS/TS:  prettier
//  命令行存在即执行；否则静默跳过
// ============================================================================
bool TextEditor::runExternalFormatter(QString* errorOut)
{
    if (m_filePath.isEmpty()) return false;

    const QString ext = QFileInfo(m_filePath).suffix().toLower();
    QString program;
    QStringList args;

    if (ext == "cpp" || ext == "cc" || ext == "cxx"
        || ext == "h"  || ext == "hpp" || ext == "hh"
        || ext == "c") {
        program = "clang-format";
        args << "-i" << m_filePath;
    } else if (ext == "py") {
        program = "black";
        args << "-q" << m_filePath;
    } else if (ext == "js" || ext == "ts" || ext == "jsx" || ext == "tsx") {
        program = "prettier";
        args << "--write" << m_filePath;
    } else {
        return false;   // 该语言暂不支持
    }

    // 检查命令是否存在
    if (QStandardPaths::findExecutable(program).isEmpty()) {
        if (errorOut) *errorOut = program + " 未安装";
        return false;
    }

    // 先把当前编辑内容写盘（否则格式化的是旧版本）
    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorOut) *errorOut = "无法写入文件";
        return false;
    }
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << toPlainText();
    file.close();

    // 执行格式化
    QProcess proc;
    proc.setWorkingDirectory(QFileInfo(m_filePath).absolutePath());
    proc.start(program, args);
    if (!proc.waitForFinished(15000)) {
        proc.kill();
        if (errorOut) *errorOut = program + " 超时";
        return false;
    }
    if (proc.exitCode() != 0) {
        if (errorOut) *errorOut = QString::fromLocal8Bit(proc.readAllStandardError());
        return false;
    }

    // 重新读回
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    const QString formatted = in.readAll();
    file.close();

    // 保存当前光标位置
    const int line = currentLine();
    const int col  = currentColumn();

    beginSilentModification();
    setPlainText(formatted);
    endSilentModification();
    document()->setModified(false);

    gotoLine(line);
    QTextCursor c = textCursor();
    c.movePosition(QTextCursor::StartOfBlock);
    c.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, col - 1);
    setTextCursor(c);

    return true;
}

// ============================================================================
//  把行号映射到滚动条 Y 坐标
// ============================================================================
void TextEditor::paintScrollBarMarkers()
{
    QScrollBar* sb = verticalScrollBar();
    if (!sb) return;

    QPainter painter(sb);
    painter.setRenderHint(QPainter::Antialiasing, false);

    const int totalLines = qMax(1, lineCount());
    const int sbHeight   = sb->height();
    const int sbWidth    = sb->width();

    // 标记宽度随滚动条宽度自适应：
    //   普通屏滚动条约 12px → markerWidth = 2；高 DPI 约 18px → 3
    //   用 /6 是为了让"红线"看起来像滚动条上一个细条，不喧宾夺主
    const int markerWidth  = qMax(2, sbWidth);
    const int markerHeight = qMax(2, sbWidth / 5);   // 高度同理
    const int markerX      = sbWidth - markerWidth - 1;

    auto drawMarker = [&](int line, const QColor& color) {
        if (line < 1 || line > totalLines) return;

        const int min = sb->minimum();
        const int max = sb->maximum();
        const int range = max - min;

        int y;
        if (range <= 0) {
            y = (line - 1) * sbHeight / totalLines;
        } else {
            const int sbValue = min + (line - 1) * range / (totalLines - 1);
            const int pageStep = sb->pageStep();
            y = (sbValue - min) * sbHeight / (range + pageStep);
        }

        painter.fillRect(markerX, y, markerWidth, markerHeight, color);
    };

    for (const ErrorMarker& m : m_errorMarkers) {
        drawMarker(m.line, QColor(220, 60, 60));
    }
    for (const ErrorMarker& m : m_warningMarkers) {
        drawMarker(m.line, QColor(230, 180, 40));
    }
    for (int line : m_breakpoints) {
        drawMarker(line, QColor(70, 140, 220));
    }
}

// ============================================================================
//  应用片段：把 $1 $2 ... $0 按顺序替换为可 Tab 跳转的占位符
//  简化实现：$0 直接落光标；$1..$9 用同一个符号占位，此处只处理 $0
//  如果要支持多 Tab 跳，可以后续接 tabstop 机制
// ============================================================================
void TextEditor::applySnippet(const QString& body, int lineIndent)
{
    QTextCursor cursor = textCursor();

    // 把 body 里所有行首加缩进
    QStringList lines = body.split('\n');
    const QString indentStr(lineIndent, ' ');
    for (int i = 0; i < lines.size(); ++i) {
        if (i == 0) continue;
        if (!lines[i].isEmpty()) lines[i] = indentStr + lines[i];
    }
    QString text = lines.join('\n');

    // 记录 $0 的位置（0-based，从 text 里数）
    int cursorOffset = -1;
    int idx = text.indexOf("$0");
    if (idx >= 0) {
        cursorOffset = idx;
        text.remove(idx, 2);
    }
    // 清掉 $1 $2 ... （简化：直接删）
    for (int i = 1; i <= 9; ++i) {
        text.remove(QString("$%1").arg(i));
    }

    cursor.beginEditBlock();
    cursor.insertText(text);
    cursor.endEditBlock();

    // 光标落到 $0 位置
    if (cursorOffset >= 0) {
        QTextCursor nc = cursor;
        nc.setPosition(cursor.position() - text.length() + cursorOffset);
        setTextCursor(nc);
    }
}

// ============================================================================
//  Alt+↑：当前行（或选中行块）整体上移一行
// ============================================================================
void TextEditor::moveLinesUp()
{
    QTextCursor cursor = textCursor();
    QTextBlock cur = cursor.block();
    QTextBlock prev = cur.previous();
    if (!prev.isValid()) return;

    // 若选区跨多行，只移动选中范围
    int startLine = cur.blockNumber();
    int endLine   = cur.blockNumber();
    if (cursor.hasSelection()) {
        QTextCursor sc = cursor;
        sc.setPosition(cursor.selectionStart());
        startLine = sc.blockNumber();
        sc.setPosition(cursor.selectionEnd());
        endLine = sc.blockNumber();
        // 选区末尾正好在行首时，不把这一行算进来
        if (sc.positionInBlock() == 0 && endLine > startLine) --endLine;
    }
    if (startLine == 0) return;   // 已在文档首行

    const int col = cursor.positionInBlock();

    cursor.beginEditBlock();
    // 把 [startLine, endLine] 整块与上一行交换
    // 简化实现：逐行互换文本内容
    QTextBlock up = document()->findBlockByNumber(startLine - 1);
    for (int i = startLine; i <= endLine; ++i) {
        QTextBlock b = document()->findBlockByNumber(i);
        const QString curText = b.text();
        const QString upText  = up.text();

        QTextCursor c1(b);
        c1.movePosition(QTextCursor::StartOfBlock);
        c1.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        c1.insertText(upText);

        QTextCursor c2(up);
        c2.movePosition(QTextCursor::StartOfBlock);
        c2.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        c2.insertText(curText);

        up = b;
    }
    cursor.endEditBlock();

    // 光标上移一行，保持列
    QTextCursor nc = cursor;
    nc.movePosition(QTextCursor::StartOfBlock);
    nc.movePosition(QTextCursor::Up);
    nc.movePosition(QTextCursor::StartOfBlock);
    nc.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, col);
    setTextCursor(nc);
}

// ============================================================================
//  Alt+↓：当前行（或选中行块）整体下移一行
// ============================================================================
void TextEditor::moveLinesDown()
{
    QTextCursor cursor = textCursor();
    QTextBlock cur = cursor.block();

    int startLine = cur.blockNumber();
    int endLine   = cur.blockNumber();
    if (cursor.hasSelection()) {
        QTextCursor sc = cursor;
        sc.setPosition(cursor.selectionStart());
        startLine = sc.blockNumber();
        sc.setPosition(cursor.selectionEnd());
        endLine = sc.blockNumber();
        if (sc.positionInBlock() == 0 && endLine > startLine) --endLine;
    }

    if (endLine >= document()->blockCount() - 1) return;  // 已在末行

    const int col = cursor.positionInBlock();

    cursor.beginEditBlock();
    // 从下往上交换
    for (int i = endLine; i >= startLine; --i) {
        QTextBlock b  = document()->findBlockByNumber(i);
        QTextBlock dn = b.next();
        const QString curText = b.text();
        const QString dnText  = dn.text();

        QTextCursor c1(b);
        c1.movePosition(QTextCursor::StartOfBlock);
        c1.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        c1.insertText(dnText);

        QTextCursor c2(dn);
        c2.movePosition(QTextCursor::StartOfBlock);
        c2.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        c2.insertText(curText);
    }
    cursor.endEditBlock();

    // 光标下移一行，保持列
    QTextCursor nc = cursor;
    nc.movePosition(QTextCursor::StartOfBlock);
    nc.movePosition(QTextCursor::Down);
    nc.movePosition(QTextCursor::StartOfBlock);
    nc.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, col);
    setTextCursor(nc);
}

// ============================================================================
//  编辑器快捷键：
//    Ctrl+D        复制当前行
//    Ctrl+Shift+K  删除当前行
// ============================================================================
bool TextEditor::handleEditorShortcuts(QKeyEvent* event)
{
    const Qt::KeyboardModifiers m = event->modifiers();

    // Ctrl+D 复制行
    if (event->key() == Qt::Key_D
        && m == Qt::ControlModifier) {
        duplicateLine();
        return true;
    }

    // Ctrl+Shift+K 删除行（也可换成 Ctrl+Y）
    if (event->key() == Qt::Key_K
        && m == (Qt::ControlModifier | Qt::ShiftModifier)) {
        deleteLine();
        return true;
    }

    return false;
}

void TextEditor::beginSilentModification()
{
    m_suppressModificationFlag = true;
}

void TextEditor::endSilentModification()
{
    m_suppressModificationFlag = false;

    // 从"脏"恢复为"净"——因为这是程序内部改动，用户没编辑
    m_isModified = false;
    document()->setModified(false);
}

// ============================================================================
//  SVG 图标缓存
//  - 第一次调用：用 QSvgRenderer 渲染到 QPixmap，存进 m_iconCache
//  - 后续调用：直接命中缓存（O(1) hash 查表）
//  - 主题切换时必须 invalidateIconCache() 否则颜色不会更新
// ============================================================================
QPixmap TextEditor::cachedIcon(const QString& svgPath, const QSize& size) const
{
    const QString key = svgPath + QString("@%1x%2")
    .arg(size.width())
        .arg(size.height());

    auto it = m_iconCache.find(key);
    if (it != m_iconCache.end()) {
        return it.value();
    }

    // ---- 首次渲染 ----
    QPixmap pm(size);
    pm.fill(Qt::transparent);

    QSvgRenderer renderer(svgPath);
    if (renderer.isValid()) {
        QPainter p(&pm);
        // 高质量缩放
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        renderer.render(&p);
        p.end();
    } else {
        qWarning() << "[TextEditor] 无效 SVG:" << svgPath;
    }

    m_iconCache.insert(key, pm);
    return pm;
}

void TextEditor::invalidateIconCache()
{
    m_iconCache.clear();
}

void TextEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    if (m_showLineNumbers) {
        QRect cr = contentsRect();
        m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(),
                                            lineNumberAreaWidth(), cr.height()));
        // qDebug() << "LineNumberArea geometry:" << m_lineNumberArea->geometry();
    }
}

void TextEditor::contextMenuEvent(QContextMenuEvent *event)
{
    if (m_contextMenu) {
        m_undoAction->setEnabled(document()->isUndoAvailable());
        m_redoAction->setEnabled(document()->isRedoAvailable());
        m_cutAction->setEnabled(textCursor().hasSelection());
        m_copyAction->setEnabled(textCursor().hasSelection());
        m_pasteAction->setEnabled(!QApplication::clipboard()->text().isEmpty());

        m_contextMenu->exec(event->globalPos());
    }
}

void TextEditor::updateLineNumberAreaWidth()
{
    if (m_showLineNumbers) {
        setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
    } else {
        setViewportMargins(0, 0, 0, 0);
    }
}

void TextEditor::highlightCurrentLine()
{
    if (!m_highlightCurrentLine) return;

    //  读取现有 selection（不清空！）
    QList<QTextEdit::ExtraSelection> all = extraSelections();

    // 移除旧的"当前行高亮"（带 FullWidthSelection 属性的）
    for (int i = all.size() - 1; i >= 0; --i) {
        if (all[i].format.boolProperty(QTextFormat::FullWidthSelection)) {
            all.removeAt(i);
        }
    }

    if (!textCursor().hasSelection()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = ThemeManager::instance().color(
            ThemeManager::UIElement::CurrentLineHighlight);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        all.append(selection);
    }

    setExtraSelections(all);
}

void TextEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy != 0) {
        m_lineNumberArea->scroll(0, dy);
    } else {
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth();
    }
}

void TextEditor::onCursorPositionChanged()
{
    if (m_highlightCurrentLine) {
        highlightCurrentLine();
    }
    matchParentheses();

    emit cursorPositionChanged(currentLine(), currentColumn());
}

void TextEditor::onTextChanged()
{
    if (m_suppressModificationFlag) {
        return;
    }

    if (!document()->isModified()) {
        if (!m_isModified) {
            return;
        }
    }

    m_isModified = true;
    emit modificationChanged(true);

    // ---- LSP didChange 走防抖 ----
    if (!m_filePath.isEmpty() && m_lspChangeTimer) {
        m_pendingLspChange = true;
        m_lspChangeTimer->start();   // 重置计时器
    }

    // updateFolding();
}

void TextEditor::onGotoLineAction()
{
    bool ok;
    int line = QInputDialog::getInt(this, tr("跳转到行"), tr("行号:"), 1, 1, lineCount(), 1, &ok);
    if (ok) {
        gotoLine(line);
    }
}

int TextEditor::findMatchingParenthesis(int position, QChar parenthesis, bool backward)
{
    QTextDocument *doc = document();
    QChar otherParenthesis;

    if (parenthesis == '(') otherParenthesis = ')';
    else if (parenthesis == ')') otherParenthesis = '(';
    else if (parenthesis == '[') otherParenthesis = ']';
    else if (parenthesis == ']') otherParenthesis = '[';
    else if (parenthesis == '{') otherParenthesis = '}';
    else if (parenthesis == '}') otherParenthesis = '{';
    else return -1;

    int depth = 1;
    int i = backward ? position - 1 : position + 1;

    while (backward ? i >= 0 : i < doc->characterCount()) {
        QChar ch = doc->characterAt(i);

        if (ch == parenthesis) {
            depth++;
        } else if (ch == otherParenthesis) {
            depth--;

            if (depth == 0) {
                return i;
            }
        }

        if (backward) {
            i--;
        } else {
            i++;
        }
    }

    return -1;
}

int TextEditor::lineNumberAreaWidth() const
{
    if (!m_showLineNumbers) {
        return 0;
    }

    int digits = 1;
    int max = qMax(1, lineCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int digitWidth = fontMetrics().horizontalAdvance(QLatin1Char('9'));
    if (digitWidth <= 0) {
        digitWidth = 8;
    }

    // 左侧留出 22px 作为断点点击区，右侧留出 16px 给断点红点
    int leftPadding = 26;
    int rightPadding = 16;
    int width = leftPadding + digitWidth * digits + rightPadding;

    // 最小宽度 55px
    return qMax(width, 60);
}

// ============================================================================
//  多行缩进 / 反缩进
//  - increase = true  → Tab（在每行行首插入一个缩进单元）
//  - increase = false → Shift+Tab（删除每行行首的一个缩进单元）
//  - 有选区：处理选区覆盖的所有完整行
//  - 无选区：Tab 在光标处插入缩进；Shift+Tab 删除光标前一个缩进单元
// ============================================================================
void TextEditor::indentSelection(bool increase)
{
    QTextCursor cursor = textCursor();

    // ------------------------------------------------------------------------
    // A. 无选区：局部处理
    // ------------------------------------------------------------------------
    if (!cursor.hasSelection()) {
        if (increase) {
            cursor.beginEditBlock();
            if (m_useSpacesForTab) {
                cursor.insertText(QString(m_tabWidth, ' '));
            } else {
                cursor.insertText("\t");
            }
            cursor.endEditBlock();
            return;
        }

        // 反缩进：删除光标前一个缩进单元
        const QTextBlock block = cursor.block();
        const QString text = block.text();

        if (text.isEmpty()) {
            return;   // 空行无可反缩进
        }

        int removeCount = 0;
        if (text.at(0) == '\t') {
            removeCount = 1;
        } else {
            // 最多删 tabWidth 个行首空格
            int i = 0;
            while (i < text.length()
                   && i < m_tabWidth
                   && text.at(i) == ' ') {
                ++i;
            }
            removeCount = i;
        }

        if (removeCount == 0) {
            return;   // 行首没有缩进
        }

        // 记住光标相对"被删区间之后"的偏移，删除后恢复
        const int oldPos = cursor.position();
        const int blockStart = block.position();
        const int removeEnd = blockStart + removeCount;   // 被删区间的结束（绝对位置）

        QTextCursor blockCursor(block);
        blockCursor.movePosition(QTextCursor::StartOfBlock);
        blockCursor.movePosition(QTextCursor::Right,
                                 QTextCursor::KeepAnchor,
                                 removeCount);

        blockCursor.beginEditBlock();
        blockCursor.removeSelectedText();
        blockCursor.endEditBlock();

        // ---- 恢复光标 ----
        // 若光标原本在被删区间之内（例如光标在行首/缩进中间），
        // 则落到删除后的行首；否则向左平移 removeCount 保持相对位置。
        QTextCursor newCursor = textCursor();
        if (oldPos <= removeEnd) {
            newCursor.setPosition(blockStart);
        } else {
            newCursor.setPosition(oldPos - removeCount);
        }
        setTextCursor(newCursor);

        return;
    }

    // ------------------------------------------------------------------------
    // B. 有选区：处理所有完整行
    // ------------------------------------------------------------------------
    const int selStart = cursor.selectionStart();
    const int selEnd   = cursor.selectionEnd();

    // 计算选区覆盖的行号范围
    QTextCursor tmp(document());
    tmp.setPosition(selStart);
    const int startBlock = tmp.blockNumber();

    tmp.setPosition(selEnd);
    int endBlock = tmp.blockNumber();
    // 选区末尾恰好停在行首 → 不算这一行（避免多缩进一行）
    if (tmp.positionInBlock() == 0 && endBlock > startBlock) {
        --endBlock;
    }

    cursor.beginEditBlock();

    // 从后往前处理，避免前面的插入/删除影响后面的行号
    for (int blockNum = endBlock; blockNum >= startBlock; --blockNum) {
        QTextBlock block = document()->findBlockByNumber(blockNum);
        if (!block.isValid()) continue;

        QTextCursor blockCursor(block);
        blockCursor.movePosition(QTextCursor::StartOfBlock);

        if (increase) {
            // ---- 缩进：行首插入一个缩进单元 ----
            if (m_useSpacesForTab) {
                blockCursor.insertText(QString(m_tabWidth, ' '));
            } else {
                blockCursor.insertText("\t");
            }
        } else {
            // ---- 反缩进：删除行首一个缩进单元 ----
            const QString text = block.text();
            if (text.isEmpty()) continue;   // 空行跳过

            int removeCount = 0;
            if (text.at(0) == '\t') {
                removeCount = 1;
            } else {
                // 最多删 tabWidth 个前导空格
                int i = 0;
                while (i < text.length()
                       && i < m_tabWidth
                       && text.at(i) == ' ') {
                    ++i;
                }
                removeCount = i;
            }

            if (removeCount > 0) {
                blockCursor.movePosition(QTextCursor::Right,
                                         QTextCursor::KeepAnchor,
                                         removeCount);
                blockCursor.removeSelectedText();
            }
        }
    }

    cursor.endEditBlock();
}

}
