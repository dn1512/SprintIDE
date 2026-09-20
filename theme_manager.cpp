// ============================================================================
//  theme_manager.cpp —— SprintIDE Polaris 主题管理器（完整版）
//
//  与 theme_manager.h 中所有声明一一对应：
//    - 24 个 public/private 方法全部实现
//    - UIElement 扩展为 40+ 个 Polaris 语义令牌
//    - buildStyleSheet() 生成全局 QSS，替代散落的 setStyleSheet
//    - saveTheme/loadTheme 支持所有令牌的 JSON 序列化
// ============================================================================

#include "theme_manager.h"

#include <QApplication>
#include <QWidget>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QTreeView>
#include <QTabBar>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QSettings>
#include <QFont>
#include <QStyle>

namespace SprintIDE {

// ============================================================================
//  一、UIElement <-> 字符串名 映射（用于 JSON 序列化）
// ============================================================================
namespace {

using UIElement = SprintIDE::ThemeManager::UIElement;

struct ElementNameEntry {
    UIElement   element;
    const char* name;
};

static const ElementNameEntry kElementNames[] = {
    // 背景层级
    { UIElement::MainWindowBackground,   "MainWindowBackground"   },
    { UIElement::EditorBackground,       "EditorBackground"       },
    { UIElement::TreeBackground,         "TreeBackground"         },
    { UIElement::ActiveTabBackground,    "ActiveTabBackground"    },
    { UIElement::BorderColor,            "BorderColor"            },
    { UIElement::MenuBackground,         "MenuBackground"         },
    { UIElement::ButtonBackground,       "ButtonBackground"       },
    { UIElement::HoverBackground,        "HoverBackground"        },

    // 文字
    { UIElement::TextColor,              "TextColor"              },
    { UIElement::SecondaryTextColor,     "SecondaryTextColor"     },
    { UIElement::DisabledTextColor,      "DisabledTextColor"      },

    // 强调色
    { UIElement::PrimaryColor,           "PrimaryColor"           },
    { UIElement::SuccessColor,           "SuccessColor"           },
    { UIElement::WarningColor,           "WarningColor"           },
    { UIElement::ErrorColor,             "ErrorColor"             },
    { UIElement::AIColor,                "AIColor"                },
    { UIElement::RunButtonColor,         "RunButtonColor"         },

    // 编辑器语法
    { UIElement::KeywordColor,           "KeywordColor"           },
    { UIElement::ControlFlowColor,       "ControlFlowColor"       },
    { UIElement::ModifierColor,          "ModifierColor"          },
    { UIElement::ConstantColor,          "ConstantColor"          },
    { UIElement::StringColor,            "StringColor"            },
    { UIElement::NumberColor,            "NumberColor"            },
    { UIElement::CommentColor,           "CommentColor"           },
    { UIElement::FunctionColor,          "FunctionColor"          },
    { UIElement::PreprocessorColor,      "PreprocessorColor"      },
    { UIElement::ClassColor,             "ClassColor"             },
    { UIElement::VariableColor,          "VariableColor"          },

    // 编辑器装饰
    { UIElement::LineNumberColor,        "LineNumberColor"        },
    { UIElement::CurrentLineNumberColor, "CurrentLineNumberColor" },
    { UIElement::LineNumberAreaBackground, "LineNumberAreaBackground" },
    { UIElement::CurrentLineHighlight,   "CurrentLineHighlight"   },
    { UIElement::SelectionBackground,    "SelectionBackground"    },
    { UIElement::BreakpointColor,        "BreakpointColor"        },
    { UIElement::DebugLineColor,         "DebugLineColor"         },
    { UIElement::ErrorMarkerColor,       "ErrorMarkerColor"       },
    { UIElement::WarningMarkerColor,     "WarningMarkerColor"     },
    { UIElement::InlineValueColor,       "InlineValueColor"       },

    // 面板专用
    { UIElement::TreeHoverBackground,    "TreeHoverBackground"    },
    { UIElement::TreeSelectedBackground, "TreeSelectedBackground" },
    { UIElement::ScrollHandleColor,      "ScrollHandleColor"      },
    { UIElement::ScrollHandleHoverColor, "ScrollHandleHoverColor" },
    { UIElement::StatusBarBackground,    "StatusBarBackground"    },
    { UIElement::AIChatUserBubble,       "AIChatUserBubble"       },
    { UIElement::AIChatAIBubble,         "AIChatAIBubble"         },
    { UIElement::AIChatAIBorder,         "AIChatAIBorder"         },
    { UIElement::TerminalBackground,     "TerminalBackground"     },
    { UIElement::OutputPanelBackground,  "OutputPanelBackground"  },
    };

static QString elementToKey(UIElement e)
{
    for (const auto& entry : kElementNames) {
        if (entry.element == e) {
            return QString::fromLatin1(entry.name);
        }
    }
    return QString();
}

static bool keyToElement(const QString& key, UIElement& out)
{
    for (const auto& entry : kElementNames) {
        if (key == QLatin1String(entry.name)) {
            out = entry.element;
            return true;
        }
    }
    return false;
}

} // anonymous namespace

// ============================================================================
//  二、ThemeManager::Private —— Polaris 双主题定义
// ============================================================================
class ThemeManager::Private
{
public:
    ThemeType currentTheme = ThemeType::Dark;
    QString   customThemePath;
    QMap<UIElement, QColor> colors;
    QFont     editorFont;

    // ------------------------------------------------------------
    //  Polaris: Dark Nexus
    // ------------------------------------------------------------
    void setupDarkTheme()
    {
        // 背景层级
        colors[UIElement::MainWindowBackground]   = QColor("#212730");
        colors[UIElement::EditorBackground]       = QColor("#1A1F26");
        colors[UIElement::TreeBackground]         = QColor("#1A1F26");
        colors[UIElement::ActiveTabBackground]    = QColor("#2A3340");
        colors[UIElement::BorderColor]            = QColor("#2F3848");
        colors[UIElement::MenuBackground]         = QColor("#212730");
        colors[UIElement::ButtonBackground]       = QColor("#2F3848");
        colors[UIElement::HoverBackground]        = QColor("#2A3340");

        // 文字
        colors[UIElement::TextColor]              = QColor("#D4DCE8");
        colors[UIElement::SecondaryTextColor]     = QColor("#7A8599");
        colors[UIElement::DisabledTextColor]      = QColor("#4A5A70");

        // 强调色
        colors[UIElement::PrimaryColor]           = QColor("#4A9EFF");
        colors[UIElement::SuccessColor]           = QColor("#46C687");
        colors[UIElement::WarningColor]           = QColor("#EBB65B");
        colors[UIElement::ErrorColor]             = QColor("#E55B6B");
        colors[UIElement::AIColor]                = QColor("#2BC0BD");
        colors[UIElement::RunButtonColor]         = QColor("#F0A55A");

        // 编辑器语法（去紫）
        colors[UIElement::KeywordColor]           = QColor("#4AB0E6");
        colors[UIElement::ControlFlowColor]       = QColor("#EF7B7B");
        colors[UIElement::ModifierColor]          = QColor("#E6B07A");
        colors[UIElement::ConstantColor]          = QColor("#66D4E0");
        colors[UIElement::StringColor]            = QColor("#86C28C");
        colors[UIElement::NumberColor]            = QColor("#D4A06A");
        colors[UIElement::CommentColor]           = QColor("#5E6D82");
        colors[UIElement::FunctionColor]          = QColor("#F0C57A");
        colors[UIElement::PreprocessorColor]      = QColor("#9BB5D4");
        colors[UIElement::ClassColor]             = QColor("#4AB0E6");
        colors[UIElement::VariableColor]          = QColor("#D4DCE8");

        // 编辑器装饰
        colors[UIElement::LineNumberAreaBackground] = QColor("#1A1F26");
        colors[UIElement::LineNumberColor]        = QColor("#6B7A92");
        colors[UIElement::CurrentLineNumberColor] = QColor("#D4DCE8");
        colors[UIElement::CurrentLineHighlight]   = QColor("#2A3340");
        colors[UIElement::SelectionBackground]    = QColor("#2A3340");
        colors[UIElement::BreakpointColor]        = QColor("#E55B6B");
        colors[UIElement::DebugLineColor]         = QColor(43, 192, 189, 38);
        colors[UIElement::ErrorMarkerColor]       = QColor("#E55B6B");
        colors[UIElement::WarningMarkerColor]     = QColor("#EBB65B");
        colors[UIElement::InlineValueColor]       = QColor("#8FA0B8");

        // 面板
        colors[UIElement::TreeHoverBackground]    = QColor("#2A3340");
        colors[UIElement::TreeSelectedBackground] = QColor("#2A3340");
        colors[UIElement::ScrollHandleColor]      = QColor(74, 158, 255, 90);
        colors[UIElement::ScrollHandleHoverColor] = QColor(74, 158, 255, 180);
        colors[UIElement::StatusBarBackground]    = QColor("#212730");
        colors[UIElement::AIChatUserBubble]       = QColor("#2A3340");
        colors[UIElement::AIChatAIBubble]         = QColor("#1E2A32");
        colors[UIElement::AIChatAIBorder]         = QColor("#2BC0BD");
        colors[UIElement::TerminalBackground]     = QColor("#1A1F26");
        colors[UIElement::OutputPanelBackground]  = QColor("#1A1F26");
    }

    // ------------------------------------------------------------
    //  Polaris: Solar Flare
    // ------------------------------------------------------------
    void setupLightTheme()
    {
        // ============================================================
        //  Solar Flare 3.0 —— Mint Milk（薄荷牛奶）
        //
        //  设计哲学：
        //    • 编辑器是"牛奶"——白得有温度，带一丝薄荷，不刺眼
        //    • 工具区是"薄荷"——比编辑器更青更深，形成层次
        //    • 色相轴：H 150° → H 170°，逐级过渡
        //    • 饱和度控制在 16~29%（低饱和，"是色不是艳"）
        // ============================================================

        // ------------------------------------------------------------
        //  编辑器层：牛奶白带薄荷
        // ------------------------------------------------------------
        colors[UIElement::EditorBackground]       = QColor("#F3FAF8");   // 薄荷牛奶
        colors[UIElement::MenuBackground]         = QColor("#F3FAF8");

        // ------------------------------------------------------------
        //  树 / 列表层：比编辑器明显一点薄荷
        //  —— 这是你要求的"project_panel 带一点颜色"
        // ------------------------------------------------------------
        colors[UIElement::TreeBackground]         = QColor("#E8F2EF");   // 浅薄荷奶油

        // ------------------------------------------------------------
        //  工具窗层：中薄荷青
        // ------------------------------------------------------------
        colors[UIElement::MainWindowBackground]   = QColor("#CAD8CF");
        colors[UIElement::StatusBarBackground]    = QColor("#CAD8CF");
        colors[UIElement::ButtonBackground]       = QColor("#D8E4E1");

        // ------------------------------------------------------------
        //  高亮层：深薄荷青
        // ------------------------------------------------------------
        colors[UIElement::ActiveTabBackground]    = QColor("#CDDDD8");
        colors[UIElement::HoverBackground]        = QColor("#CDDDD8");
        colors[UIElement::TreeHoverBackground]    = QColor("#CDDDD8");

        // ------------------------------------------------------------
        //  边框：薄荷青，清晰但柔和
        // ------------------------------------------------------------
        colors[UIElement::BorderColor]            = QColor("#B9CCC6");

        // ------------------------------------------------------------
        //  文字：青黑 —— 与整体薄荷家族同源
        // ------------------------------------------------------------
        colors[UIElement::TextColor]              = QColor("#1A2B28");   // 青黑
        colors[UIElement::SecondaryTextColor]     = QColor("#556661");   // 青灰
        colors[UIElement::DisabledTextColor]      = QColor("#93A3A0");   // 浅青灰

        // ------------------------------------------------------------
        //  强调色
        // ------------------------------------------------------------
        colors[UIElement::PrimaryColor]           = QColor("#0D6EFD");
        colors[UIElement::SuccessColor]           = QColor("#2FA668");
        colors[UIElement::WarningColor]           = QColor("#D18A28");
        colors[UIElement::ErrorColor]             = QColor("#D94858");
        colors[UIElement::AIColor]                = QColor("#159E9B");   // 与主题同源
        colors[UIElement::RunButtonColor]         = QColor("#DD8438");

        // ------------------------------------------------------------
        //  语法高亮：暖色系与冷青背景形成对比
        // ------------------------------------------------------------
        colors[UIElement::KeywordColor]           = QColor("#0D6EFD");
        colors[UIElement::ControlFlowColor]       = QColor("#C0392B");
        colors[UIElement::ModifierColor]          = QColor("#B8770E");
        colors[UIElement::ConstantColor]          = QColor("#0E8B8B");
        colors[UIElement::StringColor]            = QColor("#3E8E4E");
        colors[UIElement::NumberColor]            = QColor("#A8721E");
        colors[UIElement::CommentColor]           = QColor("#7C8B88");   // 青灰
        colors[UIElement::FunctionColor]          = QColor("#B8770E");
        colors[UIElement::PreprocessorColor]      = QColor("#5E7A96");
        colors[UIElement::ClassColor]             = QColor("#0D6EFD");
        colors[UIElement::VariableColor]          = QColor("#1A2B28");

        // ------------------------------------------------------------
        //  编辑器装饰
        // ------------------------------------------------------------
        colors[UIElement::LineNumberAreaBackground] = QColor("#EDF4F2");
        colors[UIElement::LineNumberColor]        = QColor("#7C8B88");
        colors[UIElement::CurrentLineNumberColor] = QColor("#1A2B28");
        colors[UIElement::CurrentLineHighlight]   = QColor("#ECF3F1");   // 极浅薄荷
        colors[UIElement::SelectionBackground]    = QColor("#B8D5DB");   // 冰蓝
        colors[UIElement::BreakpointColor]        = QColor("#D94858");
        colors[UIElement::DebugLineColor]         = QColor(21, 158, 155, 38);
        colors[UIElement::ErrorMarkerColor]       = QColor("#D94858");
        colors[UIElement::WarningMarkerColor]     = QColor("#D18A28");
        colors[UIElement::InlineValueColor]       = QColor("#556661");

        // ------------------------------------------------------------
        //  面板
        // ------------------------------------------------------------
        colors[UIElement::TreeSelectedBackground] = QColor("#B8D5DB");
        colors[UIElement::ScrollHandleColor]      = QColor(13, 110, 253, 90);
        colors[UIElement::ScrollHandleHoverColor] = QColor(13, 110, 253, 180);
        colors[UIElement::AIChatUserBubble]       = QColor("#D8E4E1");
        colors[UIElement::AIChatAIBubble]         = QColor("#D9E8E5");
        colors[UIElement::AIChatAIBorder]         = QColor("#159E9B");
        colors[UIElement::TerminalBackground]     = QColor("#EBF3F0");   // 与树一致
        colors[UIElement::OutputPanelBackground]  = QColor("#EBF3F0");
    }
};

// ============================================================================
//  三、构造 / 析构 / 单例
// ============================================================================
ThemeManager& ThemeManager::instance()
{
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager()
    : d(new Private)
{
    d->editorFont = QFont("Monaco", 11);

    //   关键修复：构造函数里绕过 setTheme() 的"状态未变就返回"短路，
    //   直接填色 + 应用 QSS。否则 ThemeType 默认就是 Dark，
    //   首次 setTheme(Dark) 会直接 return，QSS 永远不会被应用。
    d->setupDarkTheme();
    d->currentTheme = ThemeType::Dark;

    if (QApplication::instance()) {
        applyThemeToApplication();
    }
}

ThemeManager::~ThemeManager()
{
    delete d;
}

// ============================================================================
//  四、主题切换
// ============================================================================
void ThemeManager::setTheme(ThemeType theme)
{
    // 非 Custom 主题且状态未变，直接返回
    if (d->currentTheme == theme && theme != ThemeType::Custom) {
        return;
    }

    switch (theme) {
    case ThemeType::Light:
        d->currentTheme = ThemeType::Light;
        d->setupLightTheme();
        break;

    case ThemeType::Dark:
        d->currentTheme = ThemeType::Dark;
        d->setupDarkTheme();
        break;

    case ThemeType::Custom:
        if (!d->customThemePath.isEmpty()) {
            // loadTheme() 内部会：
            //   1) 设置 currentTheme = Custom
            //   2) 应用 QSS
            //   3) 发出 themeChanged / themeApplied / editorThemeChanged
            loadTheme(d->customThemePath);
            return;
        }
        qWarning() << "Custom theme path not set, falling back to dark";
        d->currentTheme = ThemeType::Dark;
        d->setupDarkTheme();
        break;
    }

    applyThemeToApplication();
    emit themeChanged(theme);
    emit themeApplied(theme);
    emit editorThemeChanged(isDarkTheme());
}

void ThemeManager::setCustomTheme(const QString& themeFilePath)
{
    d->customThemePath = themeFilePath;
    setTheme(ThemeType::Custom);
}

// ============================================================================
//  五、自定义主题的读写
// ============================================================================
void ThemeManager::saveTheme(const QString& themeFilePath)
{
    QJsonObject root;
    QJsonObject colorsObj;

    // 序列化所有颜色令牌
    for (const auto& entry : kElementNames) {
        const UIElement e = entry.element;
        if (d->colors.contains(e)) {
            colorsObj[QString::fromLatin1(entry.name)] =
                d->colors.value(e).name(QColor::HexRgb);
        }
    }
    root["colors"] = colorsObj;

    // 字体
    QJsonObject fontObj;
    fontObj["family"] = d->editorFont.family();
    fontObj["size"]   = d->editorFont.pointSize();
    root["font"]      = fontObj;

    // 写入文件
    QFile file(themeFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "ThemeManager::saveTheme: cannot open"
                   << themeFilePath << "for writing";
        return;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
}

void ThemeManager::loadTheme(const QString& themeFilePath)
{
    QFile file(themeFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "ThemeManager::loadTheme: cannot open" << themeFilePath;
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "ThemeManager::loadTheme: invalid JSON:"
                   << parseError.errorString();
        return;
    }

    loadThemeFromFile(themeFilePath);   // 复用私有实现

    // 上面的私有实现只做了文件读取的抽象；这里主动完成状态更新
    const QJsonObject root = doc.object();

    // 颜色
    const QJsonObject colorsObj = root.value("colors").toObject();
    for (auto it = colorsObj.constBegin(); it != colorsObj.constEnd(); ++it) {
        UIElement element;
        if (!keyToElement(it.key(), element)) {
            continue;   // 未知的键，跳过
        }
        const QColor c(it.value().toString());
        if (c.isValid()) {
            d->colors[element] = c;
        }
    }

    // 字体
    if (root.contains("font")) {
        const QJsonObject fontObj = root.value("font").toObject();
        const QString family = fontObj.value("family").toString(d->editorFont.family());
        const int     size   = fontObj.value("size").toInt(d->editorFont.pointSize());
        d->editorFont = QFont(family, size);
    }

    d->customThemePath = themeFilePath;
    d->currentTheme    = ThemeType::Custom;

    applyThemeToApplication();
    emit themeChanged(ThemeType::Custom);
    emit themeApplied(ThemeType::Custom);
    emit editorThemeChanged(isDarkTheme());
    emit fontChanged(d->editorFont);
}

// ============================================================================
//  六、颜色查询
// ============================================================================
QColor ThemeManager::color(UIElement element) const
{
    return d->colors.value(element, QColor(Qt::black));
}

QString ThemeManager::colorString(UIElement element) const
{
    return d->colors.value(element, QColor(Qt::black)).name();
}

QString ThemeManager::rgba(UIElement element, int alpha) const
{
    QColor c = d->colors.value(element, QColor(Qt::black));
    c.setAlpha(qBound(0, alpha, 255));
    return QString("rgba(%1,%2,%3,%4)")
        .arg(c.red())
        .arg(c.green())
        .arg(c.blue())
        .arg(c.alpha());
}

// ============================================================================
//  七、主题信息查询
// ============================================================================
ThemeManager::ThemeType ThemeManager::currentTheme() const
{
    return d->currentTheme;
}

QString ThemeManager::currentThemeName() const
{
    switch (d->currentTheme) {
    case ThemeType::Light:  return QStringLiteral("Light");
    case ThemeType::Dark:   return QStringLiteral("Dark");
    case ThemeType::Custom: return QStringLiteral("Custom");
    }
    return QStringLiteral("Unknown");
}

QStringList ThemeManager::availableThemes() const
{
    return { QStringLiteral("Light"), QStringLiteral("Dark") };
}

bool ThemeManager::isDarkTheme() const
{
    // Custom 主题下按背景亮度判断
    if (d->currentTheme == ThemeType::Custom) {
        const QColor bg = d->colors.value(
            UIElement::EditorBackground, QColor(Qt::black));
        return bg.lightness() < 128;
    }
    return d->currentTheme == ThemeType::Dark;
}

// ---------------------------------------------------------------------------
//  设置字号 —— 只改字号，字体族保持不变
// ---------------------------------------------------------------------------
void ThemeManager::setEditorFontSize(int pointSize)
{
    // 范围硬约束，防止设置页传进来异常值
    pointSize = qBound(8, pointSize, 24);

    QFont f = d->editorFont;
    if (f.pointSize() == pointSize) return;

    f.setPointSize(pointSize);
    d->editorFont = f;

    applyThemeToApplication();
    emit fontChanged(d->editorFont);
}

int ThemeManager::editorFontSize() const
{
    return d->editorFont.pointSize();
}

// ============================================================================
//  八、字体
// ============================================================================
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

// ============================================================================
//  九、私有辅助
// ============================================================================
void ThemeManager::loadDefaultLightTheme()
{
    d->setupLightTheme();
}

void ThemeManager::loadDefaultDarkTheme()
{
    d->setupDarkTheme();
}

void ThemeManager::loadThemeFromFile(const QString& filePath)
{
    // 这里只做"是否存在"的前置检查；真正的解析在 loadTheme() 中完成
    if (!QFile::exists(filePath)) {
        qWarning() << "ThemeManager::loadThemeFromFile: file not found:" << filePath;
    }
}

// ============================================================================
//  十、全局 QSS 生成 + 应用
// ============================================================================
QString ThemeManager::buildStyleSheet() const
{
    auto c = [this](UIElement e) { return color(e).name(); };
    auto a = [this](UIElement e, int alpha) { return this->rgba(e, alpha); };

    QString qss;

    // ---------- 全局 ----------
    qss += QString(
               "QWidget { background-color: %1; color: %2; "
               "         font-family: '%3'; font-size: %4pt; } "
               "QMainWindow { background-color: %1; } "
               "QMainWindow::separator { background-color: %5; width: 1px; height: 1px; } "
               )
               .arg(c(UIElement::MainWindowBackground))
               .arg(c(UIElement::TextColor))
               .arg(d->editorFont.family())
               .arg(d->editorFont.pointSize())
               .arg(c(UIElement::BorderColor));

    // ---------- 菜单栏 / 菜单 ----------
    qss += QString(
               "QMenuBar { background-color: %1; color: %2; border-bottom: 1px solid %3; } "
               "QMenuBar::item { padding: 6px 12px; background: transparent; } "
               "QMenuBar::item:selected { background-color: %4; color: %5; } "
               "QMenu { background-color: %1; color: %2; border: 1px solid %3; padding: 4px; } "
               "QMenu::item { padding: 6px 24px 6px 12px; border-radius: 4px; } "
               "QMenu::item:selected { background-color: %4; color: %5; } "
               "QMenu::separator { height: 1px; background: %3; margin: 4px 8px; } "
               )
               .arg(c(UIElement::MenuBackground))
               .arg(c(UIElement::TextColor))
               .arg(c(UIElement::BorderColor))
               .arg(c(UIElement::ActiveTabBackground))
               .arg(c(UIElement::PrimaryColor));

    // ---------- 工具栏 ----------
    qss += QString(
               // ToolBar：加大 spacing，让图标之间不再互相挤压
               "QToolBar {"
               "  background-color: %1;"
               "  border: none;"
               "  spacing: 6px;"                // 2px → 6px
               "  padding: 4px 6px;"            // 4px → 4px 6px（左右多一点）
               "} "
               // 分隔符：让左右有呼吸空间
               "QToolBar::separator {"
               "  background: %2;"
               "  width: 1px;"
               "  margin: 6px 8px;"             // 4px 6px → 6px 8px
               "} "
               // ToolButton：加 min-width/min-height 保证图标有最小展示空间
               "QToolButton {"
               "  background: transparent;"
               "  border: none;"
               "  border-radius: 4px;"
               "  padding: 4px;"
               "  min-width: 24px;"             //防止被压缩
               "  min-height: 24px;"
               "  color: %3;"                   // SVG 用 currentColor 时可跟随主题
               "} "
               "QToolButton:hover {"
               "  background-color: %4;"
               "} "
               "QToolButton:pressed {"
               "  background-color: %5;"
               "} "
               "QToolButton:checked {"
               "  background-color: %5;"
               "} "
               "QToolButton:disabled {"
               "  color: %6;"                    // 禁用态图标变灰
               "} "
               )
               .arg(c(UIElement::MainWindowBackground))   // %1
               .arg(c(UIElement::BorderColor))            // %2
               .arg(c(UIElement::TextColor))              // %3
               .arg(c(UIElement::HoverBackground))        // %4
               .arg(c(UIElement::ActiveTabBackground))    // %5
               .arg(c(UIElement::DisabledTextColor));     // %6

    // ---------- 按钮 ----------
    qss += QString(
               "QPushButton {"
               "  background-color: %1; color: %2;"
               "  border: 1px solid %3; border-radius: 4px;"
               "  padding: 5px 14px; min-height: 18px;"
               "} "
               "QPushButton:hover { background-color: %4; border-color: %5; } "
               "QPushButton:pressed { background-color: %6; } "
               "QPushButton:disabled { color: %7; background-color: %1; } "
               "QPushButton:default, QPushButton#sendButton {"
               "  background-color: %5; color: #FFFFFF; border-color: %5; font-weight: 600;"
               "} "
               "QPushButton#sendButton:hover { background-color: %8; } "
               )
               .arg(c(UIElement::ButtonBackground))
               .arg(c(UIElement::TextColor))
               .arg(c(UIElement::BorderColor))
               .arg(c(UIElement::HoverBackground))
               .arg(c(UIElement::PrimaryColor))
               .arg(c(UIElement::ActiveTabBackground))
               .arg(c(UIElement::DisabledTextColor))
               .arg(c(UIElement::AIColor));

    // ---------- 输入控件 ----------
    qss += QString(
               "QLineEdit, QTextEdit, QPlainTextEdit {"
               "  background-color: %1; color: %2;"
               "  border: 1px solid %3; border-radius: 4px;"
               "  padding: 4px 8px;"
               "  selection-background-color: %4; selection-color: %2;"
               "} "
               "QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {"
               "  border: 1px solid %5;"
               "} "
               )
               .arg(c(UIElement::EditorBackground))
               .arg(c(UIElement::TextColor))
               .arg(c(UIElement::BorderColor))
               .arg(c(UIElement::SelectionBackground))
               .arg(c(UIElement::PrimaryColor));

    // ---------- Dock 面板 ----------
    qss += QString(
               "QDockWidget { color: %1; } "
               "QDockWidget::title {"
               "  background-color: %2; color: %3;"
               "  padding: 6px 10px; text-align: left;"
               "  border-bottom: 1px solid %4;"
               "  font-weight: 600; font-size: 11px;"
               "} "
               "QDockWidget::close-button, QDockWidget::float-button {"
               "  background: transparent;"
               "  border: none;"
               "  padding: 2px;"
               "  icon-size: 12px;"
               "}"
               "QDockWidget::close-button:hover, QDockWidget::float-button:hover {"
               "  background-color: %5;"
               "  border-radius: 3px;"
               "}"
               "QDockWidget::close-button:hover, QDockWidget::float-button:hover {"
               "  background-color: %5; border-radius: 3px;"
               "} "
               )
               .arg(c(UIElement::TextColor))
               .arg(c(UIElement::MainWindowBackground))
               .arg(c(UIElement::SecondaryTextColor))
               .arg(c(UIElement::BorderColor))
               .arg(c(UIElement::HoverBackground));

    // ---------- 标签栏 ----------
    qss += QString(
               "QTabBar { background: transparent; } "
               "QTabBar::tab {"
               "  background: transparent; color: %1;"
               "  padding: 6px 14px; margin: 0;"
               "  border: none; border-left: 2px solid transparent;"
               "} "
               "QTabBar::tab:hover { color: %2; background-color: %3; } "
               "QTabBar::tab:selected {"
               "  color: %2; background-color: %4;"
               "  border-left: 2px solid %5;"
               "} "
               "QTabWidget::pane {"
               "  border: none; border-top: 1px solid %6; background-color: %4;"
               "} "
               "QTabWidget::tab-bar { left: 0; } "
               )
               .arg(c(UIElement::SecondaryTextColor))
               .arg(c(UIElement::TextColor))
               .arg(c(UIElement::HoverBackground))
               .arg(c(UIElement::ActiveTabBackground))
               .arg(c(UIElement::PrimaryColor))
               .arg(c(UIElement::BorderColor));

    // ---------- 底部 Tab（bottomPanel：输出/终端/调试）----------
    qss += QString(
               // pane 容器
               "QTabWidget#BottomTabWidget::pane {"
               "  border: none;"
               "  background-color: %1;"
               "} "
               // tab bar 本体（用独立的 objectName 精确命中，不用后代选择器）
               "QTabBar#BottomTabBar {"
               "  background-color: %2;"
               "  qproperty-drawBase: 0;"      // 去掉 Qt 默认画的底边
               "} "
               "QTabBar#BottomTabBar::tab {"
               "  background: transparent;"
               "  color: %3;"
               "  padding: 5px 16px;"
               "  margin: 0;"
               "  border: none;"
               "  border-top: 2px solid transparent;"
               "} "
               "QTabBar#BottomTabBar::tab:hover {"
               "  color: %4;"
               "  background-color: %5;"
               "} "
               "QTabBar#BottomTabBar::tab:selected {"
               "  color: %4;"
               "  background-color: %1;"
               "  border-top: 2px solid %6;"
               "} "
               )
               .arg(c(UIElement::EditorBackground))      // pane 背景
               .arg(c(UIElement::MainWindowBackground))  // tabBar 背景
               .arg(c(UIElement::SecondaryTextColor))    // 非活动 tab 文字
               .arg(c(UIElement::TextColor))             // 活动 tab 文字
               .arg(c(UIElement::HoverBackground))       // 悬停背景
               .arg(c(UIElement::PrimaryColor));         // 活动竖条色

    // ---------- 树 / 列表 ----------
    // ---------- 树 / 列表 ----------
    qss += QString(
               "QTreeView, QListView, QListWidget, QTreeWidget {"
               "  background-color: %1; color: %2;"
               "  border: none; outline: none;"
               "} "
               "QTreeView::item, QListView::item, QListWidget::item {"
               "  padding: 3px 6px; border: none;"
               "} "
               "QTreeView::item:hover, QListView::item:hover, QListWidget::item:hover {"
               "  background-color: %3;"
               "} "
               "QTreeView::item:selected, QListView::item:selected, QListWidget::item:selected {"
               "  background-color: %4; color: %2;"
               "} "
               "QHeaderView::section {"
               "  background-color: %1; color: %5;"
               "  border: none; border-bottom: 1px solid %6;"
               "  padding: 4px 8px;"
               "} "

               // 显式指定 branch 三角图标 macOS上必须这么做，因为branch 三角在 macOS 上默认不可见
               "QTreeView::branch:has-children:!has-siblings:closed,"
               "QTreeView::branch:closed:has-children:has-siblings {"
               "  border-image: none;"
               "  image: url(:/icons/icons/branch-closed.svg);"
               "}"
               "QTreeView::branch:open:has-children:!has-siblings,"
               "QTreeView::branch:open:has-children:has-siblings {"
               "  border-image: none;"
               "  image: url(:/icons/icons/branch-open.svg);"
               "}"
               )
               .arg(c(UIElement::TreeBackground))
               .arg(c(UIElement::TextColor))
               .arg(c(UIElement::TreeHoverBackground))
               .arg(c(UIElement::TreeSelectedBackground))
               .arg(c(UIElement::SecondaryTextColor))
               .arg(c(UIElement::BorderColor));

    // ---------- 文件夹展开/关闭 ----------
    {
        const QString closedIcon = isDarkTheme()
                                    ? ":/icons/icons/branch-closed-dark.svg"
                                    : ":/icons/icons/branch-closed-light.svg";
        const QString openIcon = isDarkTheme()
                                     ? ":/icons/icons/branch-open-dark.svg"
                                     : ":/icons/icons/branch-open-light.svg";

        qss += QString(
                   "QTreeView::branch:has-children:!has-siblings:closed,"
                   "QTreeView::branch:closed:has-children:has-siblings {"
                   "  border-image: none;"
                   "  image: url(%1);"
                   "}"
                   "QTreeView::branch:open:has-children:!has-siblings,"
                   "QTreeView::branch:open:has-children:has-siblings {"
                   "  border-image: none;"
                   "  image: url(%2);"
                   "}"
                   ).arg(closedIcon, openIcon);
    }

    // ---------- 滚动条 ----------
    qss += QString(
               "QScrollBar:vertical {"
               "  background: transparent; width: 6px; margin: 0;"
               "} "
               "QScrollBar::handle:vertical {"
               "  background: %1; min-height: 30px; border-radius: 3px;"
               "} "
               "QScrollBar::handle:vertical:hover { background: %2; } "
               "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; } "
               "QScrollBar:horizontal { background: transparent; height: 6px; margin: 0; } "
               "QScrollBar::handle:horizontal {"
               "  background: %1; min-width: 30px; border-radius: 3px;"
               "} "
               "QScrollBar::handle:horizontal:hover { background: %2; } "
               "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; } "
               )
               .arg(a(UIElement::ScrollHandleColor, 90))
               .arg(a(UIElement::ScrollHandleHoverColor, 180));

    // ---------- 状态栏 ----------
    qss += QString(
               "QStatusBar { background-color: %1; color: %2; border-top: 1px solid %3; } "
               "QStatusBar::item { border: none; } "
               "QStatusBar QLabel { color: %2; padding: 0 4px; } "
               )
               .arg(c(UIElement::StatusBarBackground))
               .arg(c(UIElement::SecondaryTextColor))
               .arg(c(UIElement::BorderColor));

    // ---------- 进度条 ----------
    qss += QString(
               "QProgressBar {"
               "  background-color: %1; border: 1px solid %2; border-radius: 3px;"
               "  text-align: center; color: %3; height: 8px;"
               "} "
               "QProgressBar::chunk { background-color: %4; border-radius: 2px; } "
               )
               .arg(c(UIElement::EditorBackground))
               .arg(c(UIElement::BorderColor))
               .arg(c(UIElement::TextColor))
               .arg(c(UIElement::PrimaryColor));

    // ---------- 复选框 ----------
    qss += QString(
               "QCheckBox { color: %1; spacing: 6px; } "
               "QCheckBox::indicator {"
               "  width: 14px; height: 14px;"
               "  border: 1px solid %2; border-radius: 3px;"
               "  background-color: %3;"
               "} "
               "QCheckBox::indicator:checked {"
               "  background-color: %4; border-color: %4;"
               "} "
               )
               .arg(c(UIElement::TextColor))
               .arg(c(UIElement::BorderColor))
               .arg(c(UIElement::EditorBackground))
               .arg(c(UIElement::PrimaryColor));

    // ---------- 下拉框 ----------
    qss += QString(
               "QComboBox {"
               "  background-color: %1; color: %2;"
               "  border: 1px solid %3; border-radius: 4px;"
               "  padding: 4px 8px; min-height: 18px;"
               "} "
               "QComboBox:hover { border-color: %4; } "
               "QComboBox::drop-down { border: none; width: 20px; } "
               "QComboBox QAbstractItemView {"
               "  background-color: %1; color: %2;"
               "  border: 1px solid %3;"
               "  selection-background-color: %4; selection-color: #FFFFFF;"
               "} "
               )
               .arg(c(UIElement::EditorBackground))
               .arg(c(UIElement::TextColor))
               .arg(c(UIElement::BorderColor))
               .arg(c(UIElement::PrimaryColor));

    // ---------- 分组框 ----------
    qss += QString(
               "QGroupBox {"
               "  color: %1; border: 1px solid %2; border-radius: 4px;"
               "  margin-top: 10px; padding-top: 8px; font-weight: 600;"
               "} "
               "QGroupBox::title {"
               "  subcontrol-origin: margin; subcontrol-position: top left;"
               "  left: 10px; padding: 0 4px; color: %3;"
               "} "
               )
               .arg(c(UIElement::TextColor))
               .arg(c(UIElement::BorderColor))
               .arg(c(UIElement::SecondaryTextColor));

    // ---------- 工具提示 ----------
    qss += QString(
               "QToolTip {"
               "  background-color: %1; color: %2;"
               "  border: 1px solid %3; padding: 4px 8px; border-radius: 4px;"
               "} "
               )
               .arg(c(UIElement::ActiveTabBackground))
               .arg(c(UIElement::TextColor))
               .arg(c(UIElement::BorderColor));

    // ---------- 编辑器 ----------
    qss += QString(
               "QPlainTextEdit#TextEditor, QTextEdit#TextEditor {"
               "  background-color: %1; color: %2;"
               "  border: none; padding: 4px 0;"
               "  selection-background-color: %3; selection-color: %2;"
               "} "
               )
               .arg(c(UIElement::EditorBackground))
               .arg(c(UIElement::TextColor))
               .arg(a(UIElement::SelectionBackground, 200));

    // ---------- 输出面板 ----------
    qss += QString(
               "QTextEdit#OutputPanel {"
               "  background-color: %1; color: %2; border: none;"
               "  font-family: 'Menlo', 'Consolas', monospace;"
               "} "
               )
               .arg(c(UIElement::OutputPanelBackground))
               .arg(c(UIElement::TextColor));

    // ---------- AI 面板 ----------
    qss += QString(
               // Dock 本体
               "QDockWidget#ChatPanel {"
               "  background-color: %1;"
               "  color: %2;"
               "} "
               "QDockWidget#ChatPanel::title {"
               "  background-color: %1;"
               "  color: %3;"
               "  padding: 8px 12px;"
               "  border-bottom: 1px solid %4;"
               "  font-weight: 600;"
               "} "
               // 工具栏
               "QToolBar#ChatToolbar {"
               "  background-color: %1;"
               "  border: none;"
               "  border-bottom: 1px solid %4;"
               "  padding: 2px 8px;"
               "  spacing: 4px;"
               "} "
               "QToolBar#ChatToolbar QPushButton {"
               "  background: transparent;"
               "  color: %3;"
               "  border: 1px solid %4;"
               "  border-radius: 4px;"
               "  padding: 2px 10px;"
               "  font-size: 11px;"
               "} "
               "QToolBar#ChatToolbar QPushButton:hover {"
               "  background-color: %5;"
               "  color: %2;"
               "} "
               // 消息区
               "QTextEdit#messageDisplay {"
               "  background-color: %1;"
               "  color: %2;"
               "  border: none;"
               "  padding: 0;"
               "  font-size: 13px;"
               "} "
               // 输入区框架
               "QFrame#inputFrame {"
               "  background-color: %1;"
               "  border-top: 1px solid %4;"
               "} "
               // 输入框
               "QTextEdit#inputEdit {"
               "  background-color: %6;"
               "  color: %2;"
               "  border: 1px solid %4;"
               "  border-radius: 6px;"
               "  padding: 6px 10px;"
               "  font-size: 13px;"
               "} "
               "QTextEdit#inputEdit:focus {"
               "  border: 1px solid %7;"        // AI 极光青
               "} "
               // 发送按钮
               "QPushButton#sendButton {"
               "  background-color: %7;"
               "  color: #FFFFFF;"
               "  border: none;"
               "  border-radius: 6px;"
               "  font-weight: 600;"
               "  font-size: 12px;"
               "} "
               "QPushButton#sendButton:hover {"
               "  background-color: %8;"
               "} "
               "QPushButton#sendButton:disabled {"
               "  background-color: %4;"
               "  color: %9;"
               "} "
               // 状态标签
               "QLabel#statusLabel {"
               "  color: %9;"
               "  font-size: 11px;"
               "  padding: 0 4px;"
               "} "
               "QLabel#statusLabel.connected {"
               "  color: %7;"
               "} "
               "QLabel#statusLabel.disconnected {"
               "  color: %10;"
               "} "
               )
               .arg(c(UIElement::MainWindowBackground))     // %1
               .arg(c(UIElement::TextColor))                // %2
               .arg(c(UIElement::SecondaryTextColor))       // %3
               .arg(c(UIElement::BorderColor))              // %4
               .arg(c(UIElement::HoverBackground))          // %5
               .arg(c(UIElement::EditorBackground))         // %6
               .arg(c(UIElement::AIColor))                  // %7 = #2BC0BD
               .arg(c(UIElement::SuccessColor))             // %8
               .arg(c(UIElement::DisabledTextColor))        // %9
               .arg(c(UIElement::ErrorColor));              // %10

    // ---------- 终端 ----------
    qss += QString(
               "QWidget#TerminalPanel { background-color: %1; } "
               "QTextEdit#TerminalOutput {"
               "  background-color: %2;"
               "  color: %3;"
               "  border: 1px solid %4;"
               "  border-radius: 4px;"
               "  font-family: 'Menlo', 'Consolas', monospace;"
               "} "
               "QLineEdit#TerminalInput {"
               "  background-color: %2;"
               "  color: %3;"
               "  border: 1px solid %4;"
               "  border-radius: 4px;"
               "  padding: 4px 8px;"
               "  font-family: 'Menlo', 'Consolas', monospace;"
               "} "
               "QLineEdit#TerminalInput:focus { border: 1px solid %5; } "
               "QLabel#TerminalPrompt {"
               "  color: %6;"                                   // 绿色提示符
               "  font-weight: bold;"
               "  padding: 2px 4px;"
               "} "
               "QPushButton#TerminalStopButton {"
               "  background-color: %7; color: #FFFFFF;"
               "  border: none; border-radius: 3px;"
               "} "
               "QPushButton#TerminalStopButton:hover {"
               "  background-color: %8;"
               "} "
               "QPushButton#TerminalStopButton:disabled {"
               "  background-color: %4; color: %9;"
               "} "
               )
               .arg(c(UIElement::MainWindowBackground))
               .arg(c(UIElement::TerminalBackground))
               .arg(c(UIElement::TextColor))
               .arg(c(UIElement::BorderColor))
               .arg(c(UIElement::PrimaryColor))
               .arg(c(UIElement::SuccessColor))
               .arg(c(UIElement::ErrorColor))
               .arg(c(UIElement::WarningColor))
               .arg(c(UIElement::DisabledTextColor));

    // ---------- 调试面板 ----------
    qss += QString(
               "QWidget#DebugPanel { background-color: %1; } "
               "QWidget#DebugPanelSection { background-color: %1; } "
               "QLabel#DebugSectionLabel {"
               "  color: %2; font-weight: 600; font-size: 11px;"
               "  padding: 2px 4px;"
               "} "
               "QTreeWidget#CallStackTree, QTreeWidget#VariableTree {"
               "  background-color: %3;"
               "  color: %4;"
               "  border: 1px solid %5;"
               "  border-radius: 4px;"
               "} "
               "QTreeWidget#CallStackTree::item:selected,"
               "QTreeWidget#VariableTree::item:selected {"
               "  background-color: %6;"
               "  color: %4;"
               "} "
               "QTreeWidget#CallStackTree::item:hover,"
               "QTreeWidget#VariableTree::item:hover {"
               "  background-color: %7;"
               "} "
               "QTreeWidget#CallStackTree QHeaderView::section,"
               "QTreeWidget#VariableTree QHeaderView::section {"
               "  background-color: %1;"
               "  color: %2;"
               "  border: none; border-bottom: 1px solid %5;"
               "  padding: 4px 8px;"
               "} "
               "QTextEdit#DebugConsole {"
               "  background-color: %3;"
               "  color: %4;"
               "  border: 1px solid %5;"
               "  border-radius: 4px;"
               "  font-family: 'Menlo', 'Consolas', monospace;"
               "} "
               "QToolBar#DebugControlBar {"
               "  background-color: %1;"
               "  border: none;"
               "  padding: 4px;"
               "  spacing: 2px;"
               "} "
               )
               .arg(c(UIElement::MainWindowBackground))
               .arg(c(UIElement::SecondaryTextColor))
               .arg(c(UIElement::EditorBackground))
               .arg(c(UIElement::TextColor))
               .arg(c(UIElement::BorderColor))
               .arg(c(UIElement::TreeSelectedBackground))
               .arg(c(UIElement::TreeHoverBackground));

    // ---------- 调试面板 · 表达式求值区 ----------
    qss += QString(
               "QTextEdit#DebugEvalOutput {"
               "  background-color: %1;"
               "  color: %2;"
               "  border: 1px solid %3;"
               "  border-radius: 4px;"
               "  padding: 4px 8px;"
               "  font-family: 'Menlo', 'Consolas', monospace;"
               "  font-size: 11px;"
               "} "
               "QLineEdit#DebugEvalInput {"
               "  background-color: %1;"
               "  color: %2;"
               "  border: 1px solid %3;"
               "  border-radius: 4px;"
               "  padding: 4px 8px;"
               "  font-family: 'Menlo', 'Consolas', monospace;"
               "} "
               "QLineEdit#DebugEvalInput:focus {"
               "  border: 1px solid %4;"      // PrimaryColor 冰蓝
               "} "
               "QLineEdit#DebugEvalInput::placeholder {"
               "  color: %5;"                 // SecondaryTextColor
               "} "
               )
               .arg(c(UIElement::EditorBackground))       // %1
               .arg(c(UIElement::TextColor))              // %2
               .arg(c(UIElement::BorderColor))            // %3
               .arg(c(UIElement::PrimaryColor))           // %4
               .arg(c(UIElement::SecondaryTextColor));    // %5

    // ---------- 滚动区域 ----------
    qss += "QScrollArea { border: none; background: transparent; } ";

    return qss;
}

void ThemeManager::applyThemeToApplication()
{
    QApplication* app = qobject_cast<QApplication*>(QApplication::instance());
    if (!app) {
        return;
    }

    // 全局字体
    app->setFont(d->editorFont);

    // 全局 QSS
    app->setStyleSheet(buildStyleSheet());
}

// ============================================================================
//  十一、对单个控件的调色板应用（兼容旧接口）
// ============================================================================
void ThemeManager::applyThemeToWidget(QWidget* widget)
{
    if (!widget) return;

    QPalette palette = widget->palette();
    palette.setColor(QPalette::Window,
                     color(UIElement::MainWindowBackground));
    palette.setColor(QPalette::WindowText,
                     color(UIElement::TextColor));
    palette.setColor(QPalette::Base,
                     color(UIElement::EditorBackground));
    palette.setColor(QPalette::Text,
                     color(UIElement::TextColor));
    palette.setColor(QPalette::Button,
                     color(UIElement::ButtonBackground));
    palette.setColor(QPalette::ButtonText,
                     color(UIElement::TextColor));
    palette.setColor(QPalette::Highlight,
                     color(UIElement::SelectionBackground));
    palette.setColor(QPalette::HighlightedText,
                     color(UIElement::TextColor));

    widget->setPalette(palette);
    widget->setAutoFillBackground(true);
}

void ThemeManager::applyThemeToTextEdit(QTextEdit* textEdit)
{
    if (!textEdit) return;

    QPalette palette = textEdit->palette();
    palette.setColor(QPalette::Base,
                     color(UIElement::EditorBackground));
    palette.setColor(QPalette::Text,
                     color(UIElement::TextColor));
    palette.setColor(QPalette::Highlight,
                     color(UIElement::SelectionBackground));
    palette.setColor(QPalette::HighlightedText,
                     color(UIElement::TextColor));

    textEdit->setPalette(palette);
}

void ThemeManager::applyThemeToPlainTextEdit(QPlainTextEdit* plainTextEdit)
{
    if (!plainTextEdit) return;

    QPalette palette = plainTextEdit->palette();
    palette.setColor(QPalette::Base,
                     color(UIElement::EditorBackground));
    palette.setColor(QPalette::Text,
                     color(UIElement::TextColor));
    palette.setColor(QPalette::Highlight,
                     color(UIElement::SelectionBackground));
    palette.setColor(QPalette::HighlightedText,
                     color(UIElement::TextColor));

    plainTextEdit->setPalette(palette);
}

void ThemeManager::applyThemeToTabBar(QTabBar* tabBar)
{
    if (!tabBar) return;

    QPalette palette = tabBar->palette();
    palette.setColor(QPalette::Window,
                     color(UIElement::MainWindowBackground));
    palette.setColor(QPalette::WindowText,
                     color(UIElement::TextColor));
    palette.setColor(QPalette::Button,
                     color(UIElement::ButtonBackground));
    palette.setColor(QPalette::ButtonText,
                     color(UIElement::TextColor));
    palette.setColor(QPalette::Highlight,
                     color(UIElement::ActiveTabBackground));
    palette.setColor(QPalette::HighlightedText,
                     color(UIElement::TextColor));

    tabBar->setPalette(palette);
}

void ThemeManager::applyThemeToTreeView(QTreeView* treeView)
{
    if (!treeView) return;

    QPalette palette = treeView->palette();
    palette.setColor(QPalette::Base,
                     color(UIElement::EditorBackground));
    palette.setColor(QPalette::Text,
                     color(UIElement::TextColor));
    palette.setColor(QPalette::Window,
                     color(UIElement::MainWindowBackground));
    palette.setColor(QPalette::WindowText,
                     color(UIElement::TextColor));
    palette.setColor(QPalette::Highlight,
                     color(UIElement::TreeSelectedBackground));
    palette.setColor(QPalette::HighlightedText,
                     color(UIElement::TextColor));
    palette.setColor(QPalette::AlternateBase,
                     color(UIElement::EditorBackground));

    treeView->setPalette(palette);
    treeView->setAutoFillBackground(true);
    treeView->setAlternatingRowColors(false);

    // 覆盖一次样式表，确保选中/悬停色生效
    const QString styleSheet = QString(
                                   "QTreeView {"
                                   "  background-color: %1;"
                                   "  color: %2;"
                                   "  border: none;"
                                   "  outline: none;"
                                   "}"
                                   "QTreeView::item:selected {"
                                   "  background-color: %3;"
                                   "  color: %2;"
                                   "}"
                                   "QTreeView::item:hover {"
                                   "  background-color: %4;"
                                   "}"
                                   )
                                   .arg(colorString(UIElement::EditorBackground))
                                   .arg(colorString(UIElement::TextColor))
                                   .arg(colorString(UIElement::TreeSelectedBackground))
                                   .arg(colorString(UIElement::TreeHoverBackground));

    treeView->setStyleSheet(styleSheet);
    treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
}

} // namespace SprintIDE
