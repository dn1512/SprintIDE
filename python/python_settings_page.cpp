#include "python_settings_page.h"
#include "application.h"
#include "config_manager.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QLabel>

namespace SprintIDE {

PythonSettingsPage::PythonSettingsPage(QWidget* parent)
    : SettingsPage(parent)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // ---------- 依赖管理 ----------
    QGroupBox* depGroup = new QGroupBox(tr("依赖管理"), this);
    QFormLayout* depLayout = new QFormLayout(depGroup);

    m_depManager = new QComboBox(this);
    m_depManager->addItem("pip", "pip");
    m_depManager->addItem("poetry", "poetry");
    depLayout->addRow(tr("默认依赖管理器:"), m_depManager);

    m_autoInstallDeps = new QCheckBox(tr("打开项目时自动安装依赖"), this);
    depLayout->addRow(m_autoInstallDeps);

    m_extraPipArgs = new QLineEdit(this);
    m_extraPipArgs->setPlaceholderText(tr("例如: --index-url https://pypi.tuna.tsinghua.edu.cn/simple"));
    depLayout->addRow(tr("pip 额外参数:"), m_extraPipArgs);

    mainLayout->addWidget(depGroup);

    // ---------- 工具选择 ----------
    QGroupBox* toolGroup = new QGroupBox(tr("工具"), this);
    QFormLayout* toolLayout = new QFormLayout(toolGroup);

    m_testRunner = new QComboBox(this);
    m_testRunner->addItem("pytest", "pytest");
    m_testRunner->addItem("unittest", "unittest");
    toolLayout->addRow(tr("测试运行器:"), m_testRunner);

    m_formatter = new QComboBox(this);
    m_formatter->addItem("black", "black");
    m_formatter->addItem("autopep8", "autopep8");
    toolLayout->addRow(tr("代码格式化工具:"), m_formatter);

    mainLayout->addWidget(toolGroup);

    mainLayout->addStretch();
}

void PythonSettingsPage::loadSettings()
{
    ConfigManager* cfg = Application::instance().configManager();
    if (!cfg) return;

    // 依赖管理
    QString dep = cfg->value("Python/DependencyManager", "pip").toString();
    int idx = m_depManager->findData(dep);
    if (idx >= 0) m_depManager->setCurrentIndex(idx);

    m_autoInstallDeps->setChecked(cfg->value("Python/AutoInstallDeps", false).toBool());
    m_extraPipArgs->setText(cfg->value("Python/ExtraPipArgs", "").toString());

    // 工具
    QString test = cfg->value("Python/TestRunner", "pytest").toString();
    idx = m_testRunner->findData(test);
    if (idx >= 0) m_testRunner->setCurrentIndex(idx);

    QString fmt = cfg->value("Python/Formatter", "black").toString();
    idx = m_formatter->findData(fmt);
    if (idx >= 0) m_formatter->setCurrentIndex(idx);
}

void PythonSettingsPage::saveSettings()
{
    ConfigManager* cfg = Application::instance().configManager();
    if (!cfg) return;

    cfg->setValue("Python/DependencyManager", m_depManager->currentData().toString());
    cfg->setValue("Python/AutoInstallDeps", m_autoInstallDeps->isChecked());
    cfg->setValue("Python/ExtraPipArgs", m_extraPipArgs->text());
    cfg->setValue("Python/TestRunner", m_testRunner->currentData().toString());
    cfg->setValue("Python/Formatter", m_formatter->currentData().toString());

    // QMessageBox::information(this, tr("保存成功"), tr("Python 设置已保存。"));
}

} // namespace SprintIDE
