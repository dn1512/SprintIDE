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
