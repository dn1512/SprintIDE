#ifndef PYTHON_PROJECT_MODEL_H
#define PYTHON_PROJECT_MODEL_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QSharedPointer>

namespace SprintIDE {

class PythonProjectModel {
public:
    explicit PythonProjectModel(const QString& projectPath);
    bool load();  // 加载项目信息

    // 属性
    QString projectPath() const { return m_projectPath; }
    QString projectName() const { return m_projectName; }
    QString interpreterPath() const { return m_interpreterPath; }
    QString venvPath() const { return m_venvPath; }
    QStringList dependencies() const { return m_dependencies; }
    QStringList devDependencies() const { return m_devDependencies; }
    QString entryPoint() const { return m_entryPoint; }  // 主要入口脚本或模块

private:
    QString m_projectPath;
    QString m_projectName;
    QString m_interpreterPath;
    QString m_venvPath;
    QStringList m_dependencies;
    QStringList m_devDependencies;
    QString m_entryPoint;

    void detectVenv();
    void parseRequirements();
    void parsePyproject();
    void parseSetupPy();
    void detectEntryPoint();
};

} // namespace SprintIDE

#endif // PYTHON_PROJECT_MODEL_H
