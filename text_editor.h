#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H

#include <QPlainTextEdit>
#include <QWidget>
#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QTextBlock>
#include <QTextDocument>
#include <QPointF>
#include <QRectF>
#include <QAction>
#include <QMenu>
#include <QList>
#include <QSet>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QString>
#include <QListWidget>

#include "language/language_highlighter.h"
#include "history/local_history.h"
#include "lsp/lsp_client.h"


namespace SprintIDE {


// 前向声明
class LineNumberArea;
class LanguageHighlighter;

/**
 * @brief 错误/警告标记
 */
struct ErrorMarker {
    QString filePath;   // 文件路径（用于跨文件标记）
    int line;           // 行号
    int column;         // 列号（0表示整行）
    QString message;    // 错误信息
    QString severity;   // "error" 或 "warning"

    ErrorMarker() : line(0), column(0) {}
    ErrorMarker(const QString& file, int l, int c, const QString& msg, const QString& sev)
        : filePath(file), line(l), column(c), message(msg), severity(sev) {}
};

/**
 * @class TextEditor
 * @brief 文本编辑器核心类，支持代码编辑、语法高亮、行号显示等基础功能
 */
class TextEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit TextEditor(QWidget *parent = nullptr);
    ~TextEditor();

    // ========== 文件操作相关方法 ==========
    bool loadFile(const QString &filePath);
    /**
     * @brief 重新加载文件（从磁盘重新读取）
     * @return 成功返回 true
     */
    bool reloadFile();
    bool saveFile();
    bool saveAs(const QString &filePath);
    bool isModified() const;
    QString filePath() const;
    QString fileName() const;

    // ========== 设置本地历史管理器 ==========
    void setLocalHistory(LocalHistory *history);

    // ========== 编辑器设置相关方法 ==========
    void setShowLineNumbers(bool show);
    void setHighlightCurrentLine(bool highlight);
    void setUseSpacesForTab(bool useSpaces, int tabWidth = 4);
    void setEditorFont(const QFont &font);

    // ========== 主题相关方法 ==========
    void applyTheme(const QString &themeName);
    void updateLineNumberAreaPalette(bool isDark);
    void applyDarkTheme();
    void applyLightTheme();

    // ========== 代码编辑相关方法 ==========
    void commentSelection();
    void uncommentSelection();
    void duplicateLine();
    void deleteLine();
    void gotoLine(int lineNumber);

    // ========== 状态查询方法 ==========
    int currentLine() const;
    int currentColumn() const;
    int lineCount() const;

    // ========== 断点相关方法 ==========
    void setBreakpoint(int line, bool enabled);  // 设置/清除断点
    bool hasBreakpoint(int line) const;          // 检查某行是否有断点
    void clearAllBreakpoints();                  // 清除所有断点

    // ========== 调试高亮 ==========
    void setCurrentDebugLine(int line);          // 设置当前执行行
    void clearCurrentDebugLine();                // 清除当前执行行高亮

    // ========== 错误/警告标记 ==========
    void setErrorMarker(const QString& filePath, int line, int column, const QString& message);
    void setWarningMarker(const QString& filePath, int line, int column, const QString& message);
    void clearErrorMarkers();
    void clearWarningMarkers();
    void clearAllMarkers();
    bool hasErrorMarker(int line) const;
    bool hasWarningMarker(int line) const;
    QList<ErrorMarker> getErrorMarkers() const;

    // ========== 内联查找栏 ==========
    void showFindBar();
    void showFindBarWithReplace();
    void hideFindBar();
    bool isFindBarVisible() const;
    void findNext();
    void findPrevious();

    // ========== 提供对protected成员的访问（用于LineNumberArea）==========
    QTextBlock firstVisibleBlock() const { return QPlainTextEdit::firstVisibleBlock(); }
    QPointF contentOffset() const { return QPlainTextEdit::contentOffset(); }
    QRectF blockBoundingGeometry(const QTextBlock &block) const {
        return QPlainTextEdit::blockBoundingGeometry(block);
    }
    QRectF blockBoundingRect(const QTextBlock &block) const {
        return QPlainTextEdit::blockBoundingRect(block);
    }
    QTextDocument* textDocument() const { return document(); }

    /**
     * @brief 根据文件路径设置语法高亮语言
     * @param filePath 文件路径
     *
     * 自动检测语言并切换高亮规则
     */
    void setLanguageByFilePath(const QString& filePath);

    /**
     * @brief 手动设置语法高亮语言
     * @param languageId 语言 ID（如 "cpp", "python"）
     */
    void setLanguage(const QString& languageId);

    /**
     * @brief 获取当前高亮语言 ID
     */
    QString currentLanguageId() const;

    /**
     * @brief 获取当前高亮语言名称
     */
    QString currentLanguageName() const;

    /**
     * @brief 切换指定行的折叠状态
     * @param line 行号（从1开始）
     */
    void toggleFold(int line);

    /**
     * @brief 重新计算所有行的折叠状态（在文件加载或编辑后调用）
     */
    void updateFolding();
    /**
     * @brief 触发代码补全（手动或自动）
     */
    void triggerCompletion();

    /**
     * @brief 更新补全列表
     */
    void updateCompletionItems(const QList<LSPCompletionItem>& items);

    void highlightLine(int line);
signals:
    void modificationChanged(bool modified);
    void cursorPositionChanged(int line, int column);
    void fileSaved(const QString &filePath);
    void fileLoaded(const QString &filePath);
    void gotoLineRequested();
    void breakpointToggled(const QString& filePath, int line);
    void findRequested();
    void replaceRequested();
    void formatRequested();

    // ========== 查找栏信号 ==========
    void findBarVisibilityChanged(bool visible);
    void replaceCompleted(int count);

    void completionRequested(const QString& filePath, int line, int column);

    void definitionRequested(const QString& filePath, int line, int column);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;  // 处理行号区域点击
    void paintEvent(QPaintEvent *event) override;  // 自定义绘制
    bool event(QEvent *event) override;                    //
    bool viewportEvent(QEvent *event) override;            //

    void focusOutEvent(QFocusEvent* event) override;

private slots:
    void updateLineNumberAreaWidth();
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);
    void onCursorPositionChanged();
    void onTextChanged();
    void onGotoLineAction();
    void onErrorMarkerTimer();

    // ========== 查找栏槽 ==========
    void onFindTextChanged(const QString& text);
    void onFindNextClicked();
    void onFindPreviousClicked();
    void onReplaceTextChanged(const QString& text);
    void onReplaceClicked();
    void onReplaceAllClicked();
    void onToggleCaseSensitive(bool checked);
    void onToggleWholeWord(bool checked);
    void onToggleReplaceMode();
    void onCloseFindBar();

    // ========== 代码补全 ==========
    void onCompletionItemClicked(QListWidgetItem* item);
    void onCompletionItemDoubleClicked(QListWidgetItem* item);

private:
    void initEditor();
    void fixParenthesisHeight();
    void createContextMenu();
    void setupFindBar();
    void matchParentheses();
    int lineNumberAreaWidth() const;
    void autoIndent();
    int findMatchingParenthesis(int position, QChar parenthesis, bool backward);  // 添加函数声明
    void applyDefaultPalette();  // 添加默认调色板设置
    void initFontMetrics();
    void fixFontRendering();
    void testFontMetrics();

    // ========== 错误标记绘制 ==========
    void drawErrorMarkers(QPainter* painter);
    void drawErrorMarkersInLineNumberArea(QPainter* painter, int lineNumber,
                                          int top, int bottom, int areaWidth);
    QString getMarkerTooltip(const QPoint& pos) const;

    // 查找辅助
    void highlightAllMatches(const QString& text, bool caseSensitive, bool wholeWord);
    void scrollToMatch(int index);
    void clearHighlights();

    /**
     * @brief 判断某行是否可折叠（是否包含大括号）
     */
    bool isFoldableBlock(int line) const;

    /**
     * @brief 判断某行是否已被折叠
     */
    bool isBlockFolded(int line) const;

    /**
     * @brief 查找匹配的大括号所在行号
     * @param line 包含大括号的行号
     * @return 匹配的行号，如果没有匹配则返回 -1
     */
    int findMatchingBraceLine(int line) const;

    /**
     * @brief 在指定块中查找大括号（'{' 或 '}'）的位置
     * @param block 文本块
     * @param opening 是否查找 '{'（true）还是 '}'（false）
     * @return 大括号在块中的位置，如果没有则返回 -1
     */
    int findBraceInBlock(const QTextBlock &block, bool opening) const;

    // 存储已折叠的行号（行号为 1-based）
    QSet<int> m_foldedLines;


    // ========== 成员变量 ==========
    QString m_filePath;
    bool m_isModified;
    bool m_showLineNumbers;
    bool m_highlightCurrentLine;
    bool m_useSpacesForTab;
    int m_tabWidth;

    LineNumberArea *m_lineNumberArea;
    // SyntaxHighlighter *m_highlighter;
    LanguageHighlighter *m_highlighter = nullptr;
    QMenu *m_contextMenu;

    // ========== 断点相关成员 ==========
    QSet<int> m_breakpoints;      // 存储有断点的行号
    int m_currentDebugLine;       // 当前调试执行的行号（-1表示无）

    // ========== 绘制相关方法 ==========
    void drawBreakpointMarkers(QPainter *painter, const QRect &rect);
    void drawCurrentLineMarker(QPainter *painter, const QRect &rect);
    /**
     * @brief 判断当前光标位置是否应该触发补全
     */
    bool shouldTriggerCompletion(const QString& text, int cursorPos) const;

    /**
     * @brief 从补全列表中插入选中的项
     */
    void insertCompletion(const QString& insertText);

    // 补全相关成员
    QList<LSPCompletionItem> m_completionItems;
    QListWidget* m_completionPopup;      // 补全下拉列表
    bool m_completionActive = false;
    QString m_completionTriggerText;     // 触发补全的前缀（例如 "System." 中的 "System."）


    // 菜单动作
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_cutAction;
    QAction *m_copyAction;
    QAction *m_pasteAction;
    QAction *m_selectAllAction;
    QAction *m_commentAction;
    QAction *m_uncommentAction;
    QAction *m_duplicateLineAction;
    QAction *m_deleteLineAction;
    QAction *m_gotoLineAction;
    // QAction *m_findAction;
    // QAction *m_replaceAction;
    // QAction *m_formatCodeAction;

    // ========== 错误/警告标记 ==========
    QList<ErrorMarker> m_errorMarkers;
    QList<ErrorMarker> m_warningMarkers;
    QTimer* m_errorMarkerTimer;

    // ========== 查找栏控件 ==========
    QWidget* m_findBar;
    QLineEdit* m_findLineEdit;
    QLineEdit* m_replaceLineEdit;
    QCheckBox* m_caseSensitiveCheck;
    QCheckBox* m_wholeWordCheck;
    QPushButton* m_findNextButton;
    QPushButton* m_findPreviousButton;
    QPushButton* m_replaceButton;
    QPushButton* m_replaceAllButton;
    QPushButton* m_closeButton;
    QPushButton* m_toggleReplaceButton;
    QLabel* m_matchCountLabel;

    // 查找状态
    QString m_lastFindText;
    QList<QTextCursor> m_matchCursors;
    int m_currentMatchIndex;
    bool m_findBarVisible;
    bool m_replaceMode;

    // 声明LineNumberArea为友元类，使其可以访问protected成员
    friend class LineNumberArea;

    // ========== 本地历史管理器指针 ==========
    LocalHistory *m_localHistory = nullptr;
};

/**
 * @class LineNumberArea
 * @brief 行号显示区域类
 */
class LineNumberArea : public QWidget
{
    Q_OBJECT

public:
    explicit LineNumberArea(TextEditor *editor);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    TextEditor *m_editor;
};

}
#endif // TEXT_EDITOR_H
