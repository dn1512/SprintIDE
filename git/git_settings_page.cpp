#include "git_settings_page.h"
#include "git_manager.h"
#include "git_command.h"
#include "application.h"
#include "config_manager.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>

namespace SprintIDE {

GitSettingsPage::GitSettingsPage(QWidget* parent)
    : SettingsPage(parent)
{
    setupUI();
}

void GitSettingsPage::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    // ============================================================
    // 1. Git 可执行文件
    // ============================================================
    auto* gitGroup = new QGroupBox(tr("Git 可执行文件"), this);
    auto* gitLayout = new QFormLayout(gitGroup);

    m_gitPathEdit = new QLineEdit(this);
    m_gitPathEdit->setPlaceholderText(tr("留空则自动检测"));
    m_browseBtn = new QPushButton(tr("浏览..."), this);
    connect(m_browseBtn, &QPushButton::clicked, this, &GitSettingsPage::onBrowseGit);

    auto* pathRow = new QHBoxLayout;
    pathRow->addWidget(m_gitPathEdit, 1);
    pathRow->addWidget(m_browseBtn);
    gitLayout->addRow(tr("路径:"), pathRow);

    m_detectBtn = new QPushButton(tr("自动检测"), this);
    connect(m_detectBtn, &QPushButton::clicked, this, &GitSettingsPage::onDetectClicked);

    auto* detectRow = new QHBoxLayout;
    detectRow->addWidget(m_detectBtn);
    detectRow->addStretch();
    gitLayout->addRow(QString(), detectRow);

    m_gitVersionLbl = new QLabel(tr("未检测"), this);
    m_gitVersionLbl->setStyleSheet("QLabel { color: #888888; }");
    m_gitVersionLbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
    gitLayout->addRow(tr("版本:"), m_gitVersionLbl);

    mainLayout->addWidget(gitGroup);

    // ============================================================
    // 2. 提交身份（写入当前仓库 config）
    // ============================================================
    auto* userGroup = new QGroupBox(tr("提交身份（git config user.*）"), this);
    auto* userLayout = new QFormLayout(userGroup);

    m_userNameEdit = new QLineEdit(this);
    m_userNameEdit->setPlaceholderText(tr("例如：张三"));
    userLayout->addRow(tr("用户名:"), m_userNameEdit);

    m_userEmailEdit = new QLineEdit(this);
    m_userEmailEdit->setPlaceholderText(tr("例如：zhangsan@example.com"));
    userLayout->addRow(tr("邮箱:"), m_userEmailEdit);

    m_saveUserBtn = new QPushButton(tr("写入当前仓库配置"), this);
    connect(m_saveUserBtn, &QPushButton::clicked,
            this, &GitSettingsPage::onSaveUserInfoClicked);

    auto* saveRow = new QHBoxLayout;
    saveRow->addWidget(m_saveUserBtn);
    saveRow->addStretch();
    userLayout->addRow(QString(), saveRow);

    mainLayout->addWidget(userGroup);

    // ============================================================
    // 3. 自动刷新
    // ============================================================
    auto* refreshGroup = new QGroupBox(tr("自动刷新"), this);
    auto* refreshLayout = new QFormLayout(refreshGroup);

    m_fetchIntervalSpin = new QSpinBox(this);
    m_fetchIntervalSpin->setRange(0, 3600);
    m_fetchIntervalSpin->setSuffix(tr(" 秒"));
    m_fetchIntervalSpin->setSpecialValueText(tr("禁用"));
    refreshLayout->addRow(tr("状态刷新间隔:"), m_fetchIntervalSpin);

    mainLayout->addWidget(refreshGroup);

    mainLayout->addStretch();
}

void GitSettingsPage::refreshVersionLabel(const QString& gitPath)
{
    if (gitPath.isEmpty()) {
        m_gitVersionLbl->setText(tr("未检测到 git"));
        return;
    }

    Git::GitCommand cmd;
    auto r = cmd.run(QDir::currentPath(), {"--version"}, 3000);
    if (r.ok()) {
        m_gitVersionLbl->setText(r.stdOut.trimmed());
    } else {
        m_gitVersionLbl->setText(gitPath);
    }
}

void GitSettingsPage::loadSettings()
{
    ConfigManager* cfg = Application::instance().configManager();
    if (!cfg) return;

    // ---- git 路径 ----
    const QString savedPath = cfg->value("Git/ExecutablePath", "").toString();
    m_gitPathEdit->setText(savedPath);
    if (!savedPath.isEmpty()) {
        Git::GitCommand::setGitExecutable(savedPath);
    } else {
        Git::GitCommand::setGitExecutable(QString());
    }
    refreshVersionLabel(Git::GitCommand::gitExecutable());

    // ---- 用户信息（从当前仓库读） ----
    auto& gm = Git::GitManager::instance();
    if (gm.isActive()) {
        m_userNameEdit->setText(gm.userName());
        m_userEmailEdit->setText(gm.userEmail());
        m_saveUserBtn->setEnabled(true);
    } else {
        m_userNameEdit->clear();
        m_userEmailEdit->clear();
        m_saveUserBtn->setEnabled(false);
    }

    // ---- 刷新间隔 ----
    const int interval = cfg->value("Git/FetchIntervalSeconds", 0).toInt();
    m_fetchIntervalSpin->setValue(interval);
}

void GitSettingsPage::saveSettings()
{
    ConfigManager* cfg = Application::instance().configManager();
    if (!cfg) return;

    const QString gitPath = m_gitPathEdit->text().trimmed();
    cfg->setValue("Git/ExecutablePath", gitPath);
    Git::GitCommand::setGitExecutable(gitPath);

    cfg->setValue("Git/FetchIntervalSeconds", m_fetchIntervalSpin->value());
}

void GitSettingsPage::onBrowseGit()
{
    const QString startDir = m_gitPathEdit->text().isEmpty()
                                 ? QDir::homePath()
                                 : m_gitPathEdit->text();
    const QString path = QFileDialog::getOpenFileName(
        this, tr("选择 Git 可执行文件"), startDir);
    if (!path.isEmpty()) {
        m_gitPathEdit->setText(path);
        Git::GitCommand::setGitExecutable(path);
        refreshVersionLabel(path);
    }
}

void GitSettingsPage::onDetectClicked()
{
    // 清空手动路径，让 GitCommand 走自动探测
    Git::GitCommand::setGitExecutable(QString());
    const QString detected = Git::GitCommand::gitExecutable();

    if (detected.isEmpty()) {
        m_gitVersionLbl->setText(tr("未检测到 git"));
        QMessageBox::warning(this, tr("检测失败"),
            tr("未在系统 PATH 或常见安装路径中找到 git。"));
        return;
    }

    m_gitPathEdit->setText(detected);
    refreshVersionLabel(detected);
}

void GitSettingsPage::onSaveUserInfoClicked()
{
    auto& gm = Git::GitManager::instance();
    if (!gm.isActive()) {
        QMessageBox::information(this, tr("提示"),
            tr("当前没有打开的 Git 仓库，无法写入仓库级配置。"));
        return;
    }

    if (gm.setUserInfo(m_userNameEdit->text().trimmed(),
                       m_userEmailEdit->text().trimmed(),
                       /*global=*/false)) {
        QMessageBox::information(this, tr("成功"),
            tr("已写入当前仓库的 git config。"));
    } else {
        QMessageBox::warning(this, tr("失败"),
            tr("写入 git config 失败，请检查仓库权限。"));
    }
}

} // namespace SprintIDE
