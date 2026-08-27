#include "find_dialog.h"
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QShortcut>

namespace SprintIDE {

FindDialog::FindDialog(QPlainTextEdit* editor, QWidget* parent)
    : QDialog(parent)
    , m_editor(editor)
{
    setWindowTitle("查找");
    setFixedWidth(400);
    setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);

    m_findLineEdit = new QLineEdit(this);
    m_findLineEdit->setPlaceholderText("输入查找内容...");
    connect(m_findLineEdit, &QLineEdit::textChanged, this, &FindDialog::onTextChanged);
    connect(m_findLineEdit, &QLineEdit::returnPressed, this, &FindDialog::onFindNext);

    m_caseSensitiveCheck = new QCheckBox("大小写敏感", this);
    m_wholeWordCheck = new QCheckBox("全字匹配", this);

    m_findNextButton = new QPushButton("查找下一个", this);
    m_findNextButton->setDefault(true);
    m_findNextButton->setEnabled(false);
    connect(m_findNextButton, &QPushButton::clicked, this, &FindDialog::onFindNext);

    m_findPreviousButton = new QPushButton("查找上一个", this);
    m_findPreviousButton->setEnabled(false);
    connect(m_findPreviousButton, &QPushButton::clicked, this, &FindDialog::onFindPrevious);

    m_closeButton = new QPushButton("关闭", this);
    connect(m_closeButton, &QPushButton::clicked, this, &FindDialog::close);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QHBoxLayout* findLayout = new QHBoxLayout();
    findLayout->addWidget(m_findLineEdit);
    findLayout->addWidget(m_findNextButton);
    findLayout->addWidget(m_findPreviousButton);
    mainLayout->addLayout(findLayout);

    QHBoxLayout* optionLayout = new QHBoxLayout();
    optionLayout->addWidget(m_caseSensitiveCheck);
    optionLayout->addWidget(m_wholeWordCheck);
    optionLayout->addStretch();
    mainLayout->addLayout(optionLayout);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_closeButton);
    mainLayout->addLayout(buttonLayout);

    QShortcut* escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escShortcut, &QShortcut::activated, this, &FindDialog::close);

    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}

void FindDialog::onFindNext()
{
    if (m_findLineEdit->text().isEmpty()) return;
    m_lastFindText = m_findLineEdit->text();
    emit findNext(m_lastFindText, m_caseSensitiveCheck->isChecked(), m_wholeWordCheck->isChecked());
}

void FindDialog::onFindPrevious()
{
    if (m_findLineEdit->text().isEmpty()) return;
    m_lastFindText = m_findLineEdit->text();
    emit findPrevious(m_lastFindText, m_caseSensitiveCheck->isChecked(), m_wholeWordCheck->isChecked());
}

void FindDialog::onTextChanged(const QString& text)
{
    bool hasText = !text.isEmpty();
    m_findNextButton->setEnabled(hasText);
    m_findPreviousButton->setEnabled(hasText);
}

} // namespace SprintIDE
