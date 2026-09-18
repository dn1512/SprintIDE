#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

#include <QFontDatabase>
#include <QString>
#include <QDebug>

class FontManager
{
public:
    static bool loadEmbeddedFonts();
    static QFont getDefaultMonoFont(int pointSize = 12);
    static bool isJetBrainsMonoAvailable();
    static QString primaryMonoFamily(); // 返回实际可用的等宽字体家族名
};

#endif // FONT_MANAGER_H
