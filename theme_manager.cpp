#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QSettings>
#include <QFont>
#include "theme_manager.h"

namespace SprintIDE {

class ThemeManager::Private
{
public:
    ThemeType currentTheme = ThemeType::Dark;
    QString customThemePath;
    QMap<UIElement, QColor> colors;
    QFont editorFont;

    // 默认主题映射
    void setupLightTheme() {
        colors[UIElement::MainWindowBackground] = QColor("#F8F9FA");
        colors[UIElement::EditorBackground] = QColor("#FFFFFF");
        colors[UIElement::TextColor] = QColor("#333333");
        colors[UIElement::KeywordColor] = QColor("#569CD6");
        colors[UIElement::VariableColor] = QColor("#7D5BBF");
        colors[UIElement::FunctionColor] = QColor("#795E26");
        colors[UIElement::CommentColor] = QColor("#008000");
        colors[UIElement::ErrorColor] = QColor("#FF0000");
        colors[UIElement::WarningColor] = QColor("#FF8C00");
        colors[UIElement::SelectionBackground] = QColor("#D3E9F7");
        colors[UIElement::MenuBackground] = QColor("#FFFFFF");
        colors[UIElement::ButtonBackground] = QColor("#E9ECEF");
    }

    void setupDarkTheme() {
        colors[UIElement::MainWindowBackground] = QColor("#2D2D2D");
        colors[UIElement::EditorBackground] = QColor("#1E1E1E");
        colors[UIElement::TextColor] = QColor("#E0E0E0");
        colors[UIElement::KeywordColor] = QColor("#569CD6");
        colors[UIElement::VariableColor] = QColor("#9CDCFE");
        colors[UIElement::FunctionColor] = QColor("#DCDCAA");
        colors[UIElement::CommentColor] = QColor("#6A9955");
        colors[UIElement::ErrorColor] = QColor("#F44747");
        colors[UIElement::WarningColor] = QColor("#FFB563");
        colors[UIElement::SelectionBackground] = QColor("#3A3D41");
        colors[UIElement::MenuBackground] = QColor("#3C3C3C");
        colors[UIElement::ButtonBackground] = QColor("#4A4A4A");
    }
};

ThemeManager& ThemeManager::instance()
{
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager()
    : d(new Private)
{
    // 设置默认字体
    d->editorFont = QFont("Monaco", 11);

    // 加载默认深色主题
    setTheme(ThemeType::Dark);

    // qDebug() << "ThemeManager initialized with" << currentThemeName() << "theme";
}

ThemeManager::~ThemeManager()
{
    delete d;
}

void ThemeManager::setTheme(ThemeType theme)
{
    if (d->currentTheme == theme && theme != ThemeType::Custom) {
        return;
    }

    d->currentTheme = theme;

    switch (theme) {
    case ThemeType::Light:
        d->setupLightTheme();
        break;
    case ThemeType::Dark:
        d->setupDarkTheme();
        break;
    case ThemeType::Custom:
        if (!d->customThemePath.isEmpty()) {
            loadTheme(d->customThemePath);
        } else {
            qWarning() << "Custom theme path not set, falling back to dark theme";
            d->setupDarkTheme();
        }
        break;
    }

    applyThemeToApplication();
    emit themeChanged(theme);
}

void ThemeManager::setCustomTheme(const QString& themeFilePath)
{
    d->customThemePath = themeFilePath;
    setTheme(ThemeType::Custom);
}

void ThemeManager::loadTheme(const QString& themeFilePath)
{
    QFile file(themeFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open theme file:" << themeFilePath;
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        qWarning() << "Invalid theme JSON file:" << themeFilePath;
        return;
    }

    QJsonObject themeObj = doc.object();

    // 解析颜色
    QJsonObject colorsObj = themeObj["colors"].toObject();
    for (auto it = colorsObj.begin(); it != colorsObj.end(); ++it) {
        QString key = it.key();
        QString value = it.value().toString();

        // 将字符串映射到UIElement（简化实现）
        if (key == "mainWindowBackground") d->colors[UIElement::MainWindowBackground] = QColor(value);
        else if (key == "editorBackground") d->colors[UIElement::EditorBackground] = QColor(value);
        else if (key == "textColor") d->colors[UIElement::TextColor] = QColor(value);
        else if (key == "keywordColor") d->colors[UIElement::KeywordColor] = QColor(value);
        else if (key == "variableColor") d->colors[UIElement::VariableColor] = QColor(value);
        else if (key == "functionColor") d->colors[UIElement::FunctionColor] = QColor(value);
        else if (key == "commentColor") d->colors[UIElement::CommentColor] = QColor(value);
        else if (key == "errorColor") d->colors[UIElement::ErrorColor] = QColor(value);
        else if (key == "warningColor") d->colors[UIElement::WarningColor] = QColor(value);
        else if (key == "selectionBackground") d->colors[UIElement::SelectionBackground] = QColor(value);
        else if (key == "menuBackground") d->colors[UIElement::MenuBackground] = QColor(value);
        else if (key == "buttonBackground") d->colors[UIElement::ButtonBackground] = QColor(value);
    }

    // 解析字体
    if (themeObj.contains("font")) {
        QJsonObject fontObj = themeObj["font"].toObject();
        QString family = fontObj["family"].toString("Monaco");
        int size = fontObj["size"].toInt(11);
        d->editorFont = QFont(family, size);
    }

    d->customThemePath = themeFilePath;
    d->currentTheme = ThemeType::Custom;

    applyThemeToApplication();
    emit themeChanged(ThemeType::Custom);
}

void ThemeManager::saveTheme(const QString& themeFilePath)
{
    QJsonObject themeObj;
    QJsonObject colorsObj;

    // 保存颜色
    colorsObj["mainWindowBackground"] = d->colors[UIElement::MainWindowBackground].name();
    colorsObj["editorBackground"] = d->colors[UIElement::EditorBackground].name();
    colorsObj["textColor"] = d->colors[UIElement::TextColor].name();
    colorsObj["keywordColor"] = d->colors[UIElement::KeywordColor].name();
    colorsObj["variableColor"] = d->colors[UIElement::VariableColor].name();
    colorsObj["functionColor"] = d->colors[UIElement::FunctionColor].name();
    colorsObj["commentColor"] = d->colors[UIElement::CommentColor].name();
    colorsObj["errorColor"] = d->colors[UIElement::ErrorColor].name();
    colorsObj["warningColor"] = d->colors[UIElement::WarningColor].name();
    colorsObj["selectionBackground"] = d->colors[UIElement::SelectionBackground].name();
    colorsObj["menuBackground"] = d->colors[UIElement::MenuBackground].name();
    colorsObj["buttonBackground"] = d->colors[UIElement::ButtonBackground].name();

    themeObj["colors"] = colorsObj;

    // 保存字体
    QJsonObject fontObj;
    fontObj["family"] = d->editorFont.family();
    fontObj["size"] = d->editorFont.pointSize();
    themeObj["font"] = fontObj;

    // 保存到文件
    QFile file(themeFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open theme file for writing:" << themeFilePath;
        return;
    }

    QJsonDocument doc(themeObj);
    file.write(doc.toJson());
    file.close();

    // qDebug() << "Theme saved to:" << themeFilePath;
}

QColor ThemeManager::color(UIElement element) const
{
    return d->colors.value(element, QColor(Qt::black));
}

QString ThemeManager::colorString(UIElement element) const
{
    return d->colors.value(element, QColor(Qt::black)).name();
}

ThemeManager::ThemeType ThemeManager::currentTheme() const
{
    return d->currentTheme;
}

QString ThemeManager::currentThemeName() const
{
    switch (d->currentTheme) {
    case ThemeType::Light: return "Light";
    case ThemeType::Dark: return "Dark";
    case ThemeType::Custom: return "Custom";
    }
    return "Unknown";
}

QStringList ThemeManager::availableThemes() const
{
    return {"Light", "Dark"};
}

void ThemeManager::setEditorFont(const QString& family, int size)
{
    d->editorFont = QFont(family, size);
    applyThemeToApplication();
    emit fontChanged(d->editorFont);
}

QFont ThemeManager::editorFont() const
{
    return d->editorFont;
}

void ThemeManager::applyThemeToWidget(QWidget* widget)
{
    if (!widget) return;

    QPalette palette = widget->palette();

    if (isDarkTheme()) {
        // 深色主题
        palette.setColor(QPalette::Window, d->colors[UIElement::MainWindowBackground]);
        palette.setColor(QPalette::WindowText, d->colors[UIElement::TextColor]);
        palette.setColor(QPalette::Base, d->colors[UIElement::EditorBackground]);
        palette.setColor(QPalette::Text, d->colors[UIElement::TextColor]);
        palette.setColor(QPalette::Button, d->colors[UIElement::ButtonBackground]);
        palette.setColor(QPalette::ButtonText, d->colors[UIElement::TextColor]);
    } else {
        // 浅色主题
        palette.setColor(QPalette::Window, QColor(240, 240, 240));
        palette.setColor(QPalette::WindowText, Qt::black);
        palette.setColor(QPalette::Base, Qt::white);
        palette.setColor(QPalette::Text, Qt::black);
        palette.setColor(QPalette::Button, QColor(240, 240, 240));
        palette.setColor(QPalette::ButtonText, Qt::black);
    }

    widget->setPalette(palette);
    widget->setAutoFillBackground(true);
}

void ThemeManager::applyThemeToTextEdit(QTextEdit* textEdit)
{
    if (!textEdit) return;

    QPalette palette = textEdit->palette();

    if (isDarkTheme()) {
        palette.setColor(QPalette::Base, d->colors[UIElement::EditorBackground]);
        palette.setColor(QPalette::Text, d->colors[UIElement::TextColor]);
        palette.setColor(QPalette::Highlight, d->colors[UIElement::SelectionBackground]);
        palette.setColor(QPalette::HighlightedText, d->colors[UIElement::TextColor]);
    } else {
        palette.setColor(QPalette::Base, Qt::white);
        palette.setColor(QPalette::Text, Qt::black);
        palette.setColor(QPalette::Highlight, QColor(173, 214, 255));
        palette.setColor(QPalette::HighlightedText, Qt::black);
    }

    textEdit->setPalette(palette);
}

void ThemeManager::applyThemeToPlainTextEdit(QPlainTextEdit* plainTextEdit)
{
    if (!plainTextEdit) return;

    QPalette palette = plainTextEdit->palette();

    if (isDarkTheme()) {
        palette.setColor(QPalette::Base, d->colors[UIElement::EditorBackground]);
        palette.setColor(QPalette::Text, d->colors[UIElement::TextColor]);
        palette.setColor(QPalette::Highlight, d->colors[UIElement::SelectionBackground]);
        palette.setColor(QPalette::HighlightedText, d->colors[UIElement::TextColor]);
    } else {
        palette.setColor(QPalette::Base, Qt::white);
        palette.setColor(QPalette::Text, Qt::black);
        palette.setColor(QPalette::Highlight, QColor(173, 214, 255));
        palette.setColor(QPalette::HighlightedText, Qt::black);
    }

    plainTextEdit->setPalette(palette);
}

void ThemeManager::applyThemeToTabBar(QTabBar* tabBar)
{
    if (!tabBar) return;

    QPalette palette = tabBar->palette();

    if (isDarkTheme()) {
        palette.setColor(QPalette::Window, d->colors[UIElement::MainWindowBackground]);
        palette.setColor(QPalette::WindowText, d->colors[UIElement::TextColor]);
        palette.setColor(QPalette::Button, d->colors[UIElement::ButtonBackground]);
        palette.setColor(QPalette::ButtonText, d->colors[UIElement::TextColor]);
        palette.setColor(QPalette::Highlight, d->colors[UIElement::SelectionBackground]);
        palette.setColor(QPalette::HighlightedText, d->colors[UIElement::TextColor]);
    } else {
        palette.setColor(QPalette::Window, QColor(240, 240, 240));
        palette.setColor(QPalette::WindowText, Qt::black);
        palette.setColor(QPalette::Button, QColor(240, 240, 240));
        palette.setColor(QPalette::ButtonText, Qt::black);
        palette.setColor(QPalette::Highlight, QColor(173, 214, 255));
        palette.setColor(QPalette::HighlightedText, Qt::black);
    }

    tabBar->setPalette(palette);
}

void ThemeManager::applyThemeToTreeView(QTreeView* treeView)
{
    if (!treeView) return;

    QPalette palette = treeView->palette();

    if (isDarkTheme()) {
        // 深色主题
        QColor bgColor = d->colors[UIElement::MainWindowBackground];
        QColor textColor = d->colors[UIElement::TextColor];
        QColor highlightColor = d->colors[UIElement::SelectionBackground];
        QColor alternateColor = bgColor.lighter(110);

        palette.setColor(QPalette::Base, bgColor);
        palette.setColor(QPalette::Text, textColor);
        palette.setColor(QPalette::Highlight, highlightColor);
        palette.setColor(QPalette::HighlightedText, textColor);
        palette.setColor(QPalette::Window, bgColor);
        palette.setColor(QPalette::WindowText, textColor);
        palette.setColor(QPalette::Button, bgColor.darker(110));
        palette.setColor(QPalette::ButtonText, textColor);
        palette.setColor(QPalette::AlternateBase, alternateColor);

        // 设置行交替颜色
        treeView->setAlternatingRowColors(false);
    } else {
        // 浅色主题
        QColor bgColor = QColor(240, 240, 240);
        QColor textColor = Qt::black;
        QColor highlightColor = QColor(173, 214, 255);
        QColor alternateColor = QColor(248, 248, 248);

        palette.setColor(QPalette::Base, Qt::white);
        palette.setColor(QPalette::Text, textColor);
        palette.setColor(QPalette::Highlight, highlightColor);
        palette.setColor(QPalette::HighlightedText, textColor);
        palette.setColor(QPalette::Window, bgColor);
        palette.setColor(QPalette::WindowText, textColor);
        palette.setColor(QPalette::Button, bgColor);
        palette.setColor(QPalette::ButtonText, textColor);
        palette.setColor(QPalette::AlternateBase, alternateColor);

        // 设置行交替颜色
        treeView->setAlternatingRowColors(false);
    }

    treeView->setPalette(palette);
    treeView->setAutoFillBackground(true);

    QColor hoverColor = isDarkTheme() ? QColor("#4A6A8A") : QColor(0, 0, 0, 20);

    // 设置样式表以确保颜色生效
    QString styleSheet = QString(
                             "QTreeView {"
                             "  background-color: %1;"
                             "  color: %2;"
                             "}"
                             "QTreeView::item:selected {"
                             "  background-color: %3;"
                             "  color: %4;"
                             "}"
                             "QTreeView::item:hover {"
                             "  background-color: %5;"
                             "}"
                             ).arg(
                                 palette.base().color().name(),
                                 palette.text().color().name(),
                                 palette.highlight().color().name(),
                                 palette.highlightedText().color().name(),
                                 hoverColor.name()
                                 );

    treeView->setStyleSheet(styleSheet);
    treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
}

bool ThemeManager::isDarkTheme() const
{
    return d->currentTheme == ThemeType::Dark;
}

void ThemeManager::applyThemeToApplication()
{
    QApplication* app = qobject_cast<QApplication*>(QApplication::instance());
    if (!app) return;

    // 应用字体
    app->setFont(d->editorFont);

    // 创建样式表
    QString styleSheet = QString(
                             "QMainWindow { background-color: %1; }"
                             "QPlainTextEdit, QTextEdit { background-color: %2; color: %3; selection-background-color: %4; }"
                             "QMenuBar { background-color: %5; }"
                             "QMenuBar::item { background-color: %5; }"
                             "QMenu { background-color: %5; }"
                             "QToolBar { background-color: %1; }"
                             "QDockWidget { background-color: %1; }"
                             "QPushButton { background-color: %6; }"
                             ).arg(
                                 d->colors[UIElement::MainWindowBackground].name(),
                                 d->colors[UIElement::EditorBackground].name(),
                                 d->colors[UIElement::TextColor].name(),
                                 d->colors[UIElement::SelectionBackground].name(),
                                 d->colors[UIElement::MenuBackground].name(),
                                 d->colors[UIElement::ButtonBackground].name()
                                 );

    app->setStyleSheet(styleSheet);

    // qDebug() << "Theme applied:" << currentThemeName();
}

} // namespace SprintIDE
