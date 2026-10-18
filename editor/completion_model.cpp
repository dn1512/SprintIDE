#include "completion_model.h"

namespace SprintIDE {

CompletionModel::CompletionModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int CompletionModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return m_filteredRows.size();
}

QVariant CompletionModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_filteredRows.size()) {
        return {};
    }
    const CompletionEntry& e = m_allEntries[m_filteredRows[index.row()]];

    switch (role) {
    case Qt::DisplayRole:    return e.label;
    case DetailRole:    return e.detail;
    case InsertTextRole:return e.insertText;
    case KindRole:      return e.kind;
    }
    return {};
}

void CompletionModel::setEntries(const QList<CompletionEntry>& entries)
{
    beginResetModel();
    m_allEntries = entries;

    // 排序（按 sortPriority 再按字母）
    std::stable_sort(m_allEntries.begin(), m_allEntries.end());

    // 重建过滤索引
    m_filteredRows.clear();
    for (int i = 0; i < m_allEntries.size(); ++i) {
        m_filteredRows.append(i);
    }

    // 如果有前缀，做一次过滤
    if (!m_prefix.isEmpty()) {
        m_filteredRows.clear();
        for (int i = 0; i < m_allEntries.size(); ++i) {
            if (m_allEntries[i].label.startsWith(m_prefix)) {
                m_filteredRows.append(i);
            }
        }
    }

    endResetModel();
}

const CompletionEntry& CompletionModel::entryAt(int row) const
{
    static CompletionEntry empty;
    if (row < 0 || row >= m_filteredRows.size()) return empty;
    return m_allEntries[m_filteredRows[row]];
}

void CompletionModel::setFilterPrefix(const QString& prefix)
{
    if (m_prefix == prefix) return;
    m_prefix = prefix;

    beginResetModel();
    m_filteredRows.clear();

    if (prefix.isEmpty()) {
        for (int i = 0; i < m_allEntries.size(); ++i) {
            m_filteredRows.append(i);
        }
    } else {
        for (int i = 0; i < m_allEntries.size(); ++i) {
            const QString& label = m_allEntries[i].label;

            // 1) 大小写敏感前缀匹配（v8.5 改：输入 "foo" 不再匹配 "Foo"）
            if (!label.startsWith(prefix)) continue;

            // 2) 完全等于输入内容时排除
            if (label == prefix) continue;

            m_filteredRows.append(i);
        }
    }
    endResetModel();
}

} // namespace SprintIDE
