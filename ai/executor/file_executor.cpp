#include "ai/executor/file_executor.h"
#include "ai/protocol/protocol_types.h"
#include "project_manager.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QJsonArray>

namespace SprintIDE {
namespace Protocol {

FileExecutor::FileExecutor(QObject* parent)
    : QObject(parent)
{
}

QString FileExecutor::resolvePath(const QString& rawPath) const
{
    if (rawPath.isEmpty()) return rawPath;

    const QFileInfo fi(rawPath);
    if (fi.isAbsolute()) {
        return QDir::cleanPath(rawPath);
    }

    ProjectManager* pm = ProjectManager::instance();
    const QString root = pm ? pm->currentProjectPath() : QString();

    if (!root.isEmpty()) {
        return QDir::cleanPath(QDir(root).absoluteFilePath(rawPath));
    }
    return QDir::cleanPath(QDir::current().absoluteFilePath(rawPath));
}

ExecutionResult FileExecutor::execute(const QJsonObject& item)
{
    const QString type = item.value("type").toString();

    if (type == QStringLiteral("file.read"))   return handleRead(item);
    if (type == QStringLiteral("file.create")) return handleCreate(item);
    if (type == QStringLiteral("file.delete")) return handleDelete(item);
    if (type == QStringLiteral("file.rename")) return handleRename(item);
    if (type == QStringLiteral("file.edit"))   return handleEdit(item);

    return ExecutionResult::fail(
        item.value("id").toString(), type,
        ErrorCode::UnsupportedType,
        QStringLiteral("FileExecutor cannot handle type: %1").arg(type));
}

// ------------------------------------------------------------
// file.read
// ------------------------------------------------------------
ExecutionResult FileExecutor::handleRead(const QJsonObject& item)
{
    const QString itemId  = item.value("id").toString();
    const QString rawPath = item.value("path").toString();
    const QString absPath = resolvePath(rawPath);

    if (!QFile::exists(absPath)) {
        return ExecutionResult::fail(itemId, QStringLiteral("file.read"),
                                     ErrorCode::PathNotFound,
                                     QStringLiteral("File not found: %1").arg(rawPath));
    }

    QFile f(absPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return ExecutionResult::fail(itemId, QStringLiteral("file.read"),
                                     ErrorCode::PermissionDenied,
                                     f.errorString());
    }
    const QString fullContent = QString::fromUtf8(f.readAll());
    f.close();

    const QStringList allLines = fullContent.split(QLatin1Char('\n'));
    const int totalLines = allLines.size();

    const int startLine = item.value("startLine").toInt(0);
    const int endLine   = item.value("endLine").toInt(0);

    QString content = fullContent;
    if (startLine > 0 || endLine > 0) {
        int s = (startLine > 0) ? startLine - 1 : 0;
        int e = (endLine   > 0) ? endLine       : totalLines;
        s = qMax(0, qMin(s, totalLines));
        e = qMax(0, qMin(e, totalLines));
        content = (s < e) ? allLines.mid(s, e - s).join(QLatin1Char('\n'))
                          : QString();
    }

    QJsonObject data;
    data["path"]        = rawPath;
    data["content"]     = content;
    data["truncated"]   = false;
    data["totalLines"]  = totalLines;

    return ExecutionResult::ok(itemId, QStringLiteral("file.read"), data);
}

// ------------------------------------------------------------
// file.create
// ------------------------------------------------------------
ExecutionResult FileExecutor::handleCreate(const QJsonObject& item)
{
    const QString itemId    = item.value("id").toString();
    const QString rawPath   = item.value("path").toString();
    const QString content   = item.value("content").toString();
    const bool    overwrite = item.value("overwriteIfExists").toBool(false);
    const QString absPath   = resolvePath(rawPath);

    if (QFile::exists(absPath) && !overwrite) {
        return ExecutionResult::fail(itemId, QStringLiteral("file.create"),
                                     ErrorCode::PathExists,
                                     QStringLiteral("File already exists: %1").arg(rawPath));
    }

    const QFileInfo fi(absPath);
    QDir dir = fi.dir();
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        return ExecutionResult::fail(itemId, QStringLiteral("file.create"),
                                     ErrorCode::PermissionDenied,
                                     QStringLiteral("Cannot create directory: %1")
                                         .arg(dir.absolutePath()));
    }

    QFile f(absPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return ExecutionResult::fail(itemId, QStringLiteral("file.create"),
                                     ErrorCode::PermissionDenied,
                                     f.errorString());
    }
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << content;
    f.close();

    QJsonObject data;
    data["path"]     = rawPath;
    data["created"]  = true;
    return ExecutionResult::ok(itemId, QStringLiteral("file.create"), data);
}

// ------------------------------------------------------------
// file.delete
// ------------------------------------------------------------
ExecutionResult FileExecutor::handleDelete(const QJsonObject& item)
{
    const QString itemId  = item.value("id").toString();
    const QString rawPath = item.value("path").toString();
    const QString absPath = resolvePath(rawPath);

    if (!QFile::exists(absPath)) {
        return ExecutionResult::fail(itemId, QStringLiteral("file.delete"),
                                     ErrorCode::PathNotFound,
                                     QStringLiteral("File not found: %1").arg(rawPath));
    }

    if (!QFile::remove(absPath)) {
        return ExecutionResult::fail(itemId, QStringLiteral("file.delete"),
                                     ErrorCode::PermissionDenied,
                                     QStringLiteral("Failed to delete: %1").arg(rawPath));
    }

    QJsonObject data;
    data["path"]    = rawPath;
    data["deleted"] = true;
    data["toTrash"] = false;   // 阶段 1：直接删除，不走回收站
    return ExecutionResult::ok(itemId, QStringLiteral("file.delete"), data);
}

// ------------------------------------------------------------
// file.rename
// ------------------------------------------------------------
ExecutionResult FileExecutor::handleRename(const QJsonObject& item)
{
    const QString itemId  = item.value("id").toString();
    const QString oldRaw  = item.value("oldPath").toString();
    const QString newRaw  = item.value("newPath").toString();
    const QString oldAbs  = resolvePath(oldRaw);
    const QString newAbs  = resolvePath(newRaw);

    if (!QFile::exists(oldAbs)) {
        return ExecutionResult::fail(itemId, QStringLiteral("file.rename"),
                                     ErrorCode::PathNotFound,
                                     QStringLiteral("Source not found: %1").arg(oldRaw));
    }
    if (QFile::exists(newAbs)) {
        return ExecutionResult::fail(itemId, QStringLiteral("file.rename"),
                                     ErrorCode::PathExists,
                                     QStringLiteral("Target already exists: %1").arg(newRaw));
    }

    const QFileInfo newFi(newAbs);
    QDir dir = newFi.dir();
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        return ExecutionResult::fail(itemId, QStringLiteral("file.rename"),
                                     ErrorCode::PermissionDenied,
                                     QStringLiteral("Cannot create directory: %1")
                                         .arg(dir.absolutePath()));
    }

    if (!QFile::rename(oldAbs, newAbs)) {
        return ExecutionResult::fail(itemId, QStringLiteral("file.rename"),
                                     ErrorCode::PermissionDenied,
                                     QStringLiteral("Failed to rename"));
    }

    QJsonObject data;
    data["oldPath"] = oldRaw;
    data["newPath"] = newRaw;
    return ExecutionResult::ok(itemId, QStringLiteral("file.rename"), data);
}

// ------------------------------------------------------------
// file.edit（骨架，阶段 3 补）
// ------------------------------------------------------------
ExecutionResult FileExecutor::handleEdit(const QJsonObject& item)
{
    return ExecutionResult::fail(
        item.value("id").toString(), QStringLiteral("file.edit"),
        ErrorCode::UnsupportedType,
        QStringLiteral("file.edit not yet implemented (planned for Phase 3)"));
}

} // namespace Protocol
} // namespace SprintIDE
