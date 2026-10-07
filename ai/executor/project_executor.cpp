#include "ai/executor/project_executor.h"
#include "ai/protocol/protocol_types.h"
#include "project_manager.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QStringConverter>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>

namespace SprintIDE {
namespace Protocol {

ProjectExecutor::ProjectExecutor(QObject* parent)
    : QObject(parent)
{
}

ExecutionResult ProjectExecutor::execute(const QJsonObject& item)
{
    const QString type = item.value("type").toString();

    if (type == QStringLiteral("project.info"))    return handleInfo(item);
    if (type == QStringLiteral("project.targets")) return handleTargets(item);

    return ExecutionResult::fail(
        item.value("id").toString(), type,
        ErrorCode::UnsupportedType,
        QStringLiteral("ProjectExecutor cannot handle type: %1").arg(type));
}

// ------------------------------------------------------------
// 基本信息
// ------------------------------------------------------------
QJsonObject ProjectExecutor::collectBasicInfo() const
{
    QJsonObject result;

    ProjectManager* pm = ProjectManager::instance();
    if (!pm) return result;

    const QString root = pm->currentProjectPath();
    if (root.isEmpty()) return result;

    // ---- projectId：路径 SHA1 前 16 个 hex 字符 ----
    const QByteArray hash = QCryptographicHash::hash(
        root.toUtf8(), QCryptographicHash::Sha1);
    const QString projectId = QString::fromLatin1(hash.toHex().left(16));

    result["projectId"]   = projectId;
    result["projectName"] = pm->currentProjectName();
    result["projectRoot"] = root;

    // ---- language + buildSystem ----
    QDir dir(root);
    QString language;
    QString buildSystem;

    if (dir.exists(QStringLiteral("CMakeLists.txt"))) {
        language    = QStringLiteral("cpp");
        buildSystem = QStringLiteral("cmake");
    } else if (dir.exists(QStringLiteral("Makefile")) ||
               dir.exists(QStringLiteral("makefile")) ||
               dir.exists(QStringLiteral("GNUmakefile"))) {
        language    = QStringLiteral("cpp");
        buildSystem = QStringLiteral("makefile");
    } else if (dir.exists(QStringLiteral("pyproject.toml"))) {
        language    = QStringLiteral("python");
        buildSystem = QStringLiteral("pyproject");
    } else if (dir.exists(QStringLiteral("requirements.txt")) ||
               dir.exists(QStringLiteral("setup.py"))) {
        language    = QStringLiteral("python");
        buildSystem = QStringLiteral("setuptools");
    } else if (dir.exists(QStringLiteral("pom.xml"))) {
        language    = QStringLiteral("java");
        buildSystem = QStringLiteral("maven");
    } else if (dir.exists(QStringLiteral("build.gradle")) ||
               dir.exists(QStringLiteral("build.gradle.kts"))) {
        language    = QStringLiteral("java");
        buildSystem = QStringLiteral("gradle");
    } else if (dir.exists(QStringLiteral("Cargo.toml"))) {
        language    = QStringLiteral("rust");
        buildSystem = QStringLiteral("cargo");
    } else if (dir.exists(QStringLiteral("go.mod"))) {
        language    = QStringLiteral("go");
        buildSystem = QStringLiteral("go");
    } else if (dir.exists(QStringLiteral("package.json"))) {
        language    = QStringLiteral("javascript");
        buildSystem = QStringLiteral("npm");
    } else {
        language    = QStringLiteral("unknown");
        buildSystem = QStringLiteral("unknown");
    }

    result["language"]    = language;
    result["buildSystem"] = buildSystem;

    // ---- defaultTarget / defaultEntry：第一版留空 ----
    result["defaultTarget"] = QString();
    result["defaultEntry"]  = QString();

    return result;
}

// ------------------------------------------------------------
// targets（第一版仅 CMake）
// ------------------------------------------------------------
QJsonArray ProjectExecutor::collectTargets() const
{
    QJsonArray targets;

    ProjectManager* pm = ProjectManager::instance();
    if (!pm) return targets;

    const QString root = pm->currentProjectPath();
    if (root.isEmpty()) return targets;

    const QString cmakePath = QDir(root).absoluteFilePath(
        QStringLiteral("CMakeLists.txt"));
    if (!QFile::exists(cmakePath)) return targets;

    QFile f(cmakePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return targets;

    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);
    const QString content = in.readAll();
    f.close();

    // 匹配 add_executable(name ...) 和 add_library(name ...)
    // 只抓以字母/下划线开头的标识符（忽略 ${VAR} 形式）
    static const QRegularExpression reExe(
        QStringLiteral(R"(add_executable\s*\(\s*([A-Za-z_][A-Za-z0-9_]*))"));
    static const QRegularExpression reLib(
        QStringLiteral(R"(add_library\s*\(\s*([A-Za-z_][A-Za-z0-9_]*))"));

    auto grab = [&](const QRegularExpression& re, const QString& type) {
        auto it = re.globalMatch(content);
        while (it.hasNext()) {
            const auto m = it.next();
            const QString name = m.captured(1);

            QJsonObject t;
            t["name"] = name;
            t["type"] = type;
            t["path"] = QString();
            targets.append(t);
        }
    };

    grab(reExe, QStringLiteral("executable"));
    grab(reLib, QStringLiteral("library"));

    return targets;
}

// ------------------------------------------------------------
// project.info
// ------------------------------------------------------------
ExecutionResult ProjectExecutor::handleInfo(const QJsonObject& item)
{
    const QString itemId = item.value("id").toString();

    QJsonObject result = collectBasicInfo();
    if (result.isEmpty()) {
        return ExecutionResult::fail(
            itemId, QStringLiteral("project.info"),
            ErrorCode::PathNotFound,
            QStringLiteral("No project is currently open"));
    }

    result["targets"] = collectTargets();

    return ExecutionResult::ok(itemId, QStringLiteral("project.info"), result);
}

// ------------------------------------------------------------
// project.targets
// ------------------------------------------------------------
ExecutionResult ProjectExecutor::handleTargets(const QJsonObject& item)
{
    const QString itemId = item.value("id").toString();

    QJsonObject result;
    result["targets"] = collectTargets();

    return ExecutionResult::ok(itemId, QStringLiteral("project.targets"), result);
}

} // namespace Protocol
} // namespace SprintIDE
