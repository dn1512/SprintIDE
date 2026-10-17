#ifndef EXTERNAL_LIBRARY_PROVIDER_H
#define EXTERNAL_LIBRARY_PROVIDER_H

#include <QString>
#include "external_library_types.h"

namespace SprintIDE {

    /**
    * @brief 外部库收集器抽象接口
    *
    * 约定：
    *  - collect() 必须是线程安全的纯函数（可放到 QtConcurrent 里跑）
    *  - 不得访问 MainWindow 主线程对象
    *  - 不得修改任何全局状态
    */
    class IExternalLibraryProvider
    {
        public:
        virtual ~IExternalLibraryProvider() = default;

        /// @param projectPath 项目根目录绝对路径
        /// @return 顶层的 Root 节点列表（一般 1~2 个，空表示未检测到外部库）
        virtual QList<ExternalLibraryNode> collect(const QString& projectPath) = 0;
    };

} // namespace SprintIDE

#endif // EXTERNAL_LIBRARY_PROVIDER_H