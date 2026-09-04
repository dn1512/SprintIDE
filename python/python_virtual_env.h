#ifndef PYTHON_VIRTUAL_ENV_H
#define PYTHON_VIRTUAL_ENV_H

#include <QString>
#include <QStringList>

namespace SprintIDE {

class PythonVirtualEnv {
public:
    // 检测项目路径下的虚拟环境
    static QString detectVenv(const QString& projectPath);
    // 创建虚拟环境（使用 system 或指定 python）
    static bool createVenv(const QString& projectPath, const QString& pythonPath = QString());
    // 获取激活脚本路径（用于终端）
    static QString activateScriptPath(const QString& venvPath);
    // 获取虚拟环境中的 Python 解释器路径
    static QString interpreterPath(const QString& venvPath);
    // 获取虚拟环境中的 pip 路径
    static QString pipPath(const QString& venvPath);
    // 判断是否为有效的虚拟环境
    static bool isValidVenv(const QString& venvPath);
};

} // namespace SprintIDE

#endif // PYTHON_VIRTUAL_ENV_H
