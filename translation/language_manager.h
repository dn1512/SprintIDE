// SPDX-License-Identifier: GPL-3.0-or-later
/*
* SprintIDE - SprintIDE is a cross-platform IDE built from scratch with Qt 6 and C++20. Its core capability is to provide AI Agents with an executable, verifiable code execution environment.
* Copyright (C) 2026  Liu Tengjun (刘腾骏)
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
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
