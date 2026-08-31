// src/build/go_builder.h
#ifndef GO_BUILDER_H
#define GO_BUILDER_H

#include "ibuilder.h"

namespace SprintIDE {

/**
 * @brief Go 语言构建器
 */
class GoBuilder : public IBuilder {
public:
    QProcessEnvironment prepareEnvironment(const BuildTask& task) override;
    bool build(const BuildTask& task, BuildResult& result) override;
    bool clean(const BuildTask& task, BuildResult& result) override;
    bool run(const BuildTask& task, const BuildResult& buildResult, QString& output) override;

private:
    /**
     * @brief 查找 go 可执行文件
     */
    QString findGoExecutable() const;

    /**
     * @brief 运行可执行文件（内部辅助）
     * @param executable 可执行文件路径
     * @param task 任务（用于环境和参数）
     * @param output 输出内容
     * @return 是否成功
     */
    bool runProgram(const QString& executable, const BuildTask& task, QString& output);
};

} // namespace SprintIDE

#endif // GO_BUILDER_H
