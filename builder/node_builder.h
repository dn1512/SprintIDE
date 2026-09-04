// src/build/node_builder.h
#ifndef NODE_BUILDER_H
#define NODE_BUILDER_H

#include "ibuilder.h"

namespace SprintIDE {

/**
 * @brief Node.js / TypeScript 构建器
 */
class NodeBuilder : public IBuilder {
public:
    QProcessEnvironment prepareEnvironment(const BuildTask& task) override;
    bool build(const BuildTask& task, BuildResult& result) override;
    bool clean(const BuildTask& task, BuildResult& result) override;
    bool run(const BuildTask& task, const BuildResult& buildResult, QString& output) override;
    QString getExecutablePath(const BuildTask& task) override;

private:
    /**
     * @brief 查找 npm / npx 可执行文件
     */
    QString findNpmExecutable() const;
    QString findNpxExecutable() const;
    QString findNodeExecutable() const;

    /**
     * @brief 检测项目类型：是否 TypeScript
     */
    bool isTypeScriptProject(const QString& projectPath) const;

    /**
     * @brief 运行 npm script
     */
    bool runNpmScript(const QString& script, const BuildTask& task, QString& output);
};

} // namespace SprintIDE

#endif // NODE_BUILDER_H
