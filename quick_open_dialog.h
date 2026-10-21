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
#ifndef QUICK_OPEN_DIALOG_H
#define QUICK_OPEN_DIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>

class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QLabel;

namespace SprintIDE {

/**
 * @brief 快速打开文件对话框（类似 VS Code 的 Cmd+P）
 *
 * 使用：
 *   QuickOpenDialog dlg(parent);
 *   dlg.setFileList(files, projectRoot);
 *   connect(&dlg, &QuickOpenDialog::fileChosen, this, [](const QString& p){...});
 *   dlg.exec();
 */
class QuickOpenDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QuickOpenDialog(QWidget* parent = nullptr);

    /// 设置候选文件（绝对路径列表）+ 用于显示相对路径的项目根
    void setFileList(const QStringList& absolutePaths, const QString& projectRoot);

    /// 当前选中的文件绝对路径（无选中时返回空）
    QString selectedFilePath() const;

signals:
    /// 用户在列表上双击 / 输入框回车确认
    void fileChosen(const QString& absolutePath);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void showEvent(QShowEvent* e) override;

private slots:
    void onTextChanged(const QString& text);
    void onItemActivated(QListWidgetItem* item);

private:
    int  scoreMatch(const QString& absPath,
                   const QString& fileName,
                   const QString& query) const;
    void refreshList(const QString& query);

    QLineEdit*   m_searchEdit = nullptr;
    QListWidget* m_listWidget = nullptr;
    QLabel*      m_hintLabel  = nullptr;

    QStringList  m_allFiles;      // 绝对路径
    QString      m_projectRoot;
    bool         m_centered = false;
};

} // namespace SprintIDE

#endif // QUICK_OPEN_DIALOG_H
