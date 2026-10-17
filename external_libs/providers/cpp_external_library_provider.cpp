#include "cpp_external_library_provider.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QDebug>

namespace SprintIDE {

    QList<ExternalLibraryNode> CppExternalLibraryProvider::collect(const QString& projectPath)
    {
        QList<ExternalLibraryNode> roots;

        // ---------- 1. compile_commands.json 的 -I 路径 ----------
        QSet<QString> userIncludes;
        QString ccPath = projectPath + "/compile_commands.json";
        if (!QFile::exists(ccPath)) {
            ccPath = projectPath + "/build/compile_commands.json";
        }
        if (QFile::exists(ccPath)) {
            parseCompileCommands(ccPath, userIncludes);
        }

        // ---------- 2. Qt 头文件 ----------
        const QString qtDir = findQtDir(projectPath);
        if (!qtDir.isEmpty() && QDir(qtDir + "/include").exists()) {
            ExternalLibraryNode qtRoot;
            qtRoot.kind = ExternalLibraryNode::Kind::Root;
            qtRoot.displayName = QStringLiteral("Qt (%1)").arg(QFileInfo(qtDir).fileName());
            qtRoot.absolutePath = qtDir + "/include";

            QDir qtInc(qtDir + "/include");
            const auto subs = qtInc.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
            for (const QString& sub : subs) {
                if (!sub.startsWith("Qt")) continue;
                ExternalLibraryNode g;
                g.kind = ExternalLibraryNode::Kind::Group;
                g.displayName = sub;
                g.absolutePath = qtInc.absoluteFilePath(sub);
                qtRoot.children.append(g);
            }
            if (!qtRoot.children.isEmpty()) roots.append(qtRoot);
        }

        // ---------- 3. 用户 include 路径（来自 -I） ----------
        if (!userIncludes.isEmpty()) {
            ExternalLibraryNode incRoot;
            incRoot.kind = ExternalLibraryNode::Kind::Root;
            incRoot.displayName = QStringLiteral("包含路径");

            QStringList sorted = userIncludes.values();
            sorted.sort(Qt::CaseInsensitive);

            for (const QString& dir : sorted) {
                if (!QDir(dir).exists()) continue;
                ExternalLibraryNode g;
                g.kind = ExternalLibraryNode::Kind::Group;
                g.displayName = dir;
                g.absolutePath = dir;
                incRoot.children.append(g);
            }
            if (!incRoot.children.isEmpty()) roots.append(incRoot);
        }

        // ---------- 4. 系统头文件 ----------
        ExternalLibraryNode sysRoot;
        sysRoot.kind = ExternalLibraryNode::Kind::Root;
        sysRoot.displayName = QStringLiteral("系统头文件");

        const QStringList systemDirs = {
            #if defined(Q_OS_MAC)
            "/Library/Developer/CommandLineTools/usr/include",
            "/usr/local/include",
            "/opt/homebrew/include",
            #elif defined(Q_OS_WIN)
            "C:/MinGW/include",
            "C:/msys64/mingw64/include",
            #else
            "/usr/include",
            "/usr/local/include",
            #endif
        };
        for (const QString& dir : systemDirs) {
            if (!QDir(dir).exists()) continue;
            ExternalLibraryNode g;
            g.kind = ExternalLibraryNode::Kind::Group;
            g.displayName = dir;
            g.absolutePath = dir;
            sysRoot.children.append(g);
        }
        if (!sysRoot.children.isEmpty()) roots.append(sysRoot);

        return roots;
    }

    void CppExternalLibraryProvider::parseCompileCommands(const QString& ccPath,
        QSet<QString>& out) const
    {
        QFile f(ccPath);
        if (!f.open(QIODevice::ReadOnly)) return;

        const QByteArray raw = f.readAll();
        f.close();

        const QString content = QString::fromUtf8(raw);
        // 简单扫：-I <path> 或 -I"path" 或 -ipath
        static const QRegularExpression re(R"(-I\s*("[^"]+"|[^\s"]+))");
                auto it = re.globalMatch(content);
                const QString ccDir = QFileInfo(ccPath).absolutePath();

                while (it.hasNext()) {
                    QString inc = it.next().captured(1);
                    if (inc.startsWith('"') && inc.endsWith('"')) {
                        inc = inc.mid(1, inc.length() - 2);
                    }
                    if (inc.isEmpty()) continue;

                    if (!QFileInfo(inc).isAbsolute()) {
                        inc = QDir(ccDir).absoluteFilePath(inc);
                    }
                    out.insert(QDir::cleanPath(inc));
                }

                qDebug() << "[CppExtLib] 从 compile_commands.json 提取" << out.size() << "个 -I 路径";
            }

            QString CppExternalLibraryProvider::findQtDir(const QString& projectPath) const
            {
                // 1. 环境变量
                QString qtDir = qEnvironmentVariable("QT_DIR");
                if (!qtDir.isEmpty() && QDir(qtDir + "/include").exists()) {
                    return qtDir;
                }

                // 2. CMakeCache.txt
                const QStringList cacheFiles = {
                    projectPath + "/build/CMakeCache.txt",
                    projectPath + "/CMakeCache.txt",
                };
                for (const QString& cachePath : cacheFiles) {
                    QFile f(cachePath);
                    if (!f.open(QIODevice::ReadOnly)) continue;
                    while (!f.atEnd()) {
                        const QString line = QString::fromUtf8(f.readLine()).trimmed();
                        // CMAKE_PREFIX_PATH:UNINITIALIZED=/path/to/Qt/6.x/macos
                        if (line.startsWith("CMAKE_PREFIX_PATH:")) {
                            const int eq = line.indexOf('=');
                            if (eq > 0) {
                                QString path = line.mid(eq + 1).trimmed();
                                if (QDir(path + "/include").exists()) {
                                    return path;
                                }
                            }
                        }
                    }
                }
                return QString();
            }

        } // namespace SprintIDE