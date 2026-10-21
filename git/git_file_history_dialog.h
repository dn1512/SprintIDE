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
#ifndef SPRINTIDE_GIT_FILE_HISTORY_DIALOG_H
#define SPRINTIDE_GIT_FILE_HISTORY_DIALOG_H

#include <QDialog>
#include <QString>
#include <QList>

#include "git_types.h"

QT_BEGIN_NAMESPACE
class QListWidget;
class QListWidgetItem;
class QLabel;
QT_END_NAMESPACE

namespace SprintIDE {
namespace Git {

/**
 * @brief 单个文件的提交历史窗口
 *
 * 数据来源：git log --follow -- <file>
 * 双击条目：打开该 commit 的 diff 窗口
 */
class GitFileHistoryDialog : public QDialog
{
    Q_OBJECT
public:
    // absoluteFilePath : 文件绝对路径
    // relativePath     : 相对仓库根路径（git 命令用）
    GitFileHistoryDialog(const QString& absoluteFilePath,
                         const QString& relativePath,
                         QWidget* parent = nullptr);
    ~GitFileHistoryDialog() override;

private slots:
    void onItemDoubleClicked(QListWidgetItem* item);

private:
    void setupUI();
    void loadHistory();

    QString         m_absoluteFilePath;
    QString         m_relativePath;
    QLabel*         m_titleLabel = nullptr;
    QLabel*         m_statsLabel = nullptr;
    QListWidget*    m_list       = nullptr;
};

} // namespace Git
} // namespace SprintIDE

#endif // SPRINTIDE_GIT_FILE_HISTORY_DIALOG_H