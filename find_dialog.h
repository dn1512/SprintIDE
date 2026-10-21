/*
* SprintIDE - SprintIDE is a cross-platform IDE built from scratch with Qt 6 and C++20. Its core capability is to provide AI Agents with an executable, verifiable code execution environment.
* Copyright (C) 2026  Liu Tengjun (刘腾骏)
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
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
