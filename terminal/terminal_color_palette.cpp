#include "terminal_color_palette.h"
#include "theme_manager.h"   // 若编译报找不到，改成 "../theme_manager.h"

TerminalColorPalette TerminalColorPalette::fromTheme() {
    // 先用深色兜底填满，再逐个令牌覆盖——这样任何令牌漏配都不会
    // 导致 palette 里出现无效 QColor（QColor() 是 invalid，fillRect 会画透明）。
    TerminalColorPalette p = fallbackDark();

    auto& tm = ThemeManager::instance();
    auto get = [&tm](UIElement e, QColor& dst) {
        const QColor c = tm.color(e);
        if (c.isValid()) dst = c;
    };

    get(UIElement::TerminalBackground,        p.defaultBg);
    get(UIElement::TerminalForeground,        p.defaultFg);
    get(UIElement::TerminalCursor,            p.cursor);
    get(UIElement::TerminalSelection,         p.selection);
    get(UIElement::TerminalSelectionInactive, p.selectionInactive);
    get(UIElement::TerminalSearchMatch,       p.searchMatch);
    get(UIElement::TerminalSearchMatchActive, p.searchMatchActive);
    get(UIElement::TerminalLink,              p.link);

    get(UIElement::TerminalAnsiBlack,         p.ansi[0]);
    get(UIElement::TerminalAnsiRed,           p.ansi[1]);
    get(UIElement::TerminalAnsiGreen,         p.ansi[2]);
    get(UIElement::TerminalAnsiYellow,        p.ansi[3]);
    get(UIElement::TerminalAnsiBlue,          p.ansi[4]);
    get(UIElement::TerminalAnsiMagenta,       p.ansi[5]);
    get(UIElement::TerminalAnsiCyan,          p.ansi[6]);
    get(UIElement::TerminalAnsiWhite,         p.ansi[7]);
    get(UIElement::TerminalAnsiBrightBlack,   p.ansi[8]);
    get(UIElement::TerminalAnsiBrightRed,     p.ansi[9]);
    get(UIElement::TerminalAnsiBrightGreen,   p.ansi[10]);
    get(UIElement::TerminalAnsiBrightYellow,  p.ansi[11]);
    get(UIElement::TerminalAnsiBrightBlue,    p.ansi[12]);
    get(UIElement::TerminalAnsiBrightMagenta, p.ansi[13]);
    get(UIElement::TerminalAnsiBrightCyan,    p.ansi[14]);
    get(UIElement::TerminalAnsiBrightWhite,   p.ansi[15]);

    return p;
}

TerminalColorPalette TerminalColorPalette::fallbackDark() {
    // 深色主题兜底值（与 Dark Nexus 一致）。
    // 只在 ThemeManager 未就绪 / 令牌未配置时生效，正常运行不会用到。
    TerminalColorPalette p;
    p.defaultBg         = QColor("#1A1F26");
    p.defaultFg         = QColor("#D4DCE8");
    p.cursor            = QColor("#4A9EFF");
    p.selection         = QColor("#2A3340");
    p.selectionInactive = QColor(42, 51, 64, 120);
    p.searchMatch       = QColor(235, 182, 91, 100);
    p.searchMatchActive = QColor("#EBB65B");
    p.link              = QColor("#4A9EFF");

    p.ansi[0]  = QColor("#000000"); p.ansi[1]  = QColor("#CD3131");
    p.ansi[2]  = QColor("#0DBC79"); p.ansi[3]  = QColor("#E5E510");
    p.ansi[4]  = QColor("#2472C8"); p.ansi[5]  = QColor("#BC3FBC");
    p.ansi[6]  = QColor("#11A8CD"); p.ansi[7]  = QColor("#E5E5E5");
    p.ansi[8]  = QColor("#666666"); p.ansi[9]  = QColor("#F14C4C");
    p.ansi[10] = QColor("#23D18B"); p.ansi[11] = QColor("#F5F543");
    p.ansi[12] = QColor("#3B8EEA"); p.ansi[13] = QColor("#D670D6");
    p.ansi[14] = QColor("#29B8DB"); p.ansi[15] = QColor("#E5E5E5");
    return p;
}