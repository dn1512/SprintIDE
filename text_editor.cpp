#include "text_editor.h"
// #include "syntax_highlighter.h"
#include "language/language_highlighter.h"
#include "font_manager.h"
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


void LineNumberArea::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(event->rect(), Qt::red);
    painter.fillRect(event->rect(), palette().window());

    QTextBlock block = m_editor->firstVisibleBlock();
    int blockNumber = block.blockNumber();
    QPointF contentOffset = m_editor->contentOffset();
    qreal top = m_editor->blockBoundingGeometry(block).translated(contentOffset).top();
    qreal bottom = top + m_editor->blockBoundingRect(block).height();
    int currentLine = m_editor->currentLine();

    // 断点点击区宽度（左侧）
    int clickAreaWidth = 22;
    // 红点位置（在点击区内，靠近中间偏左一点）
    int dotX = 8;
    int dotSize = 12;

    // 折叠按钮区域宽度（位于行号左侧）
    int foldAreaWidth = 16;
    int numberStartX = foldAreaWidth + 4;

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            int lineNumber = blockNumber + 1;
            QString number = QString::number(lineNumber);

            // ========== 绘制折叠按钮 ==========
            if (m_editor->isFoldableBlock(lineNumber)) {
                int btnX = 2;
                int btnY = (int)(top + (bottom - top) / 2 - 6);
                int btnSize = 10;

                bool isFolded = m_editor->isBlockFolded(lineNumber);

                painter.setPen(QColor(150, 150, 150));
                painter.setBrush(QColor(150, 150, 150));

                if (isFolded) {
                    // ▶ 折叠状态（向右三角形，表示可以展开）
                    QPolygon triangle;
                    triangle << QPoint(btnX, btnY)
                             << QPoint(btnX + btnSize, btnY + btnSize / 2)
                             << QPoint(btnX, btnY + btnSize);
                    painter.drawPolygon(triangle);
                } else {
                    // ▼ 展开状态（向下三角形，表示可以折叠）
                    QPolygon triangle;
                    triangle << QPoint(btnX, btnY)
                             << QPoint(btnX + btnSize, btnY)
                             << QPoint(btnX + btnSize / 2, btnY + btnSize);
                    painter.drawPolygon(triangle);
                }
            }

            // ---- 绘制断点红点（行号左侧） ----
            if (m_editor->hasBreakpoint(lineNumber)) {
                int dotY = (int)(top + (bottom - top) / 2 - dotSize / 2);
                painter.setBrush(QColor(255, 0, 0));
                painter.setPen(Qt::NoPen);
                painter.drawEllipse(dotX, dotY, dotSize, dotSize);
                // 可选白色边框
                painter.setBrush(Qt::NoBrush);
                painter.setPen(QColor(255, 255, 255, 100));
                painter.drawEllipse(dotX, dotY, dotSize, dotSize);
            }

            // ---- 绘制行号数字（右对齐） ----
            int numberStartX = clickAreaWidth; // 从点击区右侧开始绘制
            if (lineNumber == currentLine) {
                painter.setPen(QColor(0, 100, 200));
                painter.setFont(QFont(font().family(), font().pointSize(), QFont::Bold));
            } else {
                painter.setPen(QColor(100, 100, 100));
                painter.setFont(font());
            }
            painter.drawText(numberStartX, (int)top,
                             width() - numberStartX - 16, (int)(bottom - top),
                             Qt::AlignRight | Qt::AlignVCenter, number);

            // ========== 绘制错误标记（红色圈） ==========
            if (m_editor->hasErrorMarker(lineNumber)) {
                int iconSize = 12;
                int iconX = width() - 30;
                int iconY = (int)(top + (bottom - top) / 2 - iconSize / 2);

                // 红色圆圈背景
                painter.setBrush(QColor(255, 50, 50));
                painter.setPen(Qt::NoPen);
                painter.drawEllipse(iconX, iconY, iconSize, iconSize);

                // 白色叉号
                painter.setPen(QPen(Qt::white, 2));
                int padding = 3;
                painter.drawLine(iconX + padding, iconY + padding,
                                 iconX + iconSize - padding, iconY + iconSize - padding);
                painter.drawLine(iconX + iconSize - padding, iconY + padding,
                                 iconX + padding, iconY + iconSize - padding);
            }

            // ========== 绘制警告标记（黄色三角形） ==========
            if (m_editor->hasWarningMarker(lineNumber)) {
                int iconSize = 12;
                int iconX = width() - 30;
                int iconY = (int)(top + (bottom - top) / 2 - iconSize / 2);

                // 黄色三角形
                painter.setBrush(QColor(255, 200, 0));
                painter.setPen(Qt::NoPen);
                QPolygon triangle;
                triangle << QPoint(iconX + iconSize/2, iconY)
                         << QPoint(iconX, iconY + iconSize)
                         << QPoint(iconX + iconSize, iconY + iconSize);
                painter.drawPolygon(triangle);

                // 黑色感叹号
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
    int clickAreaWidth = 22;  // 左侧断点点击区宽度
    int foldAreaWidth = 16;  // 折叠按钮区域宽度


    // 点击在折叠按钮区域（最左侧）
    if (x < foldAreaWidth) {
        // 计算行号
        int line = m_editor->cursorForPosition(event->pos()).blockNumber() + 1;
        if (line >= 1 && line <= m_editor->lineCount()) {
            // 检查该行是否可折叠
            if (m_editor->isFoldableBlock(line)) {
                m_editor->toggleFold(line);
                return;
            }
        }
        return;
    }

    // 点击在左侧断点点击区
    if (x < clickAreaWidth) {
        // 计算行号
        int line = m_editor->cursorForPosition(event->pos()).blockNumber() + 1;
        if (line >= 1 && line <= m_editor->lineCount()) {
            emit m_editor->breakpointToggled(m_editor->filePath(), line);
        }
        return;  // 不继续传递事件
    }
    // qDebug() << "LineNumberArea::mousePressEvent called";
    // 将鼠标点击事件转发给编辑器
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

    //代码补全
    m_completionPopup = new QListWidget(this);
    m_completionPopup->setWindowFlags(Qt::Popup);
    m_completionPopup->setFocusPolicy(Qt::NoFocus);
    m_completionPopup->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_completionPopup, &QListWidget::itemClicked,
            this, &TextEditor::onCompletionItemClicked);
    connect(m_completionPopup, &QListWidget::itemDoubleClicked,
            this, &TextEditor::onCompletionItemDoubleClicked);

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
    // 设置编辑器基本属性
    setLineWrapMode(QPlainTextEdit::NoWrap);

    // 使用字体管理器获取最佳编程字体
    FontManager::loadEmbeddedFonts();
    QFont editorFont = FontManager::getDefaultMonoFont(12);
    setEditorFont(editorFont);

    // 关键：设置文档的默认字体
    // 这确保所有新插入的文本都使用相同字体
    document()->setDefaultFont(editorFont);

    // 设置Tab键宽度
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * m_tabWidth);

    // 创建行号区域
    m_lineNumberArea = new LineNumberArea(this);

    // 行号区域也使用相同字体
    m_lineNumberArea->setFont(editorFont);

    // 创建语法高亮器
    m_highlighter = new LanguageHighlighter(document());

    // 立即应用主题
    m_highlighter->setTheme(true); // 默认使用深色主题

    // 创建上下文菜单
    createContextMenu();

    // 连接信号槽
    connect(this, &TextEditor::blockCountChanged, this, &TextEditor::updateLineNumberAreaWidth);
    connect(this, &TextEditor::updateRequest, this, &TextEditor::updateLineNumberArea);
    connect(this, &TextEditor::cursorPositionChanged, this, &TextEditor::onCursorPositionChanged);
    connect(this, &TextEditor::textChanged, this, &TextEditor::onTextChanged);

    // 初始化行号区域宽度
    updateLineNumberAreaWidth();

    // 设置默认的调色板
    applyDefaultPalette();

    // 验证字体选择
    // qDebug() << "Final editor font:" << font().family()
    //          << "Is fixed pitch:" << font().fixedPitch();
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

    QLabel* findLabel = new QLabel("查找:", m_findBar);
    findLabel->setStyleSheet("color: #E0E0E0;");
    layout->addWidget(findLabel);

    m_findLineEdit = new QLineEdit(m_findBar);
    m_findLineEdit->setPlaceholderText("输入查找内容...");
    m_findLineEdit->setMinimumWidth(200);
    connect(m_findLineEdit, &QLineEdit::textChanged, this, &TextEditor::onFindTextChanged);
    connect(m_findLineEdit, &QLineEdit::returnPressed, this, &TextEditor::onFindNextClicked);
    layout->addWidget(m_findLineEdit);

    m_caseSensitiveCheck = new QCheckBox("Aa", m_findBar);
    m_caseSensitiveCheck->setToolTip("大小写敏感");
    m_caseSensitiveCheck->setStyleSheet("color: #E0E0E0;");
    connect(m_caseSensitiveCheck, &QCheckBox::toggled, this, &TextEditor::onToggleCaseSensitive);
    layout->addWidget(m_caseSensitiveCheck);

    m_wholeWordCheck = new QCheckBox("\"\"", m_findBar);
    m_wholeWordCheck->setToolTip("全字匹配");
    m_wholeWordCheck->setStyleSheet("color: #E0E0E0;");
    connect(m_wholeWordCheck, &QCheckBox::toggled, this, &TextEditor::onToggleWholeWord);
    layout->addWidget(m_wholeWordCheck);

    m_findPreviousButton = new QPushButton("▲", m_findBar);
    m_findPreviousButton->setToolTip("上一个 (Shift+F3)");
    m_findPreviousButton->setFixedSize(30, 24);
    connect(m_findPreviousButton, &QPushButton::clicked, this, &TextEditor::onFindPreviousClicked);
    layout->addWidget(m_findPreviousButton);

    m_findNextButton = new QPushButton("▼", m_findBar);
    m_findNextButton->setToolTip("下一个 (F3)");
    m_findNextButton->setFixedSize(30, 24);
    connect(m_findNextButton, &QPushButton::clicked, this, &TextEditor::onFindNextClicked);
    layout->addWidget(m_findNextButton);

    m_matchCountLabel = new QLabel("0/0", m_findBar);
    m_matchCountLabel->setStyleSheet("color: #888888;");
    m_matchCountLabel->setMinimumWidth(40);
    layout->addWidget(m_matchCountLabel);

    m_toggleReplaceButton = new QPushButton("▼", m_findBar);
    m_toggleReplaceButton->setToolTip("显示替换");
    m_toggleReplaceButton->setFixedSize(30, 24);
    connect(m_toggleReplaceButton, &QPushButton::clicked, this, &TextEditor::onToggleReplaceMode);
    layout->addWidget(m_toggleReplaceButton);

    m_replaceLineEdit = new QLineEdit(m_findBar);
    m_replaceLineEdit->setPlaceholderText("替换为...");
    m_replaceLineEdit->setMinimumWidth(150);
    m_replaceLineEdit->setVisible(false);
    connect(m_replaceLineEdit, &QLineEdit::textChanged, this, &TextEditor::onReplaceTextChanged);
    connect(m_replaceLineEdit, &QLineEdit::returnPressed, this, &TextEditor::onReplaceClicked);
    layout->addWidget(m_replaceLineEdit);

    m_replaceButton = new QPushButton("替换", m_findBar);
    m_replaceButton->setVisible(false);
    m_replaceButton->setFixedWidth(50);
    connect(m_replaceButton, &QPushButton::clicked, this, &TextEditor::onReplaceClicked);
    layout->addWidget(m_replaceButton);

    m_replaceAllButton = new QPushButton("全部替换", m_findBar);
    m_replaceAllButton->setVisible(false);
    m_replaceAllButton->setFixedWidth(70);
    connect(m_replaceAllButton, &QPushButton::clicked, this, &TextEditor::onReplaceAllClicked);
    layout->addWidget(m_replaceAllButton);

    layout->addStretch();

    m_closeButton = new QPushButton("×", m_findBar);
    m_closeButton->setToolTip("关闭 (Esc)");
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
    m_lineNumberArea->update();
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
    m_undoAction = m_contextMenu->addAction("撤销 (Ctrl+Z)", this, &TextEditor::undo);
    m_redoAction = m_contextMenu->addAction("重做 (Ctrl+Y)", this, &TextEditor::redo);
    m_contextMenu->addSeparator();

    m_cutAction = m_contextMenu->addAction("剪切 (Ctrl+X)", this, &TextEditor::cut);
    m_copyAction = m_contextMenu->addAction("复制 (Ctrl+C)", this, &TextEditor::copy);
    m_pasteAction = m_contextMenu->addAction("粘贴 (Ctrl+V)", this, &TextEditor::paste);
    m_contextMenu->addSeparator();

    m_selectAllAction = m_contextMenu->addAction("全选 (Ctrl+A)", this, &TextEditor::selectAll);
    m_contextMenu->addSeparator();

    m_commentAction = m_contextMenu->addAction("注释选中行 (Ctrl+/)", this, &TextEditor::commentSelection);
    m_uncommentAction = m_contextMenu->addAction("取消注释 (Ctrl+/)", this, &TextEditor::uncommentSelection);
    m_contextMenu->addSeparator();

    m_duplicateLineAction = m_contextMenu->addAction("复制当前行 (Ctrl+D)", this, &TextEditor::duplicateLine);
    m_deleteLineAction = m_contextMenu->addAction("删除当前行 (Ctrl+Y)", this, &TextEditor::deleteLine);
    m_contextMenu->addSeparator();

    // 修复：连接到一个实际的槽函数，而不是不存在的信号
    m_gotoLineAction = m_contextMenu->addAction("跳转到行 (Ctrl+G)...", this, &TextEditor::onGotoLineAction);
}

bool TextEditor::loadFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "文件打开失败",
                             QString("无法打开文件：%1\n错误：%2")
                                 .arg(filePath).arg(file.errorString()));
        return false;
    }

    QTextStream stream(&file);
    stream.setAutoDetectUnicode(true);
    QString content = stream.readAll();
    file.close();

    // 临时断开信号，防止触发修改标记
    disconnect(this, &TextEditor::textChanged, this, &TextEditor::onTextChanged);

    setPlainText(content);
    m_filePath = filePath;
    m_isModified = false;
    document()->setModified(false);

    // 重新连接信号
    connect(this, &TextEditor::textChanged, this, &TextEditor::onTextChanged);

    // 在文件加载成功后，自动检测并设置语言
    if (m_highlighter) {
        m_highlighter->setLanguageByFilePath(filePath);
    }

    emit fileLoaded(filePath);

    updateFolding();

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
            this, "保存文件", "", "C++文件 (*.cpp *.h *.hpp);;所有文件 (*.*)");

        if (filePath.isEmpty()) {
            return false;
        }

        m_filePath = filePath;
    }

    QFile file(m_filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "文件保存失败",
                             QString("无法保存文件：%1\n错误：%2")
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

QString TextEditor::fileName() const
{
    if (m_filePath.isEmpty()) {
        return "未命名";
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
    QPlainTextEdit::setFont(font);
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * m_tabWidth);
    updateLineNumberAreaWidth();
}

// ========== 主题相关方法实现 ==========

void TextEditor::applyTheme(const QString &themeName)
{
    bool isDark = (themeName == "dark");

    // 更新语法高亮器
    if (m_highlighter) {
        m_highlighter->setTheme(isDark);
        // 重新高亮整个文档（不触发 contentsChanged）
        m_highlighter->rehighlight();
    }

    // 应用主题到编辑器
    if (isDark) {
        applyDarkTheme();
    } else {
        applyLightTheme();
    }

    // 更新行号区域
    if (m_lineNumberArea) {
        updateLineNumberAreaPalette(isDark);
    }

    // 强制重绘
    update();
    viewport()->update();
}

void TextEditor::updateLineNumberAreaPalette(bool isDark)
{
    QPalette palette = m_lineNumberArea->palette();

    if (isDark) {
        palette.setColor(QPalette::Window, QColor(30, 30, 40));
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
    QPalette palette = this->palette();

    // Dark theme colors
    palette.setColor(QPalette::Base, QColor("#2b2b2b"));
    palette.setColor(QPalette::Text, QColor("#cccccc"));
    palette.setColor(QPalette::Window, QColor("#3c3f41"));
    palette.setColor(QPalette::WindowText, QColor("#cccccc"));
    palette.setColor(QPalette::Button, QColor("#3c3f41"));
    palette.setColor(QPalette::ButtonText, QColor("#cccccc"));

    // Line number area
    QPalette lineNumberPalette = m_lineNumberArea->palette();
    lineNumberPalette.setColor(QPalette::Window, QColor("#313335"));
    lineNumberPalette.setColor(QPalette::WindowText, QColor("#606060"));
    m_lineNumberArea->setPalette(lineNumberPalette);
    m_lineNumberArea->setAutoFillBackground(true);

    setPalette(palette);

    // 清空当前行高亮
    QList<QTextEdit::ExtraSelection> extraSelections;
    setExtraSelections(extraSelections);

    // 更新显示
    update();
    m_lineNumberArea->update();
}

void TextEditor::applyLightTheme()
{
    QPalette palette = this->palette();

    // Light theme colors
    palette.setColor(QPalette::Base, QColor("#ffffff"));
    palette.setColor(QPalette::Text, QColor("#000000"));
    palette.setColor(QPalette::Window, QColor("#f0f0f0"));
    palette.setColor(QPalette::WindowText, QColor("#000000"));
    palette.setColor(QPalette::Button, QColor("#f0f0f0"));
    palette.setColor(QPalette::ButtonText, QColor("#000000"));

    // Line number area
    QPalette lineNumberPalette = m_lineNumberArea->palette();
    lineNumberPalette.setColor(QPalette::Window, QColor("#f8f8f8"));
    lineNumberPalette.setColor(QPalette::WindowText, QColor("#888888"));
    m_lineNumberArea->setPalette(lineNumberPalette);
    m_lineNumberArea->setAutoFillBackground(true);

    setPalette(palette);

    // 清空当前行高亮
    QList<QTextEdit::ExtraSelection> extraSelections;
    setExtraSelections(extraSelections);

    // 更新显示
    update();
    m_lineNumberArea->update();
}

// ========== 代码编辑相关方法 ==========
void TextEditor::commentSelection()
{
    QTextCursor cursor = textCursor();

    int start = cursor.selectionStart();
    int end = cursor.selectionEnd();

    cursor.setPosition(start);
    QTextBlock startBlock = cursor.block();

    cursor.setPosition(end);
    QTextBlock endBlock = cursor.block();

    // 如果光标在行首且选中跨多行，可能包含换行符，但 block 判断已覆盖
    if (cursor.positionInBlock() == 0 && endBlock != startBlock) {
        endBlock = endBlock.previous();
    }

    // ========== 检查是否所有行都已注释 ==========
    bool allCommented = true;
    QTextBlock block = startBlock;
    while (block.isValid() && block.blockNumber() <= endBlock.blockNumber()) {
        QString text = block.text().trimmed();  // 去除前导空格，检测注释符
        if (!text.startsWith("//")) {
            allCommented = false;
            break;
        }
        block = block.next();
    }

    if (allCommented) {
        // 如果全部已注释，则取消注释
        uncommentSelection();
        return;
    }

    // ========== 添加注释（原有逻辑） ==========
    cursor.beginEditBlock();

    block = startBlock;
    while (block.isValid() && block.blockNumber() <= endBlock.blockNumber()) {
        cursor.setPosition(block.position());
        cursor.insertText("// ");
        block = block.next();
    }

    cursor.endEditBlock();
}

void TextEditor::uncommentSelection()
{
    QTextCursor cursor = textCursor();

    int start = cursor.selectionStart();
    int end = cursor.selectionEnd();

    cursor.setPosition(start);
    QTextBlock startBlock = cursor.block();

    cursor.setPosition(end);
    QTextBlock endBlock = cursor.block();

    if (cursor.positionInBlock() == 0 && endBlock != startBlock) {
        endBlock = endBlock.previous();
    }

    cursor.beginEditBlock();

    QTextBlock block = startBlock;
    while (block.isValid() && block.blockNumber() <= endBlock.blockNumber()) {
        QString text = block.text();

        if (text.startsWith("// ")) {
            cursor.setPosition(block.position());
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 3);
            cursor.removeSelectedText();
        } else if (text.startsWith("//")) {
            cursor.setPosition(block.position());
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 2);
            cursor.removeSelectedText();
        }

        block = block.next();
    }

    cursor.endEditBlock();
}

void TextEditor::duplicateLine()
{
    QTextCursor cursor = textCursor();

    cursor.select(QTextCursor::LineUnderCursor);
    QString lineText = cursor.selectedText();

    cursor.movePosition(QTextCursor::EndOfLine);
    cursor.insertText("\n" + lineText);

    cursor.movePosition(QTextCursor::Up);
    cursor.movePosition(QTextCursor::StartOfLine);
    setTextCursor(cursor);
}

void TextEditor::deleteLine()
{
    QTextCursor cursor = textCursor();

    cursor.select(QTextCursor::LineUnderCursor);

    if (!cursor.atEnd()) {
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
    }

    cursor.removeSelectedText();
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
}

void TextEditor::setCurrentDebugLine(int line)
{
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
    update();
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
}

void TextEditor::clearErrorMarkers()
{
    m_errorMarkers.clear();
    viewport()->update();
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }
}

void TextEditor::clearWarningMarkers()
{
    m_warningMarkers.clear();
    viewport()->update();
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }
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
// 新增：语言相关方法实现
// ============================================================

void TextEditor::setLanguageByFilePath(const QString& filePath)
{
    if (m_highlighter) {
        m_highlighter->setLanguageByFilePath(filePath);
    }
}

void TextEditor::setLanguage(const QString& languageId)
{
    if (m_highlighter) {
        m_highlighter->setLanguage(languageId);
    }
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

void TextEditor::onCompletionItemClicked(QListWidgetItem* item)
{
    if (!item) return;
    QString insertText = item->data(Qt::UserRole).toString();
    if (insertText.isEmpty()) {
        insertText = item->text();
    }
    insertCompletion(insertText);
    m_completionPopup->hide();
    m_completionActive = false;
}

void TextEditor::onCompletionItemDoubleClicked(QListWidgetItem* item)
{
    // 与单击行为相同，或者可以不做特殊处理
    onCompletionItemClicked(item);
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
    // 如果补全弹窗正在显示，关闭它
    if (m_completionActive && m_completionPopup) {
        m_completionPopup->hide();
        m_completionActive = false;
    }

    // 调用基类处理
    QPlainTextEdit::focusOutEvent(event);
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
            QPointF offset = contentOffset();  // 获取视口偏移
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
    // 清除所有折叠状态
    m_foldedLines.clear();

    // 显示所有行
    QTextBlock block = document()->firstBlock();
    while (block.isValid()) {
        block.setVisible(true);
        block = block.next();
    }

    // 通过点击折叠按钮来折叠
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
    // ========== 第一步：补全弹窗的键盘导航（优先级最高） ==========
    if (m_completionActive) {
        if (event->key() == Qt::Key_Escape) {
            m_completionPopup->hide();
            m_completionActive = false;
            return;
        }
        if (event->key() == Qt::Key_Down) {
            int row = m_completionPopup->currentRow();
            if (row < m_completionPopup->count() - 1) {
                m_completionPopup->setCurrentRow(row + 1);
            }
            return;
        } else if (event->key() == Qt::Key_Up) {
            int row = m_completionPopup->currentRow();
            if (row > 0) {
                m_completionPopup->setCurrentRow(row - 1);
            }
            return;
        } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            if (m_completionPopup->currentItem()) {
                insertCompletion(m_completionPopup->currentItem()->data(Qt::UserRole).toString());
                m_completionPopup->hide();
                m_completionActive = false;
                return;
            }
        } else if (event->key() == Qt::Key_Escape) {
            m_completionPopup->hide();
            m_completionActive = false;
            return;
        }
        // 如果按下其他键，隐藏补全（但不要在这里 return，继续处理按键）
        // 但注意：如果我们继续处理按键，补全弹窗会消失，然后文本插入，我们会再次触发补全
        // 所以我们允许它继续往下走
    }

    // ========== 第二步：处理特殊键（自动括号、缩进等） ==========
    // 这些逻辑必须在补全检查之前执行，因为补全检查依赖已插入的文本

    // 处理 Escape 关闭查找栏
    if (event->key() == Qt::Key_Escape && m_findBarVisible) {
        hideFindBar();
        return;
    }

    // 处理 F3 查找下一个/上一个
    if (event->key() == Qt::Key_F3) {
        if (m_findBarVisible && !m_matchCursors.isEmpty()) {
            if (event->modifiers() & Qt::ShiftModifier) {
                findPrevious();
            } else {
                findNext();
            }
            return;
        }
    }

    // 处理 Ctrl+F 查找
    if (event->key() == Qt::Key_F && (event->modifiers() & Qt::ControlModifier)) {
        showFindBar();
        return;
    }

    // 处理 Tab 键（插入空格或制表符）
    if (event->key() == Qt::Key_Tab && !event->modifiers()) {
        if (m_useSpacesForTab) {
            QTextCursor cursor = textCursor();
            cursor.insertText(QString(m_tabWidth, ' '));
        } else {
            QPlainTextEdit::keyPressEvent(event);
        }
        // 注意：Tab 键不触发补全，直接返回
        return;
    }

    // 处理回车键（自动缩进）
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QPlainTextEdit::keyPressEvent(event);
        autoIndent();
        // 回车键不触发补全，直接返回
        return;
    }

    // 处理左大括号（自动补右大括号 + 换行缩进）
    if (event->text() == "{") {
        QTextCursor cursor = textCursor();

        // 获取当前行的缩进
        QTextBlock currentBlock = cursor.block();
        QString currentLine = currentBlock.text();
        QString indentStr;
        for (int i = 0; i < currentLine.length(); ++i) {
            if (currentLine[i] == ' ' || currentLine[i] == '\t') {
                indentStr += currentLine[i];
            } else {
                break;
            }
        }

        QString innerIndent = indentStr;
        if (m_useSpacesForTab) {
            innerIndent += QString(m_tabWidth, ' ');
        } else {
            innerIndent += "\t";
        }

        cursor.insertText("{");
        cursor.insertText("\n" + innerIndent);
        cursor.insertText("\n" + indentStr + "}");

        cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor, 1);
        cursor.movePosition(QTextCursor::StartOfLine);
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, innerIndent.length());
        setTextCursor(cursor);
        // 大括号不触发补全，直接返回
        return;
    }

    // 处理普通括号/引号成对插入
    if (event->text() == "(" || event->text() == "[" || event->text() == "\"" || event->text() == "'") {
        QTextCursor cursor = textCursor();
        QString pairChar;
        if (event->text() == "(") pairChar = ")";
        else if (event->text() == "[") pairChar = "]";
        else pairChar = event->text();

        cursor.insertText(event->text() + pairChar);
        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);
        setTextCursor(cursor);
        // 括号不触发补全，直接返回
        return;
    }

    // ========== 第三步：处理普通文本输入 ==========
    // 先让基类处理文本插入
    QPlainTextEdit::keyPressEvent(event);

    // ========== 第四步：检查是否需要触发补全 ==========
    // 只有在输入了普通字符（非特殊键）并且补全弹窗未激活时才检查
    if (!event->text().isEmpty() && !m_completionActive) {
        QTextCursor cursor = textCursor();
        int pos = cursor.position();
        QString text = toPlainText();

        // 检查是否应该触发补全（输入了 . 或 -> 或 ::）
        if (shouldTriggerCompletion(text, pos)) {
            triggerCompletion();
        }
    }
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

void TextEditor::triggerCompletion()
{
    if (m_completionActive) {
        // 已经激活，可能需要更新前缀，但为简单，直接返回
        return;
    }
    // 获取当前光标位置（行、列）
    QTextCursor cursor = textCursor();
    int line = cursor.blockNumber();        // 0-based
    int column = cursor.positionInBlock();  // 0-based

    QString filePath = m_filePath;
    if (filePath.isEmpty()) {
        return;
    }

    // 获取 LSP 客户端
    // 需要从 MainWindow 或全局获取 LSPManager
    // 假设我们有一个静态方法或通过信号槽传递
    // 这里我们通过信号请求 MainWindow 去获取补全
    // 或者我们直接持有 LSPManager 指针？在 TextEditor 中不宜直接依赖 LSPManager
    // 更好的方式：TextEditor 发出信号，MainWindow 接收并调用 LSP 客户端
    // 但为简单，我们通过全局单例或信号槽

    // 让我们通过信号来请求补全
    emit completionRequested(filePath, line, column);
}

void TextEditor::updateCompletionItems(const QList<LSPCompletionItem>& items)
{
    m_completionItems = items;
    if (items.isEmpty()) {
        // 补全项为空，隐藏弹窗
        if (m_completionPopup) {
            m_completionPopup->hide();
        }
        m_completionActive = false;
        return;
    }

    // 清空并填充弹窗
    m_completionPopup->clear();
    for (const LSPCompletionItem& item : items) {
        QString displayText = item.label;
        if (!item.detail.isEmpty()) {
            displayText += "  " + item.detail;
        }
        QListWidgetItem* listItem = new QListWidgetItem(displayText);
        listItem->setData(Qt::UserRole, item.insertText);
        m_completionPopup->addItem(listItem);
    }

    // 定位并显示弹窗
    QRect rect = cursorRect();
    QPoint globalPos = viewport()->mapToGlobal(rect.bottomLeft());
    int height = qMin(200, m_completionPopup->count() * 25);
    m_completionPopup->setGeometry(globalPos.x(), globalPos.y(), 300, height);
    m_completionPopup->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    m_completionPopup->setFocusPolicy(Qt::NoFocus);
    m_completionPopup->setAttribute(Qt::WA_ShowWithoutActivating);
    m_completionPopup->show();
    m_completionActive = true;
    m_completionPopup->setCurrentRow(0);
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
    if (!m_highlightCurrentLine) {
        return;
    }

    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!textCursor().hasSelection()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(240, 248, 255);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();

        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
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
    // 如果文档没有内容变化，不标记修改
    if (!document()->isModified()) {
        // 但也要检查是否有真正的修改
        if (!m_isModified) {
            return;
        }
    }

    m_isModified = true;
    emit modificationChanged(true);

    // updateFolding();
}

void TextEditor::onGotoLineAction()
{
    bool ok;
    int line = QInputDialog::getInt(this, "跳转到行", "行号:", 1, 1, lineCount(), 1, &ok);
    if (ok) {
        gotoLine(line);
    }
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
    int leftPadding = 22;
    int rightPadding = 16;
    int width = leftPadding + digitWidth * digits + rightPadding;

    // 最小宽度 55px
    return qMax(width, 55);
}

void TextEditor::autoIndent()
{
    QTextCursor cursor = textCursor();
    QTextBlock currentBlock = cursor.block();
    QTextBlock previousBlock = currentBlock.previous();

    if (previousBlock.isValid()) {
        QString previousText = previousBlock.text();

        // 计算缩进
        QString indentStr;
        for (int i = 0; i < previousText.length(); ++i) {
            if (previousText[i] == ' ' || previousText[i] == '\t') {
                indentStr += previousText[i];
            } else {
                break;
            }
        }

        QString trimmed = previousText.trimmed();

        // 如果上一行以左大括号结束，增加缩进
        if (trimmed.endsWith('{')) {
            if (m_useSpacesForTab) {
                indentStr += QString(m_tabWidth, ' ');
            } else {
                indentStr += "\t";
            }
        }
        // 如果上一行以右大括号结束，减少缩进
        else if (trimmed == "}") {
            // 移除一个层级的缩进
            if (m_useSpacesForTab && indentStr.length() >= m_tabWidth) {
                indentStr = indentStr.left(indentStr.length() - m_tabWidth);
            } else if (!m_useSpacesForTab && indentStr.endsWith("\t")) {
                indentStr = indentStr.left(indentStr.length() - 1);
            }
        }
        // 如果上一行是空行，查找最近的左大括号
        else if (trimmed.isEmpty()) {
            QTextBlock searchBlock = previousBlock.previous();
            int braceDepth = 0;

            while (searchBlock.isValid()) {
                QString searchText = searchBlock.text().trimmed();

                if (searchText.endsWith('}')) {
                    braceDepth++;
                } else if (searchText.endsWith('{')) {
                    if (braceDepth == 0) {
                        // 获取这个左大括号的缩进
                        QString searchIndent;
                        for (int i = 0; i < searchBlock.text().length(); ++i) {
                            if (searchBlock.text()[i] == ' ' || searchBlock.text()[i] == '\t') {
                                searchIndent += searchBlock.text()[i];
                            } else {
                                break;
                            }
                        }

                        // 增加一个层级的缩进
                        if (m_useSpacesForTab) {
                            indentStr = searchIndent + QString(m_tabWidth, ' ');
                        } else {
                            indentStr = searchIndent + "\t";
                        }
                        break;
                    } else {
                        braceDepth--;
                    }
                }
                searchBlock = searchBlock.previous();
            }
        }

        // 应用缩进
        if (!indentStr.isEmpty()) {
            cursor.insertText(indentStr);
        }
    }
}
}
