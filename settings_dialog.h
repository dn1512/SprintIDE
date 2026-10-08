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
#include <QLabel>
#include <QPlainTextEdit>
#include <QKeySequenceEdit>
#include <QTableWidget>
#include <QHeaderView>



namespace SprintIDE {
    class MainWindow; 
}
using SprintIDE::MainWindow;
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

// ========== LSP 诊断设置页面 ==========
class LspSettingsPage : public SettingsPage
{
    Q_OBJECT
public:
    explicit LspSettingsPage(QWidget* parent = nullptr);
    void loadSettings() override;
    void saveSettings() override;

private slots:
    void onModeChanged(int index);

private:
    QComboBox*      m_modeCombo  = nullptr;
    QPlainTextEdit* m_rulesEdit  = nullptr;
    QLabel*         m_rulesLabel = nullptr;
    QLabel*         m_rulesHint  = nullptr;
};

// ========== 主设置对话框 ==========
class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(MainWindow* mainWindow, QWidget *parent = nullptr);

    MainWindow* mainWindow() const { return m_mainWindow; }

    // 外部调用：跳转到指定名字的设置页（例如 "Git"）
    void showCategory(const QString& name);

signals:
    void aiServerRestartRequested(const QString& host, quint16 port);
    void lspSettingsChanged();

private slots:
    void onCategoryChanged(int currentRow);
    void onApply();

private:
    void createUI();
    void loadAllSettings();
    void saveAllSettings();
    void accept() override;

    MainWindow*      m_mainWindow = nullptr;
    QListWidget      *m_categoryList;
    QStackedWidget   *m_stackedWidget;
    QDialogButtonBox *m_buttonBox;
    QList<SettingsPage*> m_pages;
    bool m_languageChanged = false;
};

// ========== 语言设置页面 ==========
class LanguageSettingsPage : public SettingsPage
{
    Q_OBJECT
public:
    explicit LanguageSettingsPage(QWidget* parent = nullptr);
    void loadSettings() override;
    void saveSettings() override;

signals:
    // 语言真的发生变化时发出（用于 SettingsDialog 提示重启）
    void languageChanged(const QString& langCode);

private:
    QComboBox* m_langCombo = nullptr;
    QLabel*    m_hintLabel = nullptr;
    QString    m_initialLang;
};

// ========== 快捷键设置页面 ==========
class ShortcutSettingsPage : public SettingsPage
{
    Q_OBJECT
public:
    explicit ShortcutSettingsPage(QWidget* parent = nullptr);
    void loadSettings() override;
    void saveSettings() override;

private slots:
    void onItemDoubleClicked(int row, int column);
    void onResetRow();
    void onResetAll();
    void onKeySequenceEdited(int row, const QKeySequence& seq);

private:
    void populateTable();
    void refreshRow(int row);
    void detectConflicts();
    int findRowById(const QString& id) const;

    QTableWidget* m_table = nullptr;
    QPushButton*  m_resetRowBtn = nullptr;
    QPushButton*  m_resetAllBtn = nullptr;
    QLabel*       m_statusLabel = nullptr;

    // 表格行缓存（行号 → id / displayName / defaultShortcut）
    struct RowData {
        QString      id;
        QString      displayName;
        QKeySequence defaultShortcut;
    };
    QList<RowData> m_rows;
};

#endif // SETTINGS_DIALOG_H
