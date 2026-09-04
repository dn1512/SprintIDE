// src/build/cpp_builder.h
#ifndef CPP_BUILDER_H
#define CPP_BUILDER_H

#include "ibuilder.h"
#include <QStringList>

namespace SprintIDE {

/**
 * @brief C++ 语言构建器
 */
class CppBuilder : public IBuilder {
public:
    CppBuilder() = default;
    ~CppBuilder() override = default;

    QProcessEnvironment prepareEnvironment(const BuildTask& task) override;
    bool build(const BuildTask& task, BuildResult& result) override;
    bool clean(const BuildTask& task, BuildResult& result) override;
    bool run(const BuildTask& task, const BuildResult& buildResult, QString& output) override;
    /**
     * @brief 确保项目根目录下存在 compile_commands.json
     * @param projectPath 项目根目录
     * @return true 表示成功生成或已存在
     */
    static bool ensureCompileCommands(const QString& projectPath);

    QString getExecutablePath(const BuildTask& task) override;
private:
    /**
     * @brief 生成编译器命令行参数
     */
    QStringList generateCompilerArgs(const BuildTask& task) const;

    /**
     * @brief 自动选择可用的 C++ 编译器
     */
    QString autoSelectCompiler(const BuildTask& task) const;

    /**
     * @brief 解析编译器输出，提取错误和警告
     */
    BuildResult parseOutput(const QString& output, const QString& projectPath) const;
};

} // namespace SprintIDE

#endif // CPP_BUILDER_H
