// project_config.h
#pragma once

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QVariantMap>
#include <QJsonObject>

namespace SprintIDE {

/**
 * @brief 项目类型枚举
 *
 * 定义四种项目模板类型，适合不同学习阶段
 */
enum class ProjectType {
    ConsoleApp,      ///< 控制台应用程序 - 基础学习
    SimpleGame,      ///< 简单游戏项目 - 趣味编程
    GuiApp,         ///< GUI应用程序 - 图形界面学习
    EmptyProject    ///< 空项目 - 自由创作
};

/**
 * @brief 编译模式枚举
 *
 * 支持两种编译配置模式，适应不同学习阶段
 */
enum class ProjectConfigMode {
    SimpleMode,     ///< 简单模式 - 自动配置，适合初学者
    AdvancedMode    ///< 高级模式 - 完整配置，适合进阶学习
};

/**
 * @brief 编译器类型枚举
 *
 * 支持主流C++编译器，自动检测可用编译器
 */
enum class ProjectCompilerType {
    AutoDetect,     ///< 自动检测 - IDE自动选择最佳编译器
    GCC,           ///< GNU编译器集合 - Linux/Windows常用
    Clang,         ///< Clang编译器 - macOS/跨平台
    MSVC           ///< Microsoft Visual C++ - Windows专用
};

/**
 * @brief 项目配置数据结构
 *
 * 存储项目的所有配置信息，包括基本信息、编译设置、文件列表等
 * 使用JSON格式持久化，支持版本升级和验证
 */
class ProjectConfig {
public:
    // ==================== 构造函数和析构函数 ====================

    /**
     * @brief 默认构造函数
     *
     * 创建默认的项目配置，使用控制台程序模板
     */
    ProjectConfig();

    /**
     * @brief 带参数的构造函数
     *
     * @param projectName 项目名称（英文标识符）
     * @param displayName 显示名称（中文描述）
     * @param projectPath 项目根目录路径
     */
    ProjectConfig(const QString& projectName, const QString& displayName, const QString& projectPath);

    ~ProjectConfig();

    // ==================== 配置验证方法 ====================

    /**
     * @brief 验证配置的完整性
     *
     * 检查所有必填字段，验证文件路径有效性
     *
     * @param errors 错误信息列表（输出参数）
     * @return bool 验证通过返回true，否则返回false
     */
    bool validate(QStringList& errors) const;

    /**
     * @brief 验证项目名称的合法性
     *
     * 项目名称必须是有效的C++标识符（字母、数字、下划线，不以数字开头）
     *
     * @param name 要验证的项目名称
     * @return bool 合法返回true，否则返回false
     */
    static bool isValidProjectName(const QString& name);

    // ==================== JSON序列化方法 ====================

    /**
     * @brief 从JSON对象加载配置
     *
     * @param json JSON对象数据
     * @param errors 错误信息列表（输出参数）
     * @return bool 加载成功返回true，否则返回false
     */
    bool fromJson(const QJsonObject& json, QStringList& errors);

    /**
     * @brief 将配置保存为JSON对象
     *
     * @return QJsonObject 包含所有配置的JSON对象
     */
    QJsonObject toJson() const;

    /**
     * @brief 从文件加载配置
     *
     * @param filePath 配置文件路径（.lightcppproj）
     * @param errors 错误信息列表（输出参数）
     * @return bool 加载成功返回true，否则返回false
     */
    bool loadFromFile(const QString& filePath, QStringList& errors);

    /**
     * @brief 保存配置到文件
     *
     * @param filePath 配置文件路径（.lightcppproj）
     * @param errors 错误信息列表（输出参数）
     * @return bool 保存成功返回true，否则返回false
     */
    bool saveToFile(const QString& filePath, QStringList& errors) const;

    // ==================== 配置迁移方法 ====================

    /**
     * @brief 升级配置到最新版本
     *
     * 当配置文件版本低于当前支持版本时，自动升级配置格式
     *
     * @param json 原始JSON配置
     * @param errors 错误信息列表（输出参数）
     * @return QJsonObject 升级后的JSON配置
     */
    static QJsonObject upgradeConfig(const QJsonObject& json, QStringList& errors);

    /**
     * @brief 检查配置版本兼容性
     *
     * @param version 配置版本号
     * @return bool 兼容返回true，否则返回false
     */
    static bool isVersionCompatible(const QString& version);

    // ==================== 项目模板方法 ====================

    /**
     * @brief 应用项目模板
     *
     * 根据项目类型设置默认的目录结构和初始文件
     *
     * @param type 项目类型
     */
    void applyTemplate(ProjectType type);

    /**
     * @brief 获取模板的描述信息
     *
     * @param type 项目类型
     * @return QString 模板的中文描述
     */
    static QString getTemplateDescription(ProjectType type);

    /**
     * @brief 获取模板的初始文件列表
     *
     * @param type 项目类型
     * @return QStringList 模板包含的初始文件列表
     */
    static QStringList getTemplateFiles(ProjectType type);

    // ==================== 编译配置方法 ====================

    /**
     * @brief 应用简单编译模式
     *
     * 设置适合初学者的默认编译选项
     */
    void applySimpleCompileMode();

    /**
     * @brief 应用高级编译模式
     *
     * 设置完整的编译选项，允许自定义所有参数
     */
    void applyAdvancedCompileMode();

    /**
     * @brief 生成编译命令
     *
     * 根据当前配置生成编译器命令行参数
     *
     * @param sourceFile 源文件路径（nullptr表示编译整个项目）
     * @return QStringList 编译器命令行参数
     */
    QStringList generateCompileCommand(const QString& sourceFile = QString()) const;

    /**
     * @brief 检测可用的编译器
     *
     * 扫描系统环境，找出所有可用的C++编译器
     *
     * @return QStringList 可用的编译器列表
     */
    static QStringList detectAvailableCompilers();

    // ==================== 属性访问器 ====================

    // ----- 基本信息 -----
    QString projectName() const { return m_projectName; }
    void setProjectName(const QString& name);

    QString displayName() const { return m_displayName; }
    void setDisplayName(const QString& name);

    QString projectPath() const { return m_projectPath; }
    void setProjectPath(const QString& path);

    ProjectType projectType() const { return m_projectType; }
    void setProjectType(ProjectType type);

    // ----- 版本信息 -----
    QString configVersion() const { return m_configVersion; }
    QDateTime createdDate() const { return m_createdDate; }
    QDateTime modifiedDate() const { return m_modifiedDate; }

    // ----- 文件列表 -----
    QStringList sourceFiles() const { return m_sourceFiles; }
    void setSourceFiles(const QStringList& files);
    void addSourceFile(const QString& file);
    void removeSourceFile(const QString& file);

    QStringList headerFiles() const { return m_headerFiles; }
    void setHeaderFiles(const QStringList& files);
    void addHeaderFile(const QString& file);
    void removeHeaderFile(const QString& file);

    // ----- 编译设置 -----
    ProjectConfigMode projectConfigMode() const { return m_projectConfigMode; }
    void setProjectConfigMode(ProjectConfigMode mode);

    ProjectCompilerType projectCompilerType() const { return m_projectCompilerType; }
    void setProjectCompilerType(ProjectCompilerType type);
    QString compilerPath() const { return m_compilerPath; }
    void setCompilerPath(const QString& path);

    QString cppStandard() const { return m_cppStandard; }
    void setCppStandard(const QString& standard);

    QString optimizationLevel() const { return m_optimizationLevel; }
    void setOptimizationLevel(const QString& level);

    bool enableDebugInfo() const { return m_enableDebugInfo; }
    void setEnableDebugInfo(bool enable);

    QString warningLevel() const { return m_warningLevel; }
    void setWarningLevel(const QString& level);

    QString outputType() const { return m_outputType; }
    void setOutputType(const QString& type);

    QString outputPath() const { return m_outputPath; }
    void setOutputPath(const QString& path);

    // ----- 包含路径和库设置 -----
    QStringList includePaths() const { return m_includePaths; }
    void setIncludePaths(const QStringList& paths);
    void addIncludePath(const QString& path);

    QStringList libraryPaths() const { return m_libraryPaths; }
    void setLibraryPaths(const QStringList& paths);
    void addLibraryPath(const QString& path);

    QStringList libraries() const { return m_libraries; }
    void setLibraries(const QStringList& libs);
    void addLibrary(const QString& lib);

    // ----- 宏定义 -----
    QStringList defines() const { return m_defines; }
    void setDefines(const QStringList& defines);
    void addDefine(const QString& define);

    // ----- 扩展配置 -----
    QVariantMap extraConfig() const { return m_extraConfig; }
    void setExtraConfig(const QVariantMap& config);
    void setExtraValue(const QString& key, const QVariant& value);
    QVariant getExtraValue(const QString& key, const QVariant& defaultValue = QVariant()) const;

private:
    // ==================== 私有方法 ====================

    /**
     * @brief 初始化默认值
     */
    void initDefaults();

    /**
     * @brief 验证文件路径
     *
     * @param path 文件路径
     * @param checkExists 是否检查文件存在
     * @return bool 路径有效返回true
     */
    bool validateFilePath(const QString& path, bool checkExists = false) const;

    /**
     * @brief 验证目录路径
     *
     * @param path 目录路径
     * @param checkExists 是否检查目录存在
     * @return bool 路径有效返回true
     */
    bool validateDirPath(const QString& path, bool checkExists = false) const;

    // ==================== 成员变量 ====================

    // ----- 基本信息 -----
    QString m_projectName;      ///< 项目名称（英文标识符）
    QString m_displayName;      ///< 显示名称（中文描述）
    QString m_projectPath;      ///< 项目根目录路径
    ProjectType m_projectType;  ///< 项目类型

    // ----- 版本信息 -----
    QString m_configVersion;    ///< 配置文件版本
    QDateTime m_createdDate;    ///< 创建时间
    QDateTime m_modifiedDate;   ///< 最后修改时间

    // ----- 文件列表 -----
    QStringList m_sourceFiles;  ///< 源文件列表（.cpp）
    QStringList m_headerFiles;  ///< 头文件列表（.h）

    // ----- 编译设置 -----
    ProjectConfigMode m_projectConfigMode;  ///< 编译模式
    ProjectCompilerType m_projectCompilerType; ///< 编译器类型
    QString m_compilerPath;     ///< 编译器路径
    QString m_cppStandard;      ///< C++标准
    QString m_optimizationLevel; ///< 优化级别
    bool m_enableDebugInfo;     ///< 启用调试信息
    QString m_warningLevel;     ///< 警告级别
    QString m_outputType;       ///< 输出类型（executable/staticlib/dynamiclib）
    QString m_outputPath;       ///< 输出文件路径

    // ----- 包含路径和库设置 -----
    QStringList m_includePaths; ///< 包含路径
    QStringList m_libraryPaths; ///< 库路径
    QStringList m_libraries;    ///< 链接库

    // ----- 宏定义 -----
    QStringList m_defines;      ///< 预定义宏

    // ----- 扩展配置 -----
    QVariantMap m_extraConfig;  ///< 扩展配置（未来兼容性）
};

} // namespace SprintIDE
