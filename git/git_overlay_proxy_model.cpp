#include "git_overlay_proxy_model.h"

#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QFileInfo>
#include <QDebug>

namespace SprintIDE {
namespace Git {

GitOverlayProxyModel::GitOverlayProxyModel(QObject* parent)
    : QIdentityProxyModel(parent)
{
}

void GitOverlayProxyModel::setFileSystemModel(QFileSystemModel* model)
{
    m_fsModel = model;
}

void GitOverlayProxyModel::setIntermediateProxy(QSortFilterProxyModel* proxy)
{
    m_intermediateProxy = proxy;
}

void GitOverlayProxyModel::setStatusMap(const QMap<QString, FileStatus>& map)
{
    m_statusMap = map;
}

void GitOverlayProxyModel::clearStatusMap()
{
    if (m_statusMap.isEmpty()) return;
    m_statusMap.clear();
}

QString GitOverlayProxyModel::resolveAbsolutePath(const QModelIndex& proxyIndex) const
{
    if (!proxyIndex.isValid()) return QString();
    if (!m_fsModel || !m_intermediateProxy) return QString();

    // 第一段：本 proxy → ExcludeDirProxyModel
    //   QIdentityProxyModel::mapToSource 会调用 sourceModel()->index(...)，
    //   对于 QSortFilterProxyModel 来说返回的就是它自己的 index
    QModelIndex excludeIdx = QIdentityProxyModel::mapToSource(proxyIndex);
    if (!excludeIdx.isValid()) return QString();

    // 第二段：ExcludeDirProxyModel → QFileSystemModel
    QModelIndex fsIdx = m_intermediateProxy->mapToSource(excludeIdx);
    if (!fsIdx.isValid()) return QString();

    return m_fsModel->filePath(fsIdx);
}

QVariant GitOverlayProxyModel::data(const QModelIndex& index, int role) const
{
    // 只覆写 ForegroundRole（文件名颜色）
    //   根据 git 状态给文件名染色 —— 比右下角圆点更醒目
    if (role == Qt::ForegroundRole && !m_statusMap.isEmpty()) {
        const QString abs = resolveAbsolutePath(index);
        if (!abs.isEmpty()) {
            auto it = m_statusMap.constFind(abs);
            if (it != m_statusMap.constEnd()
                && it.value() != FileStatus::Unmodified) {
                return QVariant::fromValue(statusColor(it.value()));
            }
        }
    }
    return QIdentityProxyModel::data(index, role);
}

void GitOverlayProxyModel::sort(int column, Qt::SortOrder order)
{
    // QIdentityProxyModel 不实现 sort()，转发到 sourceModel（ExcludeDirProxyModel）
    if (auto* src = sourceModel()) {
        src->sort(column, order);
    }
}

QColor GitOverlayProxyModel::statusColor(FileStatus s)
{
    // 这些颜色在深色主题下与原文字色 #D4DCE8 有足够反差；
    // 饱和度略高于圆点方案，因为文字面积大、需要更柔和
    switch (s) {
    case FileStatus::Added:       return QColor("#6BCC8E");   // 柔和绿
    case FileStatus::Modified:    return QColor("#E0B45C");   // 琥珀
    case FileStatus::Deleted:     return QColor("#E07A85");   // 柔和红
    case FileStatus::Renamed:     return QColor("#6AACEF");   // 蓝
    case FileStatus::Copied:      return QColor("#6AACEF");
    case FileStatus::Untracked:   return QColor("#8A95A8");   // 略亮于默认次要文字
    case FileStatus::Conflicted:  return QColor("#E55B6B");   // 冲突：突出红
    case FileStatus::TypeChanged: return QColor("#6AACEF");
    default:                      return QColor("#D4DCE8");
    }
}

} // namespace Git
} // namespace SprintIDE
