// theme_manager.h —— 完整替换
#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include "text_editor.h"
#include <QObject>
#include <QString>
#include <QMap>
#include <QColor>
#include <QTabBar>
#include <QTreeView>

namespace SprintIDE {

class ThemeManager : public QObject
{
    Q_OBJECT

public:
    //  行距比例常量 用户不可改，代码里硬约束
    static constexpr int kEditorLineHeightPercent = 125;

    enum class ThemeType { Light, Dark, Custom };

    // Polaris 语义令牌（对应设计文档"基础调色板"+"强调色"）
    enum class UIElement {
        // ---- 背景层级 ----
        MainWindowBackground,   // 侧边栏/工具窗口  #212730
        EditorBackground,       // 编辑器核心      #1A1F26
        TreeBackground,         // 项目面板 / 树 / 列表背景
        ActiveTabBackground,    // 活动标签/选中行  #2A3340
        BorderColor,            // 分割线/边框     #2F3848
        MenuBackground,         // 菜单背景        #212730
        ButtonBackground,       // 按钮背景        #2F3848
        HoverBackground,        // 悬停态          #2A3340

        // ---- 文字 ----
        TextColor,              // 主要文字        #D4DCE8
        SecondaryTextColor,     // 次要文字        #7A8599
        DisabledTextColor,      // 禁用文字        #4A5A70

        // ---- 强调色 ----
        PrimaryColor,           // 品牌冰蓝        #4A9EFF
        SuccessColor,           // 运行/成功       #46C687
        WarningColor,           // 警告            #EBB65B
        ErrorColor,             // 错误/断点       #E55B6B
        AIColor,                // AI 极光青     #2BC0BD
        RunButtonColor,         // 运行主按钮琥珀   #F0A55A

        // ---- 编辑器语义 ----
        KeywordColor,           // 存储/类型关键字 #4AB0E6
        ControlFlowColor,       // 控制流          #EF7B7B
        ModifierColor,          // 修饰符          #E6B07A
        ConstantColor,          // 常量/布尔       #66D4E0
        StringColor,            // 字符串          #86C28C
        NumberColor,            // 数字            #D4A06A
        CommentColor,           // 注释            #5E6D82
        FunctionColor,          // 函数名          #F0C57A
        PreprocessorColor,      // 预处理/宏       #9BB5D4
        ClassColor,             // 类名            #4AB0E6
        VariableColor,          // 变量            #D4DCE8

        // ---- 编辑器装饰 ----
        LineNumberColor,        // 行号            #4A5A70
        CurrentLineNumberColor, // 当前行号        #D4DCE8
        LineNumberAreaBackground,  // 行号区背景
        CurrentLineHighlight,   // 当前行高亮      #2A3340
        SelectionBackground,    // 选区背景        #2A3340
        BreakpointColor,        // 断点红点        #E55B6B
        DebugLineColor,         // 调试当前行      rgba(43,192,189,0.15)
        ErrorMarkerColor,       // 错误波浪线      #E55B6B
        WarningMarkerColor,     // 警告波浪线      #EBB65B
        InlineValueColor,       // 内联变量值

        // ---- 面板专用 ----
        TreeHoverBackground,    // 树悬停          #2A3340
        TreeSelectedBackground, // 树选中          #2A3340
        ScrollHandleColor,      // 滚动条滑块      #4A9EFF (35%)
        ScrollHandleHoverColor, // 滚动条悬停      #4A9EFF (70%)
        StatusBarBackground,    // 状态栏          #212730
        AIChatUserBubble,       // 用户气泡        #2A3340
        AIChatAIBubble,         // AI 气泡         #1E2A32
        AIChatAIBorder,         // AI 左侧竖条      #2BC0BD
        TerminalBackground,     // 终端背景        #1A1F26
        OutputPanelBackground,  // 输出面板背景    #1A1F26

        // ---- Git 面板 ----
        GitOperationBannerBackground,  // merge/rebase 警告横幅背景
        GitOperationBannerBorder,      // 警告横幅边框
        GitOperationBannerText,        // 警告横幅文字（含按钮）
        // 查找高亮（普通匹配 / 当前匹配）
        SearchMatchBackground,
        SearchCurrentMatchBackground,

        // ---- 面包屑 / 大纲 ----
        BreadcrumbBarBackground,    // 面包屑栏背景（比编辑器深一档，区分感）
        OutlineOverlayBackground,   // 大纲浮层背景（半透明遮罩下的实际底色）
        OutlineOverlayBorder,       // 浮层边框
        OutlineOverlayDivider,      // 左右分栏分割线
        OutlineOverlaySectionTitle,   // 分栏标题文字（比 SecondaryTextColor 稍亮）
    };

    static ThemeManager& instance();

    void setTheme(ThemeType theme);
    void setCustomTheme(const QString& themeFilePath);
    void loadTheme(const QString& themeFilePath);
    void saveTheme(const QString& themeFilePath);

    QColor color(UIElement element) const;
    QString colorString(UIElement element) const;
    QString rgba(UIElement element, int alpha) const;

    ThemeType currentTheme() const;
    QString currentThemeName() const;
    QStringList availableThemes() const;

    // 单独设置字号（不改变字体族）
    void setEditorFontSize(int pointSize);
    int  editorFontSize() const;

    void setEditorFont(const QString& family, int size);
    QFont editorFont() const;

    // 生成完整 QSS（替代散落各处的 setStyleSheet）
    QString buildStyleSheet() const;

    void applyThemeToWidget(QWidget* widget);
    void applyThemeToTextEdit(QTextEdit* textEdit);
    void applyThemeToPlainTextEdit(QPlainTextEdit* plainTextEdit);
    void applyThemeToTabBar(QTabBar* tabBar);
    void applyThemeToTreeView(QTreeView* treeView);

    bool isDarkTheme() const;

signals:
    void themeChanged(ThemeType newTheme);
    void colorChanged(UIElement element, const QColor& color);
    void fontChanged(const QFont& font);
    void themeApplied(ThemeType theme);
    void editorThemeChanged(bool isDark);

private:
    ThemeManager();
    ~ThemeManager();

    void loadDefaultLightTheme();
    void loadDefaultDarkTheme();
    void loadThemeFromFile(const QString& filePath);
    void applyThemeToApplication();

    class Private;
    Private* d;
};

} // namespace SprintIDE
#endif
