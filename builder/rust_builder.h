// src/build/rust_builder.h
#ifndef RUST_BUILDER_H
#define RUST_BUILDER_H

#include "ibuilder.h"

namespace SprintIDE {

/**
 * @brief Rust 语言构建器（使用 Cargo）
 */
class RustBuilder : public IBuilder {
public:
    QProcessEnvironment prepareEnvironment(const BuildTask& task) override;
    bool build(const BuildTask& task, BuildResult& result) override;
    bool clean(const BuildTask& task, BuildResult& result) override;
    bool run(const BuildTask& task, const BuildResult& buildResult, QString& output) override;

private:
    /**
     * @brief 查找 cargo 可执行文件
     */
    QString findCargoExecutable() const;
};

} // namespace SprintIDE

#endif // RUST_BUILDER_H
