#pragma once

#include <QDialog>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QDateTime>
#include "project_config.h"

namespace Ui {
class ProjectCreationDialog;
}

namespace SprintIDE {

/**
 * @brief 项目模板结构体
 *
 * 定义项目模板的详细信息，包括名称、描述、示例代码等
 */
struct ProjectTemplate {
    QString id;                 ///< 模板唯一标识符
    QString name;              ///< 模板显示名称（中文）
    QString description;       ///< 模板详细描述
    ProjectType type;          ///< 项目类型
    QString icon;              ///< 图标资源路径
    QStringList defaultFiles;  ///< 默认包含的文件列表
    QStringList directories;   ///< 需要创建的目录结构

    // 示例代码内容
    struct ExampleFile {
        QString path;          ///< 文件相对路径
        QString content;       ///< 文件内容
        QString language;      ///< 文件语言（cpp/h）
    };

    QList<ExampleFile> exampleFiles;  ///< 示例文件列表

    // 模板变量（用于替换）
    QMap<QString, QString> variables;  ///< 模板变量映射
};

/**
 * @brief 项目创建结果结构体
 *
 * 存储项目创建的结果信息，包括成功状态、错误信息等
 */
struct ProjectCreationResult {
    bool success;              ///< 创建是否成功
    QString projectPath;       ///< 项目根目录路径
    QString projectName;       ///< 项目名称（英文标识符）
    QString displayName;       ///< 显示名称（中文）
    ProjectType projectType;   ///< 项目类型
    QString errorMessage;      ///< 错误信息（如果创建失败）
    QDateTime createdTime;     ///< 创建时间

    // 创建的目录和文件列表
    QStringList createdDirectories;  ///< 成功创建的目录列表
    QStringList createdFiles;        ///< 成功创建的文件列表

    /**
     * @brief 重置结果
     */
    void reset() {
        success = false;
        projectPath.clear();
        projectName.clear();
        displayName.clear();
        projectType = ProjectType::ConsoleApp;
        errorMessage.clear();
        createdTime = QDateTime();
        createdDirectories.clear();
        createdFiles.clear();
    }
};

/**
 * @brief 项目创建对话框类
 *
 * 提供图形化界面引导用户创建新的C++项目，支持四种项目模板选择，
 * 提供简单模式和高级模式的编译配置选项，适合儿童学习使用。
 *
 * 对话框特点：
 * 1. 全中文界面，适合儿童学习
 * 2. 向导式界面，引导用户逐步完成项目创建
 * 3. 支持四种项目模板，满足不同学习需求
 * 4. 提供简单/高级两种编译模式
 * 5. 实时验证用户输入，提供友好提示
 */
class ProjectCreationDialog : public QDialog
{
    Q_OBJECT

public:
    // ==================== 构造函数和析构函数 ====================

    /**
     * @brief 构造函数
     *
     * @param parent 父窗口指针，通常为主窗口
     */
    explicit ProjectCreationDialog(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ProjectCreationDialog();

    // ==================== 公共方法 ====================

    /**
     * @brief 获取项目创建结果
     *
     * @return ProjectCreationResult 包含项目创建的所有结果信息
     */
    ProjectCreationResult getResult() const;

    /**
     * @brief 获取项目配置
     *
     * @return ProjectConfig 项目配置对象，包含所有设置
     */
    ProjectConfig getProjectConfig() const;

    /**
     * @brief 设置默认项目路径
     *
     * 设置项目创建的默认目录，如果用户没有选择其他路径，将使用此路径
     *
     * @param defaultPath 默认项目路径
     */
    void setDefaultProjectPath(const QString& defaultPath);

    /**
     * @brief 获取默认项目路径
     *
     * @return QString 当前设置的默认项目路径
     */
    QString defaultProjectPath() const;

    /**
     * @brief 加载项目模板
     *
     * 从资源文件或配置文件中加载项目模板，包括模板名称、描述、示例代码等
     *
     * @return bool 加载成功返回true，否则返回false
     */
    bool loadTemplates();

    /**
     * @brief 获取所有可用模板
     *
     * @return QList<ProjectTemplate> 项目模板列表
     */
    QList<ProjectTemplate> availableTemplates() const;

public slots:
    // ==================== 公共槽函数 ====================

    /**
     * @brief 接受对话框（验证并创建项目）
     *
     * 重写QDialog的accept方法，在关闭对话框前执行项目创建验证和实际创建
     */
    void accept() override;

private slots:
    // ==================== 私有槽函数 ====================

    /**
     * @brief 浏览项目位置
     *
     * 打开文件对话框，让用户选择项目创建的目录
     */
    void onBrowseLocation();

    /**
     * @brief 项目名称改变时的处理
     *
     * 实时验证项目名称的合法性，并更新相关提示信息
     *
     * @param name 新的项目名称
     */
    void onProjectNameChanged(const QString& name);

    /**
     * @brief 项目显示名称改变时的处理
     *
     * @param name 新的显示名称
     */
    void onDisplayNameChanged(const QString& name);

    /**
     * @brief 项目位置改变时的处理
     *
     * 验证项目路径的合法性，并更新路径预览
     *
     * @param path 新的项目位置
     */
    void onLocationChanged(const QString& path);

    /**
     * @brief 项目模板选择改变时的处理
     *
     * 根据选择的模板更新界面提示和示例代码预览
     *
     * @param index 新的模板索引
     */
    void onTemplateChanged(int index);

    /**
     * @brief 编译模式切换处理
     *
     * 根据选择的编译模式（简单/高级）显示或隐藏高级配置选项
     *
     * @param checked 是否选择高级模式
     */
    void onCompileModeChanged(bool checked);

    /**
     * @brief 编译器选择改变时的处理
     *
     * @param index 新的编译器索引
     */
    void onCompilerChanged(int index);

    /**
     * @brief C++标准改变时的处理
     *
     * @param index 新的标准索引
     */
    void onCppStandardChanged(int index);

    /**
     * @brief 优化级别改变时的处理
     *
     * @param index 新的优化级别索引
     */
    void onOptimizationChanged(int index);

    /**
     * @brief 警告级别改变时的处理
     *
     * @param index 新的警告级别索引
     */
    void onWarningLevelChanged(int index);

    /**
     * @brief 显示高级配置对话框
     *
     * 打开高级编译配置对话框，允许用户设置更多编译选项
     */
    void onShowAdvancedConfig();

    /**
     * @brief 显示示例代码
     *
     * 在新窗口中显示当前选定模板的示例代码
     */
    void onShowExampleCode();

    /**
     * @brief 重置为默认设置
     *
     * 将所有设置重置为默认值
     */
    void onResetToDefaults();

    void setDisplayName(const QString& displayName);
    void setProjectType(ProjectType type);
    void setCompilerType(ProjectCompilerType type);

private:
    // ==================== 私有方法 ====================

    /**
     * @brief 初始化界面
     *
     * 创建所有界面控件，设置布局，初始化状态
     */
    void setupUI();

    /**
     * @brief 初始化信号连接
     *
     * 连接所有信号和槽函数
     */
    void setupConnections();

    /**
     * @brief 初始化模板系统
     *
     * 创建四种项目模板的默认配置
     */
    void initTemplates();

    /**
     * @brief 初始化编译器设置
     *
     * 检测系统可用的编译器，并初始化编译器选择列表
     */
    void initCompilerSettings();

    /**
     * @brief 验证用户输入
     *
     * 验证所有输入字段的合法性，收集错误信息
     *
     * @param errors 错误信息列表（输出参数）
     * @return bool 验证通过返回true，否则返回false
     */
    bool validateInput(QStringList& errors) const;

    /**
     * @brief 验证项目名称
     *
     * 检查项目名称是否符合C++标识符规范
     *
     * @param name 项目名称
     * @param error 错误信息（输出参数）
     * @return bool 验证通过返回true，否则返回false
     */
    bool validateProjectName(const QString& name, QString& error) const;

    /**
     * @brief 验证项目路径
     *
     * 检查项目路径是否合法且可写
     *
     * @param path 项目路径
     * @param error 错误信息（输出参数）
     * @return bool 验证通过返回true，否则返回false
     */
    bool validateProjectPath(const QString& path, QString& error) const;

    /**
     * @brief 创建项目目录结构
     *
     * 根据模板创建项目所需的目录结构
     *
     * @return bool 创建成功返回true，否则返回false
     */
    bool createProjectStructure();

    /**
     * @brief 生成项目文件
     *
     * 根据模板生成项目配置文件(.lightcppproj)和示例代码文件
     *
     * @return bool 生成成功返回true，否则返回false
     */
    bool generateProjectFiles();

    /**
     * @brief 生成配置文件
     *
     * 生成.lightcppproj项目配置文件
     *
     * @return bool 生成成功返回true，否则返回false
     */
    bool generateConfigFile();

    /**
     * @brief 生成示例代码文件
     *
     * 根据模板生成示例代码文件
     *
     * @return bool 生成成功返回true，否则返回false
     */
    bool generateExampleFiles();

    /**
     * @brief 更新项目路径预览
     *
     * 根据项目名称和路径更新预览标签
     */
    void updatePathPreview();

    /**
     * @brief 更新模板描述
     *
     * 根据选定的模板更新描述和示例代码预览
     */
    void updateTemplateDescription();

    /**
     * @brief 更新编译器选项
     *
     * 根据选择的编译器更新相关选项
     */
    void updateCompilerOptions();

    /**
     * @brief 应用当前模板
     *
     * 将当前选定模板的设置应用到界面
     */
    void applyCurrentTemplate();

    /**
     * @brief 获取当前模板
     *
     * @return ProjectTemplate 当前选定的项目模板
     */
    ProjectTemplate getCurrentTemplate() const;

    /**
     * @brief 替换模板变量
     *
     * 将模板内容中的变量替换为实际值
     *
     * @param content 原始内容
     * @param variables 变量映射
     * @return QString 替换后的内容
     */
    QString replaceTemplateVariables(const QString& content,
                                     const QMap<QString, QString>& variables) const;

    /**
     * @brief 保存对话框状态
     *
     * 将对话框的当前状态保存到配置文件中
     */
    void saveDialogState();

    /**
     * @brief 加载对话框状态
     *
     * 从配置文件中加载对话框的保存状态
     */
    void loadDialogState();

    // ==================== 私有成员变量 ====================

    Ui::ProjectCreationDialog *ui;          ///< UI对象指针
    ProjectCreationResult m_result;         ///< 项目创建结果
    ProjectConfig m_projectConfig;          ///< 项目配置对象

    // 模板系统
    QList<ProjectTemplate> m_templates;     ///< 项目模板列表
    QMap<QString, ProjectTemplate> m_templateMap;  ///< 模板映射（ID->模板）

    // 设置
    QString m_defaultProjectPath;           ///< 默认项目路径
    bool m_autoOpenProject;                 ///< 创建后自动打开项目
    bool m_createGitRepo;                   ///< 创建Git仓库
    bool m_initializeGitIgnore;             ///< 初始化.gitignore文件

    // 状态
    bool m_initialized;                     ///< 是否已初始化
    QString m_lastUsedPath;                 ///< 上次使用的项目路径

    // 示例代码缓存
    QMap<QString, QString> m_exampleCodeCache;  ///< 示例代码缓存（模板ID->代码）
};

} // namespace SprintIDE
