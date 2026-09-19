// ai/ai_settings_page.cpp
#include "ai/ai_settings_page.h"

#include "application.h"
#include "config_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QMessageBox>
#include <QTcpSocket>
#include <QHostAddress>
#include <QApplication>
#include <QDebug>

namespace SprintIDE {

// ============================================================
// 构造
// ============================================================
AISettingsPage::AISettingsPage(QWidget *parent)
    : SettingsPage(parent)
{
    setupUI();
}

// ============================================================
// 构建 UI
// ============================================================
void AISettingsPage::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // ---------- 顶部说明 ----------
    QLabel* infoLabel = new QLabel(
        tr("配置 SprintIDE 与本地 AI Agent 之间的通信地址。\n"
           "默认 127.0.0.1:9876，请确保 Agent 端监听同一地址。"),
        this);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet(
        "QLabel { color: #888888; font-size: 10pt; margin-bottom: 8px; }");
    mainLayout->addWidget(infoLabel);

    // ========================================================
    // 组 1：AI 服务器
    // ========================================================
    QGroupBox* serverGroup = new QGroupBox(tr("AI 服务器"), this);
    QFormLayout* serverLayout = new QFormLayout(serverGroup);

    m_hostEdit = new QLineEdit(this);
    m_hostEdit->setPlaceholderText(QStringLiteral("127.0.0.1"));
    m_hostEdit->setToolTip(tr("Agent 绑定的地址，通常为本机 127.0.0.1"));
    serverLayout->addRow(tr("监听地址:"), m_hostEdit);

    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(9876);
    m_portSpin->setToolTip(tr("Agent 监听的端口号（1 - 65535）"));
    serverLayout->addRow(tr("监听端口:"), m_portSpin);

    m_autoStartCheck = new QCheckBox(
        tr("启动 SprintIDE 时自动启动 AI 服务器"), this);
    serverLayout->addRow(QString(), m_autoStartCheck);

    // 按钮行
    QHBoxLayout* btnRow = new QHBoxLayout;
    m_testButton    = new QPushButton(tr("测试连接"), this);
    // m_restartButton = new QPushButton(tr("重启 AI 服务器"), this);
    btnRow->addWidget(m_testButton);
    // btnRow->addWidget(m_restartButton);
    btnRow->addStretch();
    serverLayout->addRow(QString(), btnRow);

    // 状态标签
    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet("QLabel { color: #888888; }");
    serverLayout->addRow(tr("状态:"), m_statusLabel);

    mainLayout->addWidget(serverGroup);

    // ========================================================
    // 组 2：AI 行为
    // ========================================================
    QGroupBox* behaviorGroup = new QGroupBox(tr("AI 行为"), this);
    QVBoxLayout* behaviorLayout = new QVBoxLayout(behaviorGroup);

    m_autoFixCheck = new QCheckBox(
        tr("编译 / 运行错误时自动请求 AI 修复"), this);
    m_autoFixCheck->setToolTip(
        tr("对应 AI/AutoFixErrors 配置，供后续 agent.autoFix 流程使用"));
    behaviorLayout->addWidget(m_autoFixCheck);

    m_autoContextCheck = new QCheckBox(
        tr("发送消息时自动附带当前文件 / 项目上下文"), this);
    behaviorLayout->addWidget(m_autoContextCheck);

    mainLayout->addWidget(behaviorGroup);
    mainLayout->addStretch();

    // ========================================================
    // 信号连接
    // ========================================================
    connect(m_testButton,    &QPushButton::clicked,
            this, &AISettingsPage::onTestConnection);
    // connect(m_restartButton, &QPushButton::clicked,
    //         this, &AISettingsPage::onRestartServer);

    // 用户改动地址/端口时，清掉旧的状态提示，避免误导
    connect(m_hostEdit, &QLineEdit::textChanged, this, [this]() {
        m_statusLabel->setText(QString());
    });
    connect(m_portSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int) {
                m_statusLabel->setText(QString());
            });
}

// ============================================================
// loadSettings：从 ConfigManager 读，填 UI
// ============================================================
void AISettingsPage::loadSettings()
{
    ConfigManager* cfg = Application::instance().configManager();
    if (!cfg) {
        qWarning() << "[AISettingsPage] ConfigManager 不可用，跳过加载";
        return;
    }

    m_hostEdit        ->setText   (cfg->value("AI/Host", "127.0.0.1").toString());
    m_portSpin        ->setValue  (cfg->value("AI/Port", 9876).toInt());
    m_autoStartCheck  ->setChecked(cfg->value("AI/AutoStart", true).toBool());
    m_autoFixCheck    ->setChecked(cfg->value("AI/AutoFixErrors", true).toBool());
    m_autoContextCheck->setChecked(cfg->value("AI/AutoIncludeContext", true).toBool());

    // 记录“当前运行中”的地址；后续 saveSettings 用它来判断是否需要提示重启
    m_activeHost = m_hostEdit->text().trimmed();
    m_activePort = static_cast<quint16>(m_portSpin->value());

    m_statusLabel->setText(QString());
    m_statusLabel->setStyleSheet("QLabel { color: #888888; }");
}

// ============================================================
// saveSettings：UI -> ConfigManager
// ============================================================
void AISettingsPage::saveSettings()
{
    ConfigManager* cfg = Application::instance().configManager();
    if (!cfg) {
        qWarning() << "[AISettingsPage] ConfigManager 不可用，跳过保存";
        return;
    }

    const QString host = m_hostEdit->text().trimmed();
    const quint16 port = static_cast<quint16>(m_portSpin->value());

    if (host.isEmpty()) {
        QMessageBox::warning(this, tr("AI 设置"),
                             tr("监听地址不能为空，已使用默认值 127.0.0.1。"));
        m_hostEdit->setText(QStringLiteral("127.0.0.1"));
        return;   // 让用户重按一次“应用/确定”
    }

    cfg->setValue("AI/Host",              host);
    cfg->setValue("AI/Port",              static_cast<int>(port));
    cfg->setValue("AI/AutoStart",         m_autoStartCheck->isChecked());
    cfg->setValue("AI/AutoFixErrors",     m_autoFixCheck->isChecked());
    cfg->setValue("AI/AutoIncludeContext",m_autoContextCheck->isChecked());

    // 地址/端口发生变化 → 提示需要重启才生效
    if (host != m_activeHost || port != m_activePort) {
        m_statusLabel->setText(
            tr("⚠ 地址已更改，点击【重启 AI 服务器】后生效。"));
        m_statusLabel->setStyleSheet("QLabel { color: #FFAA00; }");
    } else {
        m_statusLabel->setText(tr("✓ 设置已保存"));
        m_statusLabel->setStyleSheet("QLabel { color: #33CC33; }");
    }
}

// ============================================================
// 测试连接
// ============================================================
void AISettingsPage::onTestConnection()
{
    const QString host = m_hostEdit->text().trimmed();
    const quint16 port = static_cast<quint16>(m_portSpin->value());

    if (host.isEmpty()) {
        QMessageBox::warning(this, tr("测试连接"),
                             tr("请先填写监听地址。"));
        return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_testButton->setEnabled(false);
    m_statusLabel->setText(tr("正在连接 %1:%2 ...").arg(host).arg(port));
    m_statusLabel->setStyleSheet("QLabel { color: #888888; }");

    // 说明：这是本机端口探测，同步 2s 超时对 UI 影响极小，
    //      无需引入 QTimer 做异步。
    QTcpSocket socket;
    socket.connectToHost(host, port);
    const bool ok = socket.waitForConnected(2000);

    if (ok) {
        socket.disconnectFromHost();
    }

    QApplication::restoreOverrideCursor();
    m_testButton->setEnabled(true);

    if (ok) {
        m_statusLabel->setText(tr("✓ 已连接到 %1:%2").arg(host).arg(port));
        m_statusLabel->setStyleSheet("QLabel { color: #33CC33; }");
    } else {
        m_statusLabel->setText(
            tr("✗ 无法连接：%1").arg(socket.errorString()));
        m_statusLabel->setStyleSheet("QLabel { color: #FF5555; }");
    }
}

// ============================================================
// 重启 AI 服务器
// ============================================================
void AISettingsPage::onRestartServer()
{
    const QString host = m_hostEdit->text().trimmed();
    const quint16 port = static_cast<quint16>(m_portSpin->value());

    if (host.isEmpty()) {
        QMessageBox::warning(this, tr("重启 AI 服务器"),
                             tr("请先填写监听地址。"));
        return;
    }

    // 先把当前值写回配置，保证重启后生效
    saveSettings();

    // 记录为“新的运行地址”，下次打开设置页就不会再提示“需要重启”
    m_activeHost = host;
    m_activePort = port;

    m_statusLabel->setText(
        tr("⏳ 已发送重启请求：%1:%2").arg(host).arg(port));
    m_statusLabel->setStyleSheet("QLabel { color: #88DDFF; }");

    // 交给外部执行，SettingsPage 不直接依赖 AIServer
    emit aiServerRestartRequested(host, port);
}

} // namespace SprintIDE
