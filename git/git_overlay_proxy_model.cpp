#include "git_overlay_proxy_model.h"
#include "theme_manager.h"

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
    // 复用 ThemeManager 的语义色，随主题切换
    // ProjectPanel 的文件名染色和 GitPanel 的列表项染色保持一致
    using UI = ThemeManager::UIElement;
    const auto& tm = ThemeManager::instance();

    switch (s) {
    case FileStatus::Added:       return tm.color(UI::SuccessColor);
    case FileStatus::Modified:    return tm.color(UI::WarningColor);
    case FileStatus::Deleted:     return tm.color(UI::ErrorColor);
    case FileStatus::Renamed:     return tm.color(UI::PrimaryColor);
    case FileStatus::Copied:      return tm.color(UI::PrimaryColor);
    case FileStatus::Untracked:   return tm.color(UI::SecondaryTextColor);
    case FileStatus::Conflicted:  return tm.color(UI::ErrorColor);
    case FileStatus::TypeChanged: return tm.color(UI::PrimaryColor);
    default:                      return tm.color(UI::TextColor);
    }
}

} // namespace Git
} // namespace SprintIDE
