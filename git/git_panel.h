// src/git/git_panel.h
#ifndef SPRINTIDE_GIT_PANEL_H
#define SPRINTIDE_GIT_PANEL_H

#include <QWidget>
#include <QString>
#include <QList>

#include "git_types.h"

QT_BEGIN_NAMESPACE
class QComboBox;
class QToolButton;
class QListWidget;
class QListWidgetItem;
class QPlainTextEdit;
class QCheckBox;
class QPushButton;
class QLabel;
class QProgressBar;
class QSplitter;
class QStackedWidget;
QT_END_NAMESPACE

namespace SprintIDE {
namespace Git {

class GitPanel : public QWidget
{
    Q_OBJECT
public:
    explicit GitPanel(QWidget* parent = nullptr);
    ~GitPanel() override;

    void setProjectRoot(const QString& path);
    QString projectRoot() const;

signals:
    // 双击文件 → 交给 MainWindow 打开编辑器
    void fileDoubleClicked(const QString& absolutePath);

private slots:
    void onRepositoryChanged(const GitRepositoryInfo& info);
    void onStatusUpdated(const QList<GitFileStatus>& status);
    void onBusyChanged(bool busy);
    void onCommandFailed(const QString& op, const QString& err);

    void onRefreshClicked();
    void onInitClicked();
    void onBranchActivated(int index);

    void onStageAllClicked();
    void onUnstageAllClicked();
    void onCommitClicked();

    void onStagedItemDoubleClicked(QListWidgetItem* item);
    void onUnstagedItemDoubleClicked(QListWidgetItem* item);
    void onStagedContextMenu(const QPoint& pos);
    void onUnstagedContextMenu(const QPoint& pos);
    void onHistoryItemDoubleClicked(QListWidgetItem* item);

private:
    void setupUI();
    QWidget* createTopBar();
    QWidget* createStatusSection();
    QWidget* createCommitSection();
    QWidget* createHistorySection();

    void populateBranchCombo();
    void populateStatusLists(const QList<GitFileStatus>& status);
    void populateHistory();

    void setBusy(bool busy);
    void setEmptyRepoState(bool emptyRepo);
    void updateCommitButton();

    static QString statusBadge(Git::FileStatus s);
    static QColor  statusColor(Git::FileStatus s);

    // ---- 顶部栏 ----
    QComboBox*    m_branchCombo   = nullptr;
    QToolButton*  m_refreshBtn    = nullptr;
    QToolButton*  m_initBtn       = nullptr;
    QLabel*       m_repoLabel     = nullptr;

    // ---- 状态区 ----
    QLabel*       m_stagedHeader   = nullptr;
    QListWidget*  m_stagedList     = nullptr;
    QLabel*       m_unstagedHeader = nullptr;
    QListWidget*  m_unstagedList   = nullptr;
    QToolButton*  m_stageAllBtn    = nullptr;
    QToolButton*  m_unstageAllBtn  = nullptr;

    // ---- 提交区 ----
    QPlainTextEdit* m_messageEdit = nullptr;
    QCheckBox*      m_amendCheck  = nullptr;
    QPushButton*    m_commitBtn   = nullptr;

    // ---- 历史区 ----
    QListWidget*    m_historyList = nullptr;

    // ---- 整体 ----
    QSplitter*      m_splitter     = nullptr;
    QProgressBar*   m_busyBar      = nullptr;
    QWidget*        m_contentArea  = nullptr;
    QLabel*         m_emptyRepoHint= nullptr;

    QString m_projectRoot;
};

} // namespace Git
} // namespace SprintIDE

#endif // SPRINTIDE_GIT_PANEL_H
