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

    // 强制重新检测仓库信息（分支、远程、操作状态）
    //   merge / rebase / checkout / pull 等改变仓库状态的操作后调用
    void refreshRepositoryInfo();

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
    void createBranchAndCheckout(const QString& name, const QString& base = QString());
    void checkoutBranch(const QString& name);

    // ---------- 远程 ----------
    void fetch();
    void pull();
    void push();
    bool hasRemote() const;

        // ---------- 历史 / Diff ----------
    QList<GitCommit> log(int maxCount = 100, const QString& branch = QString()) const;

    // 单个文件的提交历史（带 --follow，跟踪重命名）
    QList<GitCommit> fileLog(const QString& relativePath, int maxCount = 200) const;

    // 文件当前所在的 commit（HEAD 上的最新相关提交）
    //   relativePath 为空时返回 HEAD
    GitCommit fileRevision(const QString& relativePath) const;

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

    // ---------- 冲突解决 ----------
    // 这两个方法只做 git 层面的 --ours/--theirs，语义取决于仓库当前操作：
    //   merge  : --ours   = 当前分支（HEAD）
    //            --theirs = 被合入的分支
    //   rebase : --ours   = 目标分支（base）
    //            --theirs = 你正在重放的提交
    //
    // 调用方（UI）必须根据 repositoryInfo().operation 正确选择，
    // 否则会给用户"点了保留我的，结果保留了别人的"的误导。
    // 便捷判断见 isRebaseInProgress()。
    void acceptOurs(const QString& relativePath);     // 保留 ours 一侧
    void acceptTheirs(const QString& relativePath);   // 保留 theirs 一侧
    void markResolved(const QString& relativePath);   // 已手动解决，标记为已暂存

    // 当前是否处于 rebase 冲突中
    // UI 用它决定 ours/theirs 的语义方向
    bool isRebaseInProgress() const;

    // ---------- 丢弃更改 ----------
    // 对未跟踪文件 → 删除文件
    // 对已跟踪文件 → git checkout -- <path>（从索引恢复）
    void discardFileChanges(const QString& relativePath);

    // ---------- 合并 / Rebase ----------
    // 合并 branch 到当前分支
    void merge(const QString& branch);
    // 把当前分支 rebase 到 ontoBranch 上
    void rebase(const QString& ontoBranch);
    // 中止进行中的合并 / rebase
    void abortMerge();
    void abortRebase();
    // 继续 rebase（冲突解决后调用）
    void continueRebase();

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
