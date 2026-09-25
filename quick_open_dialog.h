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
