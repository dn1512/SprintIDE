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

/**
 * @class ThemeManager
 * @brief 主题管理器，支持深色/浅色主题切换
 *
 * 对应设计文档5.1节：主题配色规范
 */
class ThemeManager : public QObject
{
    Q_OBJECT

public:
    // 主题类型枚举
    enum class ThemeType {
        Light,
        Dark,
        Custom
    };

    // 界面元素枚举（对应设计文档5.1节）
    enum class UIElement {
        MainWindowBackground,
        EditorBackground,
        TextColor,
        KeywordColor,
        VariableColor,
        FunctionColor,
        CommentColor,
        ErrorColor,
        WarningColor,
        SelectionBackground,
        MenuBackground,
        ButtonBackground
    };

    static ThemeManager& instance();

    // 主题管理
    void setTheme(ThemeType theme);
    void setCustomTheme(const QString& themeFilePath);
    void loadTheme(const QString& themeFilePath);
    void saveTheme(const QString& themeFilePath);

    // 颜色获取
    QColor color(UIElement element) const;
    QString colorString(UIElement element) const;

    // 主题信息
    ThemeType currentTheme() const;
    QString currentThemeName() const;
    QStringList availableThemes() const;

    // 字体设置
    void setEditorFont(const QString& family, int size);
    QFont editorFont() const;

    // 主题应用方法
    void applyThemeToWidget(QWidget* widget);
    void applyThemeToTextEdit(QTextEdit* textEdit);
    void applyThemeToPlainTextEdit(QPlainTextEdit* plainTextEdit);
    void applyThemeToTabBar(QTabBar* tabBar);
    void applyThemeToTreeView(QTreeView* treeView);

    // 添加当前主题状态访问器
    bool isDarkTheme() const ;

signals:
    void themeChanged(ThemeType newTheme);
    void colorChanged(UIElement element, const QColor& color);
    void fontChanged(const QFont& font);
    // 添加详细的主题变化信号
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

#endif // THEME_MANAGER_H
