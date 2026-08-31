// settings_dialog.h
#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QStackedWidget>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QCheckBox>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>

// ========== 设置页面基类 ==========
class SettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPage(QWidget *parent = nullptr) : QWidget(parent) {}
    virtual void loadSettings() = 0;
    virtual void saveSettings() = 0;
    virtual void applySettings() { saveSettings(); }
};

// ========== 编译器设置页面 ==========
class CompilerSettingsPage : public SettingsPage
{
    Q_OBJECT
public:
    explicit CompilerSettingsPage(QWidget *parent = nullptr);
    void loadSettings() override;
    void saveSettings() override;

private slots:
    void onBrowseCompiler();

private:
    QComboBox *m_compilerTypeCombo;
    QLineEdit *m_compilerPathEdit;
    QPushButton *m_browseButton;
    QComboBox *m_cppStandardCombo;
    QCheckBox *m_debugInfoCheck;
    QCheckBox *m_warningsCheck;
    QCheckBox *m_warningsAsErrorsCheck;
};

// ========== 环境变量设置页面（全局配置） ==========
class EnvironmentSettingsPage : public SettingsPage
{
    Q_OBJECT
public:
    explicit EnvironmentSettingsPage(QWidget *parent = nullptr);
    ~EnvironmentSettingsPage() = default;

    void loadSettings() override;
    void saveSettings() override;

private slots:
    void onAddRow();
    void onRemoveRow();
    void onDetectSDK();
    void onCellChanged(int row, int column);
    void onSdkComboChanged(int index);

private:
    void setupTable();
    void addSdkRow(const QString& key, const QString& value);
    void saveToGlobalConfig(const QVariantMap& sdkEnv);
    QVariantMap loadFromGlobalConfig() const;
    QString detectSDKPath(const QString& key) const;
    QString getShellEnvironmentVariable(const QString& varName) const;

    QTableWidget *m_tableWidget;
    QPushButton *m_addButton;
    QPushButton *m_removeButton;
    QPushButton *m_detectButton;
    QComboBox *m_sdkPresetCombo;
};

// ========== 主设置对话框 ==========
class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

private slots:
    void onCategoryChanged(int currentRow);
    void onApply();

private:
    void createUI();
    void loadAllSettings();
    void saveAllSettings();
    void accept() override;

    QListWidget      *m_categoryList;
    QStackedWidget   *m_stackedWidget;
    QDialogButtonBox *m_buttonBox;
    QList<SettingsPage*> m_pages;
};

#endif // SETTINGS_DIALOG_H
