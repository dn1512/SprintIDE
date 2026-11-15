// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>
#include <QList>

namespace SprintIDE {

// 一条可点击的结构化输出项
struct ParsedOutputItem {
    enum class Kind {
        Normal,     // 普通
        Modified,   // 黄色（git modified）
        Added,      // 绿色（git added / untracked / running）
        Deleted,    // 红色（git deleted / 已退出容器 / 错误 pod）
        Warning,    // 橙色（主版本跨越 / 高 CPU / Pending）
    };

    QString primaryText;     // 主显示文本
    QString secondaryText;   // 灰色次显示文本
    Kind    kind = Kind::Normal;

    // 点击动作：
    //   "open:path"     打开文件（相对路径会拼上当前工作目录）
    //   "run:command"   把命令回填到终端（不自动执行）
    //   "copy:text"     复制到剪贴板
    QString clickAction;
};

// 一次解析的完整结果
struct ParsedOutputResult {
    bool ok = false;
    QString title;
    QString subtitle;
    QList<ParsedOutputItem> items;
};

// ---------------------------------------------------------------------------
// TerminalOutputParser
//
//   把终端命令的输出文本解析成结构化结果。
//
//   支持的命令：
//     - git status
//     - git log
//     - git branch
//     - ls -la / ls -l / ls -al
//     - docker ps / docker container ls
//     - docker images
//     - npm outdated
//     - pip list --outdated
//     - kubectl get pods
//     - ps aux / ps -ef
//
//   输入：命令 + 命令输出（含 ANSI 序列，内部会自动剥离）
//   输出：ParsedOutputResult；无匹配时 ok = false
// ---------------------------------------------------------------------------
class TerminalOutputParser
{
public:
    static bool canParse(const QString& command);
    static ParsedOutputResult parse(const QString& command, const QString& output);

private:
    static ParsedOutputResult parseGitStatus(const QString& output);
    static ParsedOutputResult parseGitLog(const QString& output);
    static ParsedOutputResult parseGitBranch(const QString& output);
    static ParsedOutputResult parseLsLong(const QString& output);
    static ParsedOutputResult parseDockerPs(const QString& output);
    static ParsedOutputResult parseDockerImages(const QString& output);
    static ParsedOutputResult parseNpmOutdated(const QString& output);
    static ParsedOutputResult parsePipOutdated(const QString& output);
    static ParsedOutputResult parseKubectlPods(const QString& output);
    static ParsedOutputResult parsePsAux(const QString& output);
};

} // namespace SprintIDE