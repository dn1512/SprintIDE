#ifndef LANGUAGE_MANAGER_H
#define LANGUAGE_MANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTranslator>

namespace SprintIDE {

class LanguageManager : public QObject
{
    Q_OBJECT
public:
    static LanguageManager& instance();

    struct LanguageInfo {
        QString code;        // "zh_CN" / "en_US"
        QString display;     // "简体中文" / "English"
    };

    // 供设置页展示
    static QList<LanguageInfo> availableLanguages();

    // 应用语言（一般在 main.cpp 早期调用一次）
    bool applyLanguage(const QString& langCode);

    // 便捷：从 ConfigManager 读取并应用
    void applyFromConfig();

    QString currentLanguage() const { return m_currentLang; }

signals:
    void languageChanged(const QString& langCode);

private:
    LanguageManager() = default;
    ~LanguageManager() = default;
    LanguageManager(const LanguageManager&) = delete;
    LanguageManager& operator=(const LanguageManager&) = delete;

    bool loadTranslator(const QString& qmPath);

    QTranslator* m_appTranslator = nullptr;
    QTranslator* m_qtTranslator  = nullptr;
    QString      m_currentLang;
};

} // namespace SprintIDE
#endif
