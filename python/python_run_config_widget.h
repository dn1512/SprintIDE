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
#ifndef PYTHON_RUN_CONFIG_WIDGET_H
#define PYTHON_RUN_CONFIG_WIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include "python_run_config.h"

QT_BEGIN_NAMESPACE
class QTableWidget;
QT_END_NAMESPACE

namespace SprintIDE {

class PythonRunConfigWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PythonRunConfigWidget(QWidget* parent = nullptr);
    ~PythonRunConfigWidget();  // 显式声明析构函数

    void setConfig(const PythonRunConfig& config);
    PythonRunConfig config() const;

signals:
    void configChanged();

private:
    QComboBox* m_entryTypeCombo;
    QLineEdit* m_entryPathEdit;
    QLineEdit* m_interpreterEdit;
    QLineEdit* m_workingDirEdit;
    QLineEdit* m_argsEdit;
    QTableWidget* m_envTable;
    QCheckBox* m_installDepsCheck;
};

} // namespace SprintIDE

#endif // PYTHON_RUN_CONFIG_WIDGET_H
