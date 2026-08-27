#include "replace_dialog.h"
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QShortcut>
#include <QMessageBox>
#include <QLabel>

namespace SprintIDE {

ReplaceDialog::ReplaceDialog(QPlainTextEdit* editor, QWidget* parent)
    : QDialog(parent)
    , m_editor(editor)
{
    setWindowTitle("查找与替换");
    setFixedWidth(450);
    setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);

    m_findLineEdit = new QLineEdit(this);
    m_findLineEdit->setPlaceholderText("查找内容...");
    connect(m_findLineEdit, &QLineEdit::textChanged, this, &ReplaceDialog::onTextChanged);
    connect(m_findLineEdit, &QLineEdit::returnPressed, this, &ReplaceDialog::onFindNext);

    m_replaceLineEdit = new QLineEdit(this);
    m_replaceLineEdit->setPlaceholderText("替换为...");
    connect(m_replaceLineEdit, &QLineEdit::returnPressed, this, &ReplaceDialog::onReplaceCurrent);

    m_caseSensitiveCheck = new QCheckBox("大小写敏感", this);
    m_wholeWordCheck = new QCheckBox("全字匹配", this);

    m_findNextButton = new QPushButton("查找下一个", this);
    m_findNextButton->setEnabled(false);
    connect(m_findNextButton, &QPushButton::clicked, this, &ReplaceDialog::onFindNext);

    m_replaceButton = new QPushButton("替换", this);
    m_replaceButton->setEnabled(false);
    connect(m_replaceButton, &QPushButton::clicked, this, &ReplaceDialog::onReplaceCurrent);

    m_replaceAllButton = new QPushButton("全部替换", this);
    m_replaceAllButton->setEnabled(false);
    connect(m_replaceAllButton, &QPushButton::clicked, this, &ReplaceDialog::onReplaceAll);

    m_closeButton = new QPushButton("关闭", this);
    connect(m_closeButton, &QPushButton::clicked, this, &ReplaceDialog::close);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QHBoxLayout* findLayout = new QHBoxLayout();
    findLayout->addWidget(new QLabel("查找:"));
    findLayout->addWidget(m_findLineEdit);
    findLayout->addWidget(m_findNextButton);
    mainLayout->addLayout(findLayout);

    QHBoxLayout* replaceLayout = new QHBoxLayout();
    replaceLayout->addWidget(new QLabel("替换:"));
    replaceLayout->addWidget(m_replaceLineEdit);
    replaceLayout->addWidget(m_replaceButton);
    replaceLayout->addWidget(m_replaceAllButton);
    mainLayout->addLayout(replaceLayout);

    QHBoxLayout* optionLayout = new QHBoxLayout();
    optionLayout->addWidget(m_caseSensitiveCheck);
    optionLayout->addWidget(m_wholeWordCheck);
    optionLayout->addStretch();
    optionLayout->addWidget(m_closeButton);
    mainLayout->addLayout(optionLayout);

    QShortcut* escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escShortcut, &QShortcut::activated, this, &ReplaceDialog::close);

    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}

void ReplaceDialog::onFindNext()
{
    if (m_findLineEdit->text().isEmpty()) return;
    emit findNext(m_findLineEdit->text(),
                  m_caseSensitiveCheck->isChecked(),
                  m_wholeWordCheck->isChecked());
}

void ReplaceDialog::onReplaceCurrent()
{
    if (m_findLineEdit->text().isEmpty()) return;
    emit replaceCurrent(m_findLineEdit->text(),
                        m_replaceLineEdit->text(),
                        m_caseSensitiveCheck->isChecked(),
                        m_wholeWordCheck->isChecked());
}

void ReplaceDialog::onReplaceAll()
{
    if (m_findLineEdit->text().isEmpty()) {
        QMessageBox::information(this, "提示", "请输入要查找的内容");
        return;
    }
    emit replaceAll(m_findLineEdit->text(),
                    m_replaceLineEdit->text(),
                    m_caseSensitiveCheck->isChecked(),
                    m_wholeWordCheck->isChecked());
}

void ReplaceDialog::onTextChanged(const QString& text)
{
    bool hasText = !text.isEmpty();
    m_findNextButton->setEnabled(hasText);
    m_replaceButton->setEnabled(hasText);
    m_replaceAllButton->setEnabled(hasText);
}

} // namespace SprintIDE
