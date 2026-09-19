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
