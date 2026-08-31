// src/build/build_result.h
#ifndef BUILD_RESULT_H
#define BUILD_RESULT_H

#include <QString>
#include <QStringList>

namespace SprintIDE {

/**
 * @brief 构建结果结构体
 */
struct BuildResult {
    bool success = false;               ///< 是否成功
    int exitCode = -1;                  ///< 退出码
    QString fullOutput;                 ///< 完整输出（stdout + stderr）
    QString executablePath;             ///< 生成的可执行文件路径（若有）
    QStringList errorLines;             ///< 错误行（解析后）
    QStringList warningLines;           ///< 警告行（解析后）
    qint64 elapsedMs = 0;               ///< 耗时（毫秒）
    QString builderName;                ///< 处理此任务的 Builder 名称
    QString errorMessage;               ///< 错误信息（一般失败时）
};

} // namespace SprintIDE

#endif // BUILD_RESULT_H
