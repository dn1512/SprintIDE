#include "font_manager.h"
#include <QApplication>
#include <QFile>
#include <QDebug>
#include <QFontDatabase>

// ============================================================
// 文件作用域：记录成功加载的内嵌等宽字体的家族名
// 用于 getDefaultMonoFont / primaryMonoFamily 跨函数访问
// ============================================================
namespace {
QString g_primaryMonoFamily;
}

// ============================================================
// loadEmbeddedFonts —— 从 qrc 加载内嵌 TTF（幂等）
// ============================================================
bool FontManager::loadEmbeddedFonts()
{
    static bool fontsLoaded = false;
    if (fontsLoaded) return true;

    const QStringList fontsToLoad = {
        ":/fonts/JetBrainsMono-Regular.ttf",
        ":/fonts/JetBrainsMono-Bold.ttf"
    };

    for (const QString& fontPath : fontsToLoad) {
        if (!QFile::exists(fontPath)) {
            qWarning() << "[FontManager] 字体文件不存在:" << fontPath;
            continue;
        }

        const int fontId = QFontDatabase::addApplicationFont(fontPath);
        if (fontId == -1) {
            qWarning() << "[FontManager] 字体加载失败:" << fontPath;
            continue;
        }

        // 记录第一个成功加载的家族名，作为 primaryMonoFamily() 的返回值
        if (g_primaryMonoFamily.isEmpty()) {
            const QStringList fams = QFontDatabase::applicationFontFamilies(fontId);
            if (!fams.isEmpty()) {
                g_primaryMonoFamily = fams.first();
                qDebug() << "[FontManager] 内嵌字体家族名:" << g_primaryMonoFamily;
            } else {
                qWarning() << "[FontManager] 字体加载成功但家族名为空:"
                           << fontPath;
            }
        }
    }

    fontsLoaded = true;
    return true;
}

// ============================================================
// isJetBrainsMonoAvailable —— 检查 JetBrains Mono 是否可用
// ============================================================
bool FontManager::isJetBrainsMonoAvailable()
{
    // 1. 优先检查我们内嵌加载的字体
    if (!g_primaryMonoFamily.isEmpty() &&
        g_primaryMonoFamily.contains("JetBrains Mono", Qt::CaseInsensitive)) {
        return true;
    }

    // 2. 检查系统安装的字体（QFontDatabase 的方法都是静态的）
    const QStringList families = QFontDatabase::families();
    for (const QString& family : families) {
        if (family.contains("JetBrains Mono", Qt::CaseInsensitive)) {
            return true;
        }
    }

    return false;
}

// ============================================================
// primaryMonoFamily —— 返回当前可用的等宽字体家族名
// ============================================================
QString FontManager::primaryMonoFamily()
{
    // 内嵌字体加载成功 用它的实际家族名（可能是 "JetBrains Mono"
    // 或 "JetBrains Mono Regular" 之类的变体，取 TTF 里的真实值）
    if (!g_primaryMonoFamily.isEmpty()) {
        return g_primaryMonoFamily;
    }

    // 回退到平台默认等宽字体
#if defined(Q_OS_WIN)
    return "Consolas";
#elif defined(Q_OS_MAC)
    return "Menlo";
#else
    return "DejaVu Sans Mono";
#endif
}

// ============================================================
// getDefaultMonoFont —— 构造等宽字体
// 优先级：
//   1. 内嵌加载的字体（JetBrains Mono）
//   2. 系统安装的首选字体列表
//   3. Qt 通用别名 "Monospace"
// ============================================================
QFont FontManager::getDefaultMonoFont(int pointSize)
{
    loadEmbeddedFonts();   // 幂等；确保 g_primaryMonoFamily 已被填充

    QString selectedFont;

    // ---- 1. 内嵌字体优先 ----
    if (!g_primaryMonoFamily.isEmpty()) {
        selectedFont = g_primaryMonoFamily;
    }

    // ---- 2. 内嵌字体不可用 探测系统字体 ----
    if (selectedFont.isEmpty()) {
        const QStringList families = QFontDatabase::families();

        // 用 static const 避免每次调用都重建列表
        static const QStringList preferredFonts = {
            "JetBrains Mono",
            "Fira Code",
            "Cascadia Code",
            "Consolas",
            "Menlo",
            "DejaVu Sans Mono",
        };

        for (const QString& preferred : preferredFonts) {
            for (const QString& family : families) {
                if (family.contains(preferred, Qt::CaseInsensitive)) {
                    selectedFont = family;
                    break;
                }
            }
            if (!selectedFont.isEmpty()) break;
        }
    }

    // ---- 3. 全都没找到 Qt 通用等宽别名 ----
    if (selectedFont.isEmpty()) {
        selectedFont = "Monospace";
    }

    // ---- 构造 QFont ----
    QFont font(selectedFont, pointSize);
    font.setFixedPitch(true);
    font.setStyleHint(QFont::Monospace);
    font.setKerning(false);           // 编程字体禁用 kerning
    font.setStyleStrategy(QFont::PreferDefault);

    return font;
}
