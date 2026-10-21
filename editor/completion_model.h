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
#ifndef COMPLETION_MODEL_H
#define COMPLETION_MODEL_H

#include <QAbstractListModel>
#include <QString>
#include <QList>

namespace SprintIDE {

/**
 * @brief 单条补全项
 */
struct CompletionEntry {
    QString label;         // 显示名："foo"
    QString insertText;    // 插入内容：可能是 "foo()" 或 "foo"
    QString detail;        // 右侧灰字："int" / "function"
    QString kind;          // "var" "func" "class" "keyword" "lsp"
    int     sortPriority = 50;   // 越小越靠前

    // 源数据（LSP 需要原样返回）
    QVariant rawData;

    bool operator<(const CompletionEntry& o) const {
        if (sortPriority != o.sortPriority) return sortPriority < o.sortPriority;
        return label.localeAwareCompare(o.label) < 0;
    }
};

/**
 * @brief 补全列表模型
 *
 * 用 QListView 显示，不用 QListWidget（便于以后做分组、图标）。
 */
class CompletionModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        DetailRole = Qt::UserRole + 1,
        InsertTextRole,
        KindRole
    };

    explicit CompletionModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    void setEntries(const QList<CompletionEntry>& entries);
    const CompletionEntry& entryAt(int row) const;

    // 前缀过滤：返回过滤后的行数
    void setFilterPrefix(const QString& prefix);
    QString filterPrefix() const { return m_prefix; }

private:
    QList<CompletionEntry> m_allEntries;    // 全量
    QList<int>             m_filteredRows;  // 过滤后的行索引
    QString                m_prefix;
};

} // namespace SprintIDE

#endif
