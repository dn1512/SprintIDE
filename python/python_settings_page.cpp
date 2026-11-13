#include "python_settings_page.h"
#include "application.h"
#include "config_manager.h"
#include "python/python_virtual_env.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QLabel>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>

namespace SprintIDE {

// ============================================================
// 构造
// ============================================================
PythonSettingsPage::PythonSettingsPage(QWidget* parent)
    : SettingsPage(parent)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // ---------- Python 解释器 / 虚拟环境 ----------
    QGroupBox* interpGroup = new QGroupBox(tr("Python 解释器 / 虚拟环境"), this);
    QVBoxLayout* interpOuter = new QVBoxLayout(interpGroup);

    QHBoxLayout* interpRow = new QHBoxLayout;
    QLabel* interpLabel = new QLabel(tr("当前:"), this);
    m_interpreterCombo = new QComboBox(this);
    m_interpreterCombo->setEditable(true);
    m_interpreterCombo->setInsertPolicy(QComboBox::NoInsert);
    m_interpreterCombo->setMinimumWidth(400);
    if (auto* le = m_interpreterCombo->lineEdit()) {
        le->setPlaceholderText(
            tr("留空则自动检测（项目 venv → 全局设置 → PYTHON_HOME → PATH）"));
    }
    m_pickVenvBtn        = new QPushButton(tr("选择虚拟环境"), this);
    m_pickInterpreterBtn = new QPushButton(tr("选择解释器"), this);

    interpRow->addWidget(interpLabel);
    interpRow->addWidget(m_interpreterCombo, 1);
    interpRow->addWidget(m_pickVenvBtn);
    interpRow->addWidget(m_pickInterpreterBtn);
    interpOuter->addLayout(interpRow);

    QLabel* hint = new QLabel(
        tr("• 选择虚拟环境：选中项目根目录即可，会自动在下面找 .venv / venv / env；\n"
           "  也可以直接选 venv 目录本身。只要求目录里有 pyvenv.cfg，不校验解释器是否可执行。\n"
           "• 选择解释器：直接指定某个 Python 可执行文件（非 venv 场景用）。\n"
           "• 优先级：项目自带 venv > 这里选的 venv > 这里选的解释器 > PYTHON_HOME > PATH。\n"
           "• 只影响「运行 / 调试 / 依赖安装 / 格式化 / 测试」，不影响编译。"),
        this);
    hint->setStyleSheet("QLabel { color: #888888; font-size: 10pt; }");
    hint->setWordWrap(true);
    interpOuter->addWidget(hint);

    m_interpreterStatusLabel = new QLabel(this);
    m_interpreterStatusLabel->setWordWrap(true);
    m_interpreterStatusLabel->setStyleSheet(
        "QLabel { color: #888888; font-size: 10pt; }");
    interpOuter->addWidget(m_interpreterStatusLabel);

    mainLayout->addWidget(interpGroup);

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
    m_extraPipArgs->setPlaceholderText(
        tr("例如: --index-url https://pypi.tuna.tsinghua.edu.cn/simple"));
    depLayout->addRow(tr("pip 额外参数:"), m_extraPipArgs);

    mainLayout->addWidget(depGroup);

    // ---------- 工具 ----------
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

    // ---------- 连接 ----------
    connect(m_pickVenvBtn, &QPushButton::clicked,
            this, &PythonSettingsPage::onPickVenv);
    connect(m_pickInterpreterBtn, &QPushButton::clicked,
            this, &PythonSettingsPage::onPickInterpreter);
    connect(m_interpreterCombo, &QComboBox::currentTextChanged,
            this, [this](const QString&) { refreshInterpreterStatus(); });
}

// ============================================================
// 加载
// ============================================================
void PythonSettingsPage::loadSettings()
{
    ConfigManager* cfg = Application::instance().configManager();
    if (!cfg) return;

    m_interpreterCombo->clear();

    // 历史值（自动收集）
    const QStringList history = cfg->value("Python/InterpreterHistory").toStringList();
    for (const QString& path : history) {
        if (!path.isEmpty()) appendToCombo(path);
    }

    // 当前值：优先 DefaultVenv，其次 DefaultInterpreter
    QString current = cfg->value("Python/DefaultVenv").toString();
    if (current.isEmpty()) {
        current = cfg->value("Python/DefaultInterpreter").toString();
    }
    if (!current.isEmpty()) appendToCombo(current);
    m_interpreterCombo->setCurrentText(current);
    refreshInterpreterStatus();

    // 依赖管理
    const QString dep = cfg->value("Python/DependencyManager", "pip").toString();
    int idx = m_depManager->findData(dep);
    if (idx >= 0) m_depManager->setCurrentIndex(idx);

    m_autoInstallDeps->setChecked(cfg->value("Python/AutoInstallDeps", false).toBool());
    m_extraPipArgs->setText(cfg->value("Python/ExtraPipArgs", "").toString());

    // 工具
    const QString test = cfg->value("Python/TestRunner", "pytest").toString();
    idx = m_testRunner->findData(test);
    if (idx >= 0) m_testRunner->setCurrentIndex(idx);

    const QString fmt = cfg->value("Python/Formatter", "black").toString();
    idx = m_formatter->findData(fmt);
    if (idx >= 0) m_formatter->setCurrentIndex(idx);
}

// ============================================================
// 保存（不做解释器校验，只按"目录 / 文件 / 空"分派存储）
// ============================================================
void PythonSettingsPage::saveSettings()
{
    ConfigManager* cfg = Application::instance().configManager();
    if (!cfg) return;

    QString path = m_interpreterCombo->currentText().trimmed();
    if (path.startsWith("~/") || path == "~") {
        path = QDir::homePath() + path.mid(1);
    }

    if (path.isEmpty()) {
        cfg->remove("Python/DefaultVenv");
        cfg->remove("Python/DefaultInterpreter");
    } else {
        const QFileInfo fi(path);
        if (fi.isDir()) {
            cfg->setValue("Python/DefaultVenv", fi.absoluteFilePath());
            cfg->remove("Python/DefaultInterpreter");
        } else {
            // 文件或断链 symlink 都归入解释器字段（断链也允许保存，运行时自然报错）
            cfg->setValue("Python/DefaultInterpreter", fi.absoluteFilePath());
            cfg->remove("Python/DefaultVenv");
        }
    }

    // 历史去重收集
    QStringList history;
    for (int i = 0; i < m_interpreterCombo->count(); ++i) {
        const QString t = m_interpreterCombo->itemText(i).trimmed();
        if (!t.isEmpty() && !history.contains(t)) history.append(t);
    }
    cfg->setValue("Python/InterpreterHistory", history);

    // 其他
    cfg->setValue("Python/DependencyManager", m_depManager->currentData().toString());
    cfg->setValue("Python/AutoInstallDeps", m_autoInstallDeps->isChecked());
    cfg->setValue("Python/ExtraPipArgs", m_extraPipArgs->text());
    cfg->setValue("Python/TestRunner", m_testRunner->currentData().toString());
    cfg->setValue("Python/Formatter", m_formatter->currentData().toString());

    refreshInterpreterStatus();
}

// ============================================================
// 槽：选择虚拟环境
//   用户选目录后：
//     1) 目录本身是 venv（有 pyvenv.cfg）→ 直接用
//     2) 目录下有 venv / .venv / env 子目录 → 用子目录
//     3) 都没有 → 提示"未找到 venv"，允许用户仍以选中目录作为 venv 根
// ============================================================
void PythonSettingsPage::onPickVenv()
{
    const QString cur = m_interpreterCombo->currentText().trimmed();
    const QString startDir =
        QFileInfo(cur).isDir() ? QFileInfo(cur).absolutePath()
                               : QDir::homePath();

    const QString picked = QFileDialog::getExistingDirectory(
        this, tr("选择虚拟环境（选项目根或 venv 目录均可）"),
        startDir,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (picked.isEmpty()) return;

    const QFileInfo pickedFi(picked);
    QString venvRoot;

    if (PythonVirtualEnv::isValidVenv(picked)) {
        // 本身是 venv
        venvRoot = pickedFi.absoluteFilePath();
    } else {
        // 找子目录
        const QString found = PythonVirtualEnv::findVenvInDir(picked);
        if (!found.isEmpty()) {
            venvRoot = found;
        }
    }

    if (venvRoot.isEmpty()) {
        const auto reply = QMessageBox::question(
            this, tr("未找到虚拟环境"),
            tr("目录\n%1\n及其子目录中都没有找到 pyvenv.cfg（venv / .venv / env）。\n\n"
               "是否仍要把该目录作为虚拟环境根使用？\n"
               "（如果这只是选错了位置，请点「否」重新选）")
                .arg(picked),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (reply != QMessageBox::Yes) return;
        venvRoot = pickedFi.absoluteFilePath();
    }

    appendToCombo(venvRoot);
    m_interpreterCombo->setCurrentText(venvRoot);
    refreshInterpreterStatus();
}

// ============================================================
// 槽：选择解释器文件（非 venv 场景）
// ============================================================
void PythonSettingsPage::onPickInterpreter()
{
    const QString cur = m_interpreterCombo->currentText().trimmed();
    const QString startDir =
        (QFileInfo(cur).isFile() || QFileInfo(cur).isSymLink())
            ? QFileInfo(cur).absolutePath()
            : QDir::homePath();

    const QString file = QFileDialog::getOpenFileName(
        this, tr("选择 Python 解释器"), startDir,
        tr("Python 解释器 (python python3 python*.exe);;所有文件 (*)"));
    if (file.isEmpty()) return;

    appendToCombo(QFileInfo(file).absoluteFilePath());
    m_interpreterCombo->setCurrentText(QFileInfo(file).absoluteFilePath());
    refreshInterpreterStatus();
}

// ============================================================
// 刷新状态：目录 → 看 pyvenv.cfg；文件 → 看可执行位
//   任何情况都只做"提示"，不阻止保存
// ============================================================
void PythonSettingsPage::refreshInterpreterStatus()
{
    if (!m_interpreterStatusLabel) return;

    const QString raw = m_interpreterCombo->currentText().trimmed();

    if (raw.isEmpty()) {
        m_interpreterStatusLabel->setText(
            tr("当前: 自动检测（项目 venv → 全局设置 → PYTHON_HOME → PATH）"));
        m_interpreterStatusLabel->setStyleSheet(
            "QLabel { color: #888888; font-size: 10pt; }");
        return;
    }

    QString path = raw;
    if (path.startsWith("~/") || path == "~") {
        path = QDir::homePath() + path.mid(1);
    }

    const QFileInfo fi(path);

    if (fi.isDir()) {
        const bool hasCfg = PythonVirtualEnv::isValidVenv(path);
        const QString interp = PythonVirtualEnv::interpreterPath(path);

        QString status;
        QString color;
        if (hasCfg) {
            color = "#4A90D9";   // 蓝
            status = tr("当前 venv: %1").arg(path);
        } else {
            color = "#EBB65B";   // 黄
            status = tr("当前 venv: %1（未找到 pyvenv.cfg，仍将使用）").arg(path);
        }

        if (interp.isEmpty()) {
            status += "\n" + tr("⚠ 该 venv 中未找到 Python 解释器（bin/python 可能缺失或 symlink 已失效）");
        } else {
            status += "\n" + tr("解释器: %1").arg(interp);
        }

        m_interpreterStatusLabel->setText(status);
        m_interpreterStatusLabel->setStyleSheet(
            QString("QLabel { color: %1; font-size: 10pt; }").arg(color));
        return;
    }

    if (fi.isFile() || fi.isSymLink()) {
        if (fi.isSymLink() && !fi.exists()) {
            m_interpreterStatusLabel->setText(
                tr("⚠ 解释器: %1（symlink 目标不存在，运行时将报错）").arg(path));
            m_interpreterStatusLabel->setStyleSheet(
                "QLabel { color: #EBB65B; font-size: 10pt; }");
            return;
        }
        if (fi.isExecutable()) {
            m_interpreterStatusLabel->setText(tr("当前解释器: %1").arg(path));
            m_interpreterStatusLabel->setStyleSheet(
                "QLabel { color: #4A90D9; font-size: 10pt; }");
        } else {
            m_interpreterStatusLabel->setText(
                tr("⚠ 解释器: %1（无执行权限）").arg(path));
            m_interpreterStatusLabel->setStyleSheet(
                "QLabel { color: #E74C3C; font-size: 10pt; }");
        }
        return;
    }

    m_interpreterStatusLabel->setText(tr("⚠ 路径无效: %1").arg(path));
    m_interpreterStatusLabel->setStyleSheet(
        "QLabel { color: #E74C3C; font-size: 10pt; }");
}

// ============================================================
// 辅助：ComboBox 追加（去重）
// ============================================================
void PythonSettingsPage::appendToCombo(const QString& path)
{
    if (path.isEmpty()) return;
    if (m_interpreterCombo->findText(path) >= 0) return;
    m_interpreterCombo->addItem(path);
}

} // namespace SprintIDE