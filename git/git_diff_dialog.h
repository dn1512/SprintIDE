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
#ifndef SPRINTIDE_GIT_DIFF_DIALOG_H
#define SPRINTIDE_GIT_DIFF_DIALOG_H

#include <QDialog>
#include <QString>

QT_BEGIN_NAMESPACE
class QTextEdit;
class QLabel;
class QPushButton;
class QToolButton;
QT_END_NAMESPACE

namespace SprintIDE {
namespace Git {

/**
 * @brief 通用 Git diff 查看窗口
 *
 * 使用场景：
 *   1. 双击 GitPanel 里未暂存的文件 → 显示工作区 vs 暂存区的 diff
 *   2. 双击 GitPanel 里已暂存的文件 → 显示暂存区 vs HEAD 的 diff
 *   3. 双击 GitPanel 里的历史条目   → 显示该 commit 的完整 diff
 *
 * 特性：
 *   - HTML 着色渲染（+/-/@@/header 各自颜色）
 *   - 左上角标题 + 右上角工具按钮
 *   - 顶部"打开文件"按钮（仅文件级 diff 时可用）
 *   - 行数上限保护（默认 5 万行）
 */
class GitDiffDialog : public QDialog
{
    Q_OBJECT
public:
    explicit GitDiffDialog(QWidget* parent = nullptr);
    ~GitDiffDialog() override;

    // ---- 显示：单文件 diff ----
    //   relativePath 为空 → 全仓库 diff
    //   openFileAbsolutePath 非空时启用"打开文件"按钮
    void showFileDiff(const QString& title,
                      const QString& diffText,
                      const QString& openFileAbsolutePath = QString());

    // ---- 显示：commit diff ----
    void showCommitDiff(const QString& title,
                        const QString& diffText);

signals:
    void openFileRequested(const QString& absolutePath);

private slots:
    void onOpenFileClicked();
    void onCopyClicked();

private:
    void setupUI();
    void renderDiff(const QString& diffText);
    void setOpenFileButtonEnabled(const QString& absPath);

    QLabel*      m_titleLabel    = nullptr;
    QLabel*      m_statsLabel    = nullptr;
    QTextEdit*   m_diffView      = nullptr;
    QPushButton* m_openFileBtn   = nullptr;
    QToolButton* m_copyBtn       = nullptr;
    QToolButton* m_closeBtn      = nullptr;

    QString m_openFileAbsPath;
};

} // namespace Git
} // namespace SprintIDE

#endif // SPRINTIDE_GIT_DIFF_DIALOG_H
