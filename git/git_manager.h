#ifndef SPRINTIDE_GIT_MANAGER_H
#define SPRINTIDE_GIT_MANAGER_H

#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QStringList>

#include "git_types.h"
#include "git_command.h"

namespace SprintIDE {
namespace Git {

class GitCommand;

class GitManager : public QObject
{
    Q_OBJECT
public:
    static GitManager& instance();

    void initialize();
    void shutdown();

    // ---------- 仓库 ----------
    bool isRepository(const QString& path) const;
    bool initRepository(const QString& path, QString* errorMsg = nullptr);
    GitRepositoryInfo repositoryInfo() const;
    bool isActive() const;

    // ---------- 项目绑定 ----------
    void setProjectRoot(const QString& path);
    QString projectRoot() const;

    // ---------- 状态 ----------
    void refreshStatus();
    QList<GitFileStatus> currentStatus() const;

    // ---------- 暂存 ----------
    void stageFile(const QString& relativePath);
    void unstageFile(const QString& relativePath);
    void stageAll();
    void unstageAll();

    // ---------- 提交 ----------
    void commit(const QString& message, bool amend = false);
    void commitFiles(const QStringList& relativePaths, const QString& message);

    // ---------- 分支 ----------
    QList<GitBranch> branches() const;
    void createBranch(const QString& name, const QString& base = QString());
    void checkoutBranch(const QString& name);

    // ---------- 远程 ----------
    void fetch();
    void pull();
    void push();
    bool hasRemote() const;

    // ---------- 历史 / Diff ----------
    QList<GitCommit> log(int maxCount = 100, const QString& branch = QString()) const;
    GitDiff diffFile(const QString& relativePath, bool staged) const;

    // 原始 diff 字符串（用于 Diff 窗口直接渲染）
    //   relativePath 为空 → 显示全仓库 diff（含所有改动文件）
    QString rawDiff(const QString& relativePath, bool staged) const;
    //   某个 commit 的完整 diff（含 commit header + 多文件 diff）
    QString rawCommitDiff(const QString& hash) const;

    // ---------- Stash ----------
    QList<GitStash> stashList() const;   // 同步（很快），立即返回当前 stash 列表

    // 创建 stash
    //   includeUntracked = true  → 走 git stash push -u
    //   thenCheckout 非空 → stash 完成后自动 checkout 该分支
    //                       （用于"先暂存再切分支"流程）
    void stashPush(const QString& message,
                   bool includeUntracked = true,
                   const QString& thenCheckout = QString());

    void stashPop(int index);
    void stashApply(int index);
    void stashDrop(int index);
    void stashClear();

    // ---------- 上游状态 ----------
    // 同步执行（git rev-parse + git rev-list，都很快）
    // 用于 push 前 / pull 前检查分支与上游的关系
    UpstreamStatus currentUpstreamStatus() const;

    // ---------- 用户信息 ----------
    QString userName() const;
    QString userEmail() const;
    bool setUserInfo(const QString& name, const QString& email, bool global = false);

signals:
    void repositoryChanged(const GitRepositoryInfo& info);
    void statusUpdated(const QList<GitFileStatus>& status);
    void commitSucceeded(const QString& hash);
    void commandFailed(const QString& operation, const QString& error);
    void busyChanged(bool busy);
    void stashListUpdated(const QList<GitStash>& stashes);

private:
    explicit GitManager(QObject* parent = nullptr);
    ~GitManager() override;

    GitManager(const GitManager&) = delete;
    GitManager& operator=(const GitManager&) = delete;

    // ---------- 解析 ----------
    QList<GitFileStatus> parseStatusPorcelainV2(const QString& output) const;
    QList<GitCommit>     parseLog(const QString& output) const;
    GitDiff              parseDiff(const QString& output, const QString& filePath) const;

    // ---------- 异步 helper ----------
    void runAsync(const QStringList& args, const QString& tag, int timeoutMs = 60000);
    void onCommandFinished(const QString& tag, const GitCommand::Result& result);

    void setBusy(bool busy);

    class Private;
    QScopedPointer<Private> d;
};

} // namespace Git
} // namespace SprintIDE

#endif // SPRINTIDE_GIT_MANAGER_H
