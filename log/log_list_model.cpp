#include "log_list_model.h"
#include "core/log_buffer.h"
#include <QDateTime>

namespace SprintIDE {

LogListModel::LogListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int LogListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return m_view.size();
}

QVariant LogListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) return {};
    const int row = index.row();
    if (row < 0 || row >= m_view.size()) return {};

    const LogEntry& e = m_all[m_view[row]];

    switch (role) {
    case Qt::DisplayRole:
    case MessageRole:
        return e.message;
    case LevelRole:
        return static_cast<int>(e.level);
    case KindRole:
        return static_cast<int>(e.kind);
    case TimeRole:
        return QDateTime::fromMSecsSinceEpoch(e.timestamp);
    case SourceRole:
        return e.source;
    case FileRole:
        return e.filePath;
    case LineRole:
        return e.lineNumber;
    case ColumnRole:
        return e.column;
    case IsGroupHeaderRole:
        return false;
    default:
        return {};
    }
}

// ============================================================
//  追加
// ============================================================
void LogListModel::appendEntry(const LogEntry& e)
{
    // 先更新全量 + 计数
    m_all.append(e);
    switch (e.level) {
    case LogLevel::Error:   ++m_counts[2]; break;
    case LogLevel::Warning: ++m_counts[1]; break;
    default:                ++m_counts[0]; break;
    }

    // 再判断是否通过过滤，通过才 insert 到 view
    if (passesFilter(e)) {
        const int viewRow = m_view.size();
        beginInsertRows(QModelIndex(), viewRow, viewRow);
        m_view.append(m_all.size() - 1);
        endInsertRows();
    }

    emit countsChanged(m_counts[2], m_counts[1], m_counts[0], m_view.size());
}

void LogListModel::appendEntries(const QList<LogEntry>& entries)
{
    if (entries.isEmpty()) return;

    // 更新全量
    const int oldAll = m_all.size();
    m_all.append(entries);
    for (const LogEntry& e : entries) {
        switch (e.level) {
        case LogLevel::Error:   ++m_counts[2]; break;
        case LogLevel::Warning: ++m_counts[1]; break;
        default:                ++m_counts[0]; break;
        }
    }

    // 收集通过过滤的新行
    QList<int> newViewRows;
    for (int i = oldAll; i < m_all.size(); ++i) {
        if (passesFilter(m_all[i])) newViewRows.append(i);
    }

    if (!newViewRows.isEmpty()) {
        const int firstView = m_view.size();
        const int lastView  = firstView + newViewRows.size() - 1;
        beginInsertRows(QModelIndex(), firstView, lastView);
        m_view.append(newViewRows);
        endInsertRows();
    }

    emit countsChanged(m_counts[2], m_counts[1], m_counts[0], m_view.size());
}

void LogListModel::clear()
{
    beginResetModel();
    m_all.clear();
    m_view.clear();
    m_counts[0] = m_counts[1] = m_counts[2] = 0;
    endResetModel();

    emit countsChanged(0, 0, 0, 0);
}

// ============================================================
//  过滤
// ============================================================
void LogListModel::setShowInfo(bool on)
{
    if (m_showInfo == on) return;
    m_showInfo = on;
    reapplyFilters();
}

void LogListModel::setShowWarning(bool on)
{
    if (m_showWarning == on) return;
    m_showWarning = on;
    reapplyFilters();
}

void LogListModel::setShowError(bool on)
{
    if (m_showError == on) return;
    m_showError = on;
    reapplyFilters();
}

void LogListModel::setTextFilter(const QString& needle)
{
    if (m_textFilter == needle) return;
    m_textFilter = needle;
    reapplyFilters();
}

void LogListModel::reapplyFilters()
{
    beginResetModel();
    m_view.clear();
    for (int i = 0; i < m_all.size(); ++i) {
        if (passesFilter(m_all[i])) m_view.append(i);
    }
    endResetModel();

    emit countsChanged(m_counts[2], m_counts[1], m_counts[0], m_view.size());
}

bool LogListModel::passesFilter(const LogEntry& e) const
{
    switch (e.level) {
    case LogLevel::Error:   if (!m_showError)   return false; break;
    case LogLevel::Warning: if (!m_showWarning) return false; break;
    default:                if (!m_showInfo)    return false; break;
    }

    if (!m_textFilter.isEmpty()) {
        if (!e.message.contains(m_textFilter, Qt::CaseInsensitive)) return false;
    }
    return true;
}

void LogListModel::recountTotals()
{
    m_counts[0] = m_counts[1] = m_counts[2] = 0;
    for (const LogEntry& e : m_all) {
        switch (e.level) {
        case LogLevel::Error:   ++m_counts[2]; break;
        case LogLevel::Warning: ++m_counts[1]; break;
        default:                ++m_counts[0]; break;
        }
    }
}

const LogEntry& LogListModel::entryAt(int viewRow) const
{
    static const LogEntry empty;
    if (viewRow < 0 || viewRow >= m_view.size()) return empty;
    return m_all[m_view[viewRow]];
}

} // namespace SprintIDE
