// src/build/cpp_builder.h
#ifndef CPP_BUILDER_H
#define CPP_BUILDER_H

#include "ibuilder.h"
#include <QStringList>
#include "process_manager.h"

namespace SprintIDE {

class CppProjectModel;

/**
 * @brief C++ 语言构建器
 *
 * 支持两类模式：
 *  1. CMake 项目：调用 cmake -S . -B build 配置，再 cmake --build build
 *  2. 单文件/无构建系统项目：直接调用 g++ / clang++ 编译（保留原有行为）
 *
 * 分支判断依据：项目根目录是否存在 CMakeLists.txt。
 */
class CppBuilder : public IBuilder {
public:
    CppBuilder() = default;
    ~CppBuilder() override = default;

    QProcessEnvironment prepareEnvironment(const BuildTask& task) override;
    bool build(const BuildTask& task, BuildResult& result) override;
    bool clean(const BuildTask& task, BuildResult& result) override;
    bool run(const BuildTask& task, const BuildResult& buildResult, QString& output) override;
    QString getExecutablePath(const BuildTask& task) override;

    /**
     * @brief 确保项目根目录下存在 compile_commands.json
     *  - CMake 项目：确保执行 cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON，
     *                并把 build/compile_commands.json 拷贝/软链到项目根
     *  - 非 CMake 项目：沿用旧的“扫描源文件手工生成”行为
     */
    static bool ensureCompileCommands(const QString& projectPath);

    /**
     * @brief 探测当前系统可用的 C++ 编译器
     * @return 编译器可执行文件（例如 "clang++" / "g++" / "cl.exe"），失败返回空
     */
    static QString detectCompiler();

private:
    // ---------- 构建 ----------
    bool buildWithCMake(const BuildTask& task, BuildResult& result);
    bool buildSingleFile(const BuildTask& task, BuildResult& result);

    // ---------- 清理 ----------
    bool cleanWithCMake(const BuildTask& task, BuildResult& result);

    // ---------- 单文件辅助 ----------
    QStringList generateCompilerArgs(const BuildTask& task) const;
    QString autoSelectCompiler(const BuildTask& task) const;
    BuildResult parseOutput(const QString& output, const QString& projectPath) const;

    ProcessManager m_processManager;
};

} // namespace SprintIDE

#endif // CPP_BUILDER_H
