// src/deps/toolchain_dialog.cpp
#include "toolchain_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>
#include <QFont>
#include <QDebug>
#include <QTimer>

namespace SprintIDE {

ToolchainDialog::ToolchainDialog(const ToolchainInstallHint& hint,
                                 ToolchainId id,
                                 QWidget* parent)
    : QDialog(parent)
    , m_id(id)
    , m_command(hint.command)
{
    setupUi(hint);
}

ToolchainDialog::~ToolchainDialog() = default;

void ToolchainDialog::setupUi(const ToolchainInstallHint& hint)
{
    setWindowTitle(QStringLiteral("缺少依赖组件"));
    setMinimumWidth(560);
    setModal(true);

    auto* mainLayout = new QVBoxLayout(this);

    // ---------- 标题 ----------
    m_titleLabel = new QLabel(hint.title.isEmpty()
                                  ? QStringLiteral("缺少依赖组件")
                                  : hint.title,
                              this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    mainLayout->addWidget(m_titleLabel);

    // ---------- 说明 ----------
    m_descLabel = new QLabel(hint.description, this);
    m_descLabel->setWordWrap(true);
    mainLayout->addWidget(m_descLabel);

    // ---------- 命令框 ----------
    if (!m_command.isEmpty()) {
        auto* cmdHint = new QLabel(QStringLiteral("在终端中执行："), this);
        mainLayout->addWidget(cmdHint);

        m_commandEdit = new QPlainTextEdit(this);
        m_commandEdit->setPlainText(m_command);
        m_commandEdit->setReadOnly(true);
        m_commandEdit->setFixedHeight(48);
        QFont monoFont(QStringLiteral("Menlo"), 11);
#ifdef Q_OS_WIN
        monoFont.setFamily(QStringLiteral("Consolas"));
#endif
        m_commandEdit->setFont(monoFont);
        mainLayout->addWidget(m_commandEdit);
    }

    // ---------- 按钮行 ----------
    auto* btnRow = new QHBoxLayout;

    if (!m_command.isEmpty()) {
        m_copyButton = new QPushButton(QStringLiteral("复制命令"), this);
        connect(m_copyButton, &QPushButton::clicked,
                this, &ToolchainDialog::onCopyCommand);
        btnRow->addWidget(m_copyButton);
    }

    if (!hint.detailUrl.isEmpty()) {
        m_detailButton = new QPushButton(QStringLiteral("查看详情"), this);
        connect(m_detailButton, &QPushButton::clicked,
                this, &ToolchainDialog::onOpenDetailUrl);
        btnRow->addWidget(m_detailButton);
    }

    btnRow->addStretch();

    m_laterButton = new QPushButton(QStringLiteral("稍后"), this);
    connect(m_laterButton, &QPushButton::clicked,
            this, &QDialog::reject);
    btnRow->addWidget(m_laterButton);

    m_recheckButton = new QPushButton(QStringLiteral("我已安装，重新检测"), this);
    m_recheckButton->setDefault(true);
    connect(m_recheckButton, &QPushButton::clicked,
            this, &QDialog::accept);
    btnRow->addWidget(m_recheckButton);

    mainLayout->addLayout(btnRow);
}

void ToolchainDialog::onCopyCommand()
{
    if (m_command.isEmpty()) return;
    QApplication::clipboard()->setText(m_command);
    if (m_copyButton) {
        m_copyButton->setText(QStringLiteral("已复制 ✓"));
        // 1.5 秒后恢复
        QTimer::singleShot(1500, this, [this]() {
            if (m_copyButton) m_copyButton->setText(QStringLiteral("复制命令"));
        });
    }
}

void ToolchainDialog::onOpenDetailUrl()
{
    // 通过 ToolchainManager 拿到 installHint 时保存的 url 最好，
    // 但这里简化处理：直接打开 llvm.org
    // 未来可以把 url 存成成员变量
    QDesktopServices::openUrl(QUrl(QStringLiteral("https://llvm.org/")));
}

} // namespace SprintIDE
