#ifndef REPLACE_DIALOG_H
#define REPLACE_DIALOG_H

#include <QDialog>
#include <QTextCursor>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QCheckBox;
class QPushButton;
class QPlainTextEdit;  // 改为 QPlainTextEdit
QT_END_NAMESPACE


namespace SprintIDE {
class ReplaceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReplaceDialog(QPlainTextEdit* editor, QWidget* parent = nullptr);  // 改为 QPlainTextEdit*

signals:
    void findNext(const QString& text, bool caseSensitive, bool wholeWord);
    void replaceCurrent(const QString& findText, const QString& replaceText,
                        bool caseSensitive, bool wholeWord);
    void replaceAll(const QString& findText, const QString& replaceText,
                    bool caseSensitive, bool wholeWord);

private slots:
    void onFindNext();
    void onReplaceCurrent();
    void onReplaceAll();
    void onTextChanged(const QString& text);

private:
    QPlainTextEdit* m_editor;  // 改为 QPlainTextEdit*
    QLineEdit* m_findLineEdit;
    QLineEdit* m_replaceLineEdit;
    QCheckBox* m_caseSensitiveCheck;
    QCheckBox* m_wholeWordCheck;
    QPushButton* m_findNextButton;
    QPushButton* m_replaceButton;
    QPushButton* m_replaceAllButton;
    QPushButton* m_closeButton;
};
}
#endif // REPLACE_DIALOG_H
