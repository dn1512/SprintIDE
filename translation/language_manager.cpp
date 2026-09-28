#include "language_manager.h"
#include "application.h"
#include "config_manager.h"

#include <QCoreApplication>
#include <QLibraryInfo>
#include <QDebug>

namespace SprintIDE {

LanguageManager& LanguageManager::instance()
{
    static LanguageManager s;
    return s;
}

QList<LanguageManager::LanguageInfo> LanguageManager::availableLanguages()
{
    return {
            { QStringLiteral("zh_CN"), QStringLiteral("简体中文") },
            { QStringLiteral("en_US"), QStringLiteral("English")  },
            };
}

bool LanguageManager::loadTranslator(const QString& qmPath)
{
    auto* tr = new QTranslator(QCoreApplication::instance());
    if (!tr->load(qmPath)) {
        delete tr;
        qWarning() << "[i18n] 无法加载翻译文件:" << qmPath;
        return false;
    }
    QCoreApplication::installTranslator(tr);
    m_appTranslator = tr;
    return true;
}

bool LanguageManager::applyLanguage(const QString& langCode)
{
    // 卸载旧翻译（若存在）
    if (m_appTranslator) {
        QCoreApplication::removeTranslator(m_appTranslator);
        m_appTranslator->deleteLater();
        m_appTranslator = nullptr;
    }
    if (m_qtTranslator) {
        QCoreApplication::removeTranslator(m_qtTranslator);
        m_qtTranslator->deleteLater();
        m_qtTranslator = nullptr;
    }

    m_currentLang = langCode;

    // 加载 Qt 内置翻译（对话框按钮"OK/Cancel"、QFileDialog 等）
    // 资源路径与系统语言目录一致
    const QString qtTrPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
    auto* qtTr = new QTranslator(QCoreApplication::instance());
    if (qtTr->load(QStringLiteral("qtbase_%1").arg(langCode), qtTrPath)) {
        QCoreApplication::installTranslator(qtTr);
        m_qtTranslator = qtTr;
    } else {
        delete qtTr;
    }

    // 加载应用自身翻译（编译进 qrc）
    const QString appQm = QStringLiteral(":/i18n/SprintIDE_%1.qm").arg(langCode);
    if (!loadTranslator(appQm)) {
        // 中文缺省：不装任何翻译，tr() 直接返回中文字面量
        if (langCode == QLatin1String("zh_CN")) {
            qDebug() << "[i18n] 使用内建中文（无 .qm 回退）";
        }
    }

    emit languageChanged(langCode);
    return true;
}

void LanguageManager::applyFromConfig()
{
    ConfigManager* cfg = Application::instance().configManager();
    const QString lang = cfg
                             ? cfg->value("General/Language", "zh_CN").toString()
                             : QStringLiteral("zh_CN");
    applyLanguage(lang);
}

} // namespace SprintIDE
