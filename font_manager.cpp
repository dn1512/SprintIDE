#include "font_manager.h"
#include <QApplication>
#include <QFile>

bool FontManager::loadEmbeddedFonts()
{
    static bool fontsLoaded = false;

    if (fontsLoaded) {
        return true;
    }

    // 尝试加载内嵌的 JetBrains Mono 字体
    QStringList fontsToLoad = {
        ":/fonts/JetBrainsMono-Regular.ttf",
        ":/fonts/JetBrainsMono-Bold.ttf"
    };

    for (const QString& fontPath : fontsToLoad) {
        if (QFile::exists(fontPath)) {
            int fontId = QFontDatabase::addApplicationFont(fontPath);
            if (fontId == -1) {
                qWarning() << "Failed to load font:" << fontPath;
            } else {
                // qDebug() << "Successfully loaded font:" << fontPath;
            }
        } else {
            qWarning() << "Font file not found:" << fontPath;
        }
    }

    fontsLoaded = true;
    return true;
}

bool FontManager::isJetBrainsMonoAvailable()
{
    QFontDatabase fontDatabase;
    QStringList families = fontDatabase.families();

    // 检查 JetBrains Mono 字体是否可用
    for (const QString& family : families) {
        if (family.contains("JetBrains Mono", Qt::CaseInsensitive)) {
            return true;
        }
    }

    // 检查通过我们加载的字体
    families = QFontDatabase::applicationFontFamilies(0);
    for (const QString& family : families) {
        if (family.contains("JetBrains Mono", Qt::CaseInsensitive)) {
            return true;
        }
    }

    return false;
}

QFont FontManager::getDefaultMonoFont(int pointSize)
{
    loadEmbeddedFonts();

    QFontDatabase fontDatabase;
    QStringList families = fontDatabase.families();

    // 字体优先级列表
    QStringList preferredFonts = {
        "JetBrains Mono",  // 首选
        "Fira Code",       // 备选1
        "Cascadia Code",   // 备选2
        "Consolas",        // 备选3
        "Menlo",           // 备选4
        "DejaVu Sans Mono",// 备选5
        "Monospace"        // 最后的选择
    };

    // 寻找可用的字体
    QString selectedFont;
    for (const QString& preferred : preferredFonts) {
        for (const QString& family : families) {
            if (family.contains(preferred, Qt::CaseInsensitive)) {
                selectedFont = family;
                break;
            }
        }
        if (!selectedFont.isEmpty()) {
            break;
        }
    }

    // 如果没找到，使用默认等宽字体
    if (selectedFont.isEmpty()) {
        selectedFont = "Monospace";
    }

    // 创建字体
    QFont font(selectedFont, pointSize);
    font.setFixedPitch(true);
    font.setStyleHint(QFont::Monospace);

    // 设置字体特性
    font.setKerning(false);  // 禁用字距调整

// 对于编程字体，我们通常不需要抗锯齿
// 但可以根据用户喜好调整
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    font.setStyleStrategy(QFont::PreferDefault);
#else
    font.setStyleStrategy(QFont::PreferDefault);
#endif

    // qDebug() << "Selected mono font:" << selectedFont
    //          << "Size:" << pointSize;

    return font;
}
