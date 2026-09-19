#ifndef SPRINTIDE_GIT_OVERLAY_PROXY_MODEL_H
#define SPRINTIDE_GIT_OVERLAY_PROXY_MODEL_H

#include <QIdentityProxyModel>
#include <QIcon>
#include <QMap>
#include <QColor>
#include <QPair>
#include <QtGlobal>

#include "git_types.h"

QT_BEGIN_NAMESPACE
class QFileSystemModel;
class QSortFilterProxyModel;
QT_END_NAMESPACE

namespace SprintIDE {
namespace Git {

/**
 * @brief 给 ProjectPanel 的文件树叠加 Git 状态圆点
 *
 * 模型链：
 *   QFileSystemModel
 *     → ExcludeDirProxyModel (QSortFilterProxyModel，过滤/排序)
 *       → GitOverlayProxyModel (本类，只覆写 DecorationRole)
 *         → QTreeView
 *
 * 只做一件事：在 DecorationRole 上，把原始图标右下角叠一个状态色圆点。
 * 不改结构、不改数据、不改拖放行为。
 */
class GitOverlayProxyModel : public QIdentityProxyModel
{
    Q_OBJECT
public:
    explicit GitOverlayProxyModel(QObject* parent = nullptr);

    // 必须设置这两者，才能把 proxy index 反查成文件绝对路径
    void setFileSystemModel(QFileSystemModel* model);
    void setIntermediateProxy(QSortFilterProxyModel* proxy);

    void setStatusMap(const QMap<QString, FileStatus>& map);   // key = absolutePath
    void clearStatusMap();

    QVariant data(const QModelIndex& index, int role) const override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    static QColor statusColor(FileStatus s);

private:
    QString resolveAbsolutePath(const QModelIndex& proxyIndex) const;

    QFileSystemModel*        m_fsModel           = nullptr;
    QSortFilterProxyModel*   m_intermediateProxy = nullptr;
    QMap<QString, FileStatus> m_statusMap;
};

} // namespace Git
} // namespace SprintIDE

#endif // SPRINTIDE_GIT_OVERLAY_PROXY_MODEL_H
