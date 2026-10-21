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
#ifndef LOG_LIST_MODEL_H
#define LOG_LIST_MODEL_H

#include <QAbstractListModel>
#include <QList>
#include "core/log_buffer.h"   // 复用 LogEntry / LogLevel / LogKind

namespace SprintIDE {

class LogListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        LevelRole = Qt::UserRole + 1,
        KindRole,
        TimeRole,
        SourceRole,
        FileRole,
        LineRole,
        ColumnRole,
        MessageRole,
        IsGroupHeaderRole,
    };

    explicit LogListModel(QObject* parent = nullptr);

    // ---- QAbstractListModel 接口 ----
    int      rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    // ---- 追加 ----
    void appendEntry(const LogEntry& e);
    void appendEntries(const QList<LogEntry>& entries);   // 批量：合并 begin/endInsertRows

    // ---- 清空 ----
    void clear();

    // ---- 过滤 ----
    void setShowInfo(bool on);
    void setShowWarning(bool on);
    void setShowError(bool on);
    void setTextFilter(const QString& needle);   // 空字符串 = 不过滤
    void reapplyFilters();                       // 手动触发重建

    // ---- 查询 ----
    const LogEntry& entryAt(int viewRow) const;  // viewRow 是过滤后的行号
    int  totalCount()  const { return m_all.size(); }
    int  viewCount()   const { return m_view.size(); }
    int  errorCount()  const { return m_counts[2]; }
    int  warningCount()const { return m_counts[1]; }
    int  infoCount()   const { return m_counts[0]; }

signals:
    /// 过滤后可见计数发生变化（视图底部状态栏用）
    void countsChanged(int errors, int warnings, int infos, int visible);

private:
    void recountTotals();
    bool passesFilter(const LogEntry& e) const;

    QList<LogEntry> m_all;     // 全量（AI 喂料 / 导出用）
    QList<int>      m_view;    // 通过过滤的 m_all 下标

    bool    m_showInfo    = true;
    bool    m_showWarning = true;
    bool    m_showError   = true;
    QString m_textFilter;

    // [info, warning, error] 全量计数（不随过滤变化）
    int     m_counts[3] = {0, 0, 0};
};

} // namespace SprintIDE

#endif // LOG_LIST_MODEL_H
