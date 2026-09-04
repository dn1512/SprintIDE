#ifndef IBUILDER_H
#define IBUILDER_H

#include "build_task.h"
#include "build_result.h"
#include <QProcessEnvironment>

namespace SprintIDE {

/**
 * @brief 构建器抽象接口
 */
class IBuilder {
public:
    virtual ~IBuilder() = default;

    /**
     * @brief 准备构建所需的环境变量
     * @param task 构建任务
     * @return 环境变量对象
     */
    virtual QProcessEnvironment prepareEnvironment(const BuildTask& task) = 0;

    /**
     * @brief 执行构建
     * @param task 构建任务
     * @param result 输出结果
     * @return 是否成功启动（不代表构建成功，需检查 result.success）
     */
    virtual bool build(const BuildTask& task, BuildResult& result) = 0;

    /**
     * @brief 清理构建产物
     * @param task 构建任务（提供项目路径等）
     * @param result 输出结果
     * @return 是否成功清理
     */
    virtual bool clean(const BuildTask& task, BuildResult& result) = 0;

    /**
     * @brief 运行生成的可执行文件
     * @param task 构建任务（含运行参数）
     * @param buildResult 之前构建的结果（含可执行文件路径）
     * @param output 运行输出
     * @return 是否成功运行
     */
    virtual bool run(const BuildTask& task, const BuildResult& buildResult, QString& output) = 0;

    /**
     * @brief 获取构建产物的可执行文件路径
     * @param task 构建任务
     * @return 可执行文件的完整路径，如果无法确定则返回空字符串
     */
    virtual QString getExecutablePath(const BuildTask& task) = 0;
};

} // namespace SprintIDE

#endif // IBUILDER_H
