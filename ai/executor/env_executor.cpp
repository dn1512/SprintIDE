#include "ai/executor/env_executor.h"
#include "ai/protocol/protocol_types.h"
#include "project_manager.h"

#include <QProcess>
#include <QStandardPaths>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

namespace SprintIDE {
namespace Protocol {

EnvExecutor::EnvExecutor(QObject* parent)
    : QObject(parent)
{
}

ExecutionResult EnvExecutor::execute(const QJsonObject& item)
{
    const QString type = item.value("type").toString();

    if (type == QStringLiteral("env.query"))  return handleQuery(item);
    if (type == QStringLiteral("env.setSdk")) return handleSetSdk(item);

    return ExecutionResult::fail(
        item.value("id").toString(), type,
        ErrorCode::UnsupportedType,
        QStringLiteral("EnvExecutor cannot handle type: %1").arg(type));
}

// ------------------------------------------------------------
// 查询键 → 可执行文件名
// ------------------------------------------------------------
QString EnvExecutor::keyToExecutable(const QString& key) const
{
    // path 类和 version 类用同一个 exe
    if (key.startsWith(QStringLiteral("python."))) {
        if (key == QStringLiteral("python.pip.path"))    return QStringLiteral("pip3");
        if (key == QStringLiteral("python.pytest.path")) return QStringLiteral("pytest");
        return QStringLiteral("python3");
    }
    if (key.startsWith(QStringLiteral("cmake.")))  return QStringLiteral("cmake");
    if (key.startsWith(QStringLiteral("gdb.")))    return QStringLiteral("gdb");
    if (key == QStringLiteral("lldb.path"))        return QStringLiteral("lldb");
    if (key == QStringLiteral("ninja.path"))       return QStringLiteral("ninja");
    if (key == QStringLiteral("make.path"))        return QStringLiteral("make");
    if (key.startsWith(QStringLiteral("node.")))   return QStringLiteral("node");
    if (key == QStringLiteral("npm.path"))         return QStringLiteral("npm");
    if (key == QStringLiteral("rust.cargo.path"))  return QStringLiteral("cargo");
    if (key == QStringLiteral("rust.rustc.path"))  return QStringLiteral("rustc");
    if (key.startsWith(QStringLiteral("go.")))     return QStringLiteral("go");
    if (key.startsWith(QStringLiteral("git.")))    return QStringLiteral("git");
    if (key.startsWith(QStringLiteral("pandoc."))) return QStringLiteral("pandoc");
    return QString();
}

// ------------------------------------------------------------
// 执行 <exe> --version
// ------------------------------------------------------------
QString EnvExecutor::execVersion(const QString& exeName) const
{
    const QString path = QStandardPaths::findExecutable(exeName);
    if (path.isEmpty()) return QString();

    QStringList args;
    if (exeName == QStringLiteral("go")) {
        args << QStringLiteral("version");
    } else {
        args << QStringLiteral("--version");
    }

    QProcess proc;
    proc.start(path, args);
    if (!proc.waitForFinished(2000)) {
        proc.kill();
        proc.waitForFinished(500);
        return QString();
    }

    QString out = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
    if (out.isEmpty()) {
        out = QString::fromLocal8Bit(proc.readAllStandardError()).trimmed();
    }
    if (out.isEmpty()) return QString();

    // 取第一行，截断到 100 字符
    const int nl = out.indexOf('\n');
    if (nl > 0) out = out.left(nl);
    if (out.length() > 100) out = out.left(100);
    return out;
}

// ------------------------------------------------------------
// 单键查询
// ------------------------------------------------------------
QJsonValue EnvExecutor::lookupEnvKey(const QString& key) const
{
    if (key.isEmpty()) return QJsonValue(QJsonValue::Null);

    const QString exeName = keyToExecutable(key);
    if (exeName.isEmpty()) {
        return QJsonValue(QJsonValue::Null);
    }

    // path 类
    if (key.endsWith(QStringLiteral(".path"))) {
        const QString path = QStandardPaths::findExecutable(exeName);
        return path.isEmpty() ? QJsonValue(QJsonValue::Null)
                              : QJsonValue(path);
    }

    // version 类
    if (key.endsWith(QStringLiteral(".version"))) {
        const QString ver = execVersion(exeName);
        return ver.isEmpty() ? QJsonValue(QJsonValue::Null)
                             : QJsonValue(ver);
    }

    return QJsonValue(QJsonValue::Null);
}

// ------------------------------------------------------------
// env.query
// ------------------------------------------------------------
ExecutionResult EnvExecutor::handleQuery(const QJsonObject& item)
{
    const QString itemId = item.value("id").toString();
    const QJsonArray keys = item.value("items").toArray();

    QJsonObject results;
    for (const QJsonValue& v : keys) {
        const QString key = v.toObject().value("key").toString();
        if (!key.isEmpty()) {
            results[key] = lookupEnvKey(key);
        }
    }

    QJsonObject data;
    data["results"] = results;

    return ExecutionResult::ok(itemId, QStringLiteral("env.query"), data);
}

// ------------------------------------------------------------
// env.setSdk（骨架，阶段 4 或后续补）
// ------------------------------------------------------------
ExecutionResult EnvExecutor::handleSetSdk(const QJsonObject& item)
{
    return ExecutionResult::fail(
        item.value("id").toString(), QStringLiteral("env.setSdk"),
        ErrorCode::UnsupportedType,
        QStringLiteral("env.setSdk not yet implemented"));
}

} // namespace Protocol
} // namespace SprintIDE
