#include "rust_external_library_provider.h"

#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QDebug>

namespace SprintIDE {

    QList<ExternalLibraryNode> RustExternalLibraryProvider::collect(const QString& projectPath)
    {
        QList<ExternalLibraryNode> roots;
        if (!QFileInfo::exists(projectPath + "/Cargo.toml")) return roots;

        QProcess proc;
        proc.setWorkingDirectory(projectPath);
        proc.start("cargo", {"metadata", "--format-version", "1", "--no-deps"});
        // --no-deps 只要直接依赖；如果要全依赖树改成不带 --no-deps
        // 但全依赖树可能有几千项，先只列直接依赖
        if (!proc.waitForFinished(8000) || proc.exitCode() != 0) {
            qWarning() << "[RustExtLib] cargo metadata 失败";
            return roots;
        }

        auto doc = QJsonDocument::fromJson(proc.readAllStandardOutput());
        if (!doc.isObject()) return roots;

        auto obj = doc.object();
        auto packages = obj["packages"].toArray();
        const QString canonicalProject = QFileInfo(projectPath).canonicalFilePath();

        ExternalLibraryNode root;
        root.kind = ExternalLibraryNode::Kind::Root;
        root.displayName = QStringLiteral("Cargo 依赖");

        for (const auto& v : packages) {
            auto pkg = v.toObject();
            const QString manifest = pkg["manifest_path"].toString();
            const QString pkgRoot = QFileInfo(manifest).absolutePath();

            // 跳过本项目自身
            if (pkgRoot == canonicalProject) continue;

            ExternalLibraryNode lib;
            lib.kind = ExternalLibraryNode::Kind::Package;
            lib.displayName = pkg["name"].toString()
            + " " + pkg["version"].toString();
            lib.absolutePath = pkgRoot;
            root.children.append(lib);
        }

        if (!root.children.isEmpty()) roots.append(root);
        return roots;
    }

} // namespace SprintIDE