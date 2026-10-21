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
#ifndef SPRINTIDE_GIT_SETTINGS_PAGE_H
#define SPRINTIDE_GIT_SETTINGS_PAGE_H

#include "settings_dialog.h"    // SettingsPage 基类

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
class QSpinBox;
class QLabel;
QT_END_NAMESPACE

namespace SprintIDE {

class GitSettingsPage : public SettingsPage
{
    Q_OBJECT
public:
    explicit GitSettingsPage(QWidget* parent = nullptr);
    ~GitSettingsPage() override = default;

    void loadSettings() override;
    void saveSettings() override;

private slots:
    void onBrowseGit();
    void onDetectClicked();
    void onSaveUserInfoClicked();

private:
    void setupUI();
    void refreshVersionLabel(const QString& gitPath);

    QLineEdit*   m_gitPathEdit      = nullptr;
    QPushButton* m_browseBtn        = nullptr;
    QPushButton* m_detectBtn        = nullptr;
    QLabel*      m_gitVersionLbl    = nullptr;

    QLineEdit*   m_userNameEdit     = nullptr;
    QLineEdit*   m_userEmailEdit    = nullptr;
    QPushButton* m_saveUserBtn      = nullptr;

    QSpinBox*    m_fetchIntervalSpin = nullptr;
};

} // namespace SprintIDE

#endif // SPRINTIDE_GIT_SETTINGS_PAGE_H
