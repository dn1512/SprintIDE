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
#ifndef PROJECT_TREE_PROXY_MODEL_H
#define PROJECT_TREE_PROXY_MODEL_H

#include <QIdentityProxyModel>
#include <QStandardItemModel>
#include <QPersistentModelIndex>
#include <QIcon>

#include "external_libs/external_library_types.h"

namespace SprintIDE {

    /**
    * @brief 在文件树的项目根层末尾追加一个"外部库"虚拟节点
    *
    * 模型链：
    *   QFileSystemModel
    *     → ExcludeDirProxyModel    (QSortFilterProxyModel)
    *       → GitOverlayProxyModel  (QIdentityProxyModel)
    *         → ProjectTreeProxyModel   ← 本类（QIdentityProxyModel）
    *           → QTreeView
    *
    * v8.5 修复：
    *   - 从 QAbstractProxyModel 改为 QIdentityProxyModel —— 复用其成熟的
    *     mapToSource / mapFromSource / internalPointer 透传，避免之前手工缓存
    *     internalId 导致的"所有节点映射到同一个 index"和"根目录丢失"问题。
    *   - QIdentityProxyModel 自动转发源模型的 dataChanged / rowsInserted /
    *     rowsRemoved / modelReset，不需要手工 connect。
    */
    class ProjectTreeProxyModel : public QIdentityProxyModel
    {
        Q_OBJECT
        public:
        explicit ProjectTreeProxyModel(QObject* parent = nullptr);

        // ============================================================
        // 项目根
        // ============================================================
        void setProjectRootSource(const QModelIndex& sourceIndex);
        void clearProjectRoot();
        QModelIndex projectRootIndex() const;
        bool isProjectRoot(const QModelIndex& idx) const;

        // ============================================================
        // 虚拟节点
        // ============================================================
        void applyExternalLibraries(const QList<ExternalLibraryNode>& roots, bool visible);
        void setExternalTitle(const QString& title);

        bool isExternalIndex(const QModelIndex& idx) const;
        bool isExternalRoot(const QModelIndex& idx) const;

        // ============================================================
        // QIdentityProxyModel 覆写
        // ============================================================
        QModelIndex mapToSource(const QModelIndex& proxy) const override;
        QModelIndex index(int row, int column,
            const QModelIndex& parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex& child) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role) const override;
        QVariant headerData(int section, Qt::Orientation orientation,
            int role) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;
        void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

        private:
        QStandardItem* decodeStandardItem(const QModelIndex& idx) const;
        quintptr encodeStandardItem(QStandardItem* item) const;
        void appendExternalNode(QStandardItem* parentItem, const ExternalLibraryNode& node);
        QIcon iconForKind(ExternalLibraryNode::Kind kind) const;

        // 虚拟节点 internalId 标记
        //   QFileSystemNode* / QSortFilterProxyModel 分配的指针，用户态地址最高位为 0
        //   所以用最高位为 1 作为虚拟节点标识，无冲突
        static constexpr quintptr kExternalRootId  = quintptr(-1);
        static constexpr quintptr kExternalItemTag = quintptr(1) << (sizeof(quintptr) * 8 - 1);

        QStandardItemModel*     m_externalModel    = nullptr;
        QPersistentModelIndex   m_projectRootSource;
        QString                 m_externalTitle    = QStringLiteral("外部库");
        bool                    m_externalVisible  = false;
    };

} // namespace SprintIDE

#endif // PROJECT_TREE_PROXY_MODEL_H