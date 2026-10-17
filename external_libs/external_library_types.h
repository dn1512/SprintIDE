#ifndef EXTERNAL_LIBRARY_TYPES_H
#define EXTERNAL_LIBRARY_TYPES_H

#include <QString>
#include <QList>

namespace SprintIDE {

    /**
    * @brief 外部库节点的通用数据结构
    *
    * 所有 Provider 都产出这种节点，由 ProjectTreeProxyModel 渲染
    */
    struct ExternalLibraryNode
    {
        enum class Kind {
            Root,        ///< 顶层环境名（"site-packages"、"Cargo 依赖"）
            Group,       ///< 中间分类（"系统头文件"、"Qt"）
            Package,     ///< 库/包（numpy、serde 等），可展开
            File,        ///< 可打开的源码文件
            Opaque,      ///< 二进制（.so / .dll / jar），双击用系统打开
        };

        QString displayName;
        QString absolutePath;   ///< 双击/复制路径用；Root/Group 可为空
        Kind    kind = Kind::Package;
        QList<ExternalLibraryNode> children;
    };

} // namespace SprintIDE

#endif // EXTERNAL_LIBRARY_TYPES_H