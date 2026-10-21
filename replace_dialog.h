// SPDX-License-Identifier: GPL-3.0-or-later
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
