#include "text_editor.h"
// #include "syntax_highlighter.h"
#include "language/language_highlighter.h"
#include "font_manager.h"
#include "theme_manager.h"
#include "language/editor_rules.h"
#include "editor/completion_delegate.h"
#include "project_manager.h"        // 读 venvPath()
#include "application.h"            // 读 Sdk/Environment
#include "config_manager.h"
#include "python/python_project_model.h"  // v8.5

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
#include <QStringConverter>
#include <limits>
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
#include <QtCore/qelapsedtimer.h>


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

            // ========== 3. 行号 ==========
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
                const bool hovered = (lineNumber == m_hoveredLine);
                const int iconSize = hovered ? 14 : 12;
                const int iconX = width() - 30 - (hovered ? 1 : 0);
                const int iconY = static_cast<int>(top + (bottom - top) / 2 - iconSize / 2);

                QColor errColor(255, 50, 50);
                if (!hovered) errColor.setAlpha(170);   // 悬停实心、否则半透明
                painter.setBrush(errColor);
                painter.setPen(Qt::NoPen);
                painter.drawEllipse(iconX, iconY, iconSize, iconSize);

                painter.setPen(QPen(Qt::white, 2));
                const int pad = hovered ? 4 : 3;
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

    //   圆点区域：x ∈ [width-30-2, width-30+14]
    const int errIconLeft = width() - 32;
    const int errIconRight = width() - 16;
    if (event->button() == Qt::LeftButton
        && x >= errIconLeft && x <= errIconRight
        && (m_editor->hasErrorMarker(line) || m_editor->hasWarningMarker(line))) {

        emit m_editor->quickFixTriggeredByClick(line, event->globalPosition().toPoint());
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

    // 4. 落在行号数字区 转发给编辑器（选中行、拖选等）
    m_editor->mousePressEvent(event);
}

void LineNumberArea::mouseMoveEvent(QMouseEvent* e)
{
    const int line = m_editor->cursorForPosition(e->pos()).blockNumber() + 1;
    if (line != m_hoveredLine) {
        m_hoveredLine = line;
        m_editor->setHoveredErrorLine(line);   // ← 通知编辑器同步（用于波浪线加粗）
        update();
    }
    QWidget::mouseMoveEvent(e);
}

void LineNumberArea::leaveEvent(QEvent* e)
{
    if (m_hoveredLine != -1) {
        m_hoveredLine = -1;
        m_editor->setHoveredErrorLine(-1);
        update();
    }
    QWidget::leaveEvent(e);
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

    connect(this, &TextEditor::quickFixTriggeredByClick,
            this, [this](int line, const QPoint& pos) {
                showQuickFixMenuForLine(line, pos);
            });
}

TextEditor::~TextEditor()
{
    // ============================================================
    //  修复析构期 UAF
    //
    //  问题：
    //    m_highlighter 的析构会触发 QSyntaxHighlighter::setDocument(nullptr)
    //    QTextDocumentPrivate::finishEdit()
    //    QPlainTextDocumentLayout::documentChanged
    //    QWidgetTextControl::updateRequest
    //    emit TextEditor::updateRequest(rect, dy)
    //    回调到 TextEditor::updateLineNumberArea
    //    访问 m_lineNumberArea->width()
    //    如果此时 m_lineNumberArea 已被 delete 崩
    //
    //  修复：
    //    1. 先断开 document 与自身的所有信号连接（避免回调）
    //    2. 按"后创建的先删"的顺序删除成员
    //       m_highlighter 依赖 document 先删
    //       m_lineNumberArea 被其他析构回调访问 后删
    //    3. m_contextMenu / m_errorMarkerTimer 无依赖 任意顺序
    // ============================================================

    // 1. 断开 document 的所有信号（防止任何回调）
    if (document()) {
        disconnect(document(), nullptr, nullptr, nullptr);
    }
    // 2. 断开自身的所有信号（避免析构期间 emit 到其他接收者）
    disconnect(this, nullptr, nullptr, nullptr);

    // 3. 按依赖顺序删除
    delete m_highlighter;        // 先删 其析构会触发 document 更新，但已断信号
    m_highlighter = nullptr;

    delete m_lineNumberArea;     // 后删 此时已无回调
    m_lineNumberArea = nullptr;

    delete m_contextMenu;
    m_contextMenu = nullptr;

    delete m_errorMarkerTimer;
    m_errorMarkerTimer = nullptr;
}

void TextEditor::initEditor()
{
    setObjectName("TextEditor");
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

    // ============================================================
    //  面包屑 + 大纲控制器
    // ============================================================
    m_breadcrumbBar = new BreadcrumbBar(this);
    m_breadcrumbBar->setVisible(m_showBreadcrumb);

    m_outlineController = new OutlineController(this, this);
    connect(m_outlineController, &OutlineController::symbolsChanged,
        this, [this]() {
            if (m_breadcrumbBar) {
                m_breadcrumbBar->setSymbols(m_outlineController->symbols());
            }
        });

    // 光标移动 更新面包屑
    connect(this, &TextEditor::cursorPositionChanged, this,
        [this](int line, int /*column*/) {
            if (m_breadcrumbBar && m_showBreadcrumb) {
                m_breadcrumbBar->updateCurrentLine(line);
            }
        });

    // 面包屑点击 跳转
    connect(m_breadcrumbBar, &BreadcrumbBar::symbolJumpRequested,
        this, [this](int line) {
            gotoLine(line);
            setFocus();
        });

    // ============================================================
    //  大纲浮层：Ctrl/Cmd + ` 呼出
    // ============================================================
    m_outlineOverlay = new OutlineOverlay(this);
    connect(m_outlineOverlay, &OutlineOverlay::symbolActivated,
        this, [this](int line) {
            gotoLine(line);
            setFocus();
        });

    // 上下文菜单
    createContextMenu();

    // 信号连接
    connect(this, &TextEditor::blockCountChanged,
            this, &TextEditor::updateLineNumberAreaWidth);
    connect(this, &TextEditor::updateRequest,
            this, &TextEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &TextEditor::onCursorPositionChanged);
    connect(this, &TextEditor::textChanged,
            this, &TextEditor::onTextChanged);

    updateLineNumberAreaWidth();
    applyDefaultPalette();
    setupCompletion();

    // ---- LSP didChange 防抖：停止输入 180ms 后才发 ----
    m_lspChangeTimer = new QTimer(this);
    m_lspChangeTimer->setSingleShot(true);
    m_lspChangeTimer->setInterval(500);
    connect(m_lspChangeTimer, &QTimer::timeout, this, [this]() {
        if (!m_pendingLspChange) return;
        m_pendingLspChange = false;
        if (m_filePath.isEmpty()) return;

        const QString path = m_filePath;
        QTimer::singleShot(0, this, [this, path]() {
            if (!isVisible()) return;
            emit documentChangedForLsp(path, toPlainText());
        });
    });

    // 滚动条错误标记
    if (verticalScrollBar()) {
        verticalScrollBar()->installEventFilter(this);
    }
    // 滚动条显隐时更新查找栏宽度
    if (verticalScrollBar()) {
        connect(verticalScrollBar(), &QScrollBar::rangeChanged,
                this, [this](int /*min*/, int /*max*/) {
                    if (m_findBar && m_findBarVisible) {
                        const int sbWidth = verticalScrollBar()->width();
                        const int barWidth = width() - sbWidth;
                        m_findBar->setGeometry(0, height() - 40, barWidth, 40);
                    }
                });
    }

    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setSingleShot(true);
    m_autoSaveTimer->setInterval(15000);   // JetBrains 默认 15 秒
    connect(m_autoSaveTimer, &QTimer::timeout, this, [this]() {
        autoSave();
    });
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
    // QString styleSheet = QString(
                             // "QPlainTextEdit {"
                             // "  font-family: '%1', monospace;"
                             // "  font-size: %2pt;"
                             // "  line-height: 1.2;"
                             // "  font-weight: normal;"
                             // "}"
                             // "QPlainTextEdit QTextBlock {"
                             // "  vertical-align: middle;"
                             // "}"
                             // ).arg(font().family()).arg(font().pointSize());

    // setStyleSheet(styleSheet);

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

    // ========== Quick Fix ==========
    //   语义：光标所在行有 LSP 诊断（错误/警告）时可用，
    //         点击弹出 codeAction 列表（与 Alt+Enter / 点击红点等价）。
    //   动态刷新 enabled 在 contextMenuEvent() 里做。
    m_quickFixAction = m_contextMenu->addAction(
        tr("显示修复 (Alt+Enter)"),
        this, [this]() {
            const int line = currentLine();
            if (line < 1) return;
            const QPoint globalPos = viewport()->mapToGlobal(
                cursorRect().bottomLeft());
            showQuickFixMenuForLine(line, globalPos);
        });
    m_quickFixAction->setEnabled(false);   // 初始禁用，弹出前刷新
    // ========== AI 辅助子菜单 ==========
    m_contextMenu->addSeparator();

    m_aiMenu          = m_contextMenu->addMenu(tr("AI agent"));
    m_aiExplainAction = m_aiMenu->addAction(tr("解释选中代码"));
    m_aiFixAction     = m_aiMenu->addAction(tr("修复选中代码"));
    m_aiCommentAction = m_aiMenu->addAction(tr("生成注释"));
    m_aiRenameAction  = m_aiMenu->addAction(tr("重命名建议"));
    m_aiMenu->addSeparator();
    m_aiAskAction     = m_aiMenu->addAction(tr("自定义提问..."));

    connect(m_aiExplainAction, &QAction::triggered, this,
            [this]() { triggerAIAssist(AIAssistKind::ExplainCode); });
    connect(m_aiFixAction,     &QAction::triggered, this,
            [this]() { triggerAIAssist(AIAssistKind::FixCode); });
    connect(m_aiCommentAction, &QAction::triggered, this,
            [this]() { triggerAIAssist(AIAssistKind::GenerateComment); });
    connect(m_aiRenameAction,  &QAction::triggered, this,
            [this]() { triggerAIAssist(AIAssistKind::RenameSuggest); });
    connect(m_aiAskAction,     &QAction::triggered, this,
            [this]() { triggerAIAssist(AIAssistKind::CustomAsk); });

    // 初始未连接 禁用
    m_aiMenu->setEnabled(false);
    m_aiMenu->setToolTip(tr("AI Agent 未连接"));
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

    // v8.5：setPlainText 之前先把 viewport margin 设好，
    //   避免首帧 paint 时 top margin 还是 0（与面包屑重叠）。
    //   updateLineNumberAreaWidth 现在用常量 24，不依赖 widget 的 isVisible/height。
    updateLineNumberAreaWidth();

    setPlainText(content);
    m_filePath = filePath;

    if (m_breadcrumbBar) {
        m_breadcrumbBar->setFileDisplayName(QFileInfo(filePath).fileName());
    }
    if (m_outlineController) {
        m_outlineController->refreshNow();
    }
    applyLineHeight();

    // 4. 应用语法高亮
    setLanguageByFilePath(filePath);

    // 5. 计算折叠
    m_foldedLines.clear();

    // 结束静默期，重置为"干净"状态
    endSilentModification();
    m_isModified = false;
    document()->setModified(false);

    // 记录加载时的磁盘 mtime，用于后续冲突检测
    m_lastSaveTime = QFileInfo(filePath).lastModified();
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

bool TextEditor::saveFile(const QString &source)
{
    const QFileInfo fi(m_filePath);
    if (fi.exists() && m_lastSaveTime.isValid()
        && fi.lastModified() > m_lastSaveTime) {

        const auto reply = QMessageBox::warning(this,
            tr("文件冲突"),
            tr("文件") + QFileInfo(m_filePath).fileName() + tr("已被外部修改。")
            + "\n\n"
            + tr("「覆盖」= 用编辑器内容覆盖磁盘\n") + "\n\n"
            + tr("「重新加载」= 丢弃编辑器的修改，从磁盘读取"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        // Save = 覆盖，Discard = 重新加载，Cancel = 放弃

        if (reply == QMessageBox::Cancel) return false;
        if (reply == QMessageBox::Discard) {
            reloadFile();
            return true;
        }
        // reply == Save 继续往下走（覆盖磁盘）
    }
    if (m_saving) {
        qDebug() << "[TextEditor] saveFile 重入，已忽略";
        return false;
    }

    m_saving = true;

    if (m_filePath.isEmpty()) {
        QString filePath = QFileDialog::getSaveFileName(
            this, tr("保存文件"), "", tr("C++文件 (*.cpp *.h *.hpp);;所有文件 (*.*)"));

        if (filePath.isEmpty()) {
            m_saving = false;
            return false;
        }

        // 手动保存前的冲突检测
        // 若磁盘在 IDE 上次保存之后被外部修改 弹对话框
        {
            QFileInfo fi(m_filePath);
            if (fi.exists() && m_lastSaveTime.isValid()
                && fi.lastModified() > m_lastSaveTime) {

                const auto reply = QMessageBox::warning(this,
                    tr("文件冲突"),
                    tr("文件") + QFileInfo(m_filePath).fileName() + tr("已被外部修改。") + "\n\n"
                    + tr("「保存」= 用编辑器内容覆盖磁盘") + "\n"
                    + tr("「放弃」= 丢弃编辑器内容，从磁盘重新加载") + "\n"
                    + tr("「取消」= 中止本次保存"),
                    QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

                if (reply == QMessageBox::Cancel) {
                    return false;
                }
                if (reply == QMessageBox::Discard) {
                    reloadFile();
                    return true;
                }
                // 走到这里 = 用户选了「保存」继续往下执行写盘
            }
        }

        m_filePath = filePath;
    }

    QFile file(m_filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("文件保存失败"),
                             QString(tr("无法保存文件：%1\n错误：%2"))
                                 .arg(m_filePath).arg(file.errorString()));
        m_saving = false;
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

    // 保存前去除每行行尾空白（trim trailing whitespace）
    //   避免"连续回车遗留的缩进空白"被写入文件
    {
        QTextCursor cur(document());
        cur.beginEditBlock();
        QTextBlock block = document()->firstBlock();
        while (block.isValid()) {
            QString text = block.text();
            int end = text.length();
            while (end > 0 && (text[end-1] == ' ' || text[end-1] == '\t')) --end;
            if (end < text.length()) {
                QTextCursor c(block);
                c.movePosition(QTextCursor::EndOfBlock);
                c.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, text.length() - end);
                c.removeSelectedText();
            }
            block = block.next();
        }
        cur.endEditBlock();
    }

    stream << toPlainText();
    file.close();

    m_isModified = false;
    document()->setModified(false);

    m_lastSaveTime = QDateTime::currentDateTime();

    // 保存成功后，记录本地历史
    if (m_localHistory && !m_filePath.isEmpty()) {
        int version = m_localHistory->saveVersion(
            m_filePath,toPlainText(),
            source == "ai" ? "from AI" : "from user",
            source
            );
        if (version > 0) {
            qDebug() << "[TextEditor] 本地历史已保存，版本:" << version;
        }
    }

    emit fileSaved(m_filePath);
    emit modificationChanged(false);

    m_saving = false;
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

    // 扩展名变了 语言可能变（例如 .txt 改成 .py）
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

    if (cursor.hasSelection()) {
        // ============================================================
        //  v8.5：有选区时复制选区覆盖的所有完整行，插到选区下方
        //
        //  边界规则（与 deleteLine 一致）：
        //    - 起点行：从行首开始复制（即使选区从行中间开始）
        //    - 终点行：如果选区终点正好在行首，不算这一行；否则复制到行尾
        //    - 复制块插到终点行行尾之后（跳过行尾换行）
        //
        //  光标/选区：复制完成后选中新块，方便用户连续 Ctrl+D 继续复制
        // ============================================================
        const int selStart = cursor.selectionStart();
        const int selEnd   = cursor.selectionEnd();

        QTextCursor tmp(document());

        // 起点行的行首
        tmp.setPosition(selStart);
        tmp.movePosition(QTextCursor::StartOfBlock);
        const int blockStart = tmp.position();

        // 终点行的行尾
        tmp.setPosition(selEnd);
        if (tmp.positionInBlock() == 0 && selEnd > blockStart) {
            tmp.movePosition(QTextCursor::Left);
        }
        tmp.movePosition(QTextCursor::EndOfBlock);
        const int blockEnd = tmp.position();

        // 取出要复制的文本（多行；行间分隔符为 ParagraphSeparator，insertText 会识别）
        QTextCursor copyCursor(document());
        copyCursor.setPosition(blockStart);
        copyCursor.setPosition(blockEnd, QTextCursor::KeepAnchor);
        const QString blockText = copyCursor.selectedText();

        // 在 blockEnd 之后插入 \n + blockText
        QTextCursor insertCursor(document());
        insertCursor.setPosition(blockEnd);
        insertCursor.insertText(QStringLiteral("\n") + blockText);

        cursor.endEditBlock();

        // 选中新块（blockEnd + 1 是跳过 \n 后的首字符）
        QTextCursor nc(document());
        nc.setPosition(blockEnd + 1);
        nc.setPosition(blockEnd + 1 + blockText.length(),
            QTextCursor::KeepAnchor);
        setTextCursor(nc);
    } else {
        // 无选区：
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        const QString lineText = cursor.selectedText();
        cursor.setPosition(cursor.selectionEnd());
        cursor.insertText(QStringLiteral("\n") + lineText);

        cursor.endEditBlock();

        QTextCursor nc(document());
        nc.setPosition(cursor.position() - lineText.length());
        nc.movePosition(QTextCursor::StartOfBlock);
        nc.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, col);
        setTextCursor(nc);
    }
}

void TextEditor::deleteLine()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    if (cursor.hasSelection()) {
        // ============================================================
        //  v8.5：有选区时删除选区覆盖的所有完整行
        //
        //  边界规则（与 JetBrains 一致）：
        //    - 起点行：从行首开始删（即使选区从行中间开始）
        //    - 终点行：如果选区终点正好在行首，不算这一行；否则从行首删到行尾
        //    - 删完后连带行尾换行符一起删，避免残留空行
        // ============================================================
        const int selStart = cursor.selectionStart();
        const int selEnd   = cursor.selectionEnd();

        QTextCursor tmp(document());

        // 起点行的行首
        tmp.setPosition(selStart);
        tmp.movePosition(QTextCursor::StartOfBlock);
        const int blockStart = tmp.position();

        // 终点行的行尾
        tmp.setPosition(selEnd);
        // 若选区终点恰好落在行首（跨行选择时常见），不把这一行算进去
        if (tmp.positionInBlock() == 0 && selEnd > blockStart) {
            tmp.movePosition(QTextCursor::Left);   // 退到上一行行尾
        }
        tmp.movePosition(QTextCursor::EndOfBlock);
        int blockEnd = tmp.position();

        // 吃掉尾部换行（除非已经到文档末尾）
        if (blockEnd < document()->characterCount() - 1) {
            ++blockEnd;
        }

        cursor.setPosition(blockStart);
        cursor.setPosition(blockEnd, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
    } else {
        // 无选区：删除光标所在的单行
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
    }

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

QList<ErrorMarker> TextEditor::getWarningMarkers() const
{
    return m_warningMarkers;
}

void TextEditor::setHoveredErrorLine(int line)
{
    if (m_hoveredErrorLine == line) return;
    m_hoveredErrorLine = line;
    viewport()->update();
    if (m_lineNumberArea) m_lineNumberArea->update();
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

    //   若当前有单行选区，自动带入查找输入框
    //   - QTextCursor::selectedText() 把段落分隔符转成 U+2029，需要归一化
    //   - 多行选区在"查找"里没有语义，跳过
    {
        const QTextCursor cur = textCursor();
        if (cur.hasSelection()) {
            QString sel = cur.selectedText();
            sel.replace(QChar::ParagraphSeparator, '\n');
            if (!sel.contains('\n') && !sel.trimmed().isEmpty()) {
                m_findLineEdit->setText(sel);   // setText 会触发 onFindTextChanged 自动高亮
            }
        }
    }

    m_findBarVisible = true;
    m_findBar->setVisible(true);

    // v8.5：交给 relayout() 统一处理：
    //   - 搜索栏几何
    //   - viewport bottom margin（让内容不被遮挡）
    //   - 行号区高度同步调整
    updateLineNumberAreaWidth();
    relayout();

    m_findLineEdit->setFocus();
    m_findLineEdit->selectAll();

    // 若查找框已有内容但当前没有高亮 重新执行一次搜索
    //   场景：关闭查找栏（Esc）clearHighlights 清空了 m_matchCursors，
    //   但查找框文本仍保留；重新 Ctrl+F 时需恢复高亮。
    //
    //   若 m_matchCursors 非空 说明高亮还在（比如只是焦点切走又切回），
    //   不重复触发。
    if (m_findLineEdit && !m_findLineEdit->text().isEmpty()
        && m_matchCursors.isEmpty()) {
        onFindTextChanged(m_findLineEdit->text());
    }

    emit findBarVisibilityChanged(true);
}

void TextEditor::showFindBarWithReplace()
{
    //  与 showFindBar 相同的带入逻辑
    if (m_findBar) {
        const QTextCursor cur = textCursor();
        if (cur.hasSelection()) {
            QString sel = cur.selectedText();
            sel.replace(QChar::ParagraphSeparator, '\n');
            if (!sel.contains('\n') && !sel.trimmed().isEmpty()) {
                m_findLineEdit->setText(sel);
            }
        }
    }

    showFindBar();

    // onToggleReplaceMode() 是"切换"不是"打开"，若替换行已经展开会被收起
    // 所以这里直接展开，避免 Ctrl+H 连按两次的表现不一致
    if (!m_replaceMode) {
        onToggleReplaceMode();
    }
}

void TextEditor::hideFindBar()
{
    if (!m_findBar) return;
    m_findBarVisible = false;
    m_findBar->setVisible(false);
    clearHighlights();

    // v8.5：去掉 viewport bottom margin，恢复内容区域
    updateLineNumberAreaWidth();
    relayout();

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
//  用户入口：Alt+Enter / 点击红点
//
//  职责：
//    - 有缓存 直接弹菜单（调 showCodeActionMenu）
//    - 无缓存 只发 LSP 请求，不弹菜单
//              响应到达后由 onCodeActionReceived 弹菜单
//
//  ⚠️ 关键：本函数在"无缓存"分支中**不能**自己弹菜单，
//     否则会和 onCodeActionReceived 互相触发，形成无限循环。
// ============================================================
void TextEditor::showQuickFixMenuForLine(int line, const QPoint& globalPos)
{
    if (line < 1 || line > lineCount()) return;

    // ---- 情况 1：有缓存 直接弹菜单，不再请求 ----
    const QList<LSPCodeAction> cached = m_codeActionsByLine.value(line);
    if (!cached.isEmpty()) {
        showCodeActionMenu(line, globalPos, cached);
        return;
    }

    // ---- 情况 2：无缓存 重置重试状态 + 发请求 ----
    m_codeActionRetryCount     = 0;
    m_codeActionRetryLine      = line;
    m_codeActionRetryGlobalPos = globalPos;
    m_codeActionPendingLine    = line;

    // 显示"正在查询"提示（3 秒超时自动消失）
    //   重试成功的路径会在 onCodeActionReceived 里显式 hideText
    QToolTip::showText(globalPos, tr("正在查询修复..."), this, QRect(), 3000);

    const QTextBlock block = document()->findBlockByNumber(line - 1);
    const int lineLen = block.isValid() ? block.length() - 1 : 0;

    emit quickFixRequested(m_filePath,
                           line - 1, 0,
                           line - 1, lineLen);
}

// ============================================================
//  私有辅助：只弹菜单，不发请求，不改缓存
//  调用者：showQuickFixMenuForLine（有缓存）、onCodeActionReceived（响应后）
// ============================================================
void TextEditor::showCodeActionMenu(int line,
                                    const QPoint& globalPos,
                                    const QList<LSPCodeAction>& actions)
{
    Q_UNUSED(line);

    QMenu menu(this);

    if (actions.isEmpty()) {
        // 走到这里 = 已重试 2 次仍空
        QAction* a = menu.addAction(tr("未找到可用修复"));
        a->setEnabled(false);
        menu.addSeparator();
        // B 方案：给用户一个"为什么"，避免误以为 IDE 坏了
        QAction* b = menu.addAction(tr("clangd 可能仍在索引，请稍后重试"));
        b->setEnabled(false);
    } else {
        for (const LSPCodeAction& act : actions) {
            QAction* a = menu.addAction(act.title);
            if (act.isPreferred) {
                QFont f = a->font();
                f.setBold(true);
                a->setFont(f);
            }
            connect(a, &QAction::triggered, this, [this, act]() {
                applyCodeAction(act);
            });
        }
    }

    menu.exec(globalPos);
}

// ============================================================
//  LSP codeAction 响应到达
//
//  三种情况：
//    1. 非空 缓存 + 弹菜单（正常成功路径）
//    2. 空 + 未超重试上限 主动触发 didChange 催 clangd，延迟重试
//    3. 空 + 超重试上限 弹"无可用修复"菜单
//
//   约束：本函数不调用 showQuickFixMenuForLine，避免递归
// ============================================================
void TextEditor::onCodeActionReceived(const QString& filePath,
                                      const QList<LSPCodeAction>& actions)
{
    if (filePath != m_filePath) return;

    const int line = m_codeActionPendingLine;
    m_codeActionPendingLine = -1;
    if (line < 0) return;

    // ---------- 情况 1：非空 缓存 + 弹菜单 ----------
    if (!actions.isEmpty()) {
        m_codeActionsByLine.insert(line, actions);
        m_codeActionRetryCount = 0;
        QToolTip::hideText();
        showCodeActionMenu(line, m_codeActionRetryGlobalPos, actions);
        return;
    }

        // ---------- 情况 2：空 + 还能重试 ----------
    constexpr int kMaxRetries = 2;
    if (m_codeActionRetryCount < kMaxRetries) {
        ++m_codeActionRetryCount;

        const int    savedLine = m_codeActionRetryLine;
        const QPoint savedPos  = m_codeActionRetryGlobalPos;

        //   让 clangd 认为"内容真的变了"
        //   clangd 对"内容相同"的 didChange 会跳过分析 诊断不更新 还是空
        //   所以发两次 didChange：
        //     第一次：末尾加一个空格
        //     第二次：250ms 后恢复原样
        //   两次间隔必须 > LSPClient::changeDocument 的 200ms 防抖窗口，
        //   否则第二次的 pendingContent 会覆盖第一次，只发一次
        if (!m_filePath.isEmpty()) {
            emit documentChangedForLsp(m_filePath,
                                       toPlainText() + QStringLiteral(" "));
        }

        QTimer::singleShot(250, this, [this, savedLine, savedPos]() {
            if (!isVisible() || currentLine() != savedLine) {
                m_codeActionRetryCount = 0;
                QToolTip::hideText();
                return;
            }

            if (!m_filePath.isEmpty()) {
                emit documentChangedForLsp(m_filePath, toPlainText());
            }

            // 再等 300ms 让 clangd 完成分析 + 诊断回传到 IDE
            QTimer::singleShot(300, this, [this, savedLine, savedPos]() {
                if (!isVisible() || currentLine() != savedLine) {
                    m_codeActionRetryCount = 0;
                    QToolTip::hideText();
                    return;
                }

                m_codeActionPendingLine = savedLine;

                const QTextBlock block =
                    document()->findBlockByNumber(savedLine - 1);
                const int lineLen = block.isValid() ? block.length() - 1 : 0;

                emit quickFixRequested(m_filePath,
                                       savedLine - 1, 0,
                                       savedLine - 1, lineLen);
                Q_UNUSED(savedPos);
            });
        });
        return;
    }

    // ---------- 情况 3：重试耗尽 弹"无可用修复" ----------
    m_codeActionRetryCount = 0;
    QToolTip::hideText();
    showCodeActionMenu(line, m_codeActionRetryGlobalPos, actions);
}

bool TextEditor::applyCodeAction(const LSPCodeAction& action)
{
    QTextCursor cursor(document());

    cursor.beginEditBlock();
    for (const LSPWorkspaceEdit& wsEdit : action.edits) {
        if (wsEdit.filePath != m_filePath) continue;   // 跨文件跳过

        // ============================================================
        //  先把"插入 #include"的 edit 重定位到 include 区域
        //
        //  clangd 返回的插入位置紧贴使用点（比如 printf 上方），
        //  对用户不友好。这里检测出 include 插入，把位置改到文件顶部的
        //  include 区域末尾。
        //
        //  判定条件：
        //    - 纯插入（start == end，没有删除内容）
        //    - newText 去除首尾空白后以 "#include" 开头
        // ============================================================
        QList<LSPTextEdit> adjusted;
        adjusted.reserve(wsEdit.edits.size());
        for (const LSPTextEdit& e : wsEdit.edits) {
            LSPTextEdit te = e;

            const bool isPureInsert =
                (te.startLine  == te.endLine &&
                 te.startColumn == te.endColumn);
            const QString trimmedNew = te.newText.trimmed();
            const bool isIncludeInsert =
                isPureInsert && trimmedNew.startsWith(QLatin1String("#include"));

            if (isIncludeInsert) {
                const int targetLine = findIncludeInsertLine();
                te.startLine   = targetLine;
                te.startColumn = 0;
                te.endLine     = targetLine;
                te.endColumn   = 0;
            }
            adjusted.append(te);
        }

        // 从后往前应用，避免位置漂移
        std::sort(adjusted.begin(), adjusted.end(),
                  [](const LSPTextEdit& a, const LSPTextEdit& b) {
                      if (a.startLine != b.startLine)
                          return a.startLine > b.startLine;
                      return a.startColumn > b.startColumn;
                  });

        for (const LSPTextEdit& e : adjusted) {
            const int sPos =
                document()->findBlockByNumber(e.startLine).position() + e.startColumn;
            const int ePos =
                document()->findBlockByNumber(e.endLine).position() + e.endColumn;

            cursor.setPosition(sPos);
            cursor.setPosition(ePos, QTextCursor::KeepAnchor);
            cursor.insertText(e.newText);
        }
    }
    cursor.endEditBlock();

    m_codeActionsByLine.clear();

    return true;
}

QList<LSPCodeAction> TextEditor::codeActionsForLine(int line) const
{
    return m_codeActionsByLine.value(line);
}

// ============================================================================
//  找到 include 区域的插入位置
// ============================================================================
int TextEditor::findIncludeInsertLine() const
{
    QTextDocument* doc = document();
    if (!doc) return 0;

    const int total = doc->blockCount();
    if (total == 0) return 0;

    int lastIncludeLine = -1;    // 最后一个 #include 的行号（0-based）
    bool inBlockComment = false;

    for (int i = 0; i < total; ++i) {
        const QString text    = doc->findBlockByNumber(i).text();
        const QString trimmed = text.trimmed();

        // ---------- 块注释内部 ----------
        if (inBlockComment) {
            if (trimmed.contains("*/")) inBlockComment = false;
            continue;
        }
        if (trimmed.startsWith("/*")) {
            if (!trimmed.contains("*/")) inBlockComment = true;
            continue;
        }

        // ---------- 空行 / 行注释 ----------
        if (trimmed.isEmpty())      continue;
        if (trimmed.startsWith("//")) continue;

        // ---------- 预处理指令（#include / #pragma / #define ...）----------
        if (trimmed.startsWith('#')) {
            if (trimmed.startsWith("#include")) {
                lastIncludeLine = i;
            }
            // 其他 # 指令也属于文件顶部区域，继续往下
            continue;
        }

        // ---------- 遇到第一行真正的代码 停 ----------
        break;
    }

    if (lastIncludeLine >= 0) {
        return lastIncludeLine + 1;   // 插到最后一个 include 之后
    }
    return 0;                          // 没有任何 include 插到文件头
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

    // 记录"搜索基准点" —— 用户按 Ctrl+F 时的光标位置
    // 注意：要在 highlightAllMatches 之前抓，因为之后 m_matchCursors
    //      的 setExtraSelections 不改文档、不移动光标，但这行更保险。
    const int basePos = textCursor().selectionStart();

    highlightAllMatches(text,
                        m_caseSensitiveCheck->isChecked(),
                        m_wholeWordCheck->isChecked());

    if (m_matchCursors.isEmpty()) {
        m_matchCountLabel->setText("0/0");
        return;
    }

    // ✅ v8.4：就近选择当前匹配
    //   - 优先取第一个 selectionStart() >= basePos 的匹配
    //   - 若没有（光标位于最后一个匹配之后），回绕到第 0 个
    int best = 0;
    for (int i = 0; i < m_matchCursors.size(); ++i) {
        if (m_matchCursors[i].selectionStart() >= basePos) {
            best = i;
            break;
        }
        // 循环跑完还没命中 保持 best = 0（回绕）
        best = 0;
    }

    m_currentMatchIndex = best;
    scrollToMatch(m_currentMatchIndex);
    m_matchCountLabel->setText(
        QString("%1/%2").arg(m_currentMatchIndex + 1).arg(m_matchCursors.size()));
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
    if (wholeWord)     flags |= QTextDocument::FindWholeWords;

    QTextCursor cursor(document());
    cursor.movePosition(QTextCursor::Start);

    while (!cursor.isNull()) {
        QTextCursor found = document()->find(text, cursor, flags);
        if (found.isNull()) break;

        // 关键修复：光标保持原位置，插入内容不扩展选区
        found.setKeepPositionOnInsert(true);
        m_matchCursors.append(found);

        cursor = found;
        cursor.setKeepPositionOnInsert(false);  // 搜索游标要正常向后推进
    }

    // 从主题取色
    ThemeManager& tm = ThemeManager::instance();
    const QColor normalColor = tm.color(ThemeManager::UIElement::SearchMatchBackground);
    const QColor currentColor = tm.color(ThemeManager::UIElement::SearchCurrentMatchBackground);

    QList<QTextEdit::ExtraSelection> selections;
    for (int i = 0; i < m_matchCursors.size(); ++i) {
        QTextEdit::ExtraSelection sel;
        sel.cursor = m_matchCursors[i];

        // 当前匹配用更显眼的色
        if (i == m_currentMatchIndex) {
            sel.format.setBackground(currentColor);
        } else {
            sel.format.setBackground(normalColor);
        }

        // 打上"搜索匹配"的 tag，供 clearHighlights 精准清除
        sel.format.setProperty(QTextFormat::UserProperty,
                               QStringLiteral("searchMatchHighlight"));

        selections.append(sel);
    }

    applySearchHighlights(selections);
}

void TextEditor::scrollToMatch(int index)
{
    if (index < 0 || index >= m_matchCursors.size()) return;

    setTextCursor(m_matchCursors[index]);
    centerCursor();

    ThemeManager& tm = ThemeManager::instance();
    const QColor normalColor  = tm.color(ThemeManager::UIElement::SearchMatchBackground);
    const QColor currentColor = tm.color(ThemeManager::UIElement::SearchCurrentMatchBackground);

    QList<QTextEdit::ExtraSelection> selections;
    selections.reserve(m_matchCursors.size());
    for (int i = 0; i < m_matchCursors.size(); ++i) {
        QTextEdit::ExtraSelection sel;
        sel.cursor = m_matchCursors[i];
        sel.format.setBackground(i == m_currentMatchIndex ? currentColor : normalColor);
        sel.format.setProperty(QTextFormat::UserProperty,
                                QStringLiteral("searchMatchHighlight"));
        selections.append(sel);
    }
    applySearchHighlights(selections);

}

void TextEditor::clearHighlights()
{
    m_matchCursors.clear();

    // 只移除"查找匹配"高亮，保留其他 ExtraSelection
    QList<QTextEdit::ExtraSelection> all = extraSelections();
    static const QString kSearchTag = QStringLiteral("searchMatchHighlight");
    for (int i = all.size() - 1; i >= 0; --i) {
        if (all[i].format.property(QTextFormat::UserProperty).toString() == kSearchTag) {
            all.removeAt(i);
        }
    }
    setExtraSelections(all);
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
    // 点击编辑器 关闭大纲浮层
    if (m_outlineOverlay && m_outlineOverlay->isVisible()) {
        m_outlineOverlay->hideOverlay();
    }
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
    //  Cmd/Ctrl 松开 立即撤销视觉反馈
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
    //  鼠标离开编辑器 撤销一切
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
    // 大纲浮层的关闭统一由 OutlineOverlay::focusChanged 和
    //       TextEditor::mousePressEvent 处理，这里不再关闭。
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
        const bool hovered = (marker.line == m_hoveredErrorLine);
        QPen pen(QColor(200, 160, 0), hovered ? 3 : 1);
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

    const QString text = block.text();
    const QChar target = opening ? '{' : '}';

    // 修复：跳过字符串、字符字面量、行注释、块注释里的括号
    bool inStr = false, inChar = false, inLine = false, inBlock = false, esc = false;
    for (int i = 0; i < text.length(); ++i) {
        const QChar c = text[i];
        if (inLine) break;
        if (inBlock) {
            if (c == '*' && i + 1 < text.length() && text[i + 1] == '/') {
                inBlock = false;
                ++i;
            }
            continue;
        }
        if (esc) { esc = false; continue; }
        if (c == '\\') { esc = true; continue; }
        if (c == '"' && !inChar) { inStr = !inStr; continue; }
        if (c == '\'' && !inStr) { inChar = !inChar; continue; }
        if (inStr || inChar) continue;
        if (c == '/' && i + 1 < text.length()) {
            if (text[i + 1] == '/') { inLine = true; break; }
            if (text[i + 1] == '*') { inBlock = true; ++i; continue; }
        }
        if (c == target) return i;
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

    // 同一比例、且已经应用过 直接返回
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

    // ========== 新增：缩进型语言（Python / Ruby）==========
    //   按缩进层级找方法体结束行
    const QString langId = currentLanguageId();
    const bool isIndentLang = (langId == "python" || langId == "ruby");
    if (isIndentLang) {
        const int curIndent = leadingIndentWidth(block.text());
        int endLine = line;   // 至少是起始行
        QTextBlock b = block.next();
        while (b.isValid()) {
            const QString t = b.text();
            if (t.trimmed().isEmpty()) { b = b.next(); continue; }   // 空行跳过
            const int bIndent = leadingIndentWidth(t);
            if (bIndent <= curIndent) break;
            endLine = b.blockNumber() + 1;
            b = b.next();
        }
        return endLine;   // 可能 == line 表示"无 body"，调用方会判 false
    }

    // ========== 大括号语言 ==========
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

    const QChar braceChar = isOpening ? '{' : '}';
    const QChar otherBrace = isOpening ? '}' : '{';
    const int step = isOpening ? 1 : -1;
    int pos = position + step;
    int depth = 1;

    // 修复：反向扫描（从 } 反查 {）简化处理，不做字符串/注释跟踪
    //   —— 该分支实际是死代码（isFoldableBlock/toggleFold 都传起始行，恒有 {），
    //      仅保留作为将来从 `}` 反查能力的接口
    if (!isOpening) {
        while (pos >= 0 && pos < doc->characterCount()) {
            const QChar ch = doc->characterAt(pos);
            if (ch == braceChar) {
                ++depth;
            } else if (ch == otherBrace) {
                --depth;
                if (depth == 0) {
                    QTextBlock mb = doc->findBlock(pos);
                    return mb.isValid() ? (mb.blockNumber() + 1) : -1;
                }
            }
            pos += step;
        }
        return -1;
    }

    // 修复：正向扫描时跟踪字符串/注释状态
    bool inLineComment = false;
    bool inBlockComment = false;
    QChar quoteChar = QChar::Null;
    bool escaped = false;

    while (pos >= 0 && pos < doc->characterCount()) {
        const QChar ch = doc->characterAt(pos);

        if (inLineComment) {
            if (ch == '\n') inLineComment = false;
            ++pos;
            continue;
        }
        if (inBlockComment) {
            if (ch == '*' && pos + 1 < doc->characterCount()
                && doc->characterAt(pos + 1) == '/') {
                inBlockComment = false;
                pos += 2;
                continue;
            }
            ++pos;
            continue;
        }
        if (!quoteChar.isNull()) {
            if (escaped) { escaped = false; ++pos; continue; }
            if (ch == '\\') { escaped = true; ++pos; continue; }
            if (ch == quoteChar) { quoteChar = QChar::Null; ++pos; continue; }
            ++pos;
            continue;
        }

        if (ch == '"' || ch == '\'') {
            quoteChar = ch;
            ++pos;
            continue;
        }
        if (ch == '/' && pos + 1 < doc->characterCount()) {
            const QChar nxt = doc->characterAt(pos + 1);
            if (nxt == '/') { inLineComment = true; pos += 2; continue; }
            if (nxt == '*') { inBlockComment = true; pos += 2; continue; }
        }

        if (ch == braceChar) {
            ++depth;
        } else if (ch == otherBrace) {
            --depth;
            if (depth == 0) {
                QTextBlock mb = doc->findBlock(pos);
                if (mb.isValid()) {
                    const int matchedLine = mb.blockNumber() + 1;
                    if (matchedLine <= document()->blockCount()) {
                        return matchedLine;
                    }
                }
                return -1;
            }
        }
        ++pos;
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

    const QString langId = currentLanguageId();
    const bool isIndentLang = (langId == "python" || langId == "ruby");

    // 修复：缩进型语言按 `:` 结尾判定
    if (isIndentLang) {
        const QString trimmed = block.text().trimmed();
        if (trimmed.startsWith('#')) return false;   // 纯注释行
        if (!trimmed.endsWith(':'))  return false;   // 非复合语句头
        const int matchLine = findMatchingBraceLine(line);
        return (matchLine > line);                   // 需要有 body
    }

    // 大括号语言
    const int bracePos = findBraceInBlock(block, true);
    if (bracePos == -1) {
        return false;
    }
    const int matchLine = findMatchingBraceLine(line);
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
        // 起始块：起始行的下一行（行号 line+1）
        QTextBlock block = document()->findBlockByLineNumber(line);
        if (!block.isValid()) {
            return;
        }
        // 遍历从 line+1 到 matchLine-1（不隐藏 matchLine 本身，它通常是 `}` 或 Python 的空行）
        while (block.isValid() && block.blockNumber() + 1 < matchLine) {
            block.setVisible(false);
            block = block.next();
        }
        m_foldedLines.insert(line);
    }

    // 折叠/展开后强制重排，避免后续折叠三角位置错位
    document()->markContentsDirty(0, document()->characterCount());
    document()->adjustSize();

    // 清空缩进缓存（块号在可见性变化后可能漂移）
    m_logicalIndentCache.clear();

    // 先 viewport 再行号区，最后再刷一次（Qt 会在 update() 后延迟到下一次事件循环，
    // 三次 update 合并成一次，无额外开销，但能确保 geometry 已稳定）
    viewport()->update();
    if (m_lineNumberArea) m_lineNumberArea->update();
}

/**
 * @brief 重新计算所有行的折叠状态（文件加载或内容变更后调用）
 */
void TextEditor::updateFolding()
{
    // 没有任何折叠记录 块本身全部可见，无需遍历整个文档
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
    const bool hasModifier =
    (event->modifiers() & (Qt::ControlModifier | Qt::MetaModifier));

    if (event->key() == Qt::Key_QuoteLeft
        && hasModifier
        && !event->isAutoRepeat()) {

        if (m_completionActive) closeCompletion();

        if (m_outlineOverlay && m_outlineOverlay->isVisible()) {
            // 已显示 关闭
            m_outlineOverlay->hideOverlay();
        } else if (m_outlineOverlay && m_outlineController
            && document()->characterCount() <= 500000) {
            // 未显示 打开
            m_outlineOverlay->setSymbols(m_outlineController->symbols());
            const QRect cr = cursorRect();
            const QPoint globalPos = viewport()->mapToGlobal(cr.bottomLeft());
            m_outlineOverlay->showAt(globalPos);
        }
        return;
    }

    // 浮层显示中：键盘导航转发
    if (m_outlineOverlay && m_outlineOverlay->isVisible()) {
        switch (event->key()) {
            case Qt::Key_Up:
            m_outlineOverlay->selectPrev();
            return;
            case Qt::Key_Down:
            m_outlineOverlay->selectNext();
            return;
            case Qt::Key_Left:
            m_outlineOverlay->focusLeft();
            return;
            case Qt::Key_Right:
            m_outlineOverlay->focusRight();
            return;
            case Qt::Key_Return:
            case Qt::Key_Enter:
            m_outlineOverlay->activateCurrent();
            return;
            case Qt::Key_Escape:
            m_outlineOverlay->hideOverlay();
            return;
            default:
            // 其他字符键：关浮层，正常输入
            m_outlineOverlay->hideOverlay();
            break;
        }
    }

    // ---- 0. Cmd/Ctrl 高亮 ----
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

    // Alt+Enter 当前行 Quick Fix
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
        && (event->modifiers() & Qt::AltModifier)) {
        const int line = currentLine();

        if (hasErrorMarker(line) || hasWarningMarker(line)) {
            const QPoint globalPos = viewport()->mapToGlobal(
                cursorRect().bottomLeft());
            showQuickFixMenuForLine(line, globalPos);
            return;
        }
    }

    // ---- 1. 补全弹窗导航 ----
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

    // ========== 2. Esc / F3 / Ctrl+F ==========
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
    if (handleSmartHomeSelection(event)) return;
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
        if (tryAlignDedentKeyword()) {
            // 已处理，跳过后续的补全逻辑（避免弹窗位置错误）
            return;
        }
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
//   - 字母、下划线 触发
//   - . -> ::       触发（且触发后带上下文前缀）
//   - 其它          不触发
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

    // v8.5：前缀起点位置
    const int newAnchor = pos - prefix.length();

    // ---- 判断"上下文是否变化" ----
    //   - 之前没有活跃补全 变化
    //   - anchor 变了 变化（用户切到了新的 . / -> / :: 后）
    //   - m_pendingLspItems 为空 变化（首次触发或上次 LSP 返回为空）
    //   只要不是"同一上下文内前缀变长/变短"，就重新请求 LSP；
    //   否则保留旧结果，本地过滤即可——避免"输入一个字符弹窗闪空 200ms"
    const bool anchorChanged =
    !m_completionActive || (newAnchor != m_completionAnchor);
    const bool needLspRequest =
    anchorChanged || m_pendingLspItems.isEmpty();

    m_completionAnchor = newAnchor;
    m_completionPrefix = prefix;
    m_completionActive = true;

    if (needLspRequest) {
        m_pendingLspItems.clear();
        m_lspResponseWaiting = true;
        m_lspDebounceTimer->start();
    }
    // else：同一上下文内继续输入字符 保留 m_pendingLspItems，
    //       rebuildCompletionList 会用新的 m_completionPrefix 重新过滤

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

    if (currentLanguageId() == "python") {
        QList<CompletionEntry> importEntries;
        if (tryImportCompletion(&importEntries)) {
            for (const auto& e : importEntries) {
                const QString key = makeKey(e);
                if (seen.contains(key)) continue;
                seen.insert(key);
                all.append(e);
            }
        }
    }

    // ---------- 1. 本地符号（优先级最高） ----------
    {
        // 快路径：把正则拆成"先看关键字 + 再看符号"，避免对每一行都跑正则
        static const QStringList kDefKeywords = {
            QStringLiteral("def "),      QStringLiteral("class "),
            QStringLiteral("function "), QStringLiteral("struct "),
            QStringLiteral("enum "),     QStringLiteral("interface "),
        };

        static const QRegularExpression reDef(
            R"((?:^|\s)(?:def|class|function|struct|enum|interface)\s+([A-Za-z_][A-Za-z0-9_]*))"
            R"(|(?:^|\s)([A-Za-z_][A-Za-z0-9_]*)\s*(?:=|\(|:))"
            );

        // 用 block 遍历替代 toPlainText().split('\n')：
        //   - 避免一次全文档拷贝
        //   - 避免一次全文档 split
        // 只扫到光标附近，剩下的靠 LSP 兜底
        //   —— 本地符号本来就是"当前作用域附近最有用"
        const int curLine = currentLine();
        constexpr int kScanBefore = 1000;   // 光标前最多 1000 行
        constexpr int kScanAfter  = 200;    // 光标后最多 200 行
        const int startLine = qMax(0, curLine - kScanBefore);
        const int endLine   = qMin(document()->blockCount() - 1,
                                   curLine + kScanAfter);

        QTextBlock block = document()->findBlockByNumber(startLine);
        int lineNo = startLine;
        while (block.isValid() && lineNo <= endLine) {
            const QString line = block.text().trimmed();

            if (!line.isEmpty()
                && !line.startsWith("//")
                && !line.startsWith('#')) {

                // 快路径：无关键字、且无 = ( : 的行直接跳过
                bool maybeDef = false;
                for (const QString& kw : kDefKeywords) {
                    if (line.contains(kw)) { maybeDef = true; break; }
                }
                if (!maybeDef
                    && !line.contains('=')
                    && !line.contains('(')
                    && !line.contains(':')) {
                    block = block.next();
                    ++lineNo;
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

            block = block.next();
            ++lineNo;
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
    //   规则：若某项的 insertText 与当前前缀完全相等 已经没有可补的内容，
    //        直接移除，避免"输入 pass 还提示 pass"这种无意义项。
    //
    //   为什么用 insertText 而不是 label：
    //     - 函数项 label="print" / insertText="print()" 用户输入 "print" 时，
    //       我们仍希望保留（按 Tab 能补出括号），所以不能被误删。
    //     - 纯标识符 label="pass"  / insertText="pass"    输入 "pass" 时删掉。
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
            // 已可见 只调整尺寸和位置，不 show
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
// LSP 响应到达 合并 + 刷新
// ============================================================
void TextEditor::updateCompletionItems(const QList<LSPCompletionItem>& items)
{
    m_pendingLspItems = items;
    m_lspResponseWaiting = false;

    if (!m_completionActive) {
        // 用户在等待期间关闭了补全 丢弃
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

// ============================================================================
//  归一化空白匹配
//  - 忽略：行首缩进、空行、行尾空白
//  - 保留：行内非空白字符必须逐字匹配
// ============================================================================
QList<QTextCursor> TextEditor::findByNormalizedWhitespace(const QString& searchText,
                                                          int startPos,
                                                          int endPos) const
{
    QList<QTextCursor> results;
    if (searchText.isEmpty()) return results;

    // ---- 1. 归一化 search 串：拆成"非空行 + 去首尾空白"的 token 列表 ----
    const QStringList rawSearchLines = searchText.split(QRegularExpression("[\r\n]"));
    QStringList searchTokens;
    for (const QString& line : rawSearchLines) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) searchTokens.append(trimmed);
    }
    if (searchTokens.isEmpty()) return results;

    QTextDocument* doc = document();
    if (!doc) return results;

    const int totalBlocks = doc->blockCount();

    // ---- 2. 把全文按"非空行 + 去首尾空白"预计算 ----
    //    为了方便扫描，把每个 block 的 trimmed 缓存到一个数组里
    QVector<QString> blockTrimmed;
    blockTrimmed.reserve(totalBlocks);
    for (int i = 0; i < totalBlocks; ++i) {
        blockTrimmed.append(doc->findBlockByNumber(i).text().trimmed());
    }

    // ---- 3. 扫描所有可能的起点（从 startPos/endPos 决定范围）----
    //    将 startPos/endPos 转换为 block 号
    int startBlock = 0;
    int endBlock   = totalBlocks - 1;
    if (startPos >= 0) {
        startBlock = doc->findBlock(startPos).blockNumber();
    }
    if (endPos >= 0) {
        endBlock = doc->findBlock(endPos).blockNumber();
    }

    const int searchCount = searchTokens.size();

    for (int i = startBlock; i <= endBlock && i < totalBlocks; ++i) {
        // ---- 首 token 必须匹配 ----
        if (blockTrimmed[i] != searchTokens[0]) continue;

        // ---- 尝试逐 token 匹配 ----
        int matchedBlocks = 0;   // 已匹配到的 token 数
        int lastMatchedLine = i - 1;
        int j = i;
        while (j <= endBlock && j < totalBlocks && matchedBlocks < searchCount) {
            // 跳过空行
            if (blockTrimmed[j].isEmpty()) {
                ++j;
                continue;
            }
            // 匹配当前 token
            if (blockTrimmed[j] == searchTokens[matchedBlocks]) {
                ++matchedBlocks;
                lastMatchedLine = j;
                ++j;
            } else {
                // 不匹配 中断本次尝试
                break;
            }
        }

        // ---- 全部 token 都匹配 记录一段 ----
        if (matchedBlocks == searchCount) {
            QTextCursor cursor(doc);
            QTextBlock firstBlock = doc->findBlockByNumber(i);
            QTextBlock lastBlock  = doc->findBlockByNumber(lastMatchedLine);

            cursor.setPosition(firstBlock.position());
            cursor.setPosition(lastBlock.position() + lastBlock.length() - 1,
                               QTextCursor::KeepAnchor);
            results.append(cursor);

            // 从 lastMatchedLine 之后继续找下一个起点
            i = lastMatchedLine;
        }
    }

    return results;
}

// ============================================================================
//  定位方法边界
//  宽松匹配：找所有包含 "<方法名>(" 的行
//  - Python：向下扫描缩进
//  - 大括号语言：数 { }
// ============================================================================
QList<TextEditor::MethodRange> TextEditor::findMethodRanges(const QString& methodName) const
{
    QList<MethodRange> results;
    if (methodName.isEmpty()) return results;

    QTextDocument* doc = document();
    if (!doc) return results;

    const QString langId = currentLanguageId();
    const bool isPython = langId.contains("python", Qt::CaseInsensitive);

    const int totalBlocks = doc->blockCount();

    // ---- 1. 宽松匹配：找所有包含 "<methodName>(" 的行 ----
    //    用词边界确保 "foo(" 不会匹配到 "barfoo(" 或 "foo2("
    QRegularExpression sigRegex(
        QStringLiteral("\\b%1\\s*\\(").arg(QRegularExpression::escape(methodName)));

    QVector<int> signatureLines;
    for (int i = 0; i < totalBlocks; ++i) {
        const QString text = doc->findBlockByNumber(i).text();
        // 跳过注释行（简单判断）
        const QString trimmed = text.trimmed();
        if (trimmed.startsWith("//") || trimmed.startsWith("#")) continue;

        if (sigRegex.match(text).hasMatch()) {
            signatureLines.append(i);
        }
    }

    // ---- 2. 对每个候选起点，找方法体范围 ----
    for (int sigLine : signatureLines) {
        MethodRange range;
        range.signatureLine = sigLine + 1;   // 1-based

        if (isPython) {
            // ---- Python：靠缩进 ----
            const QString sigText = doc->findBlockByNumber(sigLine).text();
            const int sigIndent = leadingIndentWidth(sigText);

            int bodyEnd = sigLine;
            for (int i = sigLine + 1; i < totalBlocks; ++i) {
                const QString t = doc->findBlockByNumber(i).text();
                if (t.trimmed().isEmpty()) continue;   // 空行跳过
                if (leadingIndentWidth(t) <= sigIndent) break;
                bodyEnd = i;
            }

            range.bodyStartLine = sigLine + 1;   // 1-based
            range.bodyEndLine   = bodyEnd + 1;
        } else {
            // ---- 大括号语言：数 { } ----
            //   从签名行开始，找第一个 {，然后配对
            int braceLine = -1;
            int bracePos  = -1;

            // 先从签名行本身找
            const QString sigText = doc->findBlockByNumber(sigLine).text();
            int openIdx = sigText.indexOf('{');
            if (openIdx >= 0) {
                braceLine = sigLine;
                bracePos  = openIdx;
            } else {
                // 签名行没有 { 向下找第一行有 { 的
                for (int i = sigLine + 1; i < totalBlocks; ++i) {
                    const QString t = doc->findBlockByNumber(i).text();
                    int idx = t.indexOf('{');
                    if (idx >= 0) {
                        braceLine = i;
                        bracePos  = idx;
                        break;
                    }
                    // 遇到分号说明是声明不是定义
                    if (t.contains(';')) return results.size() == 0 ? results : results;
                }
            }

            if (braceLine < 0) continue;

            // 从 braceLine 开始配对 { }
            int depth = 0;
            int endLine = -1;

            for (int i = braceLine; i < totalBlocks; ++i) {
                const QString t = doc->findBlockByNumber(i).text();
                const int startIdx = (i == braceLine) ? bracePos : 0;
                for (int k = startIdx; k < t.length(); ++k) {
                    if (t[k] == '{') ++depth;
                    else if (t[k] == '}') {
                        --depth;
                        if (depth == 0) {
                            endLine = i;
                            break;
                        }
                    }
                }
                if (endLine >= 0) break;
            }

            if (endLine < 0) continue;

            range.bodyStartLine = sigLine + 1;
            range.bodyEndLine   = endLine + 1;
        }

        results.append(range);
    }

    return results;
}

// ============================================================================
//  替换 + 缩进对齐（方案 C+）
//  1. offset = (match首行缩进 - replace首行缩进)
//  2. replace 每行加 offset
//  3. 每行结果取整到 tabWidth 的倍数（仅当 useSpaces）
// ============================================================================
void TextEditor::replaceWithIndentAlignment(QTextCursor& cursor,
                                            const QString& replaceText)
{
    if (cursor.isNull() || !cursor.hasSelection()) return;

    // ---- 1. 计算原始匹配位置的缩进 ----
    QTextBlock startBlock = cursor.block();
    const int origIndent = leadingIndentWidth(startBlock.text());

    // ---- 2. 计算 replace 首行缩进 ----
    QStringList replaceLines = replaceText.split(QRegularExpression("[\r\n]"));
    if (replaceLines.isEmpty()) return;

    const int replaceIndent = leadingIndentWidth(replaceLines[0]);
    const int offset = origIndent - replaceIndent;

    // ---- 3. 平移每行缩进 ----
    const bool useSpaces = m_useSpacesForTab;
    const int tabWidth = m_tabWidth;

    for (int i = 0; i < replaceLines.size(); ++i) {
        QString line = replaceLines[i];

        // 空行不处理
        if (line.trimmed().isEmpty()) continue;

        // 计算当前行缩进宽度
        int curIndent = leadingIndentWidth(line);
        int newIndent = curIndent + offset;
        if (newIndent < 0) newIndent = 0;

        // C+：取整到 tabWidth 的倍数（仅当使用空格）
        if (useSpaces && tabWidth > 0) {
            newIndent = (newIndent / tabWidth) * tabWidth;
        }

        // 重建该行：新缩进 + 去掉原缩进后的内容
        const QString content = line.mid(leadingWhitespace(line).length());
        QString newIndentStr;
        if (useSpaces) {
            newIndentStr = QString(newIndent, ' ');
        } else {
            // Tab 模式：按 tabWidth 换算
            const int tabs = newIndent / qMax(1, tabWidth);
            newIndentStr = QString(tabs, '\t');
        }

        replaceLines[i] = newIndentStr + content;
    }

    const QString finalText = replaceLines.join('\n');

    // ---- 4. 执行替换 ----
    cursor.beginEditBlock();
    cursor.removeSelectedText();
    cursor.insertText(finalText);
    cursor.endEditBlock();
}

//  轻量化自动保存
bool TextEditor::autoSave()
{
    if (m_filePath.isEmpty())     return false;
    if (!isModified())            return false;

    // ---- 冲突检测：磁盘 mtime 是否在 IDE 保存之后被外部改动 ----
    const QFileInfo fi(m_filePath);
    if (fi.exists() && m_lastSaveTime.isValid()
        && fi.lastModified() > m_lastSaveTime) {
        emit externalModificationDetected(m_filePath);
        return false;
    }

    // ---- 写盘（不 trim）----
    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[TextEditor] autoSave 打开失败:" << m_filePath;
        return false;
    }
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << toPlainText();
    file.close();

    // ---- 状态更新 ----
    m_isModified = false;
    document()->setModified(false);
    m_lastSaveTime = QDateTime::currentDateTime();

    // ---- LocalHistory 记录 ----
    if (m_localHistory) {
        m_localHistory->saveVersion(m_filePath, toPlainText(),
                                    "自动保存", "auto");
    }

    emit autoSaved(m_filePath);
    return true;
}

// ============================================================================
//  辅助：行首缩进宽度
// ============================================================================
int TextEditor::leadingIndentWidth(const QString& line) const
{
    int width = 0;
    for (QChar c : line) {
        if (c == ' ') {
            width += 1;
        } else if (c == '\t') {
            width += qMax(1, m_tabWidth);
        } else {
            break;
        }
    }
    return width;
}

// ============================================================================
//  辅助：按 offset 平移一行缩进
// ============================================================================
QString TextEditor::shiftIndent(const QString& line, int offset) const
{
    if (offset == 0) return line;

    const QString ws = leadingWhitespace(line);
    const int curIndent = leadingIndentWidth(line);
    int newIndent = curIndent + offset;
    if (newIndent < 0) newIndent = 0;

    const QString content = line.mid(ws.length());
    QString indentStr;
    if (m_useSpacesForTab) {
        indentStr = QString(newIndent, ' ');
    } else {
        const int tabs = newIndent / qMax(1, m_tabWidth);
        indentStr = QString(tabs, '\t');
    }
    return indentStr + content;
}

void TextEditor::applySearchHighlights(const QList<QTextEdit::ExtraSelection>& searchSels)
{
    QList<QTextEdit::ExtraSelection> kept = extraSelections();
    static const QString kTag = QStringLiteral("searchMatchHighlight");

    for (int i = kept.size() - 1; i >= 0; --i) {
        if (kept[i].format.property(QTextFormat::UserProperty).toString() == kTag) {
            kept.removeAt(i);
        }
    }

    kept.append(searchSels);
    setExtraSelections(kept);
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
    const QString prevIndent  = findLogicalLineIndent(prevBlock);

    // 空行也继承缩进
    if (prevTrimmed.isEmpty()) {
        if (!prevIndent.isEmpty()) {
            cursor.insertText(prevIndent);
            setTextCursor(cursor);
        }
        return;
    }

    const QString langId = currentLanguageId();
    const bool isPython  = langId.contains("python", Qt::CaseInsensitive);

    // ---- EditorRules 原始判定 ----
    const bool mayIndent = m_editorRules.endsWithIndentTrigger(prevTrimmed);
    const bool mayDedent = m_editorRules.startsWithDedentTrigger(prevTrimmed);

    // 大括号语义（覆盖 C/C++/Java/JS 等）
    const bool braceOpener = prevTrimmed.endsWith(QLatin1Char('{'));
    const bool braceCloser = prevTrimmed.startsWith(QLatin1Char('}'))
    && !braceOpener;

    // Python 复合语句头：`: ` 结尾（排除 `::`、排除注释行）
    const bool pythonColon =
    isPython
    && prevTrimmed.endsWith(QLatin1Char(':'))
    && !prevTrimmed.endsWith(QStringLiteral("::"))
    && !prevTrimmed.startsWith(QLatin1Char('#'));

    // 最终判定：
    //   - isOpener：需要新行 +1 缩进
    //   - isCloser：新行与上一行同级（关闭行后）
    const bool isOpener = braceOpener || mayIndent || pythonColon;
    const bool isCloser = braceCloser || (mayDedent && !braceOpener);

    if (!isOpener && !isCloser) {
        if (!prevIndent.isEmpty()) {
            cursor.insertText(prevIndent);
            setTextCursor(cursor);
        }
        return;
    }

    // 字符串/注释上下文保护（只对原本就支持 guard 的语言生效）
    const bool needGuard = m_editorRules.needsContextGuard();
    const bool prevSafe  = !needGuard
    || !isInsideString(prevBlock.position() + prevText.length());

    QString newIndent = prevIndent;

    if (prevSafe) {
        if (isCloser) {
            // 关闭行：新行和关闭行同级
            newIndent = prevIndent;
        } else if (isOpener) {
            if (pythonColon && !braceOpener && !mayIndent) {
                int logicalIndent = leadingIndentWidth(prevText);

                // 判断一行是否是 Python 复合语句头
                auto isPythonCompoundHead = [](const QString& t) -> bool {
                    static const QStringList kHeads = {
                        QStringLiteral("def "),
                        QStringLiteral("async def "),
                        QStringLiteral("class "),
                        QStringLiteral("if "),
                        QStringLiteral("elif "),
                        QStringLiteral("else:"),
                        QStringLiteral("for "),
                        QStringLiteral("async for "),
                        QStringLiteral("while "),
                        QStringLiteral("with "),
                        QStringLiteral("async with "),
                        QStringLiteral("try:"),
                        QStringLiteral("except"),
                        QStringLiteral("finally:"),
                    };
                    for (const QString& h : kHeads) {
                        if (t.startsWith(h)) return true;
                    }
                    return false;
                };

                if (!isPythonCompoundHead(prevTrimmed)) {
                    // prevBlock 是续行 向上找起始行
                    QTextBlock b = prevBlock.previous();
                    int scanned = 0;
                    while (b.isValid() && scanned < 200) {
                        const QString bt = b.text().trimmed();
                        if (!bt.isEmpty() && isPythonCompoundHead(bt)) {
                            logicalIndent = leadingIndentWidth(b.text());
                            break;
                        }
                        b = b.previous();
                        ++scanned;
                    }
                }

                newIndent = QString(logicalIndent + m_tabWidth, ' ');
            } else {
                // 通用：新行 = 上一行缩进 + 一级
                newIndent = prevIndent + indentUnit();
            }
        }
    }

    if (!newIndent.isEmpty()) {
        cursor.insertText(newIndent);
        setTextCursor(cursor);
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

    // 只在光标处于该行末尾（即刚好输入完关键字）时才处理
    if (cursor.positionInBlock() != text.length()) return false;

    // ============================================================
    //  v8.5：用 EditorRules::lineStartsWithDedentKeyword 判断，
    //    不再硬编码 else/catch/finally。
    //    这样 C++ 的 public:/private:/case/default，
    //    Python 的 else/elif/except/finally/case，
    //    Ruby 的 else/elsif/when/rescue/ensure，
    //    Shell 的 else/elif/fi/done/esac 全部自动支持。
    // ============================================================
    if (!m_editorRules.lineStartsWithDedentKeyword(trimmed)) {
        return false;
    }

    // 找出与当前关键字配对的块开始行
    // ---- 方案 A：大括号语言（C++ / Java / C#）----
    if (m_editorRules.blockStyle == EditorRules::BlockStyle::Brace) {
        // 从当前行向上扫，找最近的"块开始行"
        //   对 case 1: / default:  找 switch 行
        //   对 public: / private:   找 class 行（或上一个访问修饰符行）
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
            } else if (depth == 0) {
                // 找上一个同级的访问修饰符（public:/private:/case）
                if (m_editorRules.lineStartsWithDedentKeyword(t)) {
                    targetIndent = leadingWhitespace(b.text()).length();
                    break;
                }
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

        QTextCursor nc(block);
        nc.movePosition(QTextCursor::EndOfBlock);
        setTextCursor(nc);
        return true;
    }

    // ---- 方案 B：Python / Ruby / Shell ----
    //   缩进级别由上下文决定。用 dedentLineStarters 的核心逻辑：
    //   向上找最近一个"缩进更浅"的行，把当前行缩进对齐到它
    int curIndent = leadingIndentWidth(text);
    QTextBlock b = block.previous();
    int targetIndent = -1;
    int scanned = 0;
    while (b.isValid() && scanned < 200) {
        const QString bt = b.text();
        if (bt.trimmed().isEmpty()) {
            b = b.previous();
            ++scanned;
            continue;
        }
        const int bIndent = leadingIndentWidth(bt);
        if (bIndent < curIndent) {
            targetIndent = bIndent;
            break;
        }
        b = b.previous();
        ++scanned;
    }
    if (targetIndent < 0 || targetIndent >= curIndent) return false;

    QTextCursor c(block);
    c.movePosition(QTextCursor::StartOfBlock);
    c.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor,
        text.length() - trimmed.length());
    c.beginEditBlock();
    c.removeSelectedText();
    c.insertText(QString(targetIndent, ' '));
    c.endEditBlock();

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

    // 扫描完仍有未闭合的开括号 跳最后一个
    if (!stack.isEmpty()) {
        cursor.setPosition(stack.last().second);
    } else {
        cursor.setPosition(startPos);   // 没有不匹配
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
    // 前者内部会判断"全已注释 反注释"
    commentSelection();
    return true;
}

// ============================================================================
//  Ctrl+Enter：在当前行下方插入空行，光标落到新行
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
    // 光标放到刚插入的行
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

// ============================================================================
//  粘贴多行文本时的自动缩进对齐
// ============================================================================
void TextEditor::insertFromMimeData(const QMimeData* source)
{
    if (!source) {
        QPlainTextEdit::insertFromMimeData(source);
        return;
    }

    if (!source->hasText()) {
        QPlainTextEdit::insertFromMimeData(source);
        return;
    }

    const QString raw = source->text();
    if (raw.isEmpty()) {
        QPlainTextEdit::insertFromMimeData(source);
        return;
    }

    int savedBaseIndent = -1;    // -1 = 无选区 / 走基类
    int savedStartCol   = 0;
    {
        QTextCursor selCur = textCursor();
        if (selCur.hasSelection()) {
            const int selStart = selCur.selectionStart();
            QTextBlock selBlock = document()->findBlock(selStart);
            if (!selBlock.isValid()) {
                QPlainTextEdit::insertFromMimeData(source);
                return;
            }
            const int startCol    = selStart - selBlock.position();
            const int startIndent = leadingIndentWidth(selBlock.text());
            if (startCol > startIndent) {
                // 局部替换
                QPlainTextEdit::insertFromMimeData(source);
                return;
            }
            savedBaseIndent = startIndent;
            savedStartCol   = startCol;
            // 删除选区，光标落 selStart（选区起点）
            selCur.removeSelectedText();
            setTextCursor(selCur);
        }
    }

    // ---- 2. 切行 ----
    //   Qt 剪贴板的换行统一为 '\n'，但保险起见也兼容 '\r\n'
    QStringList lines = raw.split(QRegularExpression("[\r\n]"));
    if (lines.isEmpty()) {
        QPlainTextEdit::insertFromMimeData(source);
        return;
    }

    // ============================================================
    //  粘贴缩进 —— "拼接判断 + 第二行基准" 策略
    //
    //  步骤：
    //    1. 第一行：去掉所有前导缩进，从光标位置粘贴
    //    2. x = 粘贴后第一行的实际缩进
    //           = min(光标列, 行首缩进宽度)
    //           （光标在缩进区内 光标列；光标在代码区 行首缩进）
    //    3. 判断"是否开启块"：用**拼接后的完整第一行**（不是剪贴板第一行）
    //           拼接后 = 光标前这一行的文本（去前导空白）
    //                  + 剪贴板第一行（去前导空白）
    //           例：光标在 "    if |"，剪贴板第一行 "(x == 1) {"
    //               拼接后 "if (x == 1) {" 行末 { 开启块
    //    4. 期望第二行缩进：
    //           - 开启块：x + tabWidth
    //           - 不开启：x
    //    5. delta = 剪贴板第二行缩进 - 期望
    //    6. 从第二行到最后一行，每行缩进减 delta
    //       - delta > 0：去掉 delta 宽度的前导空白
    //       - delta < 0：补 |delta| 个空格（罕见，对称处理）
    // ============================================================

    const int tabWidth = qMax(1, m_tabWidth);

    auto indentWidthOf = [tabWidth](const QString& line) -> int {
        int w = 0;
        for (QChar c : line) {
            if (c == ' ')       w += 1;
            else if (c == '\t') w += tabWidth;
            else                break;
        }
        return w;
    };

    auto stripIndentByWidth = [&](const QString& line, int width) -> QString {
        if (width <= 0) return line;

        int consumed = 0;
        int i = 0;
        while (i < line.length() && consumed < width) {
            const QChar c = line[i];
            if (c == ' ')       { consumed += 1;       ++i; }
            else if (c == '\t') { consumed += tabWidth; ++i; }
            else                break;
        }
        return line.mid(i);
    };

    // ---- 3. 找剪贴板第一个非空行 ----
    int firstIdx = -1;
    for (int i = 0; i < lines.size(); ++i) {
        if (!lines[i].trimmed().isEmpty()) {
            firstIdx = i;
            break;
        }
    }
    if (firstIdx < 0) {
        // 全是空行 走基类
        QPlainTextEdit::insertFromMimeData(source);
        return;
    }

    // ---- 4. x = 粘贴后第一行的实际缩进 ----
    const QTextCursor cur = textCursor();
    const QTextBlock curBlock = cur.block();
    const int cursorCol = cur.positionInBlock();
    const int indentCol = leadingIndentWidth(curBlock.text());

    //  有选区时用"原选区首行缩进"作为基准；
    int x;
    if (savedBaseIndent >= 0) {
        x = savedBaseIndent;
    } else {
        x = qMin(cursorCol, indentCol);
    }

    // ---- 5. 拼接后的完整第一行 ----
    const QString beforeCursor = curBlock.text().left(cursorCol);
    const QString firstClipLine = lines.isEmpty() ? QString() : lines[0];
    const QString combinedLine = beforeCursor.trimmed() + firstClipLine.trimmed();
    const bool triggers = m_editorRules.endsWithIndentTrigger(combinedLine);

    // ---- 6. 剪贴板第二行的缩进（若无第二行，delta = 0） ----
    int secondIdx = -1;
    int cb_second = 0;
    for (int i = firstIdx + 1; i < lines.size(); ++i) {
        if (lines[i].trimmed().isEmpty()) continue;
        secondIdx = i;
        cb_second = indentWidthOf(lines[i]);
        break;
    }

    // ---- 7. delta ----
    int delta = 0;
    if (secondIdx >= 0) {
        const int expected = triggers ? (x + tabWidth) : x;
        delta = cb_second - expected;
    }

    // ---- 7.5 修正 delta：保证最小缩进非负 ----
    //    上面用"第一行 + 第二行"推 delta，是近似值。
    if (secondIdx >= 0 && delta > 0) {
        int minOrigIndent = std::numeric_limits<int>::max();
        for (int i = secondIdx; i < lines.size(); ++i) {
            if (lines[i].trimmed().isEmpty()) continue;
            const int w = indentWidthOf(lines[i]);
            if (w < minOrigIndent) minOrigIndent = w;
        }

        if (minOrigIndent != std::numeric_limits<int>::max()
            && minOrigIndent < delta) {
            delta = 0;   // 如果行会被扣成"负缩进" 放弃调整，整块原样粘贴。
        }
    }

    {
        int depth = 0;
        int maxDepth = 0;
        for (int i = firstIdx; i < lines.size(); ++i) {
            if (lines[i].trimmed().isEmpty()) continue;
            depth += bracketDelta(lines[i]);
            if (depth > maxDepth) maxDepth = depth;
        }

        if (maxDepth >= 1) {
            // baseIndent = 首行拼接后实际落入的缩进列
            const int baseIndent = x;

            QStringList out;
            out.reserve(lines.size());

            int level = 0;
            for (int i = 0; i < lines.size(); ++i) {
                const QString& l = lines[i];

                if (l.trimmed().isEmpty()) {
                    out.append(QString());
                    continue;
                }

                if (i == 0) {
                    if (savedBaseIndent >= 0) {
                        const int extra = qMax(0, savedBaseIndent - savedStartCol);
                        out.append(QString(extra, ' ')
                            + l.mid(leadingWhitespace(l).length()));
                    } else {
                        // 首行无条件去掉缩进
                        // （lines[0] == "" 时走空行分支，后续行自然另起）
                        out.append(l.mid(leadingWhitespace(l).length()));
                    }
                    level += bracketDelta(l);
                    if (level < 0) level = 0;
                    continue;
                }

                const QString tl = l.trimmed();
                const bool startsWithCloser =
                    !tl.isEmpty()
                    && (tl.at(0) == '}' || tl.at(0) == ')' || tl.at(0) == ']');

                const int effLevel = startsWithCloser
                                         ? qMax(0, level - 1)
                                         : level;

                const QString indent(baseIndent + effLevel * tabWidth, ' ');
                out.append(indent + tl);

                level += bracketDelta(l);
                if (level < 0) level = 0;
            }

            const QString joined = out.join('\n');

            QTextCursor cursor = textCursor();
            cursor.beginEditBlock();
            cursor.insertText(joined);
            cursor.endEditBlock();
            setTextCursor(cursor);

            return;   // 走完层级重建路径，跳过原来的 delta 组装
        }
    }

    // ---- 8. 逐行组装 ----
    QStringList out;
    out.reserve(lines.size());
    for (int i = 0; i < lines.size(); ++i) {
        const QString& l = lines[i];

        if (l.trimmed().isEmpty()) {
            out.append(QString());
            continue;
        }

        if (i == 0) {
            QString deindented;
            if (savedBaseIndent >= 0) {
                // 有选区：补足缩进到原选区首行缩进
                const int extra = qMax(0, savedBaseIndent - savedStartCol);
                out.append(QString(extra, ' ')
                    + l.mid(leadingWhitespace(l).length()));
            } else {
                out.append(l.mid(leadingWhitespace(l).length()));
            }
        } else {
            // 后续行：按 delta 平移（保持原逻辑）
            if (delta >= 0) {
                out.append(stripIndentByWidth(l, delta));
            } else {
                out.append(QString(-delta, ' ') + l);
            }
        }
    }

    // ---- 9. 组装 & 插入 ----
    const QString joined = out.join('\n');

    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();
    cursor.insertText(joined);
    cursor.endEditBlock();
    setTextCursor(cursor);
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
    //   pos       '{'
    //   pos+1     '\n'
    //   pos+2     innerIndent 第一个字符
    //   ...
    //   pos+2+innerLen '\n'
    //   ...            baseIndent + '}'
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
    if (event->modifiers() & Qt::ShiftModifier) return false;

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
//   智能"选到行首"——不含前导缩进
//
//
//  触发键：
//    macOS：       Cmd+Shift+Left
//    Win/Linux：   Shift+Home
//
//  实现要点：
//    - 保留锚点（anchor）不动，只移动"活动端"（cursor.position）
//    - 用 block 相对位置判断"活动端在缩进区内 / 代码区"
// ============================================================================
bool TextEditor::handleSmartHomeSelection(QKeyEvent* event)
{
        // ---- 1. 只处理指定的组合键（宽松匹配 + 调试日志） ----
    const Qt::KeyboardModifiers mods = event->modifiers();

    // 宽松匹配：Left + Shift + (Cmd 或 Ctrl 任一)
    //   不要求 modifier 完全相等——避免被 KeypadModifier 等额外标志干扰
    const bool isMacStyle =
        (event->key() == Qt::Key_Left)
        && (mods & Qt::ShiftModifier)
        && ((mods & Qt::ControlModifier) || (mods & Qt::MetaModifier));

    // Windows/Linux: Shift+Home（不含 Ctrl/Cmd）
    const bool isHomeStyle =
        (event->key() == Qt::Key_Home)
        && (mods & Qt::ShiftModifier)
        && !(mods & Qt::ControlModifier)
        && !(mods & Qt::MetaModifier);

    if (!isMacStyle && !isHomeStyle) return false;

    // ---- 2. 拿当前光标 + 区块信息 ----
    QTextCursor cursor = textCursor();
    const QTextBlock block = cursor.block();
    if (!block.isValid()) return false;

    const QString line = block.text();
    int firstNonWs = 0;
    while (firstNonWs < line.length() && line[firstNonWs].isSpace()) {
        ++firstNonWs;
    }

    // 活动端（选区 active end）相对本行的列
    const int activeCol = cursor.position() - block.position();
    const int anchorPos = cursor.anchor();   // 保持锚点

    // ---- 3. 决定目标列 ----
    int targetCol;
    if (activeCol <= 0) {
        // 已在行首 什么都不做
        return true;
    } else if (activeCol > firstNonWs) {
        // 在代码区 选到首个非空白列
        targetCol = firstNonWs;
    } else {
        // 已在缩进区 / 正好在首个非空白列 扩到列 0
        targetCol = 0;
    }

    // ---- 4. 应用新选区（锚点不变，活动端移动） ----
    const int newPos = block.position() + targetCol;
    QTextCursor newCursor(document());
    newCursor.setPosition(newPos);
    newCursor.setPosition(anchorPos, QTextCursor::KeepAnchor);
    setTextCursor(newCursor);
    return true;
}

// ============================================================================
//  选中文本时输入 opening 字符 用一对字符包裹选区
//  例：选中 foo，输入 " "foo"；输入 ( (foo)
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
//  Backspace 位于空配对中间 删除整对
//  例：(|) 按 Backspace |
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
//  在 C 风格块注释内回车 自动插入 " * "
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

// ============================================================
//  v8.5：Python import 上下文补全
// ============================================================

// 标准库路径：起一次 python 子进程拿，之后缓存
static QString cachedStandardLibraryPath()
{
    static QString cached;
    static bool tried = false;
    if (tried) return cached;
    tried = true;

    // 优先用项目配置的 PYTHON_HOME，否则退到 python3
    QString pyExe = QStringLiteral("python3");
    if (auto* cfg = Application::instance().configManager()) {
        const QVariantMap sdkEnv = cfg->value("Sdk/Environment").toMap();
        const QString pyHome = sdkEnv.value("PYTHON_HOME").toString();
        if (!pyHome.isEmpty() && QFile::exists(pyHome)) pyExe = pyHome;
    }

    QProcess proc;
    proc.start(pyExe, {"-c",
            "import sysconfig; print(sysconfig.get_paths()['stdlib'])"});
    if (proc.waitForFinished(3000) && proc.exitCode() == 0) {
        const QString out = QString::fromLocal8Bit(
            proc.readAllStandardOutput()).trimmed();
        if (!out.isEmpty() && QDir(out).exists()) {
            cached = out;
        }
    }
    return cached;
}

QString TextEditor::resolvePythonModuleDirectory(const QString& modulePath)
{
    // modulePath = "urllib" 或 "urllib.parse" 或 "numpy.linalg"
    QString rel = modulePath;
    rel.replace('.', '/');

    QStringList searchPaths;

    // 1. venv 的 site-packages
    if (auto model = ProjectManager::instance()->pythonProjectModel()) {
        if (!model->venvPath().isEmpty()) {
            QDir libDir(model->venvPath() + "/lib");
            const auto subs = libDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const auto& sub : subs) {
                if (sub.startsWith("python")) {
                    searchPaths << libDir.absoluteFilePath(sub + "/site-packages");
                }
            }
            #ifdef Q_OS_WIN
            searchPaths << model->venvPath() + "/Lib/site-packages";
            #endif
        }
    }

    // 2. 标准库
    const QString stdlib = cachedStandardLibraryPath();
    if (!stdlib.isEmpty()) searchPaths << stdlib;

    // 3. 逐个路径找
    for (const QString& base : searchPaths) {
        QDir baseDir(base);
        if (!baseDir.exists()) continue;
        const QString target = baseDir.absoluteFilePath(rel);
        if (QDir(target).exists()) return target;
    }
    return QString();
}

bool TextEditor::tryImportCompletion(QList<CompletionEntry>* out)
{
    if (!out) return false;
    if (currentLanguageId() != "python") return false;

    QTextCursor cursor = textCursor();
    const QString line = cursor.block().text();
    const int col = cursor.positionInBlock();
    const QString left = line.left(col);

    // from X.  /  import X.
    static QRegularExpression reFrom(R"(^\s*from\s+([\w.]+)\.\s*$)");
    static QRegularExpression reImport(R"(^\s*import\s+([\w.]+)\.\s*$)");

    QString modulePath;
    QRegularExpressionMatch m = reFrom.match(left);
    if (m.hasMatch()) {
        modulePath = m.captured(1);
    } else {
        m = reImport.match(left);
        if (m.hasMatch()) modulePath = m.captured(1);
        else return false;
    }

    if (modulePath.isEmpty()) return false;

    const QString dirPath = resolvePythonModuleDirectory(modulePath);
    if (dirPath.isEmpty()) return false;

    QDir dir(dirPath);
    const auto entries = dir.entryInfoList(
        QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);

    for (const QFileInfo& fi : entries) {
        if (fi.isDir()) {
            // 子包：必须有 __init__.py（否则不算 Python 包）
            if (!QFile::exists(fi.absoluteFilePath() + "/__init__.py"))
            continue;

            CompletionEntry e;
            e.label       = fi.fileName();
            e.insertText  = fi.fileName();
            e.detail      = QStringLiteral("package");
            e.kind        = QStringLiteral("module");
            e.sortPriority = 5;
            out->append(e);
        } else if (fi.suffix() == QLatin1String("py")) {
            const QString base = fi.completeBaseName();
            if (base == QLatin1String("__init__")) continue;

            CompletionEntry e;
            e.label       = base;
            e.insertText  = base;
            e.detail      = QStringLiteral("module");
            e.kind        = QStringLiteral("module");
            e.sortPriority = 5;
            out->append(e);
        }
    }

    return !out->isEmpty();
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
    QElapsedTimer _pt; _pt.start();
    QScrollBar* sb = verticalScrollBar();
    if (!sb) return;

    QPainter painter(sb);
    painter.setRenderHint(QPainter::Antialiasing, false);

    const int totalLines = qMax(1, lineCount());
    const int sbHeight   = sb->height();
    const int sbWidth    = sb->width();

    // 标记宽度随滚动条宽度自适应：
    //   普通屏滚动条约 12px markerWidth = 2；高 DPI 约 18px 3
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
    if (_pt.elapsed() > 5) qDebug() << "[SLOW] paintScrollBarMarkers:" << _pt.elapsed() << "ms";
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
//  返回一行的净括号变化（开括号数 - 关括号数）
//
//  用途：识别"续行锚点"
//    - 某行 bracketDelta > 0（有未闭合开括号）
//    - 且下一行缩进更深 下一行是这行的续行
//
//  实现：
//    - 跳过字符串（"" '' ``）内的括号
//    - 跳过行注释 // 和块注释 /* */ 内的括号
//    - 忽略 <>（避免 C++ 模版 / 比较运算符误判）
//
//  局限：不处理跨行字符串
// ============================================================================
int TextEditor::bracketDelta(const QString& line) const
{
    int d = 0;
    bool inDq = false, inSq = false, inBt = false, escaped = false;
    bool inLineComment = false;
    bool inBlockComment = false;

    for (int i = 0; i < line.length(); ++i) {
        const QChar c = line[i];

        if (inLineComment) break;
        if (inBlockComment) {
            if (c == '*' && i + 1 < line.length() && line[i + 1] == '/') {
                inBlockComment = false;
                ++i;
            }
            continue;
        }
        if (escaped) { escaped = false; continue; }
        if (c == '\\') { escaped = true; continue; }

        // 引号状态切换（互斥）
        if (c == '"'  && !inSq && !inBt) { inDq = !inDq; continue; }
        if (c == '\'' && !inDq && !inBt) { inSq = !inSq; continue; }
        if (c == '`'  && !inDq && !inSq) { inBt = !inBt; continue; }

        if (inDq || inSq || inBt) continue;

        // 注释起始
        if (c == '/' && i + 1 < line.length()) {
            if (line[i + 1] == '/') { inLineComment  = true; continue; }
            if (line[i + 1] == '*') { inBlockComment = true; ++i; continue; }
        }

        // 括号计数
        if      (c == '(' || c == '[' || c == '{') ++d;
        else if (c == ')' || c == ']' || c == '}') --d;
    }

    return d;
}

QString TextEditor::findLogicalLineIndent(const QTextBlock& block) const
{
    if (!block.isValid()) return QString();

    const int bn = block.blockNumber();
    auto it = m_logicalIndentCache.constFind(bn);
    if (it != m_logicalIndentCache.constEnd()) {
        return it.value();
    }

    const QString result = [&]() -> QString {
        const QString text = block.text();
        const QString selfIndent = leadingWhitespace(text);
        const int selfIndentWidth = leadingIndentWidth(text);

        if (m_editorRules.blockStyle == EditorRules::BlockStyle::Brace
            && !text.trimmed().isEmpty()) {
            const QString t = text.trimmed();
            const bool endsWithComma = t.endsWith(',');
            const bool hasNetClose = (bracketDelta(t) < 0);

            if (!endsWithComma && hasNetClose) {
                constexpr int kMaxScanUp = 200;
                int scanned = 0;
                int needed = 1;   // 还需找 1 个未匹配的 { 才能闭合

                QTextBlock b = block.previous();
                while (b.isValid() && scanned < kMaxScanUp) {
                    needed -= bracketDelta(b.text());
                    if (needed <= 0) {
                        return leadingWhitespace(b.text());
                    }
                    b = b.previous();
                    ++scanned;
                }
                return selfIndent;
            }
        }

        // 空行 / 零缩进 自身
        if (text.trimmed().isEmpty() || selfIndentWidth == 0) {
            return selfIndent;
        }

        // ---- 同级续行 ----
        const QTextBlock prevBlock = block.previous();
        if (prevBlock.isValid()) {
            const QString pt = prevBlock.text();
            if (!pt.trimmed().isEmpty()) {
                const int pWidth = leadingIndentWidth(pt);
                if (pWidth == selfIndentWidth) {
                    return selfIndent;
                }
            }
        }

        // ---- 向上找续行锚点 ----
        constexpr int kMaxScanUp = 200;
        int scanned = 0;
        QTextBlock b = block.previous();
        while (b.isValid() && scanned < kMaxScanUp) {
            const QString bt = b.text();
            if (bt.trimmed().isEmpty()) {
                b = b.previous();
                ++scanned;
                continue;
            }
            const int bWidth = leadingIndentWidth(bt);
            if (bWidth < selfIndentWidth) {
                if (bracketDelta(bt) > 0) {
                    return leadingWhitespace(bt);
                }
                break;
            }
            b = b.previous();
            ++scanned;
        }
        return selfIndent;
    }();

    m_logicalIndentCache.insert(bn, result);
    return result;
}

// ============================================================================
//  AI 辅助：可用性同步
// ============================================================================
void TextEditor::setAIAvailable(bool available)
{
    m_aiAvailable = available;
    if (m_aiMenu) {
        m_aiMenu->setEnabled(available);
        m_aiMenu->setToolTip(available ? QString()
                                       : tr("AI Agent 未连接"));
    }
}

// ============================================================================
//  AI 辅助：触发一个操作
//  1. 取选中文本（无选中 取当前整行）
//  2. 按 kind 拼 prompt
//  3. emit aiAssistRequested 让 MainWindow 转给 AI 模块
// ============================================================================
void TextEditor::triggerAIAssist(AIAssistKind kind)
{
    // ---- 1. 取代码 ----
    QString code = textCursor().selectedText();
    if (code.isEmpty()) {
        // 无选中 用光标所在整行
        code = textCursor().block().text();
    }
    // 选区里的段落分隔符归一化为 '\n'
    code.replace(QChar::ParagraphSeparator, '\n');

    if (code.trimmed().isEmpty()) {
        QMessageBox::information(this, tr("AI Agent"),
                                 tr("请先选中代码，或把光标放到有代码的行上。"));
        return;
    }

    // ---- 2. 拼 prompt ----
    QString prompt;
    switch (kind) {
    case AIAssistKind::ExplainCode:
        prompt = tr("请解释这段代码：") + QString("\n```\n%1\n```").arg(code);
        break;

    case AIAssistKind::FixCode:
        prompt = tr("请修复这段代码里的问题，说明修改理由：") + QString("\n```\n%1\n```").arg(code);
        break;

    case AIAssistKind::GenerateComment:
        prompt = tr("请为这段代码生成简洁的中文注释：")+ QString("\n```\n%1\n```").arg(code);
        break;

    case AIAssistKind::RenameSuggest:
        prompt = tr("请给这段代码里的变量、函数起更好的名字，并说明理由：")+ QString("\n```\n%1\n```").arg(code);
        break;

    case AIAssistKind::CustomAsk: {
        bool ok = false;
        const QString q = QInputDialog::getMultiLineText(
            this,
            tr("AI 自定义提问"),
            tr("请输入你的问题（会自动附带选中的代码）："),
            QString(), &ok);
        if (!ok || q.trimmed().isEmpty()) return;
        prompt = q + QStringLiteral("\n\n```\n") + code + QStringLiteral("\n```");
        break;
    }
    }

    // ---- 3. 发信号 ----
    emit aiAssistRequested(prompt, m_filePath, currentLine(), currentColumn());
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

    // v8.5：统一由 relayout() 处理
    //   - 面包屑、行号区、搜索栏的几何都在这个函数里算
    //   - resizeEvent 只负责触发，不参与具体计算
    relayout();
}

void TextEditor::contextMenuEvent(QContextMenuEvent *event)
{
    if (m_contextMenu) {
        m_undoAction->setEnabled(document()->isUndoAvailable());
        m_redoAction->setEnabled(document()->isRedoAvailable());
        m_cutAction->setEnabled(textCursor().hasSelection());
        m_copyAction->setEnabled(textCursor().hasSelection());
        m_pasteAction->setEnabled(!QApplication::clipboard()->text().isEmpty());

        // v8.5：Quick Fix 入口
        //   - 光标所在行有 error/warning 标记时才启用
        //   - 触发时调 showQuickFixMenuForLine，与 Alt+Enter 完全一致
        if (m_quickFixAction) {
            const int line = currentLine();
            const bool hasMarker = hasErrorMarker(line) || hasWarningMarker(line);
            m_quickFixAction->setEnabled(hasMarker);
        }

        // AI 子菜单：每次弹出时再同步一次可用状态（防止状态过期）
        if (m_aiMenu) {
            m_aiMenu->setEnabled(m_aiAvailable);
        }

        m_contextMenu->exec(event->globalPos());
    }
}

void TextEditor::updateLineNumberAreaWidth()
{
    if (!m_lineNumberArea) return;

    // v8.5：用 m_showBreadcrumb 判断是否显示，不用 isVisible()
    //   - isVisible() 在父控件未 show() 时恒为 false，
    //     导致 loadFile 时 top margin = 0 首行与面包屑重叠
    //   - 高度用常量（BreadcrumbBar::height() 在 resize 前不是 24）
    static constexpr int kBreadcrumbHeight = 24;

    const int left   = m_showLineNumbers ? lineNumberAreaWidth() : 0;
    const int top    = (m_showBreadcrumb && m_breadcrumbBar)
    ? kBreadcrumbHeight : 0;
    const int bottom = (m_findBarVisible && m_findBar)
    ? m_findBar->height() : 0;

    setViewportMargins(left, top, 0, bottom);
}

void TextEditor::relayout()
{
    if (!m_lineNumberArea) return;

    constexpr int kFindBarHeight = 40;      // 高度常量与 setGeometry 保持一致
    constexpr int kBreadcrumbHeight = 24;   // 与 updateLineNumberAreaWidth 保持一致

    // 三个区域的高度基准
    const int topMargin    = (m_showBreadcrumb && m_breadcrumbBar)
                                 ? kBreadcrumbHeight : 0;
    const int bottomMargin = (m_findBarVisible && m_findBar)
                                 ? kFindBarHeight : 0;

    const QRect cr = contentsRect();

    // ---------- 面包屑 ----------
    if (m_breadcrumbBar && m_showBreadcrumb) {
        m_breadcrumbBar->setGeometry(QRect(cr.left(), cr.top(),
                cr.width(), topMargin));
    }

    // ---------- 行号区 ----------
    if (m_showLineNumbers) {
        m_lineNumberArea->setGeometry(
            QRect(cr.left(),
                cr.top() + topMargin,
                lineNumberAreaWidth(),
                cr.height() - topMargin - bottomMargin));
    }

    // ---------- 搜索栏 ----------
    if (m_findBar && m_findBarVisible) {
        // 滚动条几何可能还没算好，直接查它的 current width 而不是靠 rect
        const int sbWidth = verticalScrollBar() ? verticalScrollBar()->width() : 0;
        const int barWidth = width() - sbWidth;
        m_findBar->setGeometry(0, height() - kFindBarHeight,
            barWidth, kFindBarHeight);
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
        selection.format.setProperty(QTextFormat::UserProperty,
                                     QStringLiteral("currentLineHighlight"));
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        all.append(selection);
    }

    setExtraSelections(all);
}

void TextEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    //   防御性空指针检查
    if (!m_lineNumberArea) return;

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

    // 文档变化 逻辑行缩进缓存全部失效
    m_logicalIndentCache.clear();

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

    if (m_autoSaveTimer && !m_filePath.isEmpty()) {
        m_autoSaveTimer->start();   // 重置为 15 秒
    }

    // highlightAllMatches 只 setExtraSelections，不会触发 textChanged，没有递归风险
    if (m_findBarVisible && m_findLineEdit
        && !m_findLineEdit->text().isEmpty()) {
        const int savedIndex = m_currentMatchIndex;
        highlightAllMatches(m_findLineEdit->text(),
            m_caseSensitiveCheck->isChecked(),
            m_wholeWordCheck->isChecked());
        if (!m_matchCursors.isEmpty()) {
            m_currentMatchIndex = qBound(0, savedIndex,
                m_matchCursors.size() - 1);
            scrollToMatch(m_currentMatchIndex);
        } else {
            m_currentMatchIndex = -1;
            m_matchCountLabel->setText("0/0");
        }
    }
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
//  - increase = true  Tab（在每行行首插入一个缩进单元）
//  - increase = false Shift+Tab（删除每行行首的一个缩进单元）
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
    // 选区末尾恰好停在行首 不算这一行（避免多缩进一行）
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

void TextEditor::setShowBreadcrumb(bool show)
{
    if (m_showBreadcrumb == show) return;
    m_showBreadcrumb = show;

    if (m_breadcrumbBar) {
        m_breadcrumbBar->setVisible(show);
    }
    updateLineNumberAreaWidth();
    relayout();   // v8.5：直接调 relayout，不再依赖 resizeEvent(nullptr)
}

void TextEditor::toggleOutlineOverlay()
{
    if (!m_outlineOverlay || !m_outlineController) return;

    if (m_outlineOverlay->isVisible()) {
        m_outlineOverlay->hideOverlay();
        return;
    }

    if (document()->characterCount() > 500000) return;

    m_outlineOverlay->setSymbols(m_outlineController->symbols());
    const QRect cr = cursorRect();
    const QPoint globalPos = viewport()->mapToGlobal(cr.bottomLeft());
    m_outlineOverlay->showAt(globalPos);
}

}
