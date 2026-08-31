// src/build/python_builder.h
#ifndef PYTHON_BUILDER_H
#define PYTHON_BUILDER_H

#include "ibuilder.h"

namespace SprintIDE {

/**
 * @brief Python 语言构建器（语法检查 + 运行）
 */
class PythonBuilder : public IBuilder {
public:
    QProcessEnvironment prepareEnvironment(const BuildTask& task) override;
    bool build(const BuildTask& task, BuildResult& result) override;
    bool clean(const BuildTask& task, BuildResult& result) override;
    bool run(const BuildTask& task, const BuildResult& buildResult, QString& output) override;
};

} // namespace SprintIDE

#endif // PYTHON_BUILDER_H
