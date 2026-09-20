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