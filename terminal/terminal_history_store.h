// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>
#include <QStringList>
#include <QList>

namespace SprintIDE {

struct TerminalHistoryEntry {
    QString command;
    QString cwd;
    QString projectPath;
    qint64  startedAt = 0;
    int     exitCode  = -1;
    qint64  durationMs = -1;
};

class TerminalHistoryStore
{
public:
    static TerminalHistoryStore* instance();

    void addCommand(const QString& command,
                    const QString& cwd,
                    const QString& projectPath,
                    qint64 durationMs = -1);

    QList<TerminalHistoryEntry> search(const QString& query,
                                       const QString& projectFilter = QString(),
                                       qint64 sinceMs = 0,
                                       int limit = 200) const;

    QStringList distinctProjects() const;
    int count() const;

    void clearAll();
    void clearProject(const QString& projectPath);

private:
    TerminalHistoryStore();
    ~TerminalHistoryStore();

    // 标记 const：只读 m_connectionName，打开连接是外部资源操作不影响对象状态
    bool ensureDatabase() const;

    QString m_connectionName;
};

} // namespace SprintIDE