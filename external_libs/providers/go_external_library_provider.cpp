#include "go_external_library_provider.h"

#include <QProcess>
#include <QFileInfo>
#include <QDebug>

namespace SprintIDE {

    QList<ExternalLibraryNode> GoExternalLibraryProvider::collect(const QString& projectPath)
    {
        QList<ExternalLibraryNode> roots;
        if (!QFileInfo::exists(projectPath + "/go.mod")) return roots;

        // 只列直接依赖（"all" 会包含整个模块图，太大）
        QProcess proc;
        proc.setWorkingDirectory(projectPath);
        proc.start("go", {"list", "-m", "-f",
                "{{.Path}}|{{.Version}}|{{.Dir}}", "all"});
        if (!proc.waitForFinished(8000) || proc.exitCode() != 0) {
            qWarning() << "[GoExtLib] go list 失败";
            return roots;
        }

        ExternalLibraryNode root;
        root.kind = ExternalLibraryNode::Kind::Root;
        root.displayName = QStringLiteral("Go Modules");

        const QString out = QString::fromLocal8Bit(proc.readAllStandardOutput());
        for (const QString& line : out.split('\n', Qt::SkipEmptyParts)) {
            const QStringList parts = line.split('|');
            if (parts.size() < 3) continue;
            if (parts[0].isEmpty() || parts[2].isEmpty()) continue;

            ExternalLibraryNode mod;
            mod.kind = ExternalLibraryNode::Kind::Package;
            mod.displayName = parts[0];
            if (!parts[1].isEmpty()) mod.displayName += " " + parts[1];
            mod.absolutePath = parts[2];
            root.children.append(mod);
        }

        if (!root.children.isEmpty()) roots.append(root);
        return roots;
    }

} // namespace SprintIDE