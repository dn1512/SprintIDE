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
#ifndef PYTHON_SETTINGS_PAGE_H
#define PYTHON_SETTINGS_PAGE_H

#include "../settings_dialog.h"

namespace SprintIDE {

class PythonSettingsPage : public SettingsPage {
    Q_OBJECT
public:
    explicit PythonSettingsPage(QWidget* parent = nullptr);
    void loadSettings() override;
    void saveSettings() override;

private:
    QComboBox* m_interpreterCombo;
    QCheckBox* m_autoVenv;
    QComboBox* m_depManager;
    QCheckBox* m_autoInstallDeps;
    QComboBox* m_testRunner;
    QComboBox* m_formatter;
    QLineEdit* m_extraPipArgs;
};

} // namespace SprintIDE

#endif // PYTHON_SETTINGS_PAGE_H
