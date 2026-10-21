/*
* SprintIDE - SprintIDE is a cross-platform IDE built from scratch with Qt 6 and C++20. Its core capability is to provide AI Agents with an executable, verifiable code execution environment.
* Copyright (C) 2026  Liu Tengjun (刘腾骏)
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef PROJECT_MANAGER_H
#define PROJECT_MANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QVariantMap>
#include <QSharedPointer>

#include "java/project_model.h"
#include "python/python_project_model.h"

namespace SprintIDE {

class PythonProjectModel;
/**
 * @class ProjectManager
 * @brief 项目管理器，负责项目的创建、打开、保存和管理
 *
 * 该类提供了项目相关的核心功能，包括：
 * - 创建新项目
 * - 打开现有项目
 * - 保存项目配置
 * - 管理最近使用的项目列表
 * - 加载和保存项目窗口状态
 */
class ProjectManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit ProjectManager(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ProjectManager();

    /**
     * @brief 获取单例实例
     * @return 单例实例指针
     */
    static ProjectManager* instance();

    /**
     * @brief 创建新项目
     * @param projectName 项目名称
     * @param projectPath 项目路径
     * @param projectType 项目类型（0: 控制台应用, 1: 简单游戏, 2: GUI应用, 3: 空项目）
     * @return 是否创建成功
     */
    bool createProject(const QString& projectName, const QString& projectPath, int projectType);

    /**
     * @brief 打开现有项目
     * @param projectPath 项目配置文件路径或项目目录
     * @return 是否打开成功
     */
    bool openProject(const QString& projectPath);

    /**
     * @brief 关闭当前项目
     * @return 是否关闭成功
     */
    bool closeProject();

    /**
     * @brief 保存当前项目
     * @return 是否保存成功
     */
    bool saveProject();

    /**
     * @brief 另存为当前项目
     * @param newPath 新的项目路径
     * @return 是否保存成功
     */
    bool saveProjectAs(const QString& newPath);

    /**
     * @brief 获取当前项目名称
     * @return 当前项目名称
     */
    QString currentProjectName() const;

    /**
     * @brief 获取当前项目路径
     * @return 当前项目路径
     */
    QString currentProjectPath() const;

    /**
     * @brief 获取当前项目配置
     * @return 项目配置（键值对）
     */
    QVariantMap currentProjectConfig() const;

    /**
     * @brief 是否有打开的项目
     * @return 是否有打开的项目
     */
    bool hasOpenProject() const;

    /**
     * @brief 获取最近使用的项目列表
     * @return 最近使用的项目列表（项目路径 -> 上次打开时间）
     */
    QMap<QString, QDateTime> recentProjects() const;

    /**
     * @brief 保存窗口状态
     * @param state 窗口状态数据
     */
    void saveWindowState(const QByteArray& state);

    /**
     * @brief 加载窗口状态
     * @return 窗口状态数据
     */
    QByteArray loadWindowState() const;

    /**
     * @brief 保存窗口几何
     * @param geometry 窗口几何数据
     */
    void saveWindowGeometry(const QByteArray& geometry);

    /**
     * @brief 加载窗口几何
     * @return 窗口几何数据
     */
    QByteArray loadWindowGeometry() const;

    /**
     * @brief 获取当前打开的 Java 项目模型（如果当前项目是 Java 项目）
     * @return ProjectModel 指针（由 ProjectManager 持有，不可 delete）
     */
    ProjectModel* javaProjectModel() const { return m_javaModel.get(); }

    QSharedPointer<PythonProjectModel> pythonProjectModel() const { return m_pythonModel; }

    bool isPythonProject() const { return m_pythonModel != nullptr; }

    /**
     * @brief 从最近项目列表中移除指定项目
     * @param projectPath 项目路径
     * @return 是否成功移除（未找到也返回 false）
     */
    bool removeRecentProject(const QString& projectPath);
signals:
    /**
     * @brief 项目打开信号
     * @param projectName 项目名称
     * @param projectPath 项目路径
     */
    void projectOpened(const QString& projectName, const QString& projectPath);

    /**
     * @brief 项目关闭信号
     */
    void projectClosed();

    /**
     * @brief 项目保存信号
     * @param projectName 项目名称
     * @param projectPath 项目路径
     */
    void projectSaved(const QString& projectName, const QString& projectPath);

    /**
     * @brief 项目配置更改信号
     */
    void projectConfigChanged();

private:
    /**
     * @brief 初始化默认项目配置
     */
    void initDefaultConfig();

    /**
     * @brief 加载项目配置
     * @param filePath 配置文件路径
     * @return 是否加载成功
     */
    bool loadProjectConfig(const QString& filePath);

    /**
     * @brief 检查项目名称是否合法
     * @param projectName 项目名称
     * @return 是否合法
     */
    bool isValidProjectName(const QString& projectName) const;

    /**
     * @brief 保存项目配置
     * @param filePath 配置文件路径
     * @return 是否保存成功
     */
    bool saveProjectConfig(const QString& filePath) const;

    /**
     * @brief 更新最近使用的项目列表
     * @param projectPath 项目路径
     */
    void updateRecentProjects(const QString& projectPath);

    /**
     * @brief 初始化项目目录结构
     * @param projectPath 项目路径
     * @param projectType 项目类型
     * @return 是否初始化成功
     */
    bool initProjectStructure(const QString& projectPath, int projectType);

    /**
     * @brief 生成项目模板文件
     * @param projectPath 项目路径
     * @param projectType 项目类型
     * @return 是否生成成功
     */
    bool generateProjectTemplate(const QString& projectPath, int projectType);

    /**
     * @brief 加载最近使用的项目列表
     */
    void loadRecentProjects();

    /**
     * @brief 保存最近使用的项目列表
     */
    void saveRecentProjects() const;

private:
    static ProjectManager* m_instance; ///< 单例实例

    QString m_currentProjectName; ///< 当前项目名称
    QString m_currentProjectPath; ///< 当前项目路径
    QString m_currentProjectConfigPath; ///< 当前项目配置文件路径
    QVariantMap m_currentProjectConfig; ///< 当前项目配置

    QSettings* m_settings; ///< 设置管理器
    QMap<QString, QDateTime> m_recentProjects; ///< 最近使用的项目列表
    std::unique_ptr<ProjectModel> m_javaModel;
    QSharedPointer<PythonProjectModel> m_pythonModel;
};

} // namespace SprintIDE

#endif // PROJECT_MANAGER_H
