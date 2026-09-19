#include "git_manager.h"
#include "git_command.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QDateTime>
#include <QDebug>

namespace SprintIDE {
namespace Git {

// ============================================================
// Private
// ============================================================
class GitManager::Private
{
public:
    QString              projectRoot;
    GitRepositoryInfo    repoInfo;
    QList<GitFileStatus> status;
    GitCommand*          cmd = nullptr;
    bool                 busy = false;
    mutable QMap<QString, QString> userInfoCache;   // "name" / "email"
};

// ============================================================
// 单例
// ============================================================
GitManager& GitManager::instance()
{
    static GitManager inst;
    return inst;
}

GitManager::GitManager(QObject* parent)
    : QObject(parent)
    , d(new Private)
{
    d->cmd = new GitCommand(this);
    connect(d->cmd, &GitCommand::finished,
            this, &GitManager::onCommandFinished);
}

GitManager::~GitManager() = default;

void GitManager::initialize()
{
    // 无额外初始化
}

void GitManager::shutdown()
{
    if (d->cmd) d->cmd->cancelAsync();
    d->projectRoot.clear();
    d->status.clear();
    d->repoInfo = GitRepositoryInfo();
    d->userInfoCache.clear();
}

// ============================================================
// 仓库操作
// ============================================================
bool GitManager::isRepository(const QString& path) const
{
    if (path.isEmpty() || !QDir(path).exists()) return false;

    GitCommand cmd;
    auto r = cmd.run(path, {"rev-parse", "--is-inside-work-tree"}, 3000);
    return r.ok() && r.stdOut.trimmed() == QLatin1String("true");
}

bool GitManager::initRepository(const QString& path, QString* errorMsg)
{
    if (path.isEmpty() || !QDir(path).exists()) {
        if (errorMsg) *errorMsg = QStringLiteral("目录不存在: ") + path;
        return false;
    }

    GitCommand cmd;
    auto r = cmd.run(path, {"init"}, 15000);
    if (!r.ok()) {
        if (errorMsg) *errorMsg = r.stdErr.trimmed();
        return false;
    }

    if (path == d->projectRoot) {
        // 清缓存重新探测
        QString saved = d->projectRoot;
        d->projectRoot.clear();
        setProjectRoot(saved);
    }
    return true;
}

GitRepositoryInfo GitManager::repositoryInfo() const
{
    return d->repoInfo;
}

bool GitManager::isActive() const
{
    return d->repoInfo.isValid && !d->projectRoot.isEmpty();
}

// ============================================================
// 项目绑定
// ============================================================
void GitManager::setProjectRoot(const QString& path)
{
    if (d->projectRoot == path) return;

    d->projectRoot = path;
    d->status.clear();
    d->repoInfo = GitRepositoryInfo();
    d->userInfoCache.clear();

    if (path.isEmpty() || !QDir(path).exists()) {
        emit repositoryChanged(d->repoInfo);
        emit statusUpdated(d->status);
        return;
    }

    GitCommand cmd;

    // ---- 检测是否是 git 仓库 ----
    auto rTop = cmd.run(path, {"rev-parse", "--show-toplevel"}, 3000);
    if (!rTop.ok()) {
        d->repoInfo.isValid = false;
        emit repositoryChanged(d->repoInfo);
        emit statusUpdated(d->status);
        return;
    }

    d->repoInfo.rootPath = QDir(rTop.stdOut.trimmed()).absolutePath();
    d->repoInfo.isValid  = true;

    // ---- 当前分支 / HEAD ----
    auto rBranch = cmd.run(d->repoInfo.rootPath,
                           {"rev-parse", "--abbrev-ref", "HEAD"}, 3000);
    if (rBranch.ok()) {
        const QString b = rBranch.stdOut.trimmed();
        if (b == QLatin1String("HEAD")) {
            d->repoInfo.isDetachedHead = true;
            auto rHash = cmd.run(d->repoInfo.rootPath,
                                 {"rev-parse", "--short", "HEAD"}, 3000);
            d->repoInfo.currentBranch = rHash.ok() ? rHash.stdOut.trimmed() : QString();
        } else {
            d->repoInfo.isDetachedHead = false;
            d->repoInfo.currentBranch  = b;
        }
    }

    // ---- 空仓库？（还没有任何提交） ----
    auto rHead = cmd.run(d->repoInfo.rootPath,
                         {"rev-parse", "--verify", "HEAD"}, 3000);
    d->repoInfo.isEmpty = !rHead.ok();

    // ---- 远程 ----
    auto rRemotes = cmd.run(d->repoInfo.rootPath, {"remote"}, 3000);
    if (rRemotes.ok()) {
        const QStringList remotes = rRemotes.stdOut.split('\n', Qt::SkipEmptyParts);
        if (!remotes.isEmpty()) {
            d->repoInfo.remoteName = remotes.first().trimmed();
            d->repoInfo.hasRemote  = true;
            auto rUrl = cmd.run(d->repoInfo.rootPath,
                                {"remote", "get-url", d->repoInfo.remoteName}, 3000);
            if (rUrl.ok()) d->repoInfo.remoteUrl = rUrl.stdOut.trimmed();
        }
    }

    emit repositoryChanged(d->repoInfo);
    refreshStatus();
}

QString GitManager::projectRoot() const
{
    return d->projectRoot;
}

// ============================================================
// 状态
// ============================================================
void GitManager::refreshStatus()
{
    if (!d->repoInfo.isValid) {
        d->status.clear();
        emit statusUpdated(d->status);
        return;
    }
    // porcelain v2 + -z：用 NUL 分隔记录，路径不做引号转义
    runAsync({"status", "--porcelain=v2", "--untracked-files=all", "-z"},
             QStringLiteral("status"), 15000);
}

QList<GitFileStatus> GitManager::currentStatus() const
{
    return d->status;
}

// ============================================================
// 暂存
// ============================================================
void GitManager::stageFile(const QString& relativePath)
{
    if (!d->repoInfo.isValid || relativePath.isEmpty()) return;
    runAsync({"add", "--", relativePath}, QStringLiteral("stageFile"), 10000);
}

void GitManager::unstageFile(const QString& relativePath)
{
    if (!d->repoInfo.isValid || relativePath.isEmpty()) return;
    QStringList args;
    if (d->repoInfo.isEmpty) {
        // 无 HEAD 时 reset HEAD 会失败，用 rm --cached
        args = {"rm", "--cached", "--", relativePath};
    } else {
        args = {"reset", "HEAD", "--", relativePath};
    }
    runAsync(args, QStringLiteral("unstageFile"), 10000);
}

void GitManager::stageAll()
{
    if (!d->repoInfo.isValid) return;
    runAsync({"add", "-A"}, QStringLiteral("stageAll"), 30000);
}

void GitManager::unstageAll()
{
    if (!d->repoInfo.isValid) return;
    QStringList args;
    if (d->repoInfo.isEmpty) {
        args = {"rm", "--cached", "-r", "."};
    } else {
        args = {"reset", "HEAD"};
    }
    runAsync(args, QStringLiteral("unstageAll"), 30000);
}

// ============================================================
// 提交
// ============================================================
void GitManager::commit(const QString& message, bool amend)
{
    if (!d->repoInfo.isValid || message.trimmed().isEmpty()) return;

    QStringList args = {"commit", "-m", message};
    if (amend) args << "--amend";

    d->cmd->cancelAsync();   // 避免与 status 抢
    setBusy(true);
    d->cmd->runAsync(d->repoInfo.rootPath, args, QStringLiteral("commit"), 60000);
}

void GitManager::commitFiles(const QStringList& relativePaths, const QString& message)
{
    if (!d->repoInfo.isValid || relativePaths.isEmpty()) return;

    // 先 add 再 commit
    for (const QString& p : relativePaths) {
        GitCommand c;
        c.run(d->repoInfo.rootPath, {"add", "--", p}, 10000);
    }
    commit(message, false);
}

// ============================================================
// 分支
// ============================================================
QList<GitBranch> GitManager::branches() const
{
    QList<GitBranch> result;
    if (!d->repoInfo.isValid) return result;

    GitCommand cmd;

    // 本地分支 + upstream + ahead/behind
    auto r = cmd.run(d->repoInfo.rootPath,
                     {"branch", "-vv",
                      "--format=%(HEAD)|%(refname:short)|%(upstream:short)|%(upstream:track)"},
                     5000);
    if (r.ok()) {
        const QStringList lines = r.stdOut.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : lines) {
            const QStringList parts = line.split('|');
            if (parts.size() < 2) continue;

            GitBranch b;
            b.isCurrent = (parts.at(0).trimmed() == QLatin1String("*"));
            b.name      = parts.at(1).trimmed();
            if (parts.size() > 2) b.upstream = parts.at(2).trimmed();
            if (parts.size() > 3) {
                const QString track = parts.at(3);
                static const QRegularExpression reA("ahead\\s+(\\d+)");
                static const QRegularExpression reB("behind\\s+(\\d+)");
                auto ma = reA.match(track);
                auto mb = reB.match(track);
                if (ma.hasMatch()) b.ahead  = ma.captured(1).toInt();
                if (mb.hasMatch()) b.behind = mb.captured(1).toInt();
            }
            result.append(b);
        }
    }

    // 远程分支
    auto rRem = cmd.run(d->repoInfo.rootPath,
                        {"branch", "-r", "--format=%(refname:short)"}, 5000);
    if (rRem.ok()) {
        const QStringList rlines = rRem.stdOut.split('\n', Qt::SkipEmptyParts);
        for (const QString& rl : rlines) {
            GitBranch b;
            b.name     = rl.trimmed();
            b.isRemote = true;
            result.append(b);
        }
    }
    return result;
}

void GitManager::createBranch(const QString& name, const QString& base)
{
    if (!d->repoInfo.isValid || name.isEmpty()) return;
    QStringList args = {"branch", name};
    if (!base.isEmpty()) args << base;
    runAsync(args, QStringLiteral("createBranch"), 10000);
}

void GitManager::checkoutBranch(const QString& name)
{
    if (!d->repoInfo.isValid || name.isEmpty()) return;
    runAsync({"checkout", name}, QStringLiteral("checkout"), 30000);
}

// ============================================================
// 远程
// ============================================================
void GitManager::fetch()
{
    if (!d->repoInfo.isValid || !d->repoInfo.hasRemote) return;
    runAsync({"fetch", "--all", "--prune"}, QStringLiteral("fetch"), 120000);
}

void GitManager::pull()
{
    if (!d->repoInfo.isValid || !d->repoInfo.hasRemote) return;
    runAsync({"pull"}, QStringLiteral("pull"), 120000);
}

void GitManager::push()
{
    if (!d->repoInfo.isValid || !d->repoInfo.hasRemote) return;
    runAsync({"push"}, QStringLiteral("push"), 120000);
}

bool GitManager::hasRemote() const
{
    return d->repoInfo.hasRemote;
}

// ============================================================
// 历史 / Diff
// ============================================================
QList<GitCommit> GitManager::log(int maxCount, const QString& branch) const
{
    QList<GitCommit> result;
    if (!d->repoInfo.isValid || d->repoInfo.isEmpty) return result;

    GitCommand cmd;
    QStringList args = {"log",
                        QString("--max-count=%1").arg(maxCount),
                        // \x1f 字段分隔，\x1e 记录分隔
                        "--pretty=format:%H%x1f%h%x1f%an%x1f%ae%x1f%at%x1f%s%x1f%D%x1e"};
    if (!branch.isEmpty()) args << branch;

    auto r = cmd.run(d->repoInfo.rootPath, args, 15000);
    if (!r.ok()) return result;
    return parseLog(r.stdOut);
}

GitDiff GitManager::diffFile(const QString& relativePath, bool staged) const
{
    GitDiff diff;
    diff.filePath = relativePath;
    if (!d->repoInfo.isValid || relativePath.isEmpty()) return diff;

    GitCommand cmd;
    QStringList args = {"diff", "--no-color", "--unified=3"};
    if (staged) args << "--cached";
    args << "--" << relativePath;

    auto r = cmd.run(d->repoInfo.rootPath, args, 10000);
    if (!r.ok() || r.stdOut.isEmpty()) {
        diff.isEmpty = true;
        return diff;
    }
    return parseDiff(r.stdOut, relativePath);
}

QString GitManager::rawDiff(const QString& relativePath, bool staged) const
{
    if (!d->repoInfo.isValid) return QString();

    GitCommand cmd;
    QStringList args = {"diff", "--no-color", "--unified=3"};
    if (staged) args << "--cached";

    // relativePath 为空 → 显示全仓库差异
    if (!relativePath.isEmpty()) {
        args << "--" << relativePath;
    }

    auto r = cmd.run(d->repoInfo.rootPath, args, 15000);
    if (!r.ok()) return r.stdErr;
    return r.stdOut;
}

QString GitManager::rawCommitDiff(const QString& hash) const
{
    if (!d->repoInfo.isValid || hash.isEmpty()) return QString();

    GitCommand cmd;
    QStringList args = {"show", "--no-color", "--unified=3", hash};

    auto r = cmd.run(d->repoInfo.rootPath, args, 20000);
    if (!r.ok()) return r.stdErr;
    return r.stdOut;
}

// ============================================================
// 用户信息
// ============================================================
QString GitManager::userName() const
{
    if (d->userInfoCache.contains("name")) return d->userInfoCache.value("name");
    const QString dir = d->repoInfo.isValid ? d->repoInfo.rootPath : d->projectRoot;
    GitCommand cmd;
    auto r = cmd.run(dir, {"config", "--get", "user.name"}, 3000);
    const QString v = r.ok() ? r.stdOut.trimmed() : QString();
    d->userInfoCache["name"] = v;
    return v;
}

QString GitManager::userEmail() const
{
    if (d->userInfoCache.contains("email")) return d->userInfoCache.value("email");
    const QString dir = d->repoInfo.isValid ? d->repoInfo.rootPath : d->projectRoot;
    GitCommand cmd;
    auto r = cmd.run(dir, {"config", "--get", "user.email"}, 3000);
    const QString v = r.ok() ? r.stdOut.trimmed() : QString();
    d->userInfoCache["email"] = v;
    return v;
}

bool GitManager::setUserInfo(const QString& name, const QString& email, bool global)
{
    const QString dir = d->repoInfo.isValid ? d->repoInfo.rootPath : d->projectRoot;
    GitCommand cmd;
    QStringList scopeArgs;
    if (global) scopeArgs << "--global";

    bool ok = true;
    if (!name.isEmpty()) {
        auto r = cmd.run(dir, QStringList{"config"} + scopeArgs +
                                  QStringList{"user.name", name}, 5000);
        ok = ok && r.ok();
    }
    if (!email.isEmpty()) {
        auto r = cmd.run(dir, QStringList{"config"} + scopeArgs +
                                  QStringList{"user.email", email}, 5000);
        ok = ok && r.ok();
    }
    d->userInfoCache.clear();
    return ok;
}

// ============================================================
// 异步分发
// ============================================================
void GitManager::runAsync(const QStringList& args, const QString& tag, int timeoutMs)
{
    if (!d->repoInfo.isValid) return;
    setBusy(true);
    d->cmd->runAsync(d->repoInfo.rootPath, args, tag, timeoutMs);
}

void GitManager::setBusy(bool b)
{
    if (d->busy == b) return;
    d->busy = b;
    emit busyChanged(b);
}

void GitManager::onCommandFinished(const QString& tag, const GitCommand::Result& result)
{
    const bool isStatus = (tag == QStringLiteral("status"));

    if (!result.ok()) {
        // status 失败通常是仓库刚被删/权限问题，静默处理
        if (!isStatus && !result.stdErr.isEmpty()
            && result.stdErr != QLatin1String("cancelled")) {
            emit commandFailed(tag, result.stdErr.trimmed());
        }
    } else {
        // ---- 分发解析 ----
        if (isStatus) {
            d->status = parseStatusPorcelainV2(result.stdOut);
            emit statusUpdated(d->status);
        }
        else if (tag == QStringLiteral("commit")) {
            // 从 "[main abc1234] msg" 中提取短 hash
            static const QRegularExpression re("\\[.*?\\s+([0-9a-f]{7,})\\]");
            auto m = re.match(result.stdOut);
            emit commitSucceeded(m.hasMatch() ? m.captured(1) : QString());
            refreshStatus();
        }
        else if (tag.startsWith(QStringLiteral("stage"))
                 || tag.startsWith(QStringLiteral("unstage"))
                 || tag == QStringLiteral("checkout")
                 || tag == QStringLiteral("createBranch")
                 || tag == QStringLiteral("pull")
                 || tag == QStringLiteral("fetch")
                 || tag == QStringLiteral("push")) {
            refreshStatus();

            // checkout / pull 后分支可能变了，重探 repositoryInfo
            if (tag == QStringLiteral("checkout")
                || tag == QStringLiteral("pull")) {
                const QString saved = d->projectRoot;
                d->projectRoot.clear();
                setProjectRoot(saved);
            }
        }
    }

    setBusy(false);
}

// ============================================================
// porcelain v2 解析（-z 模式）
// ============================================================
QList<GitFileStatus> GitManager::parseStatusPorcelainV2(const QString& output) const
{
    QList<GitFileStatus> result;
    if (output.isEmpty()) return result;

    // -z 模式：记录以 NUL 分隔
    const QChar NUL = QChar(QChar::Null);
    const QStringList tokens = output.split(NUL, Qt::SkipEmptyParts);
    const QString root = d->repoInfo.rootPath;

    // 从字符串头部依次读 N 个以空格分隔的 token，剩余部分作为 path
    auto splitFixedFields = [](const QString& s, int fieldCount) -> QStringList {
        QStringList parts;
        int start = 0;
        int count = 0;
        for (int j = 0; j < s.size(); ++j) {
            if (s.at(j) == ' ') {
                parts.append(s.mid(start, j - start));
                start = j + 1;
                if (++count >= fieldCount - 1) break;
            }
        }
        parts.append(s.mid(start));
        return parts;
    };

    for (int i = 0; i < tokens.size(); ++i) {
        const QString& rec = tokens.at(i);
        if (rec.isEmpty()) continue;

        const QChar type = rec.at(0);

        if (type == QChar('1')) {
            // 1 XY sub mH mI mW hH hI <path>     → 8 固定字段 + path
            const QStringList parts = splitFixedFields(rec, 9);
            if (parts.size() < 9) continue;

            GitFileStatus s;
            const QString xy = parts.at(1);
            s.path         = parts.at(8);
            s.absolutePath = QDir(root).absoluteFilePath(s.path);
            if (xy.size() >= 2) {
                s.indexStatus    = statusFromChar(xy.at(0));
                s.workTreeStatus = statusFromChar(xy.at(1));
            }
            result.append(s);
        }
        else if (type == QChar('2')) {
            // 2 XY sub mH mI mW hH hI <X><score> <path>
            // 在 -z 模式下 path 与 origPath 之间用 NUL 分隔 → 下一个 token
            const QStringList parts = splitFixedFields(rec, 10);
            if (parts.size() < 10) continue;

            GitFileStatus s;
            const QString xy = parts.at(1);
            s.path         = parts.at(9);
            s.absolutePath = QDir(root).absoluteFilePath(s.path);
            if (xy.size() >= 2) {
                s.indexStatus    = statusFromChar(xy.at(0));
                s.workTreeStatus = statusFromChar(xy.at(1));
            }
            if (i + 1 < tokens.size()) {
                s.origPath = tokens.at(++i);
            }
            result.append(s);
        }
        else if (type == QChar('u')) {
            // u XY sub m1 m2 m3 mW h1 h2 h3 <path>   → 10 固定字段 + path
            const QStringList parts = splitFixedFields(rec, 11);
            if (parts.size() < 11) continue;

            GitFileStatus s;
            s.path         = parts.at(10);
            s.absolutePath = QDir(root).absoluteFilePath(s.path);
            s.indexStatus    = FileStatus::Conflicted;
            s.workTreeStatus = FileStatus::Conflicted;
            result.append(s);
        }
        else if (type == QChar('?')) {
            // ? <path>
            GitFileStatus s;
            s.path         = rec.mid(2);
            s.absolutePath = QDir(root).absoluteFilePath(s.path);
            s.indexStatus    = FileStatus::Untracked;
            s.workTreeStatus = FileStatus::Untracked;
            result.append(s);
        }
        else if (type == QChar('!')) {
            GitFileStatus s;
            s.path         = rec.mid(2);
            s.absolutePath = QDir(root).absoluteFilePath(s.path);
            s.indexStatus    = FileStatus::Ignored;
            s.workTreeStatus = FileStatus::Ignored;
            result.append(s);
        }
    }

    return result;
}

// ============================================================
// log 解析
// ============================================================
QList<GitCommit> GitManager::parseLog(const QString& output) const
{
    QList<GitCommit> result;
    if (output.isEmpty()) return result;

    const QStringList records = output.split(QChar(0x1e), Qt::SkipEmptyParts);
    for (const QString& rec : records) {
        const QStringList parts = rec.split(QChar(0x1f));
        if (parts.size() < 6) continue;

        GitCommit c;
        c.hash      = parts.at(0);
        c.shortHash = parts.at(1);
        c.author    = parts.at(2);
        c.email     = parts.at(3);
        c.timestamp = QDateTime::fromSecsSinceEpoch(parts.at(4).toLongLong());
        c.message   = parts.at(5);
        if (parts.size() > 6) {
            c.refs = parts.at(6).split(", ", Qt::SkipEmptyParts);
        }
        result.append(c);
    }
    return result;
}

// ============================================================
// diff 解析
// ============================================================
GitDiff GitManager::parseDiff(const QString& output, const QString& filePath) const
{
    GitDiff diff;
    diff.filePath = filePath;

    if (output.contains(QLatin1String("Binary files"))) {
        diff.isBinary = true;
        return diff;
    }

    GitDiffHunk currentHunk;
    bool inHunk = false;

    static const QRegularExpression reHunk(
        "^@@ -(\\d+)(?:,(\\d+))? \\+(\\d+)(?:,(\\d+))? @@");

    const QStringList lines = output.split('\n');
    for (const QString& line : lines) {
        if (line.startsWith("@@")) {
            if (inHunk) diff.hunks.append(currentHunk);
            currentHunk = GitDiffHunk();
            currentHunk.header = line;

            auto m = reHunk.match(line);
            if (m.hasMatch()) {
                currentHunk.oldStart = m.captured(1).toInt();
                currentHunk.oldLines = m.captured(2).isEmpty() ? 1 : m.captured(2).toInt();
                currentHunk.newStart = m.captured(3).toInt();
                currentHunk.newLines = m.captured(4).isEmpty() ? 1 : m.captured(4).toInt();
            }
            inHunk = true;
        }
        else if (inHunk) {
            GitDiffLine dl;
            if (line.startsWith('+'))       { dl.type = GitDiffLine::Added;   dl.text = line.mid(1); }
            else if (line.startsWith('-'))  { dl.type = GitDiffLine::Removed; dl.text = line.mid(1); }
            else if (line.startsWith(' '))  { dl.type = GitDiffLine::Context; dl.text = line.mid(1); }
            else if (line.startsWith('\\')) { continue; }   // "\ No newline at end of file"
            else                            { dl.type = GitDiffLine::Context; dl.text = line; }
            currentHunk.lines.append(dl);
        }
    }

    if (inHunk) diff.hunks.append(currentHunk);
    if (diff.hunks.isEmpty()) diff.isEmpty = true;
    return diff;
}

} // namespace Git
} // namespace SprintIDE
