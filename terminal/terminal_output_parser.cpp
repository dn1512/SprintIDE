// SPDX-License-Identifier: GPL-3.0-or-later
#include "terminal_output_parser.h"

#include <QRegularExpression>
#include <QStringList>

namespace SprintIDE {

// ============================================================================
// ANSI 序列剥离
//
//   CSI: \x1b[...字母
//   OSC: \x1b]...BEL 或 \x1b]...ESC\
//   其他 2 字符 ESC 序列
// ============================================================================
static QString stripAnsi(const QString& s)
{
    static const QRegularExpression re(
        QStringLiteral(
            R"(\x1b\[[0-9;?]*[a-zA-Z])"
            R"(|\x1b\][^\x07\x1b]*(?:\x07|\x1b\\)?)"
            R"(|\x1b[@-_])"
        )
    );
    QString copy = s;
    copy.remove(re);
    return copy;
}

// ============================================================================
// canParse
// ============================================================================
bool TerminalOutputParser::canParse(const QString& command)
{
    const QString t = command.trimmed();
    if (t.isEmpty()) return false;

    static const QStringList kPatterns = {
        QStringLiteral("git status"),
        QStringLiteral("git log"),
        QStringLiteral("git branch"),
        QStringLiteral("ls -la"),
        QStringLiteral("ls -al"),
        QStringLiteral("ls -l "),
        QStringLiteral("docker ps"),
        QStringLiteral("docker container ls"),
        QStringLiteral("docker images"),
        QStringLiteral("npm outdated"),
        QStringLiteral("pip list --outdated"),
        QStringLiteral("pip3 list --outdated"),
        QStringLiteral("kubectl get pods"),
        QStringLiteral("kubectl get po"),
        QStringLiteral("ps aux"),
        QStringLiteral("ps -ef"),
    };

    for (const QString& p : kPatterns) {
        if (t.startsWith(p)) return true;
    }
    return false;
}

// ============================================================================
// parse —— 路由
// ============================================================================
ParsedOutputResult TerminalOutputParser::parse(const QString& command,
                                               const QString& output)
{
    const QString t = command.trimmed();
    const QString clean = stripAnsi(output);

    if (t.startsWith(QStringLiteral("git status")))
        return parseGitStatus(clean);
    if (t.startsWith(QStringLiteral("git log")))
        return parseGitLog(clean);
    if (t.startsWith(QStringLiteral("git branch")))
        return parseGitBranch(clean);
    if (t.startsWith(QStringLiteral("ls -")))
        return parseLsLong(clean);
    if (t.startsWith(QStringLiteral("docker ps"))
        || t.startsWith(QStringLiteral("docker container ls")))
        return parseDockerPs(clean);
    if (t.startsWith(QStringLiteral("docker images")))
        return parseDockerImages(clean);
    if (t.startsWith(QStringLiteral("npm outdated")))
        return parseNpmOutdated(clean);
    if (t.startsWith(QStringLiteral("pip list --outdated"))
        || t.startsWith(QStringLiteral("pip3 list --outdated")))
        return parsePipOutdated(clean);
    if (t.startsWith(QStringLiteral("kubectl get po")))
        return parseKubectlPods(clean);
    if (t.startsWith(QStringLiteral("ps ")))
        return parsePsAux(clean);

    return ParsedOutputResult{};
}

// ============================================================================
// git status
// ============================================================================
ParsedOutputResult TerminalOutputParser::parseGitStatus(const QString& output)
{
    ParsedOutputResult result;
    result.ok = true;
    result.title = QStringLiteral("Git Status");

    enum State { Idle, NotStaged, Staged, Untracked };
    State state = Idle;

    const QStringList lines = output.split(QLatin1Char('\n'));

    for (const QString& rawLine : lines) {
        const QString t = rawLine.trimmed();

        if (t == QLatin1String("Changes not staged for commit:")) { state = NotStaged; continue; }
        if (t == QLatin1String("Changes to be committed:"))       { state = Staged;    continue; }
        if (t == QLatin1String("Untracked files:"))               { state = Untracked; continue; }
        if (t == QLatin1String("Unmerged paths:"))                { state = Idle;      continue; }

        if (t.startsWith(QLatin1String("On branch "))) {
            result.subtitle = t.mid(10);
            continue;
        }
        if (t.startsWith(QLatin1String("HEAD detached at "))) {
            result.subtitle = QStringLiteral("detached: ") + t.mid(18);
            continue;
        }

        if (state == Idle) continue;
        if (t.startsWith(QLatin1Char('('))) continue;
        if (t.isEmpty()) continue;

        ParsedOutputItem item;
        QString path;

        if (t.startsWith(QLatin1String("modified:"))) {
            path = t.mid(9).trimmed();
            item.kind = ParsedOutputItem::Kind::Modified;
            item.secondaryText = QStringLiteral("modified");
        } else if (t.startsWith(QLatin1String("new file:"))) {
            path = t.mid(9).trimmed();
            item.kind = ParsedOutputItem::Kind::Added;
            item.secondaryText = QStringLiteral("new file");
        } else if (t.startsWith(QLatin1String("deleted:"))) {
            path = t.mid(8).trimmed();
            item.kind = ParsedOutputItem::Kind::Deleted;
            item.secondaryText = QStringLiteral("deleted");
        } else if (t.startsWith(QLatin1String("renamed:"))) {
            path = t.mid(8).trimmed();
            item.secondaryText = QStringLiteral("renamed");
        } else if (state == Untracked) {
            path = t;
            item.kind = ParsedOutputItem::Kind::Added;
            item.secondaryText = QStringLiteral("untracked");
        } else {
            continue;
        }

        if (path.isEmpty()) continue;

        item.primaryText = path;
        item.clickAction = QStringLiteral("open:") + path;
        result.items.append(item);
    }

    if (result.items.isEmpty()) result.ok = false;
    return result;
}

// ============================================================================
// git log
//   - 支持 oneline：<hash> <message>
//   - 支持完整格式：commit <hash> / Author: / Date: / 缩进消息
// ============================================================================
ParsedOutputResult TerminalOutputParser::parseGitLog(const QString& output)
{
    ParsedOutputResult result;
    result.ok = true;
    result.title = QStringLiteral("Git Log");

    static const QRegularExpression commitRe(
        QStringLiteral("^commit\\s+([0-9a-f]{7,40})"));
    static const QRegularExpression onelineRe(
        QStringLiteral("^([0-9a-f]{7,40})\\s+(.+)$"));

    const QStringList lines = output.split(QLatin1Char('\n'));

    QString currentHash;
    QString currentMessage;

    auto flushCurrent = [&]() {
        if (currentHash.isEmpty()) return;
        ParsedOutputItem item;
        item.primaryText = currentMessage.isEmpty()
                               ? QStringLiteral("(no message)")
                               : currentMessage;
        item.secondaryText = currentHash;
        item.clickAction = QStringLiteral("copy:") + currentHash;
        result.items.append(item);
        currentHash.clear();
        currentMessage.clear();
    };

    for (const QString& raw : lines) {
        const QString t = raw.trimmed();
        if (t.isEmpty()) continue;

        // 完整格式优先：commit <hash>
        const auto cm = commitRe.match(t);
        if (cm.hasMatch()) {
            flushCurrent();
            currentHash = cm.captured(1);
            continue;
        }

        // 完整格式的中间行：跳过 Author / Date / Merge
        if (!currentHash.isEmpty()) {
            if (t.startsWith(QLatin1String("Author:"))) continue;
            if (t.startsWith(QLatin1String("Date:")))   continue;
            if (t.startsWith(QLatin1String("Merge:")))  continue;

            // 消息：缩进行（4 空格），取第一条
            if (raw.startsWith(QLatin1String("    ")) && currentMessage.isEmpty()) {
                currentMessage = t;
            }
            continue;
        }

        // oneline 格式：无 commit 前缀，直接 <hash> <msg>
        const auto om = onelineRe.match(t);
        if (om.hasMatch()) {
            ParsedOutputItem item;
            item.primaryText = om.captured(2);
            item.secondaryText = om.captured(1);
            item.clickAction = QStringLiteral("copy:") + om.captured(1);
            result.items.append(item);
        }
    }
    flushCurrent();

    if (result.items.isEmpty()) result.ok = false;
    return result;
}

// ============================================================================
// git branch
// ============================================================================
ParsedOutputResult TerminalOutputParser::parseGitBranch(const QString& output)
{
    ParsedOutputResult result;
    result.ok = true;
    result.title = QStringLiteral("Git Branches");

    const QStringList lines = output.split(QLatin1Char('\n'));

    for (const QString& raw : lines) {
        if (raw.trimmed().isEmpty()) continue;
        if (raw.startsWith(QLatin1String("(HEAD detached"))) continue;
        if (raw.startsWith(QLatin1String("(no branch"))) continue;

        const bool isCurrent = raw.startsWith(QLatin1String("* "));
        QString name = raw;
        if (isCurrent)                                 name = name.mid(2);
        else if (raw.startsWith(QLatin1String("  ")))  name = name.mid(2);
        else if (raw.startsWith(QLatin1String("+")))   name = name.mid(2);
        else continue;

        name = name.trimmed();
        if (name.isEmpty()) continue;

        ParsedOutputItem item;
        item.primaryText = isCurrent ? QStringLiteral("* ") + name : name;
        item.secondaryText = isCurrent ? QStringLiteral("当前分支") : QString();
        item.kind = isCurrent ? ParsedOutputItem::Kind::Added
                              : ParsedOutputItem::Kind::Normal;
        item.clickAction = isCurrent
                               ? QStringLiteral("copy:%1").arg(name)
                               : QStringLiteral("run:git checkout %1").arg(name);
        result.items.append(item);
    }

    if (result.items.isEmpty()) result.ok = false;
    return result;
}

// ============================================================================
// ls -la / ls -l / ls -al
// ============================================================================
ParsedOutputResult TerminalOutputParser::parseLsLong(const QString& output)
{
    ParsedOutputResult result;
    result.ok = true;
    result.title = QStringLiteral("Files");

    static const QRegularExpression lsRe(
        QStringLiteral(
            R"(^([dl\-][rwxsStT\-]{9})[.@]?\s+)"    // perms
            R"((\d+)\s+)"                           // links
            R"((\S+)\s+)"                           // owner
            R"((\S+)\s+)"                           // group
            R"((\d+)\s+)"                           // size
            R"((\S+\s+\S+\s+\S+)\s+)"               // date (Jan 1 12:00 or 2026-01-01)
            R"((.+)$)"                              // name
        )
    );

    const QStringList lines = output.split(QLatin1Char('\n'));

    for (const QString& raw : lines) {
        if (raw.isEmpty()) continue;
        if (raw.startsWith(QLatin1String("total "))) continue;

        const auto m = lsRe.match(raw);
        if (!m.hasMatch()) continue;

        const QString perms = m.captured(1);
        const QString size  = m.captured(5);
        QString     name    = m.captured(7).trimmed();

        if (name == QLatin1String(".") || name == QLatin1String("..")) continue;

        QString linkTarget;
        const int arrow = name.indexOf(QLatin1String(" -> "));
        if (arrow > 0) {
            linkTarget = name.mid(arrow + 4);
            name = name.left(arrow);
        }

        const bool isDir = perms.startsWith(QLatin1Char('d'));

        ParsedOutputItem item;
        item.primaryText = name;
        item.secondaryText = QStringLiteral("%1  ·  %2").arg(perms, size);
        if (!linkTarget.isEmpty()) {
            item.secondaryText += QStringLiteral("  →  ") + linkTarget;
        }
        item.kind = isDir ? ParsedOutputItem::Kind::Added
                          : ParsedOutputItem::Kind::Normal;
        item.clickAction = isDir
                               ? QStringLiteral("run:cd %1").arg(name)
                               : QStringLiteral("open:%1").arg(name);
        result.items.append(item);
    }

    if (result.items.isEmpty()) result.ok = false;
    return result;
}

// ============================================================================
// docker ps
// ============================================================================
ParsedOutputResult TerminalOutputParser::parseDockerPs(const QString& output)
{
    ParsedOutputResult result;
    result.ok = true;
    result.title = QStringLiteral("Docker Containers");

    static const QRegularExpression colSplit(QStringLiteral("\\s{2,}"));

    const QStringList lines = output.split(QLatin1Char('\n'));

    for (const QString& raw : lines) {
        const QString t = raw.trimmed();
        if (t.isEmpty()) continue;
        if (t.startsWith(QLatin1String("CONTAINER ID"))) continue;
        if (t.contains(QLatin1String("command not found"))) { result.ok = false; return result; }
        if (t.contains(QLatin1String("Cannot connect")))    { result.ok = false; return result; }
        if (t.contains(QLatin1String("permission denied"))) { result.ok = false; return result; }

        const QStringList cols = t.split(colSplit, Qt::SkipEmptyParts);
        if (cols.size() < 6) continue;

        const QString name   = cols.last();
        const QString image  = cols.value(1);
        const QString status = cols.value(4);
        const QString ports  = (cols.size() >= 7) ? cols.value(5) : QString();

        ParsedOutputItem item;
        item.primaryText = name;
        QStringList secondary;
        if (!image.isEmpty()) secondary << image;
        if (!ports.isEmpty() && ports != QLatin1String("PORTS")) secondary << ports;
        item.secondaryText = secondary.join(QStringLiteral("  ·  "));
        item.kind = status.contains(QLatin1String("Exited"), Qt::CaseInsensitive)
                        ? ParsedOutputItem::Kind::Deleted
                        : ParsedOutputItem::Kind::Normal;
        item.clickAction = QStringLiteral("run:docker logs --tail 100 %1").arg(name);
        result.items.append(item);
    }

    if (result.items.isEmpty()) result.ok = false;
    return result;
}

// ============================================================================
// docker images
// ============================================================================
ParsedOutputResult TerminalOutputParser::parseDockerImages(const QString& output)
{
    ParsedOutputResult result;
    result.ok = true;
    result.title = QStringLiteral("Docker Images");

    static const QRegularExpression colSplit(QStringLiteral("\\s{2,}"));

    const QStringList lines = output.split(QLatin1Char('\n'));

    for (const QString& raw : lines) {
        const QString t = raw.trimmed();
        if (t.isEmpty()) continue;
        if (t.startsWith(QLatin1String("REPOSITORY"))) continue;
        if (t.contains(QLatin1String("Cannot connect"))) { result.ok = false; return result; }
        if (t.contains(QLatin1String("permission denied"))) { result.ok = false; return result; }

        const QStringList cols = t.split(colSplit, Qt::SkipEmptyParts);
        if (cols.size() < 4) continue;

        const QString repo = cols.value(0);
        const QString tag  = cols.value(1);
        const QString id   = cols.value(2);
        const QString size = cols.value(cols.size() - 2);

        ParsedOutputItem item;
        item.primaryText = QStringLiteral("%1:%2").arg(repo, tag);
        item.secondaryText = QStringLiteral("%1  ·  %2").arg(id.left(12), size);
        item.kind = (tag == QLatin1String("<none>"))
                        ? ParsedOutputItem::Kind::Warning
                        : ParsedOutputItem::Kind::Normal;
        item.clickAction = QStringLiteral("run:docker rmi %1:%2").arg(repo, tag);
        result.items.append(item);
    }

    if (result.items.isEmpty()) result.ok = false;
    return result;
}

// ============================================================================
// npm outdated
// ============================================================================
ParsedOutputResult TerminalOutputParser::parseNpmOutdated(const QString& output)
{
    ParsedOutputResult result;
    result.ok = true;
    result.title = QStringLiteral("npm Outdated");

    static const QRegularExpression colSplit(QStringLiteral("\\s+"));

    const QStringList lines = output.split(QLatin1Char('\n'));

    for (const QString& raw : lines) {
        const QString t = raw.trimmed();
        if (t.isEmpty()) continue;
        if (t.startsWith(QLatin1String("Package"))) continue;
        if (t.startsWith(QLatin1String("npm ERR"))) { result.ok = false; return result; }

        const QStringList cols = t.split(colSplit, Qt::SkipEmptyParts);
        if (cols.size() < 4) continue;

        const QString pkg     = cols.value(0);
        const QString current = cols.value(1);
        const QString latest  = cols.value(3);

        ParsedOutputItem item;
        item.primaryText = pkg;
        item.secondaryText = QStringLiteral("%1  →  %2").arg(current, latest);

        const int curMajor = current.split(QLatin1Char('.')).value(0).toInt();
        const int latMajor = latest.split(QLatin1Char('.')).value(0).toInt();
        item.kind = (curMajor != latMajor)
                        ? ParsedOutputItem::Kind::Warning
                        : ParsedOutputItem::Kind::Normal;

        item.clickAction = QStringLiteral("run:npm install %1@latest").arg(pkg);
        result.items.append(item);
    }

    if (result.items.isEmpty()) result.ok = false;
    return result;
}

// ============================================================================
// pip list --outdated
// ============================================================================
ParsedOutputResult TerminalOutputParser::parsePipOutdated(const QString& output)
{
    ParsedOutputResult result;
    result.ok = true;
    result.title = QStringLiteral("pip Outdated");

    static const QRegularExpression colSplit(QStringLiteral("\\s+"));

    const QStringList lines = output.split(QLatin1Char('\n'));

    for (const QString& raw : lines) {
        const QString t = raw.trimmed();
        if (t.isEmpty()) continue;
        if (t.startsWith(QLatin1String("Package"))) continue;
        if (t.startsWith(QLatin1String("WARNING"))) continue;
        if (t.contains(QLatin1String("notice"), Qt::CaseInsensitive)) continue;

        const QStringList cols = t.split(colSplit, Qt::SkipEmptyParts);
        if (cols.size() < 3) continue;

        const QString pkg     = cols.value(0);
        const QString current = cols.value(1);
        const QString latest  = cols.value(2);

        ParsedOutputItem item;
        item.primaryText = pkg;
        item.secondaryText = QStringLiteral("%1  →  %2").arg(current, latest);

        const int curMajor = current.split(QLatin1Char('.')).value(0).toInt();
        const int latMajor = latest.split(QLatin1Char('.')).value(0).toInt();
        item.kind = (curMajor != latMajor)
                        ? ParsedOutputItem::Kind::Warning
                        : ParsedOutputItem::Kind::Normal;

        item.clickAction = QStringLiteral("run:pip install --upgrade %1").arg(pkg);
        result.items.append(item);
    }

    if (result.items.isEmpty()) result.ok = false;
    return result;
}

// ============================================================================
// kubectl get pods
// ============================================================================
ParsedOutputResult TerminalOutputParser::parseKubectlPods(const QString& output)
{
    ParsedOutputResult result;
    result.ok = true;
    result.title = QStringLiteral("kubectl Pods");

    static const QRegularExpression colSplit(QStringLiteral("\\s+"));

    const QStringList lines = output.split(QLatin1Char('\n'));

    for (const QString& raw : lines) {
        const QString t = raw.trimmed();
        if (t.isEmpty()) continue;
        if (t.startsWith(QLatin1String("NAME"))) continue;
        if (t.startsWith(QLatin1String("No resources"))) { result.ok = false; return result; }
        if (t.contains(QLatin1String("error:"), Qt::CaseInsensitive)) { result.ok = false; return result; }
        if (t.contains(QLatin1String("Unable to connect"))) { result.ok = false; return result; }

        const QStringList cols = t.split(colSplit, Qt::SkipEmptyParts);
        if (cols.size() < 3) continue;

        const QString name     = cols.value(0);
        const QString ready    = cols.value(1);
        const QString status   = cols.value(2);
        const QString restarts = cols.value(3);
        const QString age      = cols.value(4);

        ParsedOutputItem item;
        item.primaryText = name;
        QStringList secondary;
        if (!status.isEmpty())   secondary << status;
        if (!ready.isEmpty())    secondary << ready;
        if (!restarts.isEmpty()) secondary << QStringLiteral("restarts: %1").arg(restarts);
        if (!age.isEmpty())      secondary << age;
        item.secondaryText = secondary.join(QStringLiteral("  ·  "));

        if (status == QLatin1String("Running") || status == QLatin1String("Completed")) {
            item.kind = ParsedOutputItem::Kind::Added;
        } else if (status == QLatin1String("Error")
                   || status == QLatin1String("CrashLoopBackOff")
                   || status == QLatin1String("ImagePullBackOff")) {
            item.kind = ParsedOutputItem::Kind::Deleted;
        } else if (status == QLatin1String("Pending")) {
            item.kind = ParsedOutputItem::Kind::Warning;
        }

        item.clickAction = QStringLiteral("run:kubectl logs --tail 100 %1").arg(name);
        result.items.append(item);
    }

    if (result.items.isEmpty()) result.ok = false;
    return result;
}

// ============================================================================
// ps aux / ps -ef
// ============================================================================
ParsedOutputResult TerminalOutputParser::parsePsAux(const QString& output)
{
    ParsedOutputResult result;
    result.ok = true;
    result.title = QStringLiteral("Processes");

    static const QRegularExpression psRe(
        QStringLiteral(
            R"(^(\S+)\s+)"             // user
            R"((\d+)\s+)"              // pid
            R"((\d+(?:\.\d+)?)\s+)"    // %cpu
            R"((\d+(?:\.\d+)?)\s+)"    // %mem
            R"(\d+\s+)"                // vsz
            R"(\d+\s+)"                // rss
            R"(\S+\s+)"                // tty
            R"(\S+\s+)"                // stat
            R"(\S+\s+)"                // start
            R"(\S+\s+)"                // time
            R"((.+)$)"                 // command
        )
    );

    const QStringList lines = output.split(QLatin1Char('\n'));

    for (const QString& raw : lines) {
        const QString t = raw.trimmed();
        if (t.isEmpty()) continue;
        if (t.startsWith(QLatin1String("USER"))) continue;

        const auto m = psRe.match(t);
        if (!m.hasMatch()) continue;

        const QString user    = m.captured(1);
        const QString pid     = m.captured(2);
        const QString cpu     = m.captured(3);
        const QString mem     = m.captured(4);
        QString       command = m.captured(11).trimmed();

        if (command.length() > 80) {
            command = command.left(77) + QStringLiteral("...");
        }

        ParsedOutputItem item;
        item.primaryText = QStringLiteral("%1  %2").arg(pid, command);
        item.secondaryText = QStringLiteral("%1  cpu: %2%  mem: %3%")
                                 .arg(user, cpu, mem);

        if (cpu.toDouble() > 50 || mem.toDouble() > 50) {
            item.kind = ParsedOutputItem::Kind::Warning;
        }

        item.clickAction = QStringLiteral("run:kill -TERM %1").arg(pid);
        result.items.append(item);
    }

    if (result.items.isEmpty()) result.ok = false;
    return result;
}

} // namespace SprintIDE