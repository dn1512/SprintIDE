#include "core/log_buffer.h"
#include <QDateTime>

namespace SprintIDE {

// ============================================================
// LogEntry
// ============================================================
LogEntry LogEntry::make(LogKind kind, LogLevel level,
                        const QString& source, const QString& message,
                        const QString& filePath, int lineNumber, int column)
{
    LogEntry e;
    e.timestamp  = QDateTime::currentMSecsSinceEpoch();
    e.kind       = kind;
    e.level      = level;
    e.source     = source;
    e.message    = message;
    e.filePath   = filePath;
    e.lineNumber = lineNumber;
    e.column     = column;
    return e;
}

// ============================================================
// LogBuffer
// ============================================================
LogBuffer::LogBuffer(int capacity)
    : m_capacity(qMax(50, capacity))
{
    m_entries.reserve(qMin(m_capacity, 256));
}

void LogBuffer::append(const LogEntry& entry)
{
    QMutexLocker locker(&m_mutex);

    LogEntry e = entry;
    if (e.timestamp == 0) {
        e.timestamp = QDateTime::currentMSecsSinceEpoch();
    }
    m_entries.append(e);

    // 超过容量：从头部丢弃最旧的，保留最新 m_capacity 条
    if (m_entries.size() > m_capacity) {
        m_entries.remove(0, m_entries.size() - m_capacity);
    }
}

void LogBuffer::append(LogKind kind, LogLevel level,
                       const QString& source, const QString& message,
                       const QString& filePath, int lineNumber)
{
    append(LogEntry::make(kind, level, source, message, filePath, lineNumber));
}

void LogBuffer::attach(qint64 timestamp, const QString& relPath)
{
    QMutexLocker locker(&m_mutex);
    for (int i = m_entries.size() - 1; i >= 0; --i) {
        if (m_entries[i].timestamp == timestamp) {
            m_entries[i].attachments.append(relPath);
            return;
        }
    }
}

void LogBuffer::clear()
{
    QMutexLocker locker(&m_mutex);
    m_entries.clear();
}

int LogBuffer::size() const
{
    QMutexLocker locker(&m_mutex);
    return m_entries.size();
}

int LogBuffer::capacity() const
{
    QMutexLocker locker(&m_mutex);
    return m_capacity;
}

void LogBuffer::setCapacity(int capacity)
{
    QMutexLocker locker(&m_mutex);
    m_capacity = qMax(50, capacity);
    if (m_entries.size() > m_capacity) {
        m_entries.remove(0, m_entries.size() - m_capacity);
    }
}

QList<LogEntry> LogBuffer::recent(int n) const
{
    QMutexLocker locker(&m_mutex);
    const int total = m_entries.size();
    if (n <= 0 || n >= total) return m_entries;
    return m_entries.mid(total - n);
}

QList<LogEntry> LogBuffer::since(qint64 timestamp) const
{
    QMutexLocker locker(&m_mutex);
    QList<LogEntry> result;
    for (const auto& e : m_entries) {
        if (e.timestamp >= timestamp) result.append(e);
    }
    return result;
}

QList<LogEntry> LogBuffer::byKind(LogKind kind, int n) const
{
    QMutexLocker locker(&m_mutex);
    QList<LogEntry> result;
    for (int i = m_entries.size() - 1; i >= 0 && result.size() < n; --i) {
        if (m_entries[i].kind == kind) {
            result.prepend(m_entries[i]);
        }
    }
    return result;
}

QList<LogEntry> LogBuffer::onlyErrors(int n) const
{
    QMutexLocker locker(&m_mutex);
    QList<LogEntry> result;
    for (int i = m_entries.size() - 1; i >= 0 && result.size() < n; --i) {
        if (m_entries[i].level == LogLevel::Error) {
            result.prepend(m_entries[i]);
        }
    }
    return result;
}

QString LogBuffer::kindToString(LogKind kind)
{
    switch (kind) {
    case LogKind::Compile:  return QStringLiteral("compile");
    case LogKind::Run:      return QStringLiteral("run");
    case LogKind::Debug:    return QStringLiteral("debug");
    case LogKind::Terminal: return QStringLiteral("terminal");
    case LogKind::System:   return QStringLiteral("system");
    case LogKind::UI:       return QStringLiteral("ui");
    default:                return QStringLiteral("unknown");
    }
}

QString LogBuffer::levelToString(LogLevel level)
{
    switch (level) {
    case LogLevel::Info:    return QStringLiteral("info");
    case LogLevel::Warning: return QStringLiteral("warning");
    case LogLevel::Error:   return QStringLiteral("error");
    default:                return QStringLiteral("unknown");
    }
}

LogKind LogBuffer::stringToKind(const QString& s)
{
    const QString v = s.toLower();
    if (v == "compile")  return LogKind::Compile;
    if (v == "run")      return LogKind::Run;
    if (v == "debug")    return LogKind::Debug;
    if (v == "terminal") return LogKind::Terminal;
    if (v == "system")   return LogKind::System;
    if (v == "ui")       return LogKind::UI;
    return LogKind::Unknown;
}

LogLevel LogBuffer::stringToLevel(const QString& s)
{
    const QString v = s.toLower();
    if (v == "warning") return LogLevel::Warning;
    if (v == "error")   return LogLevel::Error;
    return LogLevel::Info;
}

QString LogBuffer::toText(const QList<LogEntry>& entries)
{
    QStringList lines;
    lines.reserve(entries.size());
    for (const auto& e : entries) {
        QString line = QStringLiteral("[%1] [%2/%3] %4")
                           .arg(QDateTime::fromMSecsSinceEpoch(e.timestamp)
                                    .toString(QStringLiteral("HH:mm:ss.zzz")),
                                kindToString(e.kind),
                                levelToString(e.level),
                                e.message);
        if (!e.filePath.isEmpty()) {
            line += QStringLiteral(" (") + e.filePath;
            if (e.lineNumber > 0)
                line += QStringLiteral(":") + QString::number(e.lineNumber);
            line += QLatin1Char(')');
        }
        if (!e.attachments.isEmpty()) {
            line += QStringLiteral(" {attachments: ")
                    + e.attachments.join(QStringLiteral(", "))
                    + QLatin1Char('}');
        }
        lines.append(line);
    }
    return lines.join(QLatin1Char('\n'));
}

} // namespace SprintIDE
