#ifndef LOCAL_HISTORY_H
#define LOCAL_HISTORY_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QDateTime>
#include <QList>
#include <QCoreApplication>

namespace SprintIDE {

/**
 * @brief 单个历史版本信息
 */
struct HistoryVersion
{
    int version;            ///< 版本号（从 1 开始递增）
    QString filePath;       ///< 文件路径
    QDateTime timestamp;    ///< 保存时间
    qint64 fileSize;        ///< 文件大小（字节）
    QString contentHash;    ///< 内容哈希（用于检测重复）
    QString comment;        ///< 版本注释（可选，如 "AI 自动修复"）
    QString source;         ///< 来源："user"（用户）或 "ai"（AI）

    // 用于显示
    QString displayTime() const;
    QString displaySize() const;
    QString displaySource() const;

    // 给非 QObject 的结构体提供 tr()，上下文为 "SprintIDE::HistoryVersion"
    Q_DECLARE_TR_FUNCTIONS(SprintIDE::HistoryVersion);
};

/**
 * @brief 本地历史管理器
 *
 * 核心功能：
 * 1. 每次保存文件时自动保存历史版本
 * 2. 支持恢复到任意历史版本
 * 3. 与 AI 操作集成：AI 修改前自动创建检查点
 * 4. 限制最大历史数量（默认 100）
 *
 * 存储位置：项目根目录/.sprintide/history/
 */
class LocalHistory : public QObject
{
    Q_OBJECT

public:
    explicit LocalHistory(QObject *parent = nullptr);
    ~LocalHistory();

    /**
     * @brief 设置项目根目录（初始化历史存储）
     */
    void setProjectRoot(const QString &projectPath);

    /**
     * @brief 保存历史版本
     * @param filePath 文件路径
     * @param content 文件内容
     * @param comment 版本注释（可选）
     * @param source 来源："user"（默认，用户）或 "ai"（AI）
     * @return 新版本号，失败返回 -1
     */
    int saveVersion(const QString &filePath, const QString &content,
                    const QString &comment = QString(),
                    const QString &source = "user");

    /**
     * @brief 获取文件的所有历史版本列表
     */
    QList<HistoryVersion> getVersions(const QString &filePath) const;

    /**
     * @brief 获取文件的当前版本号
     */
    int getCurrentVersion(const QString &filePath) const;

    /**
     * @brief 恢复到指定版本
     * @param filePath 文件路径
     * @param version 目标版本号
     * @return 版本内容，如果失败返回空字符串
     */
    QString restoreVersion(const QString &filePath, int version);

    /**
     * @brief 获取指定版本的内容
     */
    QString getVersionContent(const QString &filePath, int version) const;

    /**
     * @brief 删除指定文件的历史记录（文件删除时调用）
     */
    bool clearHistory(const QString &filePath);

    /**
     * @brief 清理过期的历史版本（超过最大数量）
     */
    void cleanOldVersions(const QString &filePath);

    /**
     * @brief 获取最大历史数量
     */
    int maxVersions() const { return m_maxVersions; }

    /**
     * @brief 设置最大历史数量
     */
    void setMaxVersions(int count);

    /**
     * @brief 检查是否支持本地历史
     */
    bool isEnabled() const { return !m_projectRoot.isEmpty(); }

signals:
    /**
     * @brief 新版本保存信号
     */
    void versionSaved(const QString &filePath, int version, const QString &comment);

    /**
     * @brief 版本恢复信号
     */
    void versionRestored(const QString &filePath, int version);

    /**
     * @brief 历史已清理信号
     */
    void historyCleaned(const QString &filePath, int removedCount);

private:
    /**
     * @brief 获取文件的历史存储目录
     */
    QString getHistoryDir(const QString &filePath) const;

    /**
     * @brief 获取版本的存储路径
     */
    QString getVersionFilePath(const QString &filePath, int version) const;

    /**
     * @brief 获取元数据文件路径
     */
    QString getMetadataPath() const;

    /**
     * @brief 加载元数据
     */
    void loadMetadata();

    /**
     * @brief 保存元数据
     */
    void saveMetadata();

    /**
     * @brief 计算内容哈希
     */
    QString calculateHash(const QString &content) const;

    /**
     * @brief 生成时间戳文件名
     */
    QString generateTimestamp() const;

    // 成员变量
    QString m_projectRoot;
    int m_maxVersions = 100;

    // 元数据缓存：文件路径 → 版本列表
    QMap<QString, QList<HistoryVersion>> m_metadata;
    bool m_metadataLoaded = false;
};

} // namespace SprintIDE

#endif // LOCAL_HISTORY_H