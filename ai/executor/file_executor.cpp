#include "ai/executor/file_executor.h"
#include "ai/protocol/protocol_types.h"
#include "project_manager.h"
#include "mainwindow.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QJsonArray>

namespace SprintIDE {
namespace Protocol {

FileExecutor::FileExecutor(MainWindow* mw, QObject* parent)
    : QObject(parent)
    , m_mw(mw)
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
// file.edit
// ------------------------------------------------------------
ExecutionResult FileExecutor::handleEdit(const QJsonObject& item)
{
    const QString itemId  = item.value("id").toString();
    const QString rawPath = item.value("path").toString();
    const QString mode    = item.value("mode").toString();
    const QString absPath = resolvePath(rawPath);

    // ---- 0. 基础校验 ----
    if (rawPath.isEmpty() || mode.isEmpty()) {
        return ExecutionResult::fail(itemId, QStringLiteral("file.edit"),
                                     ErrorCode::InvalidFields,
                                     QStringLiteral("Missing 'path' or 'mode'"));
    }

    if (!QFile::exists(absPath)) {
        return ExecutionResult::fail(itemId, QStringLiteral("file.edit"),
                                     ErrorCode::PathNotFound,
                                     QStringLiteral("File not found: %1").arg(rawPath));
    }

    if (!m_mw) {
        return ExecutionResult::fail(itemId, QStringLiteral("file.edit"),
                                     ErrorCode::InternalError,
                                     QStringLiteral("MainWindow is null"));
    }

    // ---- 1. 找编辑器，若未打开则尝试打开 ----
    TextEditor* editor = m_mw->findEditorByFilePath(absPath);
    if (!editor) {
        // 尝试打开
        m_mw->openFile(absPath);
        editor = m_mw->findEditorByFilePath(absPath);
    }
    if (!editor) {
        return ExecutionResult::fail(itemId, QStringLiteral("file.edit"),
                                     ErrorCode::InternalError,
                                     QStringLiteral("Failed to open or find editor: %1").arg(rawPath));
    }

    // ---- 2. 分 mode ----
    const int origLineCount = editor->lineCount();

    if (mode == QStringLiteral("replaceWhole")) {
        if (!item.contains("newContent")) {
            return ExecutionResult::fail(itemId, QStringLiteral("file.edit"),
                                         ErrorCode::InvalidMode,
                                         QStringLiteral("replaceWhole requires 'newContent'"));
        }
        const QString newContent = item.value("newContent").toString();

        // 用 QTextCursor 走 undo 栈
        QTextCursor cursor(editor->document());
        cursor.select(QTextCursor::Document);
        cursor.beginEditBlock();
        cursor.removeSelectedText();
        cursor.insertText(newContent);
        cursor.endEditBlock();

    } else if (mode == QStringLiteral("insertAt")) {
        if (!item.contains("insertLine") || !item.contains("newContent")) {
            return ExecutionResult::fail(itemId, QStringLiteral("file.edit"),
                                         ErrorCode::InvalidMode,
                                         QStringLiteral("insertAt requires 'insertLine' and 'newContent'"));
        }
        int insertLine = item.value("insertLine").toInt(0);
        const QString content = item.value("newContent").toString();

        const int totalLines = editor->lineCount();
        // 负数从末尾倒数
        if (insertLine < 0) {
            insertLine = totalLines + insertLine + 1;
        }
        if (insertLine < 1 || insertLine > totalLines + 1) {
            return ExecutionResult::fail(itemId, QStringLiteral("file.edit"),
                                         ErrorCode::InvalidFields,
                                         QStringLiteral("insertLine out of range: %1").arg(insertLine));
        }

        QTextBlock block = editor->document()->findBlockByNumber(insertLine - 1);
        QTextCursor cursor(block);
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.beginEditBlock();
        // 保证插入内容后面带换行（除非内容已经以 \n 结尾）
        QString toInsert = content;
        if (!toInsert.endsWith('\n')) toInsert += '\n';
        cursor.insertText(toInsert);
        cursor.endEditBlock();

    } else if (mode == QStringLiteral("searchReplace")) {
        if (!item.contains("replacements") || !item.value("replacements").isArray()) {
            return ExecutionResult::fail(itemId, QStringLiteral("file.edit"),
                                         ErrorCode::InvalidMode,
                                         QStringLiteral("searchReplace requires 'replacements' array"));
        }

        const QJsonArray reps = item.value("replacements").toArray();
        if (reps.isEmpty()) {
            return ExecutionResult::fail(itemId, QStringLiteral("file.edit"),
                                         ErrorCode::InvalidFields,
                                         QStringLiteral("Empty replacements array"));
        }

        int appliedCount = 0;

        for (const QJsonValue& v : reps) {
            const QJsonObject rep = v.toObject();
            const QString search  = rep.value("search").toString();
            const QString replace = rep.value("replace").toString();
            const QString scopeMethod = rep.value("scopeMethod").toString();

            // ---- 硬约束 1：search 为空 ----
            if (search.isEmpty()) {
                return ExecutionResult::fail(itemId, QStringLiteral("file.edit"),
                                             ErrorCode::InvalidFields,
                                             QStringLiteral("Empty 'search' string"));
            }
            // ---- 硬约束 2：search == replace → 跳过（无操作，视为成功） ----
            if (search == replace) {
                continue;
            }

            // ---- 确定搜索范围 ----
            int rangeStart = -1;
            int rangeEnd   = -1;

            if (!scopeMethod.isEmpty()) {
                const auto ranges = editor->findMethodRanges(scopeMethod);
                if (ranges.isEmpty()) {
                    return ExecutionResult::fail(
                        itemId, QStringLiteral("file.edit"),
                        ErrorCode::MethodNotFound,
                        QStringLiteral("Method not found: %1").arg(scopeMethod));
                }
                if (ranges.size() > 1) {
                    return ExecutionResult::fail(
                        itemId, QStringLiteral("file.edit"),
                        ErrorCode::MethodAmbiguous,
                        QStringLiteral("Multiple methods match '%1'").arg(scopeMethod));
                }
                // 用方法体范围作为搜索范围
                const auto& r = ranges.first();
                QTextBlock startBlk = editor->document()->findBlockByNumber(r.bodyStartLine - 1);
                QTextBlock endBlk   = editor->document()->findBlockByNumber(r.bodyEndLine - 1);
                rangeStart = startBlk.position();
                rangeEnd   = endBlk.position() + endBlk.length() - 1;
            }

            // ---- 查找 ----
            const auto matches = editor->findByNormalizedWhitespace(search, rangeStart, rangeEnd);

            if (matches.isEmpty()) {
                return ExecutionResult::fail(itemId, QStringLiteral("file.edit"),
                                             ErrorCode::SearchNotFound,
                                             QStringLiteral("Search text not found"));
            }
            if (matches.size() > 1) {
                QJsonArray locations;
                for (const QTextCursor& c : matches) {
                    QJsonObject loc;
                    loc["line"] = c.block().blockNumber() + 1;
                    locations.append(loc);
                }
                QJsonObject data;
                data["matchLocations"] = locations;
                data["suggestions"] = QJsonArray::fromStringList({
                    QStringLiteral("Use 'scopeMethod' to limit search to a specific method"),
                    QStringLiteral("Use 'replaceWhole' with the full method code"),
                    QStringLiteral("Include more surrounding lines in 'search'")
                });
                return ExecutionResult::fail(itemId, QStringLiteral("file.edit"),
                                             ErrorCode::AmbiguousSearch,
                                             QStringLiteral("Found %1 matches").arg(matches.size()),
                                             data);
            }

            // ---- 唯一匹配 → 替换 ----
            QTextCursor cursor = matches.first();
            editor->replaceWithIndentAlignment(cursor, replace);
            ++appliedCount;
        }

        Q_UNUSED(appliedCount);

    } else {
        return ExecutionResult::fail(itemId, QStringLiteral("file.edit"),
                                     ErrorCode::InvalidMode,
                                     QStringLiteral("Unknown mode: %1").arg(mode));
    }

    // ---- 3. 自动保存（走 LocalHistory，"ai" 来源） ----
    editor->saveFile(QStringLiteral("ai"));

    // ---- 4. 构造回执 data ----
    const int newLineCount = editor->lineCount();

    QJsonObject data;
    data["path"]          = rawPath;
    data["mode"]          = mode;
    data["applied"]       = true;
    data["linesAdded"]    = qMax(0, newLineCount - origLineCount);
    data["linesRemoved"]  = qMax(0, origLineCount - newLineCount);
    // snapshotId 阶段 1 先占位（LocalHistory 版本号通过信号回传，暂不取）
    data["snapshotId"]    = QString();

    return ExecutionResult::ok(itemId, QStringLiteral("file.edit"), data);
}

} // namespace Protocol
} // namespace SprintIDE
