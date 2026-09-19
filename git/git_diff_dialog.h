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
