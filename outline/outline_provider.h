#ifndef OUTLINE_PROVIDER_H
#define OUTLINE_PROVIDER_H

#include "outline_types.h"
#include <QString>

namespace SprintIDE {

    /**
    * @brief 符号扫描器接口
    *
    * 约定：
    *  - scan() 是纯函数，不访问主线程对象
    *  - 输入全文 + 文件路径，输出符号树
    *  - 大文件由调用方截断（Provider 不负责）
    */
    class IOutlineProvider
    {
        public:
        virtual ~IOutlineProvider() = default;

        virtual QList<OutlineSymbol> scan(const QString& text,
            const QString& filePath) = 0;
    };

} // namespace SprintIDE

#endif