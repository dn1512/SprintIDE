#include "python_external_library_provider.h"

#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QtCore/qprocess.h>

namespace SprintIDE {

    QList<ExternalLibraryNode> PythonExternalLibraryProvider::collect(const QString& projectPath)
    {
        QList<ExternalLibraryNode> roots;

        const QString sp = findSitePackages(projectPath);
        if (sp.isEmpty()) return roots;

        ExternalLibraryNode root;
        root.kind = ExternalLibraryNode::Kind::Root;
        // v8.5 修复：QFileInfo 没有 parent()，用 dir().dirName() 取父目录名
        //   sp = ".../.venv/lib/python3.11/site-packages"
        //   QFileInfo(sp).dir() → QDir(".../.venv/lib/python3.11")
        //   .dirName()          → "python3.11"
        root.displayName = QFileInfo(sp).dir().dirName()
                           + QStringLiteral(" (site-packages)");
        root.absolutePath = sp;

        QDir dir(sp);
        const auto entries = dir.entryInfoList(
            QDir::Dirs | QDir::NoDotAndDotDot,
            QDir::Name | QDir::IgnoreCase);

        for (const QFileInfo& fi : entries) {
            const QString name = fi.fileName();
            // 过滤非库目录
            if (name.endsWith(".dist-info") || name.endsWith(".egg-info")
                || name == "__pycache__" || name.startsWith('.')) {
                continue;
            }

            ExternalLibraryNode pkg;
            pkg.kind = ExternalLibraryNode::Kind::Package;
            pkg.displayName = name;
            pkg.absolutePath = fi.absoluteFilePath();
            root.children.append(pkg);
        }

        if (!root.children.isEmpty()) {
            roots.append(root);
        }
        return roots;
    }

    QString PythonExternalLibraryProvider::findSitePackages(const QString& projectPath) const
    {
        // ---- 1. 项目内 venv（优先） ----
        const QStringList venvCandidates = {
            projectPath + "/.venv",
            projectPath + "/venv",
            projectPath + "/env",
            projectPath + "/.env",
        };

        for (const QString& base : venvCandidates) {
            QDir lib(base + "/lib");
            if (!lib.exists()) continue;
            const auto subdirs = lib.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QString& sub : subdirs) {
                if (!sub.startsWith("python")) continue;
                const QString sp = lib.absoluteFilePath(sub + "/site-packages");
                if (QDir(sp).exists()) {
                    qDebug() << "[PythonExtLib] venv site-packages:" << sp;
                    return sp;
                }
            }
        }

        // ---- 2. Fallback：系统 Python 的 site-packages ----
        //   v8.5：项目没 venv 时（比如一组裸 .py 文件），显示系统装的包
        //   通过 python3 自己报路径，比硬编码更准
        {
            QProcess proc;
            proc.start("python3", {"-c",
                    "import site; print(site.getsitepackages()[0])"});
            if (proc.waitForFinished(3000) && proc.exitCode() == 0) {
                const QString sp = QString::fromLocal8Bit(
                    proc.readAllStandardOutput()).trimmed();
                if (!sp.isEmpty() && QDir(sp).exists()) {
                    qDebug() << "[PythonExtLib] 系统 site-packages:" << sp;
                    return sp;
                }
            }
        }

        // ---- 3. macOS 特殊路径兜底（python3 命令不可用时） ----
        #ifdef Q_OS_MAC
        for (const QString& ver : {"3.13", "3.12", "3.11", "3.10", "3.9", "3.8"}) {
            const QStringList candidates = {
                QString("/Library/Frameworks/Python.framework/Versions/%1/lib/python%1/site-packages").arg(ver),
                QString("/opt/homebrew/lib/python%1/site-packages").arg(ver),
                QString("/usr/local/lib/python%1/site-packages").arg(ver),
            };
            for (const QString& sp : candidates) {
                if (QDir(sp).exists()) {
                    qDebug() << "[PythonExtLib] 系统 site-packages:" << sp;
                    return sp;
                }
            }
        }
        #endif

        qDebug() << "[PythonExtLib] 未找到任何 site-packages，projectPath=" << projectPath;
        return QString();
    }

} // namespace SprintIDE