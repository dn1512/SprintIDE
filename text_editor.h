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
#include <QMap>
#include <QRegularExpression>
#include <QMimeData>
#include <QDateTime>

#include "language/language_highlighter.h"
#include "history/local_history.h"
#include "lsp/lsp_client.h"
#include "language/editor_rules.h"
#include "editor/completion_model.h"
#include "editor/completion_popup.h"


namespace SprintIDE {


// 前向声明
class LineNumberArea;
class LanguageHighlighter;
struct LSPCodeAction;

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
    /**
     * @brief 保存文件
     * @param source 保存来源："user"（默认，用户）或 "ai"（AI）
     * @return 保存是否成功
     */
    bool saveFile(const QString &source = "user");
    bool saveAs(const QString &filePath);
    bool isModified() const;
    QString filePath() const;
    /**
     * @brief 文件被外部重命名时同步路径
     *
     * 只更新 m_filePath 和语言（如果扩展名变了）。
     * 不触碰 document、不触发 modified、不 emit 任何保存信号。
     */
    void setFilePath(const QString& path);
    QString fileName() const;

    // ========== 设置本地历史管理器 ==========
    void setLocalHistory(LocalHistory *history);

    // ========== AI 辅助（供 MainWindow 同步状态） ==========
    enum class AIAssistKind {
        ExplainCode,        // 解释选中代码
        FixCode,            // 修复选中代码
        GenerateComment,    // 生成注释
        RenameSuggest,      // 重命名建议
        CustomAsk           // 自定义提问
    };

    /// 设置 AI 是否可用（MainWindow 在 AI 状态变化时调用）
    void setAIAvailable(bool available);

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

    /// 设置某一行行尾要显示的变量文本（例如 "x = 42, name = \"abc\""）
    void setInlineValues(int line, const QString& text);
    /// 清空所有内联变量显示
    void clearInlineValues();

    // ========== 错误/警告标记 ==========
    void setErrorMarker(const QString& filePath, int line, int column, const QString& message);
    void setWarningMarker(const QString& filePath, int line, int column, const QString& message);
    void clearErrorMarkers();
    void clearWarningMarkers();
    void clearAllMarkers();
    bool hasErrorMarker(int line) const;
    bool hasWarningMarker(int line) const;
    QList<ErrorMarker> getErrorMarkers() const;
    QList<ErrorMarker> getWarningMarkers() const;
    void setHoveredErrorLine(int line);

    // ========== 内联查找栏 ==========
    void showFindBar();
    void showFindBarWithReplace();
    void hideFindBar();
    bool isFindBarVisible() const;
    void findNext();
    void findPrevious();

    // ========== Quick Fix（错误修复） ==========
    /// 显示指定行的 quick-fix 菜单（globalPos 是屏幕坐标）
    void showQuickFixMenuForLine(int line, const QPoint& globalPos);

    /// 收集指定行可用的修复方案（从缓存的 LSP codeAction 结果里取）
    QList<LSPCodeAction> codeActionsForLine(int line) const;

    /// 找到 include 区域的插入位置（0-based 行号）
    ///   规则：跳过顶部注释/空行，返回最后一个 #include 之后的行
    int findIncludeInsertLine() const;

    /// 接收到 MainWindow 转发来的 LSP codeAction 结果
    void onCodeActionReceived(const QString& filePath,
                              const QList<LSPCodeAction>& actions);

    /// 应用一个 LSP 代码动作到本文档（暂不支持跨文件）
    bool applyCodeAction(const LSPCodeAction& action);

    // ========== Quick Fix 重试机制 ==========
    int    m_codeActionRetryCount = 0;      ///< 当前已重试次数（0 表示未重试）
    int    m_codeActionRetryLine = -1;      ///< 重试针对的行（1-based）
    QPoint m_codeActionRetryGlobalPos;      ///< 菜单最终弹出的屏幕坐标

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

    /**
     * @brief LSP 补全响应槽（供 MainWindow 连接 LSPClient::completionReceived）
     * @param filePath 触发补全的文件路径
     * @param items LSP 返回的补全项
     *
     * 为什么需要这个槽而不是直接用 lambda：
     *   Qt 6 的 Qt::UniqueConnection 只接受成员函数指针，
     *   用 lambda 会触发 "Unique connection requires the slot to be
     *   a pointer to a member function of a QObject subclass" 断言。
     */
    void onLSPCompletionReceived(const QString& filePath,
                                 const QList<LSPCompletionItem>& items);

    void onLSPDefinitionReceived(const QString& filePath,
                                 int line, int column);

    void highlightLine(int line);

    void setBreakpointWithCondition(int line, bool enabled, bool hasCondition);

    void setBreakpointEnabled(int line, bool enabled);      /// 设置断点的启用/禁用状态（与 DebugManager 同步用）

    /**
     * @brief 归一化空白后查找 search 文本
     *
     * 归一化规则：
     *  - 忽略行首缩进（任何空白）
     *  - 忽略空行
     *  - 行尾空白忽略
     *  - 行内非空白字符必须逐字匹配
     *
     * @param searchText  要搜索的多行文本
     * @param startPos    搜索起点（-1 = 从头）
     * @param endPos      搜索终点（-1 = 到末尾）
     * @return 所有匹配位置的 QTextCursor（每个 cursor 选中匹配块）
     */
    QList<QTextCursor> findByNormalizedWhitespace(const QString& searchText,
                                                  int startPos = -1,
                                                  int endPos = -1) const;

    /**
     * @brief 定位指定方法名的边界
     *
     * 宽松匹配：
     *  - 找所有包含 "<方法名>(" 的行
     *  - 若只有一行 → 用它作为方法起点
     *  - 若多行 → 返回所有候选（让上层判定 METHOD_AMBIGUOUS）
     *
     * 边界识别：
     *  - Python → 缩进层级
     *  - 大括号语言 → { } 配对
     *
     * @return 每个候选方法体范围（首行号、末行号，1-based）
     */
    struct MethodRange {
        int signatureLine;   // 方法声明所在行
        int bodyStartLine;   // 方法体第一行
        int bodyEndLine;     // 方法体最后一行
    };
    QList<MethodRange> findMethodRanges(const QString& methodName) const;

    /**
     * @brief 用一个新文本替换 cursor 选中的区间，缩进对齐
     *
     * @param cursor     选中要替换的区间（来自 findByNormalizedWhitespace）
     * @param replaceText 新文本
     *
     * 缩进对齐规则（方案 C+）：
     *  1. 计算 offset = (match首行缩进 - replace首行缩进)
     *  2. replace 每一行加上 offset
     *  3. 若 tabWidth > 0，每行结果取整到 tabWidth 的倍数
     */
    void replaceWithIndentAlignment(QTextCursor& cursor, const QString& replaceText);

    bool autoSave();

    /// 上次手动/自动保存成功的时间（用于冲突检测）
    QDateTime lastSaveTime() const { return m_lastSaveTime; }
    void setLastSaveTime(const QDateTime& t) { m_lastSaveTime = t; }

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

    /// 用户请求对某行执行 Quick Fix（点击错误标记 / Alt+Enter）
    /// 参数（均为 1-based，行号从 1 开始）：
    ///   filePath    —— 触发文件
    ///   startLine   —— 诊断起止行
    ///   startColumn —— 起始列
    ///   endLine     —— 结束行（可等于 startLine）
    ///   endColumn   —— 结束列
    void quickFixRequested(const QString& filePath,
                           int startLine, int startColumn,
                           int endLine,   int endColumn);

    /// 文档内容变化已防抖，供 LSP 发送 didChange 使用
    void documentChangedForLsp(const QString& filePath, const QString& content);

    void definitionRequested(const QString& filePath, int line, int column);

    void breakpointConditionChanged(const QString& filePath, int line,
                                    const QString& condition);

    void definitionJumpRequested(const QString& file, int line, int column);

    // 当用户输入 a-z 时，把上下文发给 LSP 补全（不同于 completionRequested）
    void completionRequestedAt(const QString& filePath, int line, int column,
                               const QString& prefix);

    void quickFixTriggeredByClick(int line, const QPoint& globalPos);

    // ========== AI 辅助 ==========
    /// 用户在右键菜单里触发了 AI 操作
    /// @param prompt    拼好的完整问题（含选中代码）
    /// @param filePath  触发文件路径
    /// @param line      光标行号（1-based）
    /// @param column    光标列号（1-based）
    void aiAssistRequested(const QString& prompt,
                           const QString& filePath,
                           int line, int column);

    void autoSaved(const QString& filePath);
    void externalModificationDetected(const QString& filePath);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;  // 处理行号区域点击
    void paintEvent(QPaintEvent *event) override;  // 自定义绘制
    bool event(QEvent *event) override;                    //
    bool viewportEvent(QEvent *event) override;            //

    void focusOutEvent(QFocusEvent* event) override;

    bool eventFilter(QObject* obj, QEvent* event) override;

    // 粘贴时按目标缩进基准对齐多行文本
    void insertFromMimeData(const QMimeData* source) override;

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
    // void onCompletionItemClicked(QListWidgetItem* item);
    // void onCompletionItemDoubleClicked(QListWidgetItem* item);

private:
    void initEditor();
    void fixParenthesisHeight();
    void createContextMenu();
    void setupFindBar();
    void matchParentheses();
    int lineNumberAreaWidth() const;
    void indentSelection(bool increase);
    int findMatchingParenthesis(int position, QChar parenthesis, bool backward);  // 添加函数声明
    void applyDefaultPalette();  // 添加默认调色板设置
    void initFontMetrics();
    void fixFontRendering();
    void testFontMetrics();

    // 私有辅助——只弹菜单，不发请求。避免和 onCodeActionReceived 互触发
    void showCodeActionMenu(int line,
                            const QPoint& globalPos,
                            const QList<LSPCodeAction>& actions);

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

    bool tryExpandPairOnReturn();

    void applyLineHeight();

    /// 行首缩进宽度（tab 算 tabWidth 个）
    int leadingIndentWidth(const QString& line) const;

    /// 按 offset 平移一行缩进
    QString shiftIndent(const QString& line, int offset) const;

    /// 只替换"搜索匹配"高亮，保留其他 ExtraSelection
    void applySearchHighlights(const QList<QTextEdit::ExtraSelection>& searchSels);

    // 跳转定义的视觉反馈
    bool         m_ctrlHeldForDefinition = false;
    QPoint       m_lastMousePos;
    bool         m_hasValidMousePos = false;
    QTextCursor  m_definitionCursor;

    // 辅助方法
    QTextCursor  getIdentifierAt(const QPoint& pos) const;
    void         updateDefinitionHighlight();
    void         clearDefinitionHighlight();

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
    QSet<int> m_disabledBreakpoints;      // 被禁用（灰色）的断点行

    QMap<int, QString> m_inlineValues;   // 行号 → 显示的文本

    // ========== Quick Fix（错误修复） ==========
    /// 鼠标悬停/光标停留的"错误行"（1-based，-1 = 无）
    /// 影响：行号区红点变大变实、行内波浪线加粗
    int m_hoveredErrorLine = -1;

    /// LSP codeAction 结果缓存：起始行（1-based） → 该行的可用修复
    QHash<int, QList<LSPCodeAction>> m_codeActionsByLine;

    /// 正在等待 LSP codeAction 响应的目标行（-1 = 无）
    int m_codeActionPendingLine = -1;

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


    // ========== 代码补全 ==========
    void setupCompletion();
    void requestLocalCompletions(const QString& prefix);
    void requestLspCompletions(const QString& prefix);
    void rebuildCompletionList();
    void commitCompletion(int row);
    void closeCompletion();
    bool isCompletionTriggerChar(QChar c) const;
    bool isWordChar(QChar c) const;
    QString currentWordPrefix() const;   // 光标前连续 word 字符

    // ========== 语言相关的编辑辅助 ==========
    bool  isInsideString(int position) const;
    bool  isInsideLineComment(int position) const;
    bool  isInsideStringOrComment(int position) const;

    void  autoIndentAfterReturn();       // 回车后调用
    bool  tryAutoPair(const QString& input);      // 返回 true 表示已处理
    bool  tryBraceNewline(const QString& input);  // 返回 true 表示已处理

    QString indentUnit() const;          // 返回一个缩进单元（"    " 或 "\t"）
    QString leadingWhitespace(const QString& line) const;

    void  applyEditorRulesForLanguage(const QString& languageId);

    // Tab 对齐到 tab stop
    void  handleTabKey(bool increase);

    // 智能 Home：第一次跳到首个非空白列，再按跳到列 0
    bool  handleSmartHome(QKeyEvent* event);
    bool  handleSmartEnd(QKeyEvent* event);

    ///   智能"选到行首"——不含前导缩进
    ///   macOS：Cmd+Shift+Left
    ///   Windows/Linux：Shift+Home
    ///   - 活动端在代码区 → 选到首个非空白列
    ///   - 再按一次     → 扩到列 0
    bool  handleSmartHomeSelection(QKeyEvent* event);

    // 选中文本被引号/括号包裹
    bool  trySurroundSelection(const QString& input);

    // Backspace 删除空配对（() [] {} "" '' ``）
    bool  tryDeletePairOnBackspace();

    // 块注释（/** ... */ 或 /* ... */）内回车自动补 " * "
    bool  tryContinueBlockComment();

    // Tab 按下且前缀命中片段 → 展开；返回 true 表示已处理
    bool  tryExpandSnippet();

    // ========== include 路径补全 ==========
    bool  isInIncludeContext(QString* prefixOut = nullptr) const;
    void  requestIncludeCompletions();

    // ========== 保存时自动格式化 ==========
    bool  runExternalFormatter(QString* errorOut = nullptr);

    // ========== 滚动条错误标记 ==========
    void  paintScrollBarMarkers();
    void  scheduleScrollBarRepaint();

    // 应用一条片段：body 里 $0 表示最终光标位置，$1 等按序 Tab 跳
    void  applySnippet(const QString& body, int lineIndent);

    // 光标处的前缀（比如 "for" / "if"）
    QString currentIdentPrefix() const;

    // Alt+↑ / Alt+↓ 移动当前行（或选中行块）
    void  moveLinesUp();
    void  moveLinesDown();

    // else / catch / finally 回车对齐到对应的 if / try
    bool  tryAlignDedentKeyword();

    // Ctrl+Shift+\ ：跳到不匹配的括号
    void  jumpToMismatchedBracket();

    // Ctrl+/ ：切换注释
    bool  handleCommentShortcut(QKeyEvent* event);

    // Ctrl+Enter / Ctrl+Shift+Enter ：上下插入空行
    void  insertLineBelow();
    void  insertLineAbove();

    // 括号/引号配色：匹配黄、不匹配红
    void  matchParenthesesWithColor();
    QColor m_matchedParenColor;
    QColor m_unmatchedParenColor;

    // 智能退格：光标在纯空白前缀时，一次退一个缩进级别
    bool  trySmartBackspace();

    // 双击选词：把 `_` 当词边界
    QString wordBoundaryAt(const QPoint& pos) const;   // 可选，若用 Qt 默认即可不实现

    void mouseDoubleClickEvent(QMouseEvent* event) override;

    // 快捷键：复制行 / 删除行
    bool  handleEditorShortcuts(QKeyEvent* event);

    // 保存/恢复 modified 状态的辅助函数
    void beginSilentModification();
    void endSilentModification();

    ///   返回一行的"净括号变化"（开括号数 - 关括号数）
    ///   跳过字符串、注释、字符字面量
    ///   用途：识别"续行锚点"——上一行 bracketDelta > 0 说明当前行是它的续行
    int bracketDelta(const QString& line) const;

    ///   返回 block 所在"逻辑行"的缩进
    ///   - 上一行缩进相同 → 同级续行，返回自身缩进
    ///   - 向上找缩进更浅的"锚点行"（bracketDelta > 0）→ 返回锚点行缩进
    ///   - 都没找到 → 返回自身缩进
    QString findLogicalLineIndent(const QTextBlock& block) const;

    bool m_formatOnSave = false;

    // 补全相关成员
    /*QList<LSPCompletionItem> m_completionItems;
    QListWidget* m_completionPopup;      // 补全下拉列表
    bool m_completionActive = false;
    QString m_completionTriggerText;*/     // 触发补全的前缀（例如 "System." 中的 "System."）
    CompletionModel*  m_completionModel  = nullptr;
    CompletionPopup*  m_completionPopup  = nullptr;
    bool              m_completionActive = false;

    // 触发位置：光标在此位置之前的前缀才是补全前缀
    int               m_completionAnchor = -1;   // 文档绝对位置
    QString           m_completionPrefix;        // 当前前缀（已输入的部分）

    QTimer*           m_lspDebounceTimer = nullptr;   // LSP 请求防抖
    QTimer*           m_lspChangeTimer   = nullptr;   // didChange 防抖
    bool              m_pendingLspChange = false;

    // 已收到的 LSP 原始结果（未合并本地符号时缓存）
    QList<LSPCompletionItem> m_pendingLspItems;
    bool              m_lspResponseWaiting = false;

    bool m_saving = false;   // 防重入


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

    qreal m_lastAppliedLineHeight = -1.0;
    bool m_suppressModificationFlag = false;    // 静默修改守卫：在这些操作期间不允许 modified 被置 true
    QSet<int> m_conditionalBreakpoints;   // 存储带条件的断点行号

    // ========== 语言编辑规则（打开文件时确定一次）==========
    EditorRules m_editorRules;

    // SVG 图标缓存（key = "路径@宽x高"）
    mutable QHash<QString, QPixmap> m_iconCache;

    // 取（并在必要时渲染 + 缓存）图标
    QPixmap cachedIcon(const QString& svgPath, const QSize& size) const;

    // 主题切换时调用，清空缓存（让下次绘制重新渲染正确色版本）
    void invalidateIconCache();

    // ========== AI 辅助 ==========
    void triggerAIAssist(AIAssistKind kind);

    bool      m_aiAvailable       = false;
    QMenu*    m_aiMenu            = nullptr;
    QAction*  m_aiExplainAction   = nullptr;
    QAction*  m_aiFixAction       = nullptr;
    QAction*  m_aiCommentAction   = nullptr;
    QAction*  m_aiRenameAction    = nullptr;
    QAction*  m_aiAskAction       = nullptr;

    QTimer* m_autoSaveTimer = nullptr;
    QDateTime m_lastSaveTime;
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
    void contextMenuEvent(QContextMenuEvent* event);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void leaveEvent(QEvent* e) override;

private:
    TextEditor *m_editor;
    int m_hoveredLine = -1;
};

}
#endif // TEXT_EDITOR_H
