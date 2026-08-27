#include "path_utils.h"
#include <QDebug>
#include <QStandardPaths>
#include <QDirIterator>
#include <QRegularExpression>
#include <QCoreApplication>

namespace SprintIDE {

// ==================== 路径规范化 ====================

QString PathUtils::normalizePath(const QString& path)
{
    if (path.isEmpty()) {
        return path;
    }

    QString normalized = QDir::cleanPath(path);

    // 移除末尾的路径分隔符（除非是根目录）
    if (normalized.endsWith('/') && normalized != "/") {
        normalized.chop(1);
    }

#ifdef Q_OS_WIN
    // Windows上，确保盘符大写
    if (normalized.length() >= 2 && normalized[1] == ':') {
        normalized[0] = normalized[0].toUpper();
    }
#endif

    return normalized;
}

QString PathUtils::toNativeSeparators(const QString& path)
{
    return QDir::toNativeSeparators(path);
}

QString PathUtils::toUniversalSeparators(const QString& path)
{
    return QDir::fromNativeSeparators(path);
}

// ==================== 相对路径计算 ====================

QString PathUtils::relativePath(const QString& fromPath, const QString& toPath)
{
    QDir fromDir(fromPath);
    QString relative = fromDir.relativeFilePath(toPath);

    // 如果无法计算相对路径（例如跨盘符），返回绝对路径
    if (relative.startsWith("../") || QFileInfo(relative).isAbsolute()) {
        return QDir::cleanPath(toPath);
    }

    return relative;
}

QString PathUtils::absolutePath(const QString& path, const QString& baseDir)
{
    if (path.isEmpty()) {
        return QString();
    }

    // 如果已经是绝对路径，直接返回
    if (QFileInfo(path).isAbsolute()) {
        return QDir::cleanPath(path);
    }

    // 使用基础目录或当前目录
    QString base = baseDir.isEmpty() ? QDir::currentPath() : baseDir;
    QDir dir(base);

    return dir.absoluteFilePath(path);
}

// ==================== 路径验证 ====================

bool PathUtils::isValidPath(const QString& path)
{
    if (path.isEmpty()) {
        return false;
    }

    // 检查非法字符（跨平台）
#ifdef Q_OS_WIN
    // Windows非法字符: < > : " | ? *
    static const QString illegalChars = "<>:\"|?*";
    // Windows上也不能包含控制字符
    for (const QChar& ch : path) {
        if (illegalChars.contains(ch) || ch.unicode() < 32) {
            return false;
        }
    }

    // Windows上不能以空格或点结尾
    if (path.endsWith(' ') || path.endsWith('.')) {
        return false;
    }

    // Windows上保留的设备名
    static const QStringList reservedNames = {
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };

    QString fileName = QFileInfo(path).fileName().toUpper();
    for (const QString& reserved : reservedNames) {
        if (fileName == reserved || fileName.startsWith(reserved + ".")) {
            return false;
        }
    }
#else
    // Unix-like系统：不能包含空字符和斜杠（除了路径分隔符）
    if (path.contains('\0')) {
        return false;
    }

    // 检查是否包含除了路径分隔符之外的斜杠
    QString normalized = toUniversalSeparators(path);
    if (normalized.contains("//") || normalized.contains("/./") || normalized.contains("/../")) {
        // 这些会在规范化时处理，但可以作为早期检查
        return false;
    }
#endif

    return true;
}

bool PathUtils::isValidFilePath(const QString& filePath, bool checkExists)
{
    if (!isValidPath(filePath)) {
        return false;
    }

    // 检查是否为文件路径（不能以分隔符结尾）
    if (filePath.endsWith('/') || filePath.endsWith('\\')) {
        return false;
    }

    if (checkExists) {
        return QFileInfo::exists(filePath) && QFileInfo(filePath).isFile();
    }

    return true;
}

bool PathUtils::isValidDirPath(const QString& dirPath, bool checkExists)
{
    if (!isValidPath(dirPath)) {
        return false;
    }

    if (checkExists) {
        return QFileInfo::exists(dirPath) && QFileInfo(dirPath).isDir();
    }

    return true;
}

// ==================== 目录操作 ====================

bool PathUtils::ensureDirExists(const QString& dirPath)
{
    if (dirPath.isEmpty()) {
        return false;
    }

    QDir dir(dirPath);
    if (dir.exists()) {
        return true;
    }

    // 递归创建目录
    return dir.mkpath(".");
}

// ==================== 文件名操作 ====================

QString PathUtils::fileExtension(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix();

    // 对于像".tar.gz"这样的多重扩展名，可能需要特殊处理
    // 这里简化处理，只返回最后一个点之后的部分
    return suffix;
}

QString PathUtils::fileNameWithoutExtension(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    QString baseName = fileInfo.baseName();

    // 如果baseName为空但文件存在（如".gitignore"），返回完整的文件名
    if (baseName.isEmpty()) {
        baseName = fileInfo.fileName();
    }

    return baseName;
}

// ==================== 平台特定路径 ====================

QString PathUtils::platformConfigPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}

QString PathUtils::platformDataPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString PathUtils::platformTempPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::TempLocation);
}

// ==================== 路径比较 ====================

bool PathUtils::pathsEqual(const QString& path1, const QString& path2)
{
    if (path1.isEmpty() || path2.isEmpty()) {
        return path1.isEmpty() && path2.isEmpty();
    }

    QString normalized1 = QDir::cleanPath(path1);
    QString normalized2 = QDir::cleanPath(path2);

#ifdef Q_OS_WIN
    // Windows上不区分大小写
    return normalized1.compare(normalized2, Qt::CaseInsensitive) == 0;
#else
    // Unix-like系统区分大小写
    return normalized1 == normalized2;
#endif
}

// ==================== URL转换 ====================

QUrl PathUtils::toFileUrl(const QString& filePath)
{
    if (filePath.isEmpty()) {
        return QUrl();
    }

    QString normalized = QDir::cleanPath(filePath);

#ifdef Q_OS_WIN
    // Windows上需要特殊处理盘符
    if (normalized.length() >= 2 && normalized[1] == ':') {
        // 转换为"file:///C:/path"格式
        return QUrl::fromLocalFile(normalized);
    } else {
        // 相对路径或网络路径
        return QUrl::fromUserInput(normalized);
    }
#else
    // Unix-like系统
    return QUrl::fromLocalFile(normalized);
#endif
}

QString PathUtils::fromFileUrl(const QUrl& url)
{
    if (!url.isValid()) {
        return QString();
    }

    if (url.isLocalFile()) {
        return url.toLocalFile();
    }

    // 尝试处理非本地文件URL
    return url.toString();
}

// ==================== 文件名清理 ====================

QString PathUtils::sanitizeFileName(const QString& fileName)
{
    if (fileName.isEmpty()) {
        return QString();
    }

    QString sanitized = fileName;

    // 替换非法字符
#ifdef Q_OS_WIN
    static const QRegularExpression illegalCharsRegex("[<>:\"|?*\\x00-\\x1F]");
    sanitized.replace(illegalCharsRegex, "_");

    // 移除末尾的空格和点
    sanitized = sanitized.trimmed();
    while (sanitized.endsWith('.')) {
        sanitized.chop(1);
    }

    // 检查保留名称
    static const QStringList reservedNames = {
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };

    QString upperName = sanitized.toUpper();
    QString baseName = QFileInfo(sanitized).baseName().toUpper();

    for (const QString& reserved : reservedNames) {
        if (upperName == reserved || baseName == reserved) {
            sanitized = "_" + sanitized;
            break;
        }
    }
#else
    // Unix-like系统：主要替换空字符和斜杠
    static const QRegularExpression illegalCharsRegex("[\\x00/]");
    sanitized.replace(illegalCharsRegex, "_");
#endif

    // 限制长度（避免过长的文件名）
    const int MAX_FILENAME_LENGTH = 255;
    if (sanitized.length() > MAX_FILENAME_LENGTH) {
        QString extension = QFileInfo(sanitized).suffix();
        QString baseName = QFileInfo(sanitized).completeBaseName();

        // 保留扩展名，截断基础名称
        int baseMaxLength = MAX_FILENAME_LENGTH - extension.length() - (extension.isEmpty() ? 0 : 1);
        if (baseName.length() > baseMaxLength) {
            baseName = baseName.left(baseMaxLength);
        }

        sanitized = extension.isEmpty() ? baseName : baseName + "." + extension;
    }

    return sanitized;
}

// ==================== 路径合并 ====================

QString PathUtils::joinPaths(const QStringList& parts)
{
    if (parts.isEmpty()) {
        return QString();
    }

    QString result;

    for (int i = 0; i < parts.size(); ++i) {
        const QString& part = parts[i];

        if (part.isEmpty()) {
            continue;
        }

        if (i == 0) {
            result = part;
        } else {
            // 确保不重复分隔符
            if (!result.endsWith('/') && !result.endsWith('\\') &&
                !part.startsWith('/') && !part.startsWith('\\')) {
                result += QDir::separator();
            }
            result += part;
        }
    }

    return QDir::cleanPath(result);
}

} // namespace SprintIDE
