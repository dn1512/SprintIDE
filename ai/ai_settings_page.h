// ai/ai_settings_page.h
#ifndef AI_SETTINGS_PAGE_H
#define AI_SETTINGS_PAGE_H

#include "settings_dialog.h"   // 复用 SettingsPage 基类

class QLineEdit;
class QSpinBox;
class QCheckBox;
class QPushButton;
class QLabel;

namespace SprintIDE {

class AISettingsPage : public SettingsPage
{
    Q_OBJECT
public:
    explicit AISettingsPage(QWidget *parent = nullptr);
    ~AISettingsPage() override = default;

    void loadSettings() override;
    void saveSettings() override;

signals:
    /** 用户在设置页点了"重启 AI 服务器"按钮时发出，
     *  MainWindow 收到后执行 stopServer + startServer(newPort/newAddr)。
     *  设计成信号是为了保持 SettingsPage 与 MainWindow 解耦。
     */
    void aiServerRestartRequested(const QString& host, quint16 port);

private slots:
    void onTestConnection();
    void onRestartServer();

private:
    void setupUI();

    QLineEdit*   m_hostEdit        = nullptr;
    QSpinBox*    m_portSpin        = nullptr;
    QCheckBox*   m_autoStartCheck  = nullptr;
    QCheckBox*   m_autoFixCheck    = nullptr;
    QCheckBox*   m_autoContextCheck= nullptr;
    // 策略配置（PolicyEngine）
    QCheckBox*   m_fileCreateCheck    = nullptr;
    QCheckBox*   m_fileEditCheck      = nullptr;
    QCheckBox*   m_fileDeleteCheck    = nullptr;
    QCheckBox*   m_fileRenameCheck    = nullptr;
    QCheckBox*   m_commandRunCheck    = nullptr;
    QCheckBox*   m_runShellCheck      = nullptr;
    QCheckBox*   m_installDepCheck    = nullptr;
    QCheckBox*   m_allowShellCheck    = nullptr;
    QPushButton* m_testButton      = nullptr;
    QPushButton* m_restartButton   = nullptr;
    QLabel*      m_statusLabel     = nullptr;

    QString m_activeHost;    // 加载时记录"当前运行中"的 host
    quint16 m_activePort = 0;
};

} // namespace SprintIDE

#endif // AI_SETTINGS_PAGE_H
