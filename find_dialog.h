// find_dialog.h
#ifndef FIND_DIALOG_H
#define FIND_DIALOG_H

#include <QDialog>
#include <QTextCursor>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QCheckBox;
class QPushButton;
class QPlainTextEdit;  // 改为 QPlainTextEdit
QT_END_NAMESPACE


namespace SprintIDE {
class FindDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindDialog(QPlainTextEdit* editor, QWidget* parent = nullptr);  // 参数改为 QPlainTextEdit*

signals:
    void findNext(const QString& text, bool caseSensitive, bool wholeWord);
    void findPrevious(const QString& text, bool caseSensitive, bool wholeWord);

private slots:
    void onFindNext();
    void onFindPrevious();
    void onTextChanged(const QString& text);

private:
    QPlainTextEdit* m_editor;  // 改为 QPlainTextEdit*
    QLineEdit* m_findLineEdit;
    QCheckBox* m_caseSensitiveCheck;
    QCheckBox* m_wholeWordCheck;
    QPushButton* m_findNextButton;
    QPushButton* m_findPreviousButton;
    QPushButton* m_closeButton;
    QString m_lastFindText;
};
}
#endif // FIND_DIALOG_H
