// SPDX-License-Identifier: GPL-3.0-or-later
#include "terminal_history_store.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>
#include <QVariant>
#include <QDebug>

namespace SprintIDE {

TerminalHistoryStore* TerminalHistoryStore::instance()
{
    static TerminalHistoryStore s;
    return &s;
}

TerminalHistoryStore::TerminalHistoryStore()
{
    // 每个实例一个唯一连接名，避免和 Qt 其他 SQL 连接冲突
    m_connectionName = QStringLiteral("terminal_history_%1")
                           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

TerminalHistoryStore::~TerminalHistoryStore()
{
    if (!QCoreApplication::instance()) {
        // 应用已经销毁，不要再动 QSqlDatabase 了（避免告警）
        return;
    }
    if (QSqlDatabase::contains(m_connectionName)) {
        {
            QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
            if (db.isOpen()) db.close();
        }
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool TerminalHistoryStore::ensureDatabase() const
{
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
        if (db.isOpen()) return true;
    }

    // 数据库文件路径
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) {
        qWarning() << "[TerminalHistory] AppDataLocation 为空，无法保存历史";
        return false;
    }
    QDir().mkpath(dir);
    const QString dbPath = dir + QStringLiteral("/terminal_history.db");

    // 打开连接
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                                m_connectionName);
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        qWarning() << "[TerminalHistory] 无法打开数据库:" << dbPath
                   << db.lastError().text();
        return false;
    }

    // 建表
    QSqlQuery q(db);
    const QString createSql = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS commands ("
        "  id           INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  command      TEXT NOT NULL,"
        "  cwd          TEXT,"
        "  project_path TEXT,"
        "  started_at   INTEGER,"
        "  exit_code    INTEGER,"
        "  duration_ms  INTEGER"
        ")");
    if (!q.exec(createSql)) {
        qWarning() << "[TerminalHistory] 建表失败:" << q.lastError().text();
        return false;
    }

    // 索引
    q.exec(QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_cmd_command ON commands(command)"));
    q.exec(QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_cmd_project ON commands(project_path)"));
    q.exec(QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_cmd_started ON commands(started_at)"));

    qDebug() << "[TerminalHistory] 数据库就绪:" << dbPath;
    return true;
}

void TerminalHistoryStore::addCommand(const QString& command,
                                      const QString& cwd,
                                      const QString& projectPath,
                                      qint64 durationMs)
{
    if (command.trimmed().isEmpty()) return;
    if (!ensureDatabase()) return;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);

    // 30 秒去重：相同命令 + 相同 cwd 不重复记录
    {
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "SELECT COUNT(*) FROM commands "
            "WHERE command = :cmd AND cwd = :cwd AND started_at > :since"));
        q.bindValue(QStringLiteral(":cmd"),  command);
        q.bindValue(QStringLiteral(":cwd"),  cwd);
        q.bindValue(QStringLiteral(":since"),
                    QDateTime::currentMSecsSinceEpoch() - 30000);
        if (q.exec() && q.next() && q.value(0).toInt() > 0) {
            // 已有近似重复记录，不重复插入
            return;
        }
    }

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO commands "
        "(command, cwd, project_path, started_at, exit_code, duration_ms) "
        "VALUES (:cmd, :cwd, :proj, :ts, -1, :dur)"));
    q.bindValue(QStringLiteral(":cmd"),  command);
    q.bindValue(QStringLiteral(":cwd"),  cwd);
    q.bindValue(QStringLiteral(":proj"), projectPath);
    q.bindValue(QStringLiteral(":ts"),   QDateTime::currentMSecsSinceEpoch());
    q.bindValue(QStringLiteral(":dur"),  durationMs);

    if (!q.exec()) {
        qWarning() << "[TerminalHistory] 插入失败:" << q.lastError().text();
    }
}

QList<TerminalHistoryEntry> TerminalHistoryStore::search(const QString& query,
                                                         const QString& projectFilter,
                                                         qint64 sinceMs,
                                                         int limit) const
{
    QList<TerminalHistoryEntry> out;
    if (!ensureDatabase()) return out;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);

    QString sql = QStringLiteral(
        "SELECT command, cwd, project_path, started_at, exit_code, duration_ms "
        "FROM commands WHERE 1=1");

    if (!query.isEmpty())      sql += QStringLiteral(" AND command LIKE :pattern");
    if (!projectFilter.isEmpty()) sql += QStringLiteral(" AND project_path = :proj");
    if (sinceMs > 0)           sql += QStringLiteral(" AND started_at >= :since");

    sql += QStringLiteral(" ORDER BY started_at DESC LIMIT :limit");

    QSqlQuery q(db);
    q.prepare(sql);
    if (!query.isEmpty())
        q.bindValue(QStringLiteral(":pattern"), QStringLiteral("%%1%").arg(query));
    if (!projectFilter.isEmpty())
        q.bindValue(QStringLiteral(":proj"), projectFilter);
    if (sinceMs > 0)
        q.bindValue(QStringLiteral(":since"), sinceMs);
    q.bindValue(QStringLiteral(":limit"), qBound(1, limit, 10000));

    if (!q.exec()) {
        qWarning() << "[TerminalHistory] 查询失败:" << q.lastError().text();
        return out;
    }

    while (q.next()) {
        TerminalHistoryEntry e;
        e.command     = q.value(0).toString();
        e.cwd         = q.value(1).toString();
        e.projectPath = q.value(2).toString();
        e.startedAt   = q.value(3).toLongLong();
        e.exitCode    = q.value(4).toInt();
        e.durationMs  = q.value(5).toLongLong();
        out.append(e);
    }
    return out;
}

QStringList TerminalHistoryStore::distinctProjects() const
{
    QStringList out;
    if (!ensureDatabase()) return out;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);

    QSqlQuery q(db);
    q.exec(QStringLiteral(
        "SELECT DISTINCT project_path FROM commands "
        "WHERE project_path IS NOT NULL AND project_path != '' "
        "ORDER BY project_path"));
    while (q.next()) out.append(q.value(0).toString());
    return out;
}

int TerminalHistoryStore::count() const
{
    if (!ensureDatabase()) return 0;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    QSqlQuery q(db);
    if (q.exec(QStringLiteral("SELECT COUNT(*) FROM commands")) && q.next())
        return q.value(0).toInt();
    return 0;
}

void TerminalHistoryStore::clearAll()
{
    if (!ensureDatabase()) return;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    QSqlQuery q(db);
    q.exec(QStringLiteral("DELETE FROM commands"));
}

void TerminalHistoryStore::clearProject(const QString& projectPath)
{
    if (projectPath.isEmpty()) return;
    if (!ensureDatabase()) return;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM commands WHERE project_path = :proj"));
    q.bindValue(QStringLiteral(":proj"), projectPath);
    q.exec();
}

} // namespace SprintIDE