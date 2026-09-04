#pragma once

#include <QString>
#include <memory>
#include "project_model.h"

namespace SprintIDE {

class ProjectResolver {
public:
    // 根据项目根目录检测并返回对应的 ProjectModel 指针（调用者负责内存管理）
    static std::unique_ptr<ProjectModel> resolveProject(const QString& projectPath);
};

} // namespace SprintIDE
