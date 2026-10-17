#include "node_external_library_provider.h"

#include <QDir>
#include <QFileInfo>
#include <QDebug>

namespace SprintIDE {

    QList<ExternalLibraryNode> NodeExternalLibraryProvider::collect(const QString& projectPath)
    {
        QList<ExternalLibraryNode> roots;

        const QString nm = projectPath + "/node_modules";
        if (!QDir(nm).exists()) return roots;

        ExternalLibraryNode root;
        root.kind = ExternalLibraryNode::Kind::Root;
        root.displayName = QStringLiteral("node_modules");

        QDir nmDir(nm);
        const auto entries = nmDir.entryInfoList(
            QDir::Dirs | QDir::NoDotAndDotDot,
            QDir::Name | QDir::IgnoreCase);

        for (const QFileInfo& fi : entries) {
            const QString name = fi.fileName();
            if (name.startsWith('.')) continue;

            // @scope/pkg 两级结构
            if (name.startsWith('@')) {
                ExternalLibraryNode scope;
                scope.kind = ExternalLibraryNode::Kind::Group;
                scope.displayName = name;
                scope.absolutePath = fi.absoluteFilePath();

                QDir scopeDir(fi.absoluteFilePath());
                const auto subs = scopeDir.entryInfoList(
                    QDir::Dirs | QDir::NoDotAndDotDot,
                    QDir::Name | QDir::IgnoreCase);
                for (const QFileInfo& sub : subs) {
                    ExternalLibraryNode pkg;
                    pkg.kind = ExternalLibraryNode::Kind::Package;
                    pkg.displayName = sub.fileName();
                    pkg.absolutePath = sub.absoluteFilePath();
                    scope.children.append(pkg);
                }
                if (!scope.children.isEmpty()) root.children.append(scope);
            } else {
                ExternalLibraryNode pkg;
                pkg.kind = ExternalLibraryNode::Kind::Package;
                pkg.displayName = name;
                pkg.absolutePath = fi.absoluteFilePath();
                root.children.append(pkg);
            }
        }

        if (!root.children.isEmpty()) roots.append(root);
        return roots;
    }

} // namespace SprintIDE