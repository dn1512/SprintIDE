#include "editor_widget.h"
#include "theme_manager.h"

#include <QPainter>
#include <QTextBlock>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QKeyEvent>
#include <QMenu>
#include <QDebug>
#include <QRegularExpression>
#include <QTextDocument>
#include <QStringConverter>  // 添加QStringConverter头文件

namespace SprintIDE {

class EditorWidget::Private
{
public:
    QString filePath;
    QString language;
    bool isModified = false;
    int tabWidth = 4;
    bool useSpaces = true;
    bool showLineNumbers = true;
    bool highlightCurrentLine = true;
    bool wordWrap = false;

    LineNumberArea* lineNumberArea = nullptr;

    // 查找状态
    QString lastFindText;
    bool lastFindCaseSensitive = false;
    bool lastFindWholeWord = false;
};

EditorWidget::EditorWidget(QWidget *parent)
    : QPlainTextEdit(parent)
    , d(new Private)
{
    setupEditor();
    setupLineNumberArea();
    setupHighlighter();

    connect(this, &EditorWidget::blockCountChanged, this, &EditorWidget::updateLineNumberAreaWidth);
    connect(this, &EditorWidget::updateRequest, this, &EditorWidget::updateLineNumberArea);
    connect(this, &EditorWidget::cursorPositionChanged, this, &EditorWidget::highlightCurrentLine);
    connect(this->document(), &QTextDocument::contentsChanged, this, [this]() {
        d->isModified = true;
        emit textChanged();
    });

    highlightCurrentLine();
}

EditorWidget::~EditorWidget()
{
    delete d;
}

void EditorWidget::setupEditor()
{
    // 设置字体
    QFont font = ThemeManager::instance().editorFont();
    setFont(font);

    // 设置tab宽度
    QFontMetrics metrics(font);
    setTabStopDistance(metrics.horizontalAdvance(' ') * d->tabWidth);

    // 设置其他属性
    setWordWrapMode(d->wordWrap ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);

    // qDebug() << "EditorWidget initialized";
}

void EditorWidget::setupLineNumberArea()
{
    d->lineNumberArea = new LineNumberArea(this);

    updateLineNumberAreaWidth();
    highlightCurrentLine();
}

void EditorWidget::setupHighlighter()
{
    // 简单的高亮器（后续可以集成libclang）
    // 这里暂时留空，后续实现
}

bool EditorWidget::loadFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file:" << filePath;
        return false;
    }

    QTextStream stream(&file);
    // Qt 6中使用QStringConverter
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf8);
#else
    stream.setCodec("UTF-8");
#endif
    setPlainText(stream.readAll());
    file.close();

    d->filePath = filePath;
    d->isModified = false;

    // 根据文件扩展名检测语言
    if (filePath.endsWith(".cpp") || filePath.endsWith(".cc") || filePath.endsWith(".cxx")) {
        d->language = "C++";
    } else if (filePath.endsWith(".h") || filePath.endsWith(".hpp") || filePath.endsWith(".hxx")) {
        d->language = "C++ Header";
    } else if (filePath.endsWith(".c")) {
        d->language = "C";
    } else {
        d->language = "Plain Text";
    }

    emit fileLoaded(filePath);
    return true;
}

bool EditorWidget::saveFile()
{
    if (d->filePath.isEmpty()) {
        return false;
    }

    QFile file(d->filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << d->filePath;
        return false;
    }

    QTextStream stream(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf8);
#else
    stream.setCodec("UTF-8");
#endif
    stream << toPlainText();
    file.close();

    d->isModified = false;
    emit fileSaved(d->filePath);

    return true;
}

bool EditorWidget::saveAs(const QString &filePath)
{
    QString oldPath = d->filePath;
    d->filePath = filePath;

    if (saveFile()) {
        return true;
    } else {
        d->filePath = oldPath;
        return false;
    }
}

void EditorWidget::closeFile()
{
    QString path = d->filePath;
    d->filePath.clear();
    d->language.clear();
    clear();
    d->isModified = false;

    emit fileClosed(path);
}

QString EditorWidget::selectedText() const
{
    return textCursor().selectedText();
}

void EditorWidget::duplicateLine()
{
    QTextCursor cursor = textCursor();
    cursor.select(QTextCursor::LineUnderCursor);
    QString line = cursor.selectedText();
    cursor.movePosition(QTextCursor::EndOfLine);
    cursor.insertText("\n" + line);
}

void EditorWidget::deleteLine()
{
    QTextCursor cursor = textCursor();
    cursor.select(QTextCursor::LineUnderCursor);
    cursor.removeSelectedText();
    cursor.deleteChar(); // 删除换行符
}

void EditorWidget::moveLineUp()
{
    // 简化实现：交换当前行和上一行
    QTextCursor cursor = textCursor();
    int currentLine = cursor.blockNumber();
    if (currentLine == 0) return;

    cursor.movePosition(QTextCursor::StartOfLine);
    cursor.select(QTextCursor::LineUnderCursor);
    QString currentLineText = cursor.selectedText();
    cursor.removeSelectedText();
    cursor.deleteChar(); // 删除换行符

    // 移动到上一行
    cursor.movePosition(QTextCursor::PreviousBlock);
    cursor.movePosition(QTextCursor::StartOfLine);
    cursor.insertText(currentLineText + "\n");

    // 移动光标到新位置
    cursor.movePosition(QTextCursor::PreviousBlock);
    cursor.movePosition(QTextCursor::StartOfLine);
    setTextCursor(cursor);
}

void EditorWidget::moveLineDown()
{
    // 简化实现：交换当前行和下一行
    QTextCursor cursor = textCursor();
    int currentLine = cursor.blockNumber();
    int totalLines = document()->blockCount();
    if (currentLine >= totalLines - 1) return;

    cursor.movePosition(QTextCursor::StartOfLine);
    cursor.select(QTextCursor::LineUnderCursor);
    QString currentLineText = cursor.selectedText();
    cursor.removeSelectedText();
    cursor.deleteChar();

    // 移动到下一行
    cursor.movePosition(QTextCursor::NextBlock);
    cursor.movePosition(QTextCursor::EndOfLine);
    cursor.insertText("\n" + currentLineText);

    // 移动光标到新位置
    cursor.movePosition(QTextCursor::PreviousBlock);
    cursor.movePosition(QTextCursor::StartOfLine);
    setTextCursor(cursor);
}

void EditorWidget::commentSelection()
{
    QTextCursor cursor = textCursor();
    QString selected = cursor.selectedText();

    if (d->language.contains("C++") || d->language.contains("C")) {
        if (!selected.contains("\n")) {
            // 单行注释
            cursor.insertText("// " + selected);
        } else {
            // 多行注释
            cursor.insertText("/* " + selected + " */");
        }
    }
}

void EditorWidget::uncommentSelection()
{
    QTextCursor cursor = textCursor();
    QString selected = cursor.selectedText();

    if (selected.startsWith("// ")) {
        cursor.insertText(selected.mid(3));
    } else if (selected.startsWith("/* ") && selected.endsWith(" */")) {
        cursor.insertText(selected.mid(3, selected.length() - 6));
    }
}

int EditorWidget::cursorLine() const
{
    return textCursor().blockNumber() + 1;
}

int EditorWidget::cursorColumn() const
{
    return textCursor().columnNumber() + 1;
}

void EditorWidget::gotoLine(int line)
{
    QTextDocument *doc = document();
    QTextBlock block = doc->findBlockByLineNumber(line - 1);
    if (block.isValid()) {
        QTextCursor cursor(block);
        setTextCursor(cursor);
        ensureCursorVisible();
    }
}

void EditorWidget::gotoPosition(int line, int column)
{
    QTextDocument *doc = document();
    QTextBlock block = doc->findBlockByLineNumber(line - 1);
    if (block.isValid()) {
        QTextCursor cursor(block);
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column - 1);
        setTextCursor(cursor);
        ensureCursorVisible();
    }
}

void EditorWidget::find(const QString &text, bool caseSensitive, bool wholeWord)
{
    d->lastFindText = text;
    d->lastFindCaseSensitive = caseSensitive;
    d->lastFindWholeWord = wholeWord;

    QTextDocument::FindFlags flags;
    if (caseSensitive) flags |= QTextDocument::FindCaseSensitively;
    if (wholeWord) flags |= QTextDocument::FindWholeWords;

    QTextCursor cursor = textCursor();
    cursor = document()->find(text, cursor, flags);

    if (!cursor.isNull()) {
        setTextCursor(cursor);
    }
}

void EditorWidget::replace(const QString &findStr, const QString &replace, bool caseSensitive, bool wholeWord)
{
    if (textCursor().selectedText() == findStr) {
        textCursor().insertText(replace);
    }
    find(findStr, caseSensitive, wholeWord);
}

void EditorWidget::replaceAll(const QString &find, const QString &replace, bool caseSensitive, bool wholeWord)
{
    QTextCursor cursor(document());
    cursor.beginEditBlock();

    QTextDocument::FindFlags flags;
    if (caseSensitive) flags |= QTextDocument::FindCaseSensitively;
    if (wholeWord) flags |= QTextDocument::FindWholeWords;

    int count = 0;
    QTextCursor found = document()->find(find, cursor, flags);
    while (!found.isNull()) {
        found.insertText(replace);
        count++;
        found = document()->find(find, found, flags);
    }

    cursor.endEditBlock();

    // qDebug() << "Replaced" << count << "occurrences";
}

bool EditorWidget::isModified() const
{
    return d->isModified;
}

QString EditorWidget::filePath() const
{
    return d->filePath;
}

QString EditorWidget::fileName() const
{
    return QFileInfo(d->filePath).fileName();
}

QString EditorWidget::language() const
{
    return d->language;
}

int EditorWidget::lineCount() const
{
    return blockCount();
}

void EditorWidget::setTabWidth(int width)
{
    d->tabWidth = width;
    QFontMetrics metrics(font());
    setTabStopDistance(metrics.horizontalAdvance(' ') * width);
}

void EditorWidget::setUseSpaces(bool useSpaces)
{
    d->useSpaces = useSpaces;
}

void EditorWidget::setShowLineNumbers(bool show)
{
    d->showLineNumbers = show;
    updateLineNumberAreaWidth();
    if (!show && d->lineNumberArea) {
        d->lineNumberArea->hide();
    } else if (show && d->lineNumberArea) {
        d->lineNumberArea->show();
    }
}

void EditorWidget::setHighlightCurrentLine(bool highlight)
{
    d->highlightCurrentLine = highlight;
    highlightCurrentLine();
}

void EditorWidget::setWordWrap(bool wrap)
{
    d->wordWrap = wrap;
    setWordWrapMode(wrap ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
}

int EditorWidget::lineNumberAreaWidth() const
{
    if (!d->showLineNumbers) return 0;

    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    QFontMetrics metrics(font());
    int space = 3 + metrics.horizontalAdvance(QLatin1Char('9')) * digits;

    return space;
}

void EditorWidget::updateLineNumberAreaWidth()
{
    if (!d->showLineNumbers) return;

    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void EditorWidget::updateLineNumberArea(const QRect &rect, int dy)
{
    if (!d->showLineNumbers) return;

    if (dy) {
        d->lineNumberArea->scroll(0, dy);
    } else {
        d->lineNumberArea->update(0, rect.y(), d->lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth();
    }
}

void EditorWidget::highlightCurrentLine()
{
    if (!d->highlightCurrentLine) return;

    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = ThemeManager::instance().color(ThemeManager::UIElement::SelectionBackground);
        lineColor.setAlpha(50); // 半透明

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void EditorWidget::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    if (!d->showLineNumbers) return;

    QPainter painter(d->lineNumberArea);
    painter.fillRect(event->rect(), ThemeManager::instance().color(ThemeManager::UIElement::EditorBackground));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    QColor textColor = ThemeManager::instance().color(ThemeManager::UIElement::TextColor);
    painter.setPen(textColor);

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.drawText(0, top, d->lineNumberArea->width() - 3, fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void EditorWidget::keyPressEvent(QKeyEvent *event)
{
    // 处理Tab键：插入空格或Tab
    if (event->key() == Qt::Key_Tab) {
        if (textCursor().hasSelection()) {
            // 缩进选中行
            QTextCursor cursor = textCursor();
            int start = cursor.selectionStart();
            int end = cursor.selectionEnd();

            cursor.setPosition(start);
            int startBlock = cursor.blockNumber();
            cursor.setPosition(end, QTextCursor::KeepAnchor);
            int endBlock = cursor.blockNumber();

            cursor.beginEditBlock();
            for (int i = startBlock; i <= endBlock; ++i) {
                cursor.movePosition(QTextCursor::StartOfBlock);
                if (d->useSpaces) {
                    cursor.insertText(QString(d->tabWidth, ' '));
                } else {
                    cursor.insertText("\t");
                }
            }
            cursor.endEditBlock();
        } else {
            // 插入Tab或空格
            if (d->useSpaces) {
                insertPlainText(QString(d->tabWidth, ' '));
            } else {
                QPlainTextEdit::keyPressEvent(event);
            }
        }
        return;
    }

    // 处理Ctrl+/注释
    if (event->key() == Qt::Key_Slash && event->modifiers() == Qt::ControlModifier) {
        if (textCursor().hasSelection()) {
            commentSelection();
        }
        return;
    }

    QPlainTextEdit::keyPressEvent(event);
}

void EditorWidget::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    if (d->lineNumberArea) {
        d->lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
    }
}

void EditorWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();

    // 添加自定义菜单项
    menu->addSeparator();
    menu->addAction("Duplicate Line", this, &EditorWidget::duplicateLine, QKeySequence("Ctrl+D"));
    menu->addAction("Delete Line", this, &EditorWidget::deleteLine, QKeySequence("Ctrl+Y"));
    menu->addSeparator();
    menu->addAction("Comment Selection", this, &EditorWidget::commentSelection, QKeySequence("Ctrl+/"));
    menu->addAction("Uncomment Selection", this, &EditorWidget::uncommentSelection, QKeySequence("Ctrl+Shift+/"));

    menu->exec(event->globalPos());
    delete menu;
}

} // namespace SprintIDE
