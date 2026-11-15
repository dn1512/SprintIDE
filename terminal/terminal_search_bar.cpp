// SPDX-License-Identifier: GPL-3.0-or-later
#include "terminal_search_bar.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>

#include "theme_manager.h"

namespace SprintIDE {

TerminalSearchBar::TerminalSearchBar(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(6, 4, 6, 4);
    layout->setSpacing(4);

    m_edit = new QLineEdit(this);
    m_edit->setPlaceholderText(tr("查找…"));
    m_edit->setClearButtonEnabled(true);
    m_edit->setMinimumWidth(180);
    m_edit->setMaximumWidth(320);
    layout->addWidget(m_edit, 1);

    m_caseBtn = new QToolButton(this);
    m_caseBtn->setText(QStringLiteral("Aa"));
    m_caseBtn->setCheckable(true);
    m_caseBtn->setToolTip(tr("区分大小写"));
    m_caseBtn->setAutoRaise(true);
    m_caseBtn->setFixedSize(24, 24);
    layout->addWidget(m_caseBtn);

    m_prevBtn = new QToolButton(this);
    m_prevBtn->setText(QStringLiteral("▲"));
    m_prevBtn->setToolTip(tr("上一个 (Shift+Enter)"));
    m_prevBtn->setAutoRaise(true);
    m_prevBtn->setFixedSize(24, 24);
    layout->addWidget(m_prevBtn);

    m_nextBtn = new QToolButton(this);
    m_nextBtn->setText(QStringLiteral("▼"));
    m_nextBtn->setToolTip(tr("下一个 (Enter)"));
    m_nextBtn->setAutoRaise(true);
    m_nextBtn->setFixedSize(24, 24);
    layout->addWidget(m_nextBtn);

    m_countLabel = new QLabel(this);
    m_countLabel->setText(QStringLiteral("0/0"));
    m_countLabel->setMinimumWidth(50);
    m_countLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_countLabel);

    m_closeBtn = new QToolButton(this);
    m_closeBtn->setText(QStringLiteral("×"));
    m_closeBtn->setToolTip(tr("关闭 (Esc)"));
    m_closeBtn->setAutoRaise(true);
    m_closeBtn->setFixedSize(24, 24);
    layout->addWidget(m_closeBtn);

    layout->addStretch();

    connect(m_edit, &QLineEdit::textChanged,
            this, &TerminalSearchBar::onTextChanged);
    connect(m_edit, &QLineEdit::returnPressed, this, [this]() {
        if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
            emit findPrev();
        } else {
            emit findNext();
        }
    });
    connect(m_caseBtn, &QToolButton::toggled,
            this, &TerminalSearchBar::onCaseToggled);
    connect(m_prevBtn, &QToolButton::clicked, this, [this]() { emit findPrev(); });
    connect(m_nextBtn, &QToolButton::clicked, this, [this]() { emit findNext(); });
    connect(m_closeBtn, &QToolButton::clicked, this, [this]() { emit closeRequested(); });

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &TerminalSearchBar::applyTheme);
}

void TerminalSearchBar::showAndFocus(const QString& initialText)
{
    show();
    if (!initialText.isEmpty()) {
        m_edit->setText(initialText);
        m_edit->selectAll();
    }
    m_edit->setFocus();
}

void TerminalSearchBar::setMatchInfo(int current, int total)
{
    if (total <= 0) {
        m_countLabel->setText(QStringLiteral("0/0"));
    } else {
        m_countLabel->setText(QStringLiteral("%1/%2").arg(current + 1).arg(total));
    }
}

QString TerminalSearchBar::query() const
{
    return m_edit ? m_edit->text() : QString();
}

void TerminalSearchBar::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        emit closeRequested();
        e->accept();
        return;
    }
    QWidget::keyPressEvent(e);
}

void TerminalSearchBar::showEvent(QShowEvent* e)
{
    QWidget::showEvent(e);
    m_edit->setFocus();
}

void TerminalSearchBar::onTextChanged()
{
    emit searchChanged(m_edit->text(), m_caseBtn->isChecked());
}

void TerminalSearchBar::onCaseToggled(bool on)
{
    Q_UNUSED(on);
    emit searchChanged(m_edit->text(), m_caseBtn->isChecked());
}

void TerminalSearchBar::applyTheme()
{
    auto& tm = ThemeManager::instance();
    const QString bg     = tm.colorString(ThemeManager::UIElement::MainWindowBackground);
    const QString border = tm.colorString(ThemeManager::UIElement::BorderColor);
    const QString text   = tm.colorString(ThemeManager::UIElement::TextColor);
    const QString hover  = tm.colorString(ThemeManager::UIElement::HoverBackground);

    setStyleSheet(QStringLiteral(
        "TerminalSearchBar { background: %1; border-bottom: 1px solid %2; }"
        "QLineEdit { background: transparent; border: 1px solid %2;"
        "  border-radius: 3px; padding: 2px 6px; color: %3; }"
        "QLineEdit:focus { border-color: %4; }"
        "QToolButton { background: transparent; border: none;"
        "  color: %3; border-radius: 3px; }"
        "QToolButton:hover { background: %4; }"
        "QToolButton:checked { background: %4; }"
        "QLabel { color: %3; }")
        .arg(bg, border, text, hover));
}

} // namespace SprintIDE