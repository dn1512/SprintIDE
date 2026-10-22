// project_config.cpp
#include "project_config.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QProcess>
#include <QCoreApplication>
#include <QDebug>
#include <QStandardPaths>

namespace SprintIDE {

// ==================== 静态常量和辅助函数 ====================

// 当前支持的配置版本
static const QString CURRENT_CONFIG_VERSION = "1.0.0";

// 支持的C++标准列表
static const QStringList SUPPORTED_CPP_STANDARDS = {
    "c++11", "c++14", "c++17", "c++20", "c++23"
};

// 支持的优化级别
static const QStringList SUPPORTED_OPTIMIZATION_LEVELS = {
    "none", "fast", "faster", "fastest", "size"
};

// 支持的警告级别
static const QStringList SUPPORTED_WARNING_LEVELS = {
    "none", "low", "medium", "high", "all"
};

// 项目类型对应的目录结构模板
static const QMap<ProjectType, QStringList> TEMPLATE_DIRECTORIES = {
    {ProjectType::ConsoleApp, {"src", "include", "bin", "docs"}},
    {ProjectType::SimpleGame, {"src", "include", "assets", "bin", "docs"}},
    {ProjectType::GuiApp, {"src", "include", "ui", "resources", "bin", "docs"}},
    {ProjectType::EmptyProject, {"src", "include"}}
};

// ==================== 构造函数和析构函数 ====================

ProjectConfig::ProjectConfig()
    : m_projectType(ProjectType::ConsoleApp)
    , m_projectConfigMode(ProjectConfigMode::SimpleMode)
    , m_projectCompilerType(ProjectCompilerType::AutoDetect)
    , m_enableDebugInfo(true)
{
    initDefaults();
}

ProjectConfig::ProjectConfig(const QString& projectName,
                             const QString& displayName,
                             const QString& projectPath)
    : m_projectName(projectName)
    , m_displayName(displayName)
    , m_projectPath(projectPath)
    , m_projectType(ProjectType::ConsoleApp)
    , m_projectConfigMode(ProjectConfigMode::SimpleMode)
    , m_projectCompilerType(ProjectCompilerType::AutoDetect)
    , m_enableDebugInfo(true)
{
    initDefaults();
}

ProjectConfig::~ProjectConfig()
{
    // 析构函数，目前没有需要手动释放的资源
}

// ==================== 私有初始化方法 ====================

void ProjectConfig::initDefaults()
{
    m_projectLanguage = QStringLiteral("cpp");
    // 设置默认版本信息
    m_configVersion = CURRENT_CONFIG_VERSION;
    m_createdDate = QDateTime::currentDateTime();
    m_modifiedDate = m_createdDate;

    // 设置默认编译配置
    m_cppStandard = "c++17";
    m_optimizationLevel = "none";
    m_warningLevel = "all";
    m_outputType = "executable";
    m_outputPath = "bin";

    // 设置默认包含路径
    m_includePaths = {"include"};

    // 设置默认库路径
    m_libraryPaths = {"lib"};

    // 清空文件列表
    m_sourceFiles.clear();
    m_headerFiles.clear();
    m_libraries.clear();
    m_defines.clear();
    m_extraConfig.clear();
}

// ==================== 配置验证方法 ====================

bool ProjectConfig::validate(QStringList& errors) const
{
    errors.clear();

    // 验证项目名称
    if (m_projectName.isEmpty()) {
        errors.append("项目名称不能为空");
    } else if (!isValidProjectName(m_projectName)) {
        errors.append("项目名称只能包含字母、数字和下划线，且不能以数字开头");
    }

    // 验证显示名称
    if (m_displayName.isEmpty()) {
        errors.append("显示名称不能为空");
    }

    // 验证项目路径
    if (m_projectPath.isEmpty()) {
        errors.append("项目路径不能为空");
    } else if (!validateDirPath(m_projectPath, false)) {
        errors.append("项目路径包含非法字符或格式不正确");
    }

    // 验证配置文件版本
    if (!isVersionCompatible(m_configVersion)) {
        errors.append(QString("不支持的配置文件版本: %1，当前支持版本: %2")
                          .arg(m_configVersion).arg(CURRENT_CONFIG_VERSION));
    }

    // 验证C++标准
    if (!SUPPORTED_CPP_STANDARDS.contains(m_cppStandard)) {
        errors.append(QString("不支持的C++标准: %1，支持的选项: %2")
                          .arg(m_cppStandard).arg(SUPPORTED_CPP_STANDARDS.join(", ")));
    }

    // 验证优化级别
    if (!SUPPORTED_OPTIMIZATION_LEVELS.contains(m_optimizationLevel)) {
        errors.append(QString("不支持的优化级别: %1，支持的选项: %2")
                          .arg(m_optimizationLevel).arg(SUPPORTED_OPTIMIZATION_LEVELS.join(", ")));
    }

    // 验证警告级别
    if (!SUPPORTED_WARNING_LEVELS.contains(m_warningLevel)) {
        errors.append(QString("不支持的警告级别: %1，支持的选项: %2")
                          .arg(m_warningLevel).arg(SUPPORTED_WARNING_LEVELS.join(", ")));
    }

    // 验证编译器路径（如果不是自动检测）
    if (m_projectCompilerType != ProjectCompilerType::AutoDetect && !m_compilerPath.isEmpty()) {
        if (!validateFilePath(m_compilerPath, true)) {
            errors.append("编译器路径不存在或不可访问");
        }
    }

    // 验证输出路径
    if (!m_outputPath.isEmpty() && !validateDirPath(m_outputPath, false)) {
        errors.append("输出路径包含非法字符或格式不正确");
    }

    // 验证源文件列表
    for (const QString& file : m_sourceFiles) {
        if (!validateFilePath(file, false)) {
            errors.append(QString("源文件路径无效: %1").arg(file));
        }
    }

    // 验证头文件列表
    for (const QString& file : m_headerFiles) {
        if (!validateFilePath(file, false)) {
            errors.append(QString("头文件路径无效: %1").arg(file));
        }
    }

    // 验证包含路径
    for (const QString& path : m_includePaths) {
        if (!validateDirPath(path, false)) {
            errors.append(QString("包含路径无效: %1").arg(path));
        }
    }

    // 验证库路径
    for (const QString& path : m_libraryPaths) {
        if (!validateDirPath(path, false)) {
            errors.append(QString("库路径无效: %1").arg(path));
        }
    }

    return errors.isEmpty();
}

bool ProjectConfig::isValidProjectName(const QString& name)
{
    // 项目名称必须是有效的C++标识符
    if (name.isEmpty()) {
        return false;
    }

    // 第一个字符必须是字母或下划线
    QChar firstChar = name.at(0);
    if (!firstChar.isLetter() && firstChar != '_') {
        return false;
    }

    // 后续字符可以是字母、数字或下划线
    for (int i = 1; i < name.length(); ++i) {
        QChar ch = name.at(i);
        if (!ch.isLetterOrNumber() && ch != '_') {
            return false;
        }
    }

    // 不能是C++关键字（简化检查）
    static const QStringList cppKeywords = {
        "auto", "break", "case", "char", "const", "continue", "default",
        "do", "double", "else", "enum", "extern", "float", "for", "goto",
        "if", "int", "long", "register", "return", "short", "signed",
        "sizeof", "static", "struct", "switch", "typedef", "union",
        "unsigned", "void", "volatile", "while"
    };

    return !cppKeywords.contains(name.toLower());
}

// ==================== JSON序列化方法 ====================

bool ProjectConfig::fromJson(const QJsonObject& json, QStringList& errors)
{
    errors.clear();

    // 检查版本并进行升级
    QJsonObject upgradedJson = upgradeConfig(json, errors);
    if (!errors.isEmpty()) {
        return false;
    }

    // 基本信息
    m_projectName = upgradedJson.value("project_name").toString();
    m_displayName = upgradedJson.value("display_name").toString();
    m_projectPath = upgradedJson.value("project_path").toString();
    m_projectLanguage = upgradedJson.value("language").toString(QStringLiteral("cpp"));

    // 项目类型
    QString typeStr = upgradedJson.value("project_type").toString("console");
    if (typeStr == "console") {
        m_projectType = ProjectType::ConsoleApp;
    } else if (typeStr == "game") {
        m_projectType = ProjectType::SimpleGame;
    } else if (typeStr == "gui") {
        m_projectType = ProjectType::GuiApp;
    } else if (typeStr == "empty") {
        m_projectType = ProjectType::EmptyProject;
    }

    // 版本信息
    m_configVersion = upgradedJson.value("config_version").toString(CURRENT_CONFIG_VERSION);
    m_createdDate = QDateTime::fromString(upgradedJson.value("created_date").toString(), Qt::ISODate);
    m_modifiedDate = QDateTime::fromString(upgradedJson.value("modified_date").toString(), Qt::ISODate);

    // 如果日期无效，使用当前时间
    if (!m_createdDate.isValid()) {
        m_createdDate = QDateTime::currentDateTime();
    }
    if (!m_modifiedDate.isValid()) {
        m_modifiedDate = QDateTime::currentDateTime();
    }

    // 文件列表
    m_sourceFiles.clear();
    QJsonArray sourceArray = upgradedJson.value("source_files").toArray();
    for (const QJsonValue& value : sourceArray) {
        m_sourceFiles.append(value.toString());
    }

    m_headerFiles.clear();
    QJsonArray headerArray = upgradedJson.value("header_files").toArray();
    for (const QJsonValue& value : headerArray) {
        m_headerFiles.append(value.toString());
    }

    // 编译模式
    QString modeStr = upgradedJson.value("compile_mode").toString("simple");
    m_projectConfigMode = (modeStr == "advanced") ? ProjectConfigMode::AdvancedMode : ProjectConfigMode::SimpleMode;

    // 编译器设置
    QString compilerStr = upgradedJson.value("compiler_type").toString("auto");
    if (compilerStr == "gcc") {
        m_projectCompilerType = ProjectCompilerType::GCC;
    } else if (compilerStr == "clang") {
        m_projectCompilerType = ProjectCompilerType::Clang;
    } else if (compilerStr == "msvc") {
        m_projectCompilerType = ProjectCompilerType::MSVC;
    } else {
        m_projectCompilerType = ProjectCompilerType::AutoDetect;
    }

    m_compilerPath = upgradedJson.value("compiler_path").toString();
    m_cppStandard = upgradedJson.value("cpp_standard").toString("c++17");
    m_optimizationLevel = upgradedJson.value("optimization_level").toString("none");
    m_enableDebugInfo = upgradedJson.value("enable_debug_info").toBool(true);
    m_warningLevel = upgradedJson.value("warning_level").toString("all");
    m_outputType = upgradedJson.value("output_type").toString("executable");
    m_outputPath = upgradedJson.value("output_path").toString("bin");

    // 包含路径和库设置
    m_includePaths.clear();
    QJsonArray includeArray = upgradedJson.value("include_paths").toArray();
    for (const QJsonValue& value : includeArray) {
        m_includePaths.append(value.toString());
    }

    m_libraryPaths.clear();
    QJsonArray libPathArray = upgradedJson.value("library_paths").toArray();
    for (const QJsonValue& value : libPathArray) {
        m_libraryPaths.append(value.toString());
    }

    m_libraries.clear();
    QJsonArray libArray = upgradedJson.value("libraries").toArray();
    for (const QJsonValue& value : libArray) {
        m_libraries.append(value.toString());
    }

    // 宏定义
    m_defines.clear();
    QJsonArray defineArray = upgradedJson.value("defines").toArray();
    for (const QJsonValue& value : defineArray) {
        m_defines.append(value.toString());
    }

    // 扩展配置
    m_extraConfig.clear();
    QJsonObject extraObj = upgradedJson.value("extra_config").toObject();
    for (const QString& key : extraObj.keys()) {
        m_extraConfig[key] = extraObj.value(key).toVariant();
    }

    QJsonObject sdkObj = upgradedJson.value("sdk_env").toObject();
    m_sdkEnv.clear();
    for (auto it = sdkObj.begin(); it != sdkObj.end(); ++it) {
        m_sdkEnv[it.key()] = it.value().toVariant();
    }

    // 通用配置
    QJsonObject genericObj = upgradedJson.value("generic_config").toObject();
    m_genericConfig.clear();
    for (auto it = genericObj.begin(); it != genericObj.end(); ++it) {
        m_genericConfig[it.key()] = it.value().toVariant();
    }

    // Python 专属
    m_pythonInterpreter = upgradedJson.value("python_interpreter").toString();
    m_pythonVenv        = upgradedJson.value("python_venv").toString();

    // 验证加载的配置
    return validate(errors);
}

QJsonObject ProjectConfig::toJson() const
{
    QJsonObject json;

    // ============================================================
    //  通用字段（所有语言都写）
    // ============================================================
    json["project_name"]   = m_projectName;
    json["display_name"]   = m_displayName;
    json["project_path"]   = m_projectPath;
    json["language"]       = m_projectLanguage;
    json["config_version"] = CURRENT_CONFIG_VERSION;
    json["created_date"]   = m_createdDate.toString(Qt::ISODate);
    json["modified_date"]  = QDateTime::currentDateTime().toString(Qt::ISODate);

    // 项目类型（所有语言共用，语义弱但保留）
    QString typeStr;
    switch (m_projectType) {
        case ProjectType::ConsoleApp:   typeStr = "console"; break;
        case ProjectType::SimpleGame:   typeStr = "game";    break;
        case ProjectType::GuiApp:       typeStr = "gui";     break;
        case ProjectType::EmptyProject: typeStr = "empty";   break;
    }
    json["project_type"] = typeStr;

    // 源文件列表（所有语言都应有）
    QJsonArray sourceArray;
    for (const QString& file : m_sourceFiles) {
        sourceArray.append(file);
    }
    json["source_files"] = sourceArray;

    // 输出路径（各语言语义不同但都可能有）
    json["output_path"] = m_outputPath;

    // 通用扩展槽
    QJsonObject extraObj;
    for (auto it = m_extraConfig.constBegin(); it != m_extraConfig.constEnd(); ++it) {
        extraObj[it.key()] = QJsonValue::fromVariant(it.value());
    }
    json["extra_config"] = extraObj;

    QJsonObject sdkObj;
    for (auto it = m_sdkEnv.begin(); it != m_sdkEnv.end(); ++it) {
        sdkObj[it.key()] = QJsonValue::fromVariant(it.value());
    }
    json["sdk_env"] = sdkObj;

    // Python 专属（顺便补上之前漏掉的序列化）
    if (m_projectLanguage.toLower() == "python") {
        if (!m_pythonInterpreter.isEmpty()) {
            json["python_interpreter"] = m_pythonInterpreter;
        }
        if (!m_pythonVenv.isEmpty()) {
            json["python_venv"] = m_pythonVenv;
        }
    }

    // ============================================================
    //  C++ / Qt 专属字段
    // ============================================================
    const QString lang = m_projectLanguage.toLower();
    const bool isCppFamily = (lang.isEmpty()
        || lang == "cpp" || lang == "c" || lang == "c++"
        || lang == "qt");

    if (isCppFamily) {
        json["compile_mode"] = (m_projectConfigMode == ProjectConfigMode::AdvancedMode)
        ? "advanced" : "simple";

        QString compilerStr;
        switch (m_projectCompilerType) {
            case ProjectCompilerType::GCC:        compilerStr = "gcc";   break;
            case ProjectCompilerType::Clang:      compilerStr = "clang"; break;
            case ProjectCompilerType::MSVC:       compilerStr = "msvc";  break;
            case ProjectCompilerType::AutoDetect: compilerStr = "auto";  break;
        }
        json["compiler_type"]      = compilerStr;
        json["compiler_path"]      = m_compilerPath;
        json["cpp_standard"]       = m_cppStandard;
        json["optimization_level"] = m_optimizationLevel;
        json["warning_level"]      = m_warningLevel;
        json["enable_debug_info"]  = m_enableDebugInfo;
        json["output_type"]        = m_outputType;

        QJsonArray includeArray;
        for (const QString& p : m_includePaths) {
            includeArray.append(p);
        }
        json["include_paths"] = includeArray;

        QJsonArray libPathArray;
        for (const QString& p : m_libraryPaths) {
            libPathArray.append(p);
        }
        json["library_paths"] = libPathArray;

        QJsonArray libArray;
        for (const QString& l : m_libraries) {
            libArray.append(l);
        }
        json["libraries"] = libArray;

        QJsonArray defineArray;
        for (const QString& d : m_defines) {
            defineArray.append(d);
        }
        json["defines"] = defineArray;

        QJsonArray headerArray;
        for (const QString& f : m_headerFiles) {
            headerArray.append(f);
        }
        json["header_files"] = headerArray;
    }

    // ============================================================
    //  GenericAI 专属
    // ============================================================
    if (lang == "generic_ai") {
        QJsonObject genericObj;
        for (auto it = m_genericConfig.constBegin(); it != m_genericConfig.constEnd(); ++it) {
            genericObj[it.key()] = QJsonValue::fromVariant(it.value());
        }
        json["generic_config"] = genericObj;
    }

    return json;
}

bool ProjectConfig::loadFromFile(const QString& filePath, QStringList& errors)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errors.append(QString("无法打开配置文件: %1").arg(file.errorString()));
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        errors.append(QString("JSON解析错误: %1").arg(parseError.errorString()));
        return false;
    }

    if (!doc.isObject()) {
        errors.append("配置文件格式错误: 不是有效的JSON对象");
        return false;
    }

    // 设置项目路径为配置文件所在目录
    QFileInfo fileInfo(filePath);
    m_projectPath = fileInfo.absolutePath();

    return fromJson(doc.object(), errors);
}

bool ProjectConfig::saveToFile(const QString& filePath, QStringList& errors) const
{
    // 先验证配置
    if (!validate(errors)) {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        errors.append(QString("无法保存配置文件: %1").arg(file.errorString()));
        return false;
    }

    QJsonDocument doc(toJson());
    QByteArray jsonData = doc.toJson(QJsonDocument::Indented);

    if (file.write(jsonData) != jsonData.size()) {
        errors.append(QString("写入文件失败: %1").arg(file.errorString()));
        file.close();
        return false;
    }

    file.close();
    return true;
}

// ==================== 配置迁移方法 ====================

QJsonObject ProjectConfig::upgradeConfig(const QJsonObject& json, QStringList& errors)
{
    QJsonObject upgraded = json;

    // 获取原始版本号
    QString version = json.value("config_version").toString("1.0.0");

    // 版本1.0.0到1.0.0（当前版本）的迁移
    // 目前没有迁移逻辑，因为这是第一个版本
    // 未来添加版本时，在这里添加迁移逻辑

    // 设置升级后的版本号
    upgraded["config_version"] = CURRENT_CONFIG_VERSION;

    // 确保所有必填字段存在
    if (!upgraded.contains("project_name")) {
        upgraded["project_name"] = "Untitled";
    }
    if (!upgraded.contains("display_name")) {
        upgraded["display_name"] = "未命名项目";
    }
    if (!upgraded.contains("project_path")) {
        upgraded["project_path"] = ".";
    }
    if (!upgraded.contains("project_type")) {
        upgraded["project_type"] = "console";
    }
    if (!upgraded.contains("language")) {
        upgraded["language"] = QStringLiteral("cpp");   // 老配置默认 C++
    }

    return upgraded;
}

bool ProjectConfig::isVersionCompatible(const QString& version)
{
    // 简单版本兼容性检查：主版本号相同即为兼容
    QStringList currentParts = CURRENT_CONFIG_VERSION.split('.');
    QStringList targetParts = version.split('.');

    if (currentParts.size() < 1 || targetParts.size() < 1) {
        return false;
    }

    // 主版本号必须相同
    return currentParts[0] == targetParts[0];
}

// ==================== 项目模板方法 ====================

void ProjectConfig::applyTemplate(ProjectType type)
{
    m_projectType = type;

    // 根据模板类型设置默认配置
    switch (type) {
    case ProjectType::ConsoleApp:
        // 控制台程序：包含标准库，调试信息开启
        applySimpleCompileMode();
        m_sourceFiles = getTemplateFiles(type);
        m_headerFiles.clear();
        m_defines.clear();
        break;

    case ProjectType::SimpleGame:
        // 简单游戏：可能需要额外的库
        applySimpleCompileMode();
        m_sourceFiles = getTemplateFiles(type);
        m_headerFiles.clear();
        m_defines.clear();
        break;

    case ProjectType::GuiApp:
        // GUI程序：可能需要Qt库
        applySimpleCompileMode();
        m_sourceFiles = getTemplateFiles(type);
        m_headerFiles.clear();

        // 添加Qt相关定义
        m_defines = {"QT_CORE_LIB", "QT_GUI_LIB", "QT_WIDGETS_LIB"};
        break;

    case ProjectType::EmptyProject:
        // 空项目：最小配置
        applySimpleCompileMode();
        m_sourceFiles.clear();
        m_headerFiles.clear();
        m_defines.clear();
        break;
    }

    // 更新修改时间
    m_modifiedDate = QDateTime::currentDateTime();
}

QString ProjectConfig::getTemplateDescription(ProjectType type)
{
    switch (type) {
    case ProjectType::ConsoleApp:
        return "控制台应用程序：创建简单的命令行程序，适合学习基础语法";
    case ProjectType::SimpleGame:
        return "简单游戏项目：创建小游戏项目，增加编程趣味性";
    case ProjectType::GuiApp:
        return "图形界面程序：创建带窗口界面的应用程序";
    case ProjectType::EmptyProject:
        return "空白项目：创建完全自定义的项目结构";
    }
    return QString();
}

QStringList ProjectConfig::getTemplateFiles(ProjectType type)
{
    QStringList files;

    switch (type) {
    case ProjectType::ConsoleApp:
        files << "src/main.cpp";
        break;
    case ProjectType::SimpleGame:
        files << "src/main.cpp" << "src/game.cpp";
        break;
    case ProjectType::GuiApp:
        files << "src/main.cpp" << "src/mainwindow.cpp";
        break;
    case ProjectType::EmptyProject:
        // 空白项目没有初始文件
        break;
    }

    return files;
}

// ==================== 编译配置方法 ====================

void ProjectConfig::applySimpleCompileMode()
{
    m_projectConfigMode = ProjectConfigMode::SimpleMode;

    // 简单模式的默认设置
    m_projectCompilerType = ProjectCompilerType::AutoDetect;
    m_compilerPath.clear();
    m_cppStandard = "c++17";
    m_optimizationLevel = "none";
    m_enableDebugInfo = true;
    m_warningLevel = "all";
    m_outputType = "executable";
    m_outputPath = "bin";

    // 简单模式不需要用户设置这些
    m_includePaths = {"include"};
    m_libraryPaths = {"lib"};
    m_libraries.clear();
    m_defines.clear();
}

void ProjectConfig::applyAdvancedCompileMode()
{
    m_projectConfigMode = ProjectConfigMode::AdvancedMode;

    // 高级模式保留当前设置，不重置
    // 只是切换模式，设置由用户控制
}

QStringList ProjectConfig::generateCompileCommand(const QString& sourceFile) const
{
    QStringList args;

    // 确定编译器路径
    QString compiler = m_compilerPath;
    if (compiler.isEmpty()) {
        // 根据编译器类型选择默认编译器
        switch (m_projectCompilerType) {
        case ProjectCompilerType::GCC:
            compiler = "g++";
            break;
        case ProjectCompilerType::Clang:
            compiler = "clang++";
            break;
        case ProjectCompilerType::MSVC:
            compiler = "cl.exe";  // Windows专用
            break;
        case ProjectCompilerType::AutoDetect:
            compiler = "g++";  // 默认使用g++
            break;
        }
    }

    args << compiler;

    // C++标准
    if (!m_cppStandard.isEmpty()) {
        args << "-std=" + m_cppStandard;
    }

    // 优化级别
    if (m_optimizationLevel == "fast") {
        args << "-O1";
    } else if (m_optimizationLevel == "faster") {
        args << "-O2";
    } else if (m_optimizationLevel == "fastest") {
        args << "-O3";
    } else if (m_optimizationLevel == "size") {
        args << "-Os";
    }

    // 调试信息
    if (m_enableDebugInfo) {
        args << "-g";
    }

    // 警告级别
    if (m_warningLevel == "low") {
        args << "-Wall";
    } else if (m_warningLevel == "medium") {
        args << "-Wall" << "-Wextra";
    } else if (m_warningLevel == "high" || m_warningLevel == "all") {
        args << "-Wall" << "-Wextra" << "-pedantic" << "-Werror";
    }

    // 包含路径
    for (const QString& path : m_includePaths) {
        args << "-I" + QDir(m_projectPath).absoluteFilePath(path);
    }

    // 宏定义
    for (const QString& define : m_defines) {
        args << "-D" + define;
    }

    // 库路径
    for (const QString& path : m_libraryPaths) {
        args << "-L" + QDir(m_projectPath).absoluteFilePath(path);
    }

    // 源文件
    if (sourceFile.isEmpty()) {
        // 编译整个项目
        for (const QString& file : m_sourceFiles) {
            args << QDir(m_projectPath).absoluteFilePath(file);
        }
    } else {
        // 编译单个文件
        args << QDir(m_projectPath).absoluteFilePath(sourceFile);
    }

    // 输出文件
    QString outputFile;
    if (m_outputType == "executable") {
        outputFile = m_projectName;
    } else if (m_outputType == "staticlib") {
        outputFile = "lib" + m_projectName + ".a";
    } else if (m_outputType == "dynamiclib") {
        outputFile = "lib" + m_projectName + ".so";
    }

    QString outputPath = QDir(m_projectPath).absoluteFilePath(m_outputPath);
    args << "-o" << QDir(outputPath).absoluteFilePath(outputFile);

    // 链接库
    for (const QString& lib : m_libraries) {
        args << "-l" + lib;
    }

    return args;
}

QStringList ProjectConfig::detectAvailableCompilers()
{
    QStringList compilers;
    QStringList commands = {"g++", "clang++", "cl"};

    for (const QString& cmd : commands) {
        QProcess process;
        process.start(cmd, QStringList() << "--version");

        if (process.waitForStarted(1000)) {
            process.waitForFinished(1000);
            if (process.exitStatus() == QProcess::NormalExit) {
                compilers << cmd;
            }
        }
    }

    return compilers;
}

// ==================== 属性设置器 ====================

void ProjectConfig::setDisplayName(const QString& name)
{
    if (name != m_displayName) {
        m_displayName = name;
        m_modifiedDate = QDateTime::currentDateTime();
    }
}

void ProjectConfig::setProjectType(ProjectType type)
{
    if (type != m_projectType) {
        m_projectType = type;
        m_modifiedDate = QDateTime::currentDateTime();
    }
}

void ProjectConfig::setProjectCompilerType(ProjectCompilerType type)
{
    if (type != m_projectCompilerType) {
        m_projectCompilerType = type;
        m_modifiedDate = QDateTime::currentDateTime();
    }
}

void ProjectConfig::setProjectLanguage(const QString& langId)
{
    const QString normalized = langId.toLower().trimmed();
    if (normalized != m_projectLanguage) {
        m_projectLanguage = normalized;
        m_modifiedDate = QDateTime::currentDateTime();
    }
}

void ProjectConfig::setProjectName(const QString& name)
{
    if (name != m_projectName && isValidProjectName(name)) {
        m_projectName = name;
        m_modifiedDate = QDateTime::currentDateTime();
    }
}

void ProjectConfig::setProjectPath(const QString& path)
{
    if (path != m_projectPath && validateDirPath(path, false)) {
        m_projectPath = path;
        m_modifiedDate = QDateTime::currentDateTime();
    }
}

void ProjectConfig::setProjectConfigMode(ProjectConfigMode mode)
{
    if (mode != m_projectConfigMode) {
        m_projectConfigMode = mode;
        m_modifiedDate = QDateTime::currentDateTime();
    }
}

void ProjectConfig::setCompilerPath(const QString& path)
{
    if (path != m_compilerPath) {
        m_compilerPath = path;
        m_modifiedDate = QDateTime::currentDateTime();
    }
}

void ProjectConfig::setCppStandard(const QString& standard)
{
    if (SUPPORTED_CPP_STANDARDS.contains(standard) && standard != m_cppStandard) {
        m_cppStandard = standard;
        m_modifiedDate = QDateTime::currentDateTime();
    }
}

void ProjectConfig::setOptimizationLevel(const QString& level)
{
    if (SUPPORTED_OPTIMIZATION_LEVELS.contains(level) && level != m_optimizationLevel) {
        m_optimizationLevel = level;
        m_modifiedDate = QDateTime::currentDateTime();
    }
}

void ProjectConfig::setEnableDebugInfo(bool enable)
{
    if (enable != m_enableDebugInfo) {
        m_enableDebugInfo = enable;
        m_modifiedDate = QDateTime::currentDateTime();
    }
}

void ProjectConfig::setWarningLevel(const QString& level)
{
    if (SUPPORTED_WARNING_LEVELS.contains(level) && level != m_warningLevel) {
        m_warningLevel = level;
        m_modifiedDate = QDateTime::currentDateTime();
    }
}

void ProjectConfig::setOutputType(const QString& type)
{
    if (type != m_outputType) {
        m_outputType = type;
        m_modifiedDate = QDateTime::currentDateTime();
    }
}

void ProjectConfig::setOutputPath(const QString& path)
{
    if (path != m_outputPath && validateDirPath(path, false)) {
        m_outputPath = path;
        m_modifiedDate = QDateTime::currentDateTime();
    }
}

void ProjectConfig::setSourceFiles(const QStringList& files)
{
    m_sourceFiles = files;
    m_modifiedDate = QDateTime::currentDateTime();
}

void ProjectConfig::addSourceFile(const QString& file)
{
    if (validateFilePath(file, false) && !m_sourceFiles.contains(file)) {
        m_sourceFiles.append(file);
        m_modifiedDate = QDateTime::currentDateTime();
    }
}

void ProjectConfig::removeSourceFile(const QString& file)
{
    if (m_sourceFiles.removeOne(file)) {
        m_modifiedDate = QDateTime::currentDateTime();
    }
}

void ProjectConfig::setHeaderFiles(const QStringList& files)
{
    m_headerFiles = files;
    m_modifiedDate = QDateTime::currentDateTime();
}

void ProjectConfig::addHeaderFile(const QString& file)
{
    if (validateFilePath(file, false) && !m_headerFiles.contains(file)) {
        m_headerFiles.append(file);
        m_modifiedDate = QDateTime::currentDateTime();
    }
}

void ProjectConfig::removeHeaderFile(const QString& file)
{
    if (m_headerFiles.removeOne(file)) {
        m_modifiedDate = QDateTime::currentDateTime();
    }
}

void ProjectConfig::setIncludePaths(const QStringList& paths)
{
    m_includePaths = paths;
    m_modifiedDate = QDateTime::currentDateTime();
}

void ProjectConfig::addIncludePath(const QString& path)
{
    if (validateDirPath(path, false) && !m_includePaths.contains(path)) {
        m_includePaths.append(path);
        m_modifiedDate = QDateTime::currentDateTime();
    }
}

void ProjectConfig::setLibraryPaths(const QStringList& paths)
{
    m_libraryPaths = paths;
    m_modifiedDate = QDateTime::currentDateTime();
}

void ProjectConfig::addLibraryPath(const QString& path)
{
    if (validateDirPath(path, false) && !m_libraryPaths.contains(path)) {
        m_libraryPaths.append(path);
        m_modifiedDate = QDateTime::currentDateTime();
    }
}

void ProjectConfig::setLibraries(const QStringList& libs)
{
    m_libraries = libs;
    m_modifiedDate = QDateTime::currentDateTime();
}

void ProjectConfig::addLibrary(const QString& lib)
{
    if (!m_libraries.contains(lib)) {
        m_libraries.append(lib);
        m_modifiedDate = QDateTime::currentDateTime();
    }
}

void ProjectConfig::setDefines(const QStringList& defines)
{
    m_defines = defines;
    m_modifiedDate = QDateTime::currentDateTime();
}

void ProjectConfig::addDefine(const QString& define)
{
    if (!m_defines.contains(define)) {
        m_defines.append(define);
        m_modifiedDate = QDateTime::currentDateTime();
    }
}

void ProjectConfig::setExtraConfig(const QVariantMap& config)
{
    m_extraConfig = config;
    m_modifiedDate = QDateTime::currentDateTime();
}

void ProjectConfig::setExtraValue(const QString& key, const QVariant& value)
{
    m_extraConfig[key] = value;
    m_modifiedDate = QDateTime::currentDateTime();
}


QVariant ProjectConfig::getExtraValue(const QString& key, const QVariant& defaultValue) const
{
    return m_extraConfig.value(key, defaultValue);
}

// ==================== 私有验证方法 ====================

bool ProjectConfig::validateFilePath(const QString& path, bool checkExists) const
{
    if (path.isEmpty()) {
        return false;
    }

    // 检查路径是否包含非法字符（简化检查）
    static const QString illegalChars = "<>:\"|?*";
    for (const QChar& ch : illegalChars) {
        if (path.contains(ch)) {
            return false;
        }
    }

    // 如果是相对路径，相对于项目路径
    QString fullPath;
    if (QDir::isRelativePath(path)) {
        fullPath = QDir(m_projectPath).absoluteFilePath(path);
    } else {
        fullPath = path;
    }

    // 检查文件是否存在
    if (checkExists) {
        return QFileInfo::exists(fullPath);
    }

    return true;
}

bool ProjectConfig::validateDirPath(const QString& path, bool checkExists) const
{
    if (path.isEmpty()) {
        return false;
    }

    // 检查路径是否包含非法字符
    static const QString illegalChars = "<>:\"|?*";
    for (const QChar& ch : illegalChars) {
        if (path.contains(ch)) {
            return false;
        }
    }

    // 如果是相对路径，相对于项目路径
    QString fullPath;
    if (QDir::isRelativePath(path)) {
        fullPath = QDir(m_projectPath).absoluteFilePath(path);
    } else {
        fullPath = path;
    }

    // 检查目录是否存在
    if (checkExists) {
        return QDir(fullPath).exists();
    }

    return true;
}

// ==================== 配置文件定位 / 扩展名（v8.5 新增） ====================

QString ProjectConfig::configExtensionForLanguage(const QString& langId)
{
    const QString s = langId.toLower().trimmed();

    // C++ / Qt 保持 .lightcppproj：
    //   1. 向后兼容大量已有项目
    //   2. Qt 项目本质也是 CMake 项目，语义一致
    if (s == "cpp" || s == "c" || s == "c++" || s == "qt" || s.isEmpty()) {
        return QStringLiteral(".lightcppproj");
    }

    // Python / Go / Rust / Node / GenericAI 等用语义正确的扩展名
    return QStringLiteral(".sprintide.json");
}

bool ProjectConfig::isKnownConfigExtension(const QString& fileName)
{
    static const QStringList kExtensions = {
        QStringLiteral(".lightcppproj"),
        QStringLiteral(".sprintide.json"),
        QStringLiteral(".sprintide.toml")   // 预留：未来可能支持 TOML
    };
    const QString lower = fileName.toLower();
    for (const QString& ext : kExtensions) {
        if (lower.endsWith(ext)) return true;
    }
    return false;
}

QString ProjectConfig::findConfigFile(const QString& projectPath)
{
    QDir dir(projectPath);
    if (!dir.exists()) return QString();

    static const QStringList kExtensions = {
        QStringLiteral(".lightcppproj"),
        QStringLiteral(".sprintide.json"),
        QStringLiteral(".sprintide.toml")
    };

    // 1. 优先：<目录名>.<ext> —— 最常见约定
    const QString dirName = dir.dirName();
    for (const QString& ext : kExtensions) {
        const QString candidate = dir.absoluteFilePath(dirName + ext);
        if (QFileInfo::exists(candidate)) return candidate;
    }

    // 2. 次选：目录里任意一个已知扩展名的文件
    //    （用 entryList 而不是 QDirIterator，只扫当前层，不递归）
    for (const QString& ext : kExtensions) {
        const QStringList matches = dir.entryList(
            QStringList{ "*" + ext },
            QDir::Files | QDir::NoDotAndDotDot);
        if (!matches.isEmpty()) {
            return dir.absoluteFilePath(matches.first());
        }
    }

    return QString();
}

bool ProjectConfig::loadFromProjectDir(const QString& projectPath, QStringList& errors)
{
    const QString configPath = findConfigFile(projectPath);
    if (configPath.isEmpty()) {
        errors.append(
            QString("在项目目录里找不到配置文件"
                "（支持 .lightcppproj / .sprintide.json）: %1")
            .arg(projectPath));
        return false;
    }
    return loadFromFile(configPath, errors);
}

bool ProjectConfig::saveToProjectDir(QStringList& errors) const
{
    if (m_projectPath.isEmpty()) {
        errors.append("项目路径为空，无法保存配置");
        return false;
    }
    if (m_projectName.isEmpty()) {
        errors.append("项目名称为空，无法保存配置");
        return false;
    }

    const QString ext = configExtensionForLanguage(m_projectLanguage);
    const QString configPath =
    QDir(m_projectPath).absoluteFilePath(m_projectName + ext);

    return saveToFile(configPath, errors);
}

} // namespace SprintIDE
