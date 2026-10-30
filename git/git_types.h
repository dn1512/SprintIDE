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
#ifndef SPRINTIDE_GIT_TYPES_H
#define SPRINTIDE_GIT_TYPES_H

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QList>

namespace SprintIDE {
namespace Git {

// ============================================================
// 单文件状态（对应 git status --porcelain 的 XY）
// ============================================================
enum class FileStatus {
    Unmodified,
    Added,
    Modified,
    Deleted,
    Renamed,
    Copied,
    Untracked,
    Ignored,
    Conflicted,
    TypeChanged,
    Unknown
};

struct GitFileStatus {
    QString path;                    // 相对仓库根的路径
    QString absolutePath;            // 绝对路径（供编辑器/项目树使用）
    QString origPath;                // 重命名时的原路径
    FileStatus indexStatus    = FileStatus::Unmodified;   // X：暂存区
    FileStatus workTreeStatus = FileStatus::Unmodified;   // Y：工作区

    bool isStaged() const {
        return indexStatus != FileStatus::Unmodified &&
               indexStatus != FileStatus::Untracked &&
               indexStatus != FileStatus::Ignored;
    }
    bool hasUnstagedChanges() const {
        return workTreeStatus != FileStatus::Unmodified &&
               workTreeStatus != FileStatus::Untracked &&
               workTreeStatus != FileStatus::Ignored;
    }
    bool isUntracked() const {
        return indexStatus == FileStatus::Untracked ||
               workTreeStatus == FileStatus::Untracked;
    }
    bool hasConflict() const {
        return indexStatus == FileStatus::Conflicted ||
               workTreeStatus == FileStatus::Conflicted;
    }
};

// ============================================================
// 提交
// ============================================================
struct GitCommit {
    QString hash;
    QString shortHash;
    QString author;
    QString email;
    QString message;
    QDateTime timestamp;
    QStringList parents;
    QStringList refs;      // 分支/标签装饰（%D）
};

// ============================================================
// 分支
// ============================================================
struct GitBranch {
    QString name;
    bool isCurrent = false;
    bool isRemote  = false;
    QString upstream;      // 跟踪的远程分支，如 origin/main
    int ahead  = 0;
    int behind = 0;
};

// ============================================================
// 进行中的 Git 操作（merge / rebase / cherry-pick / revert）
// ============================================================
enum class GitOperation {
    None,
    Merge,        // 正在合并（.git/MERGE_HEAD 存在）
    Rebase,       // 正在 rebase（.git/rebase-merge 或 .git/rebase-apply 存在）
    CherryPick,   // 正在 cherry-pick（.git/CHERRY_PICK_HEAD 存在）
    Revert        // 正在 revert（.git/REVERT_HEAD 存在）
};

inline QString gitOperationName(GitOperation op)
{
    switch (op) {
    case GitOperation::Merge:      return QStringLiteral("合并");
    case GitOperation::Rebase:     return QStringLiteral("Rebase");
    case GitOperation::CherryPick: return QStringLiteral("Cherry-pick");
    case GitOperation::Revert:     return QStringLiteral("Revert");
    default:                       return QString();
    }
}

// ============================================================
// 仓库信息
// ============================================================
struct GitRepositoryInfo {
    QString rootPath;
    QString currentBranch;
    bool isValid        = false;
    bool isDetachedHead = false;
    bool hasRemote      = false;
    bool isEmpty        = false;   // 还没有任何提交
    QString remoteName;            // origin / upstream
    QString remoteUrl;

    // 进行中的操作（merge / rebase 等）
    GitOperation operation = GitOperation::None;
    QString      operationDetail;  // 例如 "main ← feature/xxx"，可留空

    bool hasOperationInProgress() const {
        return operation != GitOperation::None;
    }
};

// ============================================================
// Stash
// ============================================================
struct GitStash {
    int       index      = 0;         // stash@{N} 里的 N
    QString   ref;                    // "stash@{0}"
    QString   branch;                 // 原分支名（从 reflog subject 解析）
    QString   message;                // stash 消息
    QString   commitHash;
    QDateTime timestamp;
};

// ============================================================
// 当前分支与上游的关系
// ============================================================
struct UpstreamStatus {
    bool    hasUpstream = false;   // 是否配置了 @{u}
    QString branch;                // 本地分支名
    QString upstream;              // 例如 origin/main
    int     ahead  = 0;            // 本地领先上游的提交数（待 push）
    int     behind = 0;            // 本地落后上游的提交数（待 pull）

    bool isDiverge() const { return ahead > 0 && behind > 0; }
};

// ============================================================
// Diff
// ============================================================
struct GitDiffLine {
    enum Type { Context, Added, Removed };
    Type    type = Context;
    QString text;
};

struct GitDiffHunk {
    int oldStart = 0;
    int oldLines = 0;
    int newStart = 0;
    int newLines = 0;
    QString header;
    QList<GitDiffLine> lines;
};

struct GitDiff {
    QString filePath;
    bool isBinary = false;
    bool isEmpty  = false;
    QList<GitDiffHunk> hunks;
};

// ============================================================
// 字符 枚举
// ============================================================
inline FileStatus statusFromChar(QChar c)
{
    switch (c.unicode()) {
    case 'A': return FileStatus::Added;
    case 'M': return FileStatus::Modified;
    case 'D': return FileStatus::Deleted;
    case 'R': return FileStatus::Renamed;
    case 'C': return FileStatus::Copied;
    case '?': return FileStatus::Untracked;
    case '!': return FileStatus::Ignored;
    case 'U': return FileStatus::Conflicted;
    case 'T': return FileStatus::TypeChanged;
    default:  return FileStatus::Unmodified;
    }
}

inline QString statusToString(FileStatus s)
{
    switch (s) {
    case FileStatus::Added:       return QStringLiteral("A");
    case FileStatus::Modified:    return QStringLiteral("M");
    case FileStatus::Deleted:     return QStringLiteral("D");
    case FileStatus::Renamed:     return QStringLiteral("R");
    case FileStatus::Copied:      return QStringLiteral("C");
    case FileStatus::Untracked:   return QStringLiteral("?");
    case FileStatus::Ignored:     return QStringLiteral("!");
    case FileStatus::Conflicted:  return QStringLiteral("U");
    case FileStatus::TypeChanged: return QStringLiteral("T");
    default:                      return QStringLiteral(" ");
    }
}

} // namespace Git
} // namespace SprintIDE

#endif // SPRINTIDE_GIT_TYPES_H
