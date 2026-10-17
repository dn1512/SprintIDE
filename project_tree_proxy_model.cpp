#include "project_tree_proxy_model.h"

#include <QStandardItem>
#include <QFileInfo>
#include <QFont>
#include <QDebug>

namespace SprintIDE {

    ProjectTreeProxyModel::ProjectTreeProxyModel(QObject* parent)
    : QIdentityProxyModel(parent)
    {
        m_externalModel = new QStandardItemModel(this);
    }

    // ============================================================
    //  项目根
    // ============================================================

    void ProjectTreeProxyModel::setProjectRootSource(const QModelIndex& sourceIndex)
    {
        m_projectRootSource = QPersistentModelIndex(sourceIndex);
    }

    void ProjectTreeProxyModel::clearProjectRoot()
    {
        m_projectRootSource = QPersistentModelIndex();
    }

    QModelIndex ProjectTreeProxyModel::projectRootIndex() const
    {
        if (!m_projectRootSource.isValid() || !sourceModel()) return QModelIndex();
        return mapFromSource(m_projectRootSource);
    }

    bool ProjectTreeProxyModel::isProjectRoot(const QModelIndex& idx) const
    {
        if (!idx.isValid() || isExternalIndex(idx) || !m_projectRootSource.isValid())
        return false;
        return idx.internalId() == m_projectRootSource.internalId()
        && idx.row()       == m_projectRootSource.row()
        && idx.column()    == m_projectRootSource.column();
    }

    // ============================================================
    //  虚拟节点管理
    // ============================================================

    void ProjectTreeProxyModel::applyExternalLibraries(const QList<ExternalLibraryNode>& roots,
        bool visible)
    {
        QModelIndex projRoot = projectRootIndex();

        // ---- 项目根未建立：只更新内容 ----
        if (!projRoot.isValid()) {
            m_externalModel->clear();
            for (const auto& node : roots) {
                appendExternalNode(m_externalModel->invisibleRootItem(), node);
            }
            m_externalVisible = visible;
            return;
        }

        const int srcCount = QIdentityProxyModel::rowCount(projRoot);

        // ---- 隐藏 → 显示 ----
        if (!m_externalVisible && visible) {
            beginInsertRows(projRoot, srcCount, srcCount);
            m_externalModel->clear();
            for (const auto& node : roots) {
                appendExternalNode(m_externalModel->invisibleRootItem(), node);
            }
            m_externalVisible = true;
            endInsertRows();
            return;
        }

        // ---- 显示 → 隐藏 ----
        if (m_externalVisible && !visible) {
            beginRemoveRows(projRoot, srcCount, srcCount);
            m_externalVisible = false;
            m_externalModel->clear();
            endRemoveRows();
            return;
        }

        // ---- 都显示，只更新内容 ----
        if (m_externalVisible && visible) {
            beginResetModel();
            m_externalModel->clear();
            for (const auto& node : roots) {
                appendExternalNode(m_externalModel->invisibleRootItem(), node);
            }
            endResetModel();
            return;
        }

        // ---- 都隐藏 ----
        m_externalModel->clear();
        for (const auto& node : roots) {
            appendExternalNode(m_externalModel->invisibleRootItem(), node);
        }
    }

    void ProjectTreeProxyModel::setExternalTitle(const QString& title)
    {
        if (m_externalTitle == title) return;
        m_externalTitle = title;
        if (m_externalVisible) {
            QModelIndex root = projectRootIndex();
            if (root.isValid()) {
                int srcCount = QIdentityProxyModel::rowCount(root);
                QModelIndex vr = index(srcCount, 0, root);
                if (vr.isValid()) emit dataChanged(vr, vr);
            }
        }
    }

    bool ProjectTreeProxyModel::isExternalRoot(const QModelIndex& idx) const
    {
        return idx.isValid() && idx.internalId() == kExternalRootId;
    }

    bool ProjectTreeProxyModel::isExternalIndex(const QModelIndex& idx) const
    {
        if (!idx.isValid()) return false;
        const quintptr id = idx.internalId();
        if (id == kExternalRootId) return true;
        return (id & kExternalItemTag) != 0;
    }

    // ============================================================
    //  QIdentityProxyModel 覆写
    // ============================================================

    QModelIndex ProjectTreeProxyModel::mapToSource(const QModelIndex& proxy) const
    {
        if (!proxy.isValid()) return QModelIndex();
        if (isExternalIndex(proxy)) return QModelIndex();    // 虚拟节点无源
        return QIdentityProxyModel::mapToSource(proxy);
    }

    QModelIndex ProjectTreeProxyModel::index(int row, int column,
        const QModelIndex& parent) const
    {
        // ---- 虚拟子树 ----
        if (isExternalIndex(parent)) {
            QStandardItem* pItem = isExternalRoot(parent)
            ? m_externalModel->invisibleRootItem()
            : decodeStandardItem(parent);
            if (!pItem) return QModelIndex();
            QStandardItem* item = pItem->child(row, column);
            if (!item) return QModelIndex();
            return createIndex(row, column, encodeStandardItem(item));
        }

        // ---- 项目根的子行 ----
        if (isProjectRoot(parent)) {
            int srcCount = QIdentityProxyModel::rowCount(parent);
            if (row < srcCount) {
                return QIdentityProxyModel::index(row, column, parent);
            }
            if (row == srcCount && column == 0 && m_externalVisible) {
                return createIndex(row, column, kExternalRootId);
            }
            return QModelIndex();
        }

        return QIdentityProxyModel::index(row, column, parent);
    }

    QModelIndex ProjectTreeProxyModel::parent(const QModelIndex& child) const
    {
        if (!child.isValid()) return QModelIndex();

        // 虚拟根节点 → parent 是项目根
        if (isExternalRoot(child)) {
            return projectRootIndex();
        }

        // 虚拟子树内部 → parent 是父 QStandardItem
        if (isExternalIndex(child)) {
            QStandardItem* cItem = decodeStandardItem(child);
            if (!cItem) return QModelIndex();
            QStandardItem* pItem = cItem->parent();
            if (!pItem || pItem == m_externalModel->invisibleRootItem()) {
                // 直接子节点 → parent 是虚拟根
                QModelIndex projRoot = projectRootIndex();
                if (!projRoot.isValid()) return QModelIndex();
                int srcCount = QIdentityProxyModel::rowCount(projRoot);
                return createIndex(srcCount, 0, kExternalRootId);
            }
            return createIndex(pItem->row(), 0, encodeStandardItem(pItem));
        }

        // 普通节点 → 基类处理（不递归）
        return QIdentityProxyModel::parent(child);
    }

    int ProjectTreeProxyModel::rowCount(const QModelIndex& parent) const
    {
        if (!sourceModel()) return 0;

        if (isExternalRoot(parent)) {
            return m_externalModel->invisibleRootItem()->rowCount();
        }
        if (isExternalIndex(parent)) {
            QStandardItem* item = decodeStandardItem(parent);
            return item ? item->rowCount() : 0;
        }
        if (isProjectRoot(parent)) {
            return QIdentityProxyModel::rowCount(parent) + (m_externalVisible ? 1 : 0);
        }
        return QIdentityProxyModel::rowCount(parent);
    }

    int ProjectTreeProxyModel::columnCount(const QModelIndex& parent) const
    {
        if (isExternalIndex(parent)) return 1;
        return QIdentityProxyModel::columnCount(parent);
    }

    QVariant ProjectTreeProxyModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid()) return QVariant();

        if (isExternalRoot(index)) {
            switch (role) {
                case Qt::DisplayRole:
                return m_externalTitle;
                case Qt::DecorationRole:
                return QIcon(QStringLiteral(":/icons/icons/folder.svg"));
                case Qt::FontRole: {
                    QFont f; f.setBold(true); return f;
                }
                case Qt::ToolTipRole:
                return QStringLiteral("检测到的外部依赖");
                default:
                return QVariant();
            }
        }
        if (isExternalIndex(index)) {
            QStandardItem* item = decodeStandardItem(index);
            return item ? item->data(role) : QVariant();
        }
        return QIdentityProxyModel::data(index, role);
    }

    QVariant ProjectTreeProxyModel::headerData(int section, Qt::Orientation orientation,
        int role) const
    {
        if (!sourceModel()) return QVariant();
        return QIdentityProxyModel::headerData(section, orientation, role);
    }

    Qt::ItemFlags ProjectTreeProxyModel::flags(const QModelIndex& index) const
    {
        if (!index.isValid()) return Qt::NoItemFlags;
        if (isExternalRoot(index) || isExternalIndex(index)) {
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        }
        return QIdentityProxyModel::flags(index);
    }

    bool ProjectTreeProxyModel::hasChildren(const QModelIndex& parent) const
    {
        if (isExternalRoot(parent)) {
            return m_externalModel->invisibleRootItem()->rowCount() > 0;
        }
        if (isExternalIndex(parent)) {
            QStandardItem* item = decodeStandardItem(parent);
            return item && item->rowCount() > 0;
        }
        if (isProjectRoot(parent)) {
            return rowCount(parent) > 0;
        }
        return QIdentityProxyModel::hasChildren(parent);
    }

    void ProjectTreeProxyModel::sort(int column, Qt::SortOrder order)
    {
        if (sourceModel()) sourceModel()->sort(column, order);
    }

    // ============================================================
    //  内部辅助
    // ============================================================

    QStandardItem* ProjectTreeProxyModel::decodeStandardItem(const QModelIndex& idx) const
    {
        if (!isExternalIndex(idx) || isExternalRoot(idx)) return nullptr;
        quintptr id = idx.internalId() & ~kExternalItemTag;
        return reinterpret_cast<QStandardItem*>(id);
    }

    quintptr ProjectTreeProxyModel::encodeStandardItem(QStandardItem* item) const
    {
        return reinterpret_cast<quintptr>(item) | kExternalItemTag;
    }

    void ProjectTreeProxyModel::appendExternalNode(QStandardItem* parentItem,
        const ExternalLibraryNode& node)
    {
        auto* item = new QStandardItem(node.displayName);
        item->setEditable(false);
        item->setData(static_cast<int>(node.kind), Qt::UserRole + 1);
        item->setData(node.absolutePath, Qt::UserRole + 2);
        item->setIcon(iconForKind(node.kind));

        if (node.kind == ExternalLibraryNode::Kind::Opaque) {
            item->setForeground(QBrush(QColor(128, 128, 128)));
        }

        parentItem->appendRow(item);

        for (const auto& child : node.children) {
            appendExternalNode(item, child);
        }
    }

    QIcon ProjectTreeProxyModel::iconForKind(ExternalLibraryNode::Kind kind) const
    {
        switch (kind) {
            case ExternalLibraryNode::Kind::Root:
            case ExternalLibraryNode::Kind::Group:
            return QIcon(QStringLiteral(":/icons/icons/folder.svg"));
            case ExternalLibraryNode::Kind::Package:
            return QIcon(QStringLiteral(":/icons/icons/file_project.svg"));
            case ExternalLibraryNode::Kind::File:
            case ExternalLibraryNode::Kind::Opaque:
            return QIcon(QStringLiteral(":/icons/icons/file_default.svg"));
        }
        return QIcon();
    }

} // namespace SprintIDE