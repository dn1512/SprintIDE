#include "rust_project_model.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>

namespace SprintIDE {

    RustProjectModel::RustProjectModel(const QString& projectPath)
    : m_projectPath(QDir(projectPath).absolutePath())
    {
    }

    QString RustProjectModel::cargoTomlPath() const
    {
        return m_projectPath + "/Cargo.toml";
    }

    QString RustProjectModel::workspaceRoot() const
    {
        return m_workspaceRoot.isEmpty() ? m_projectPath : m_workspaceRoot;
    }

    bool RustProjectModel::load()
    {
        m_valid = false;
        m_targets.clear();
        m_packageName.clear();
        m_workspaceRoot.clear();

        if (!QFile::exists(cargoTomlPath())) {
            // 可能是 workspace 成员，向上找 Cargo.toml
            QDir dir(m_projectPath);
            while (!dir.isRoot()) {
                if (dir.exists("Cargo.toml")) {
                    m_workspaceRoot = dir.absolutePath();
                    break;
                }
                if (!dir.cdUp()) break;
            }
            if (m_workspaceRoot.isEmpty()) return false;
        }

        if (!parseCargoToml()) return false;
        if (!detectTargets())  return false;

        m_valid = true;
        return true;
    }

    bool RustProjectModel::parseCargoToml()
    {
        QFile f(cargoTomlPath());
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

        QTextStream ts(&f);
        ts.setEncoding(QStringConverter::Utf8);
        const QString content = ts.readAll();
        f.close();

        // 简单解析：找 [package] 段里的 name
        // 生产环境可换 toml++，但这里为不引入新依赖用轻量解析
        static const QRegularExpression pkgRe(
        R"re(\[package\][^\[]*?^name\s*=\s*"([^"]+)")re",
        QRegularExpression::MultilineOption | QRegularExpression::DotMatchesEverythingOption);
        auto m = pkgRe.match(content);
        if (m.hasMatch()) {
            m_packageName = m.captured(1);
        }

        return !m_packageName.isEmpty();
    }

    bool RustProjectModel::detectTargets()
    {
        QFile f(cargoTomlPath());
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
        QTextStream ts(&f);
        ts.setEncoding(QStringConverter::Utf8);
        const QString content = ts.readAll();
        f.close();

        QDir root(m_projectPath);

        // 1. 显式 [[bin]] 段
        static const QRegularExpression binRe(
            R"(\[\[bin\]\]([^\[]*))",
            QRegularExpression::MultilineOption | QRegularExpression::DotMatchesEverythingOption);

        auto it = binRe.globalMatch(content);
        while (it.hasNext()) {
            const QString block = it.next().captured(1);
            RustTarget t;
            t.kind = "bin";

        auto nameMatch = QRegularExpression(R"re(name\s*=\s*"([^"]+)")re").match(block);
        if (nameMatch.hasMatch()) t.name = nameMatch.captured(1);

        auto pathMatch = QRegularExpression(R"re(path\s*=\s*"([^"]+)")re").match(block);
                if (pathMatch.hasMatch()) t.path = pathMatch.captured(1);

                if (t.name.isEmpty()) continue;

                // 如果没写 path，默认 src/bin/<name>.rs 或 src/main.rs
                if (t.path.isEmpty()) {
                    if (QFile::exists(root.filePath("src/bin/" + t.name + ".rs")))
                    t.path = "src/bin/" + t.name + ".rs";
                    else if (t.name == m_packageName && QFile::exists(root.filePath("src/main.rs")))
                    t.path = "src/main.rs";
                }

                // 绝对化 path 用于 targetForSource 比对
                if (!t.path.isEmpty())
                t.path = QFileInfo(root.filePath(t.path)).absoluteFilePath();

                m_targets.append(t);
            }

            // 2. 隐式默认 bin：src/main.rs
            if (QFile::exists(root.filePath("src/main.rs"))) {
                bool already = false;
                for (const auto& t : m_targets) {
                    if (t.name == m_packageName) { already = true; break; }
                }
                if (!already) {
                    RustTarget t;
                    t.name = m_packageName;       // cargo 约定：默认 bin 名 = 包名
                    t.kind = "bin";
                    t.path = QFileInfo(root.filePath("src/main.rs")).absoluteFilePath();
                    t.isDefault = true;
                    m_targets.prepend(t);         // 默认 bin 放最前
                }
            }

            // 3. src/bin/*.rs 下的自动 bin
            QDir binDir(root.filePath("src/bin"));
            if (binDir.exists()) {
                for (const QString& fn : binDir.entryList({"*.rs"}, QDir::Files)) {
                    const QString name = QFileInfo(fn).completeBaseName();
                    bool already = false;
                    for (const auto& t : m_targets)
                    if (t.name == name) { already = true; break; }
                    if (already) continue;

                    RustTarget t;
                    t.name = name;
                    t.kind = "bin";
                    t.path = QFileInfo(binDir.filePath(fn)).absoluteFilePath();
                    m_targets.append(t);
                }
            }

            return !m_targets.isEmpty();
        }

        RustTarget RustProjectModel::defaultBinary() const
        {
            for (const auto& t : m_targets)
            if (t.isDefault) return t;
            for (const auto& t : m_targets)
            if (t.kind == "bin" && QFileInfo(t.path).fileName() == "main.rs") return t;
            return m_targets.isEmpty() ? RustTarget{} : m_targets.first();
        }

        QStringList RustProjectModel::binaryNames() const
        {
            QStringList out;
            for (const auto& t : m_targets)
            if (t.kind == "bin") out << t.name;
            return out;
        }

        RustTarget RustProjectModel::targetForSource(const QString& sourceFile) const
        {
            const QString abs = QFileInfo(sourceFile).absoluteFilePath();
            for (const auto& t : m_targets)
            if (QFileInfo(t.path).absoluteFilePath() == abs) return t;

            // 兜底：包名同名 target
            RustTarget def = defaultBinary();
            if (!def.name.isEmpty()) return def;

            RustTarget fallback;
            fallback.name = m_packageName;
            fallback.kind = "bin";
            return fallback;
        }

        QString RustProjectModel::executablePath(const QString& targetName, bool release) const
        {
            const QString root = workspaceRoot();
            return QDir(root).filePath(
                QString("target/%1/%2").arg(release ? "release" : "debug", targetName));
        }

    } // namespace SprintIDE