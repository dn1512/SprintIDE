#include "local_history.h"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QDebug>
#include <QFileInfo>

namespace SprintIDE {

// ============================================================
// 辅助方法
// ============================================================

QString HistoryVersion::displayTime() const
{
    return timestamp.toString("yyyy-MM-dd hh:mm:ss");
}

QString HistoryVersion::displaySize() const
{
    if (fileSize < 1024) {
        return QString("%1 B").arg(fileSize);
    } else if (fileSize < 1024 * 1024) {
        return QString("%1 KB").arg(fileSize / 1024.0, 0, 'f', 1);
    } else {
        return QString("%1 MB").arg(fileSize / (1024.0 * 1024.0), 0, 'f', 1);
    }
}

// ============================================================
// 构造函数/析构函数
// ============================================================

LocalHistory::LocalHistory(QObject *parent)
    : QObject(parent)
{
    qDebug() << "[LocalHistory] 已创建";
}

LocalHistory::~LocalHistory()
{
    saveMetadata();
    qDebug() << "[LocalHistory] 已销毁";
}

// ============================================================
// 设置项目根目录
// ============================================================

void LocalHistory::setProjectRoot(const QString &projectPath)
{
    qDebug() << "[LocalHistory] setProjectRoot called with path:" << projectPath;
    if (projectPath.isEmpty()) {
        qWarning() << "[LocalHistory] setProjectRoot: empty path, ignored";
        return;
    }

    // 如果根目录与当前不同，必须清空旧数据并重新加载
    if (m_projectRoot != projectPath) {
        qDebug() << "[LocalHistory] 检测到根目录变化，清空旧元数据并重新加载";
        m_metadata.clear();
        m_metadataLoaded = false;
        m_projectRoot = projectPath;
    } else {
        // 路径相同，无需重复加载
        qDebug() << "[LocalHistory] 根目录未变化，跳过加载";
        return;
    }

    // 创建历史目录
    QDir historyDir(getHistoryDir(""));
    if (!historyDir.exists()) {
        if (!historyDir.mkpath(".")) {
            qWarning() << "[LocalHistory] 无法创建历史目录:" << historyDir.absolutePath();
            return;
        }
    }

    // 加载元数据
    loadMetadata();

    qDebug() << "[LocalHistory] 项目根目录已设置:" << projectPath
             << "历史目录:" << historyDir.absolutePath();
}

// ============================================================
// 保存历史版本
// ============================================================

int LocalHistory::saveVersion(const QString &filePath, const QString &content,
                              const QString &comment)
{
    qDebug() << "[LocalHistory] saveVersion called, isEnabled=" << isEnabled()
    << " file=" << filePath;
    if (!isEnabled() || filePath.isEmpty()) {
        qDebug() << "[LocalHistory] saveVersion skipped (not enabled or empty path)";
        return -1;
    }

    QFileInfo info(filePath);
    if (!info.exists()) {
        qWarning() << "[LocalHistory] 文件不存在:" << filePath;
        return -1;
    }

    // 生成内容哈希（用于检测重复）
    QString hash = calculateHash(content);

    // 获取当前版本列表
    QList<HistoryVersion> versions = getVersions(filePath);

    // 检查是否与最新版本相同（避免重复保存）
    if (!versions.isEmpty()) {
        HistoryVersion &latest = versions.last();
        if (latest.contentHash == hash) {
            // 内容未变化，不保存新版本
            qDebug() << "[LocalHistory] 内容未变化，跳过保存:" << filePath;
            return latest.version;
        }
    }

    // 创建新版本
    int newVersion = versions.isEmpty() ? 1 : versions.last().version + 1;

    HistoryVersion version;
    version.version = newVersion;
    version.filePath = filePath;
    version.timestamp = QDateTime::currentDateTime();
    version.fileSize = content.toUtf8().size();
    version.contentHash = hash;
    version.comment = comment.isEmpty() ? "自动保存" : comment;

    // 保存版本文件
    QString versionFilePath = getVersionFilePath(filePath, newVersion);

    // 创建父目录
    QDir dir(QFileInfo(versionFilePath).absoluteDir());
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qWarning() << "[LocalHistory] 无法创建版本文件目录:" << dir.absolutePath();
            return -1;
        }
    }

    QFile versionFile(versionFilePath);
    if (!versionFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[LocalHistory] 无法保存版本文件:" << versionFilePath;
        return -1;
    }
    QTextStream stream(&versionFile);
    stream << content;
    versionFile.close();

    // 更新元数据
    versions.append(version);
    m_metadata[filePath] = versions;
    saveMetadata();

    // 清理旧版本
    cleanOldVersions(filePath);

    qDebug() << "[LocalHistory] 版本已保存:" << filePath
             << "版本号:" << newVersion
             << "大小:" << version.displaySize();

    emit versionSaved(filePath, newVersion, comment);

    return newVersion;
}

// ============================================================
// 获取文件的历史版本列表
// ============================================================

QList<HistoryVersion> LocalHistory::getVersions(const QString &filePath) const
{
    qDebug() << "[LocalHistory] saveVersion: enabled=" << isEnabled()
    << " root=" << m_projectRoot
    << " file=" << filePath;
    return m_metadata.value(filePath);
}

// ============================================================
// 获取当前版本号
// ============================================================

int LocalHistory::getCurrentVersion(const QString &filePath) const
{
    QList<HistoryVersion> versions = getVersions(filePath);
    if (versions.isEmpty()) {
        return 0;
    }
    return versions.last().version;
}

// ============================================================
// 恢复到指定版本
// ============================================================

QString LocalHistory::restoreVersion(const QString &filePath, int version)
{
    if (!isEnabled() || filePath.isEmpty()) {
        return QString();
    }

    QString content = getVersionContent(filePath, version);
    if (content.isNull()) {
        qWarning() << "[LocalHistory] 无法恢复版本:" << filePath << "版本:" << version;
        return QString();
    }

    emit versionRestored(filePath, version);
    qDebug() << "[LocalHistory] 已恢复版本:" << filePath << "版本:" << version;

    return content;
}

// ============================================================
// 获取指定版本的内容
// ============================================================

QString LocalHistory::getVersionContent(const QString &filePath, int version) const
{
    if (!isEnabled() || version < 1) {
        return QString();
    }

    // 验证版本是否存在
    QList<HistoryVersion> versions = getVersions(filePath);
    bool found = false;
    for (const HistoryVersion &v : versions) {
        if (v.version == version) {
            found = true;
            break;
        }
    }
    if (!found) {
        qWarning() << "[LocalHistory] 版本不存在:" << filePath << "版本:" << version;
        return QString();
    }

    // 读取版本文件
    QString versionFilePath = getVersionFilePath(filePath, version);
    QFile file(versionFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[LocalHistory] 无法读取版本文件:" << versionFilePath;
        return QString();
    }

    QTextStream stream(&file);
    return stream.readAll();
}

// ============================================================
// 清除文件历史
// ============================================================

bool LocalHistory::clearHistory(const QString &filePath)
{
    if (!isEnabled() || filePath.isEmpty()) {
        return false;
    }

    // 获取历史目录
    QString historyDirPath = getHistoryDir(filePath);
    QDir dir(historyDirPath);

    // 删除所有版本文件
    if (dir.exists()) {
        QFileInfoList entries = dir.entryInfoList(QDir::Files);
        for (const QFileInfo &entry : entries) {
            QFile::remove(entry.absoluteFilePath());
        }
        dir.rmdir(historyDirPath);
    }

    // 移除元数据
    m_metadata.remove(filePath);
    saveMetadata();

    qDebug() << "[LocalHistory] 已清除历史:" << filePath;

    return true;
}

// ============================================================
// 清理过期历史
// ============================================================

void LocalHistory::cleanOldVersions(const QString &filePath)
{
    QList<HistoryVersion> versions = m_metadata.value(filePath);
    if (versions.size() <= m_maxVersions) {
        return;
    }

    int removed = versions.size() - m_maxVersions;

    // 删除最旧的版本文件
    for (int i = 0; i < removed; ++i) {
        const HistoryVersion &oldVersion = versions[i];
        QString versionFilePath = getVersionFilePath(filePath, oldVersion.version);
        QFile::remove(versionFilePath);
    }

    // 更新元数据
    versions = versions.mid(removed);
    m_metadata[filePath] = versions;
    saveMetadata();

    emit historyCleaned(filePath, removed);

    qDebug() << "[LocalHistory] 已清理历史:" << filePath
             << "删除了" << removed << "个旧版本";
}

// ============================================================
// 设置最大历史数量
// ============================================================

void LocalHistory::setMaxVersions(int count)
{
    if (count < 1) {
        qWarning() << "[LocalHistory] 最大历史数量不能小于 1";
        return;
    }

    m_maxVersions = count;
}

// ============================================================
// 私有辅助方法
// ============================================================

QString LocalHistory::getHistoryDir(const QString &filePath) const
{
    if (filePath.isEmpty()) {
        return QDir(m_projectRoot).absoluteFilePath(".sprintide/history");
    }

    // 使用文件名的哈希作为子目录名（避免特殊字符问题）
    QString fileName = QFileInfo(filePath).fileName();
    QString safeName = fileName;
    safeName.replace("/", "_").replace("\\", "_").replace(":", "_");
    safeName = safeName.isEmpty() ? "unknown" : safeName;

    return QDir(m_projectRoot).absoluteFilePath(
        QString(".sprintide/history/%1").arg(safeName));
}

QString LocalHistory::getVersionFilePath(const QString &filePath, int version) const
{
    QString dir = getHistoryDir(filePath);
    // 格式: {版本号}-{时间戳}.{扩展名}
    QFileInfo info(filePath);
    QString ext = info.suffix();
    QString baseName = info.completeBaseName();

    // 获取版本的时间戳
    QString timestamp = generateTimestamp();
    // 从元数据中查找该版本的准确时间戳
    QList<HistoryVersion> versions = getVersions(filePath);
    for (const HistoryVersion &v : versions) {
        if (v.version == version) {
            timestamp = v.timestamp.toString("yyyyMMdd-HHmmss");
            break;
        }
    }

    QString fileName = QString("%1-%2").arg(version).arg(timestamp);
    if (!ext.isEmpty()) {
        fileName += "." + ext;
    }

    return QDir(dir).absoluteFilePath(fileName);
}

QString LocalHistory::getMetadataPath() const
{
    return QDir(m_projectRoot).absoluteFilePath(".sprintide/history/versions.json");
}

void LocalHistory::loadMetadata()
{
    if (m_metadataLoaded) {
        qDebug() << "[LocalHistory] 元数据已加载，跳过";
        return;
    }

    QString metadataPath = getMetadataPath();
    qDebug() << "[LocalHistory] 尝试加载元数据文件:" << metadataPath;

    QFile file(metadataPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "[LocalHistory] 无历史元数据文件，将创建新文件";
        m_metadataLoaded = true;
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "[LocalHistory] 元数据解析失败:" << error.errorString();
        m_metadataLoaded = true;
        return;
    }

    if (!doc.isObject()) {
        qWarning() << "[LocalHistory] 元数据格式无效";
        m_metadataLoaded = true;
        return;
    }

    QJsonObject root = doc.object();
    for (const QString &key : root.keys()) {
        QJsonArray versionsArray = root[key].toArray();
        QList<HistoryVersion> versions;

        for (const QJsonValue &val : versionsArray) {
            QJsonObject obj = val.toObject();
            HistoryVersion v;
            v.version = obj["version"].toInt();
            v.filePath = key;
            v.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
            v.fileSize = obj["fileSize"].toInt();
            v.contentHash = obj["contentHash"].toString();
            v.comment = obj["comment"].toString("自动保存");
            versions.append(v);
        }

        if (!versions.isEmpty()) {
            m_metadata[key] = versions;
        }
    }

    m_metadataLoaded = true;
    qDebug() << "[LocalHistory] 已加载" << m_metadata.size() << "个文件的历史元数据";
}

void LocalHistory::saveMetadata()
{
    if (m_projectRoot.isEmpty()) {
        return;
    }

    QJsonObject root;

    for (auto it = m_metadata.begin(); it != m_metadata.end(); ++it) {
        const QString &filePath = it.key();
        const QList<HistoryVersion> &versions = it.value();

        QJsonArray versionsArray;
        for (const HistoryVersion &v : versions) {
            QJsonObject obj;
            obj["version"] = v.version;
            obj["timestamp"] = v.timestamp.toString(Qt::ISODate);
            obj["fileSize"] = (qint64)v.fileSize;
            obj["contentHash"] = v.contentHash;
            obj["comment"] = v.comment;
            versionsArray.append(obj);
        }

        root[filePath] = versionsArray;
    }

    QJsonDocument doc(root);
    QString metadataPath = getMetadataPath();

    // 确保目录存在
    QDir dir(QFileInfo(metadataPath).absolutePath());
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QFile file(metadataPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[LocalHistory] 无法保存元数据:" << metadataPath;
        return;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}

QString LocalHistory::calculateHash(const QString &content) const
{
    QByteArray data = content.toUtf8();
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha1);
    return hash.toHex();
}

QString LocalHistory::generateTimestamp() const
{
    return QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
}

} // namespace SprintIDE
