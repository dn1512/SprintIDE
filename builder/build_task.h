// src/build/build_task.h
#ifndef BUILD_TASK_H
#define BUILD_TASK_H

#include <QString>
#include <QStringList>
#include <QVariantMap>
#include "language/language_type.h"

namespace SprintIDE {

/**
 * @brief 通用构建任务结构体
 */
struct BuildTask {
    QString taskId;                     ///< 唯一任务标识
    LanguageType language;              ///< 语言类型
    QString projectPath;                ///< 项目根目录（绝对路径）
    QStringList sourceFiles;            ///< 源文件列表（绝对或相对项目根目录）
    QString outputName;                 ///< 输出文件名（不含路径）
    QString outputDir;                  ///< 输出目录（相对项目根目录）
    bool enableDebug = true;            ///< 是否启用调试信息

    QVariantMap sdkConfig;              ///< SDK 环境变量（如 JAVA_HOME）
    QVariantMap aiCustomPayload;        ///< AI 通用配置负载（GenericBuilder 专用）
    QStringList extraArgs;              ///< 额外命令行参数
};

} // namespace SprintIDE

#endif // BUILD_TASK_H
