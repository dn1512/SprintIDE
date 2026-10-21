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
#ifndef SEARCH_IN_FILES_DIALOG_H
#define SEARCH_IN_FILES_DIALOG_H

#include <QDialog>
#include <QRegularExpression>
#include <QStringList>
#include <QShowEvent>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QCheckBox;
class QPushButton;
class QLabel;
class QTreeWidget;
class QTreeWidgetItem;
class QProgressBar;
QT_END_NAMESPACE

namespace SprintIDE {

/**
 * @brief 全局搜索对话框（在项目文件中查找）
 *
 * 设计要点：
 *  - 文件列表由外部注入（MainWindow::collectProjectFiles），本类不负责扫描目录
 *  - 分批异步处理文件，每批 kBatchSize 个文件后让出事件循环，避免阻塞 UI
 *  - 结果按文件分组展示，双击跳转到对应行
 *  - 上限 kMaxMatches 条，防止大项目一次列出几十万条
 */
class SearchInFilesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SearchInFilesDialog(const QString& rootPath = QString(),
                                 QWidget* parent = nullptr);
    ~SearchInFilesDialog() override;

    void setRootPath(const QString& path);
    void setFileList(const QStringList& files);
    void setSearchText(const QString& text);
    QString searchText() const;

signals:
    /// 双击某条结果时发出，line / column 均为 1-based
    void openFileAt(const QString& filePath, int line, int column);

protected:
    void showEvent(QShowEvent* event) override;

private slots:
    void onSearchClicked();
    void onCancelClicked();
    void onSearchTextChanged(const QString& text);
    void onRegexToggled(bool checked);
    void onItemActivated(QTreeWidgetItem* item, int column);
    void processNextBatch();

private:
    void buildUi();
    void clearResults();
    void finishSearch();
    void restoreInputUi();
    void updateStatusLabel();

    /// 在单行里查找，返回第一个匹配列的 1-based 位置，无匹配返回 -1
    int findInLine(const QString& line) const;

private:
    // ---- 状态 ----
    QString            m_rootPath;
    QStringList        m_fileList;
    int                m_currentIndex     = 0;
    bool               m_searching        = false;
    int                m_totalMatches     = 0;

    QString            m_needle;
    QRegularExpression m_regex;
    bool               m_caseSensitiveFlag = false;
    bool               m_wholeWordFlag     = false;
    bool               m_useRegexFlag      = false;

    // 结果树分组缓存（同一文件的匹配连续 append，无需每次 findItems）
    QString            m_lastFilePath;
    QTreeWidgetItem*   m_lastFileItem = nullptr;

    // ---- UI ----
    QLineEdit*    m_searchEdit     = nullptr;
    QCheckBox*    m_caseSensitive  = nullptr;
    QCheckBox*    m_wholeWord      = nullptr;
    QCheckBox*    m_useRegex       = nullptr;
    QPushButton*  m_searchButton   = nullptr;
    QPushButton*  m_cancelButton   = nullptr;
    QPushButton*  m_closeButton    = nullptr;
    QLabel*       m_statusLabel    = nullptr;
    QProgressBar* m_progressBar    = nullptr;
    QTreeWidget*  m_resultTree     = nullptr;

    static constexpr int kMaxMatches = 5000;
    static constexpr int kBatchSize  = 5;
};

} // namespace SprintIDE

#endif // SEARCH_IN_FILES_DIALOG_H
