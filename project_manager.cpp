// project_manager.cpp
#include "project_manager.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QApplication>
#include <QStandardPaths>

namespace SprintIDE {

ProjectManager* ProjectManager::m_instance = nullptr;

ProjectManager::ProjectManager(QObject *parent)
    : QObject(parent),
    m_currentProjectName(""),
    m_currentProjectPath(""),
    m_currentProjectConfigPath("")
{
    // 初始化设置管理器
    m_settings = new QSettings(QCoreApplication::organizationName(),
                               QCoreApplication::applicationName(), this);

    // 加载最近使用的项目列表
    loadRecentProjects();
}

ProjectManager::~ProjectManager()
{
    // 保存最近使用的项目列表
    saveRecentProjects();
}

ProjectManager* ProjectManager::instance()
{
    if (!m_instance) {
        m_instance = new ProjectManager;
    }
    return m_instance;
}

bool ProjectManager::createProject(const QString& projectName, const QString& projectPath, int projectType)
{
    // 检查项目名称是否合法
    if (projectName.isEmpty() || !isValidProjectName(projectName)) {
        qWarning() << "Invalid project name:" << projectName;
        return false;
    }

    // 检查项目路径是否存在
    QDir projectDir(projectPath);
    if (projectDir.exists()) {
        qWarning() << "Project directory already exists:" << projectPath;
        return false;
    }

    // 创建项目目录
    if (!projectDir.mkpath(".")) {
        qWarning() << "Failed to create project directory:" << projectPath;
        return false;
    }

    // 初始化项目结构
    if (!initProjectStructure(projectPath, projectType)) {
        qWarning() << "Failed to initialize project structure";
        projectDir.removeRecursively(); // 清理已创建的目录
        return false;
    }

    // 初始化默认配置
    initDefaultConfig();
    m_currentProjectName = projectName;
    m_currentProjectPath = projectPath;
    m_currentProjectConfigPath = projectPath + "/.lightcppproj";

    // 保存项目配置
    if (!saveProjectConfig(m_currentProjectConfigPath)) {
        qWarning() << "Failed to save project config";
        projectDir.removeRecursively(); // 清理已创建的目录
        return false;
    }

    // 更新最近使用的项目列表
    updateRecentProjects(m_currentProjectConfigPath);

    qDebug() << "Project created successfully:" << projectName;
    emit projectOpened(m_currentProjectName, m_currentProjectPath);

    return true;
}

bool ProjectManager::openProject(const QString& projectPath)
{
    // 检查路径是否存在
    if (!QFile::exists(projectPath)) {
        qWarning() << "Project path does not exist:" << projectPath;
        return false;
    }

    // 判断是项目目录还是项目配置文件
    QString configPath;
    QFileInfo fileInfo(projectPath);
    if (fileInfo.isDir()) {
        // 是目录，查找配置文件
        configPath = projectPath + "/.lightcppproj";
        if (!QFile::exists(configPath)) {
            qWarning() << "Project config file not found in directory:" << projectPath;
            return false;
        }
    } else {
        // 是文件，检查是否是配置文件
        if (fileInfo.suffix() != "lightcppproj") {
            qWarning() << "Not a project config file:" << projectPath;
            return false;
        }
        configPath = projectPath;
    }

    // 加载项目配置
    if (!loadProjectConfig(configPath)) {
        qWarning() << "Failed to load project config:" << configPath;
        return false;
    }

    // 更新当前项目信息
    m_currentProjectConfigPath = configPath;
    m_currentProjectPath = fileInfo.isDir() ? projectPath : fileInfo.dir().absolutePath();
    m_currentProjectName = m_currentProjectConfig.value("project_name").toString();

    // 更新最近使用的项目列表
    updateRecentProjects(m_currentProjectConfigPath);

    qDebug() << "Project opened successfully:" << m_currentProjectName;
    emit projectOpened(m_currentProjectName, m_currentProjectPath);

    return true;
}

bool ProjectManager::closeProject()
{
    if (!hasOpenProject()) {
        return true; // 没有打开的项目，直接返回成功
    }

    // 清空当前项目信息
    m_currentProjectName.clear();
    m_currentProjectPath.clear();
    m_currentProjectConfigPath.clear();
    m_currentProjectConfig.clear();

    qDebug() << "Project closed successfully";
    emit projectClosed();

    return true;
}

bool ProjectManager::saveProject()
{
    if (!hasOpenProject()) {
        qWarning() << "No open project to save";
        return false;
    }

    if (m_currentProjectConfigPath.isEmpty()) {
        // 还没有保存过，调用另存为
        QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        defaultPath += "/" + m_currentProjectName + "/.lightcppproj";
        return saveProjectAs(defaultPath);
    }

    // 保存项目配置
    if (!saveProjectConfig(m_currentProjectConfigPath)) {
        qWarning() << "Failed to save project config:" << m_currentProjectConfigPath;
        return false;
    }

    qDebug() << "Project saved successfully:" << m_currentProjectName;
    emit projectSaved(m_currentProjectName, m_currentProjectPath);

    return true;
}

bool ProjectManager::saveProjectAs(const QString& newPath)
{
    if (!hasOpenProject()) {
        qWarning() << "No open project to save as";
        return false;
    }

    // 检查路径是否合法
    QFileInfo fileInfo(newPath);
    if (fileInfo.suffix() != "lightcppproj") {
        qWarning() << "Invalid project config file suffix:" << newPath;
        return false;
    }

    // 保存项目配置
    if (!saveProjectConfig(newPath)) {
        qWarning() << "Failed to save project config as:" << newPath;
        return false;
    }

    // 更新当前项目配置路径
    m_currentProjectConfigPath = newPath;
    m_currentProjectPath = fileInfo.dir().absolutePath();

    // 更新最近使用的项目列表
    updateRecentProjects(m_currentProjectConfigPath);

    qDebug() << "Project saved as successfully:" << newPath;
    emit projectSaved(m_currentProjectName, m_currentProjectPath);

    return true;
}

QString ProjectManager::currentProjectName() const
{
    return m_currentProjectName;
}

QString ProjectManager::currentProjectPath() const
{
    return m_currentProjectPath;
}

QVariantMap ProjectManager::currentProjectConfig() const
{
    return m_currentProjectConfig;
}

bool ProjectManager::hasOpenProject() const
{
    return !m_currentProjectName.isEmpty() && !m_currentProjectPath.isEmpty();
}

QMap<QString, QDateTime> ProjectManager::recentProjects() const
{
    return m_recentProjects;
}

void ProjectManager::saveWindowState(const QByteArray& state)
{
    m_settings->setValue("window/state", state);
}

QByteArray ProjectManager::loadWindowState() const
{
    return m_settings->value("window/state").toByteArray();
}

void ProjectManager::saveWindowGeometry(const QByteArray& geometry)
{
    m_settings->setValue("window/geometry", geometry);
}

QByteArray ProjectManager::loadWindowGeometry() const
{
    return m_settings->value("window/geometry").toByteArray();
}

// ============== Private Methods ==============

void ProjectManager::initDefaultConfig()
{
    m_currentProjectConfig.clear();

    // 基本配置
    m_currentProjectConfig["project_name"] = m_currentProjectName;
    m_currentProjectConfig["display_name"] = m_currentProjectName;
    m_currentProjectConfig["version"] = "1.0.0";
    m_currentProjectConfig["created_date"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    m_currentProjectConfig["modified_date"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    // 编译配置
    QVariantMap compileConfig;
    compileConfig["compiler"] = "auto";
    compileConfig["cpp_standard"] = "c++17";
    compileConfig["optimization"] = "none";
    compileConfig["debug_info"] = true;
    compileConfig["warnings"] = "all";
    m_currentProjectConfig["compile"] = compileConfig;

    // 源文件列表
    QVariantList sourceFiles;
    sourceFiles << "main.cpp";
    m_currentProjectConfig["source_files"] = sourceFiles;

    // 包含路径
    QVariantList includePaths;
    includePaths << "." << "include";
    m_currentProjectConfig["include_paths"] = includePaths;
}

bool ProjectManager::loadProjectConfig(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open project config file:" << file.errorString();
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse project config file:" << parseError.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qWarning() << "Project config file is not a JSON object";
        return false;
    }

    QJsonObject json = doc.object();

    // 解析基本配置
    m_currentProjectConfig["project_name"] = json["project_name"].toString();
    m_currentProjectConfig["display_name"] = json["display_name"].toString();
    m_currentProjectConfig["version"] = json["version"].toString();
    m_currentProjectConfig["created_date"] = json["created_date"].toString();
    m_currentProjectConfig["modified_date"] = json["modified_date"].toString();

    // 解析编译配置
    if (json.contains("compile")) {
        QJsonObject compileJson = json["compile"].toObject();
        QVariantMap compileConfig;
        compileConfig["compiler"] = compileJson["compiler"].toString();
        compileConfig["cpp_standard"] = compileJson["cpp_standard"].toString();
        compileConfig["optimization"] = compileJson["optimization"].toString();
        compileConfig["debug_info"] = compileJson["debug_info"].toBool();
        compileConfig["warnings"] = compileJson["warnings"].toString();
        m_currentProjectConfig["compile"] = compileConfig;
    }

    // 解析源文件列表
    if (json.contains("source_files")) {
        QJsonArray sourceFilesJson = json["source_files"].toArray();
        QVariantList sourceFiles;
        for (const QJsonValue& value : sourceFilesJson) {
            sourceFiles << value.toString();
        }
        m_currentProjectConfig["source_files"] = sourceFiles;
    }

    // 解析包含路径
    if (json.contains("include_paths")) {
        QJsonArray includePathsJson = json["include_paths"].toArray();
        QVariantList includePaths;
        for (const QJsonValue& value : includePathsJson) {
            includePaths << value.toString();
        }
        m_currentProjectConfig["include_paths"] = includePaths;
    }

    return true;
}

bool ProjectManager::saveProjectConfig(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open project config file for writing:" << file.errorString();
        return false;
    }

    // 创建JSON对象
    QJsonObject json;

    // 基本配置
    json["project_name"] = m_currentProjectConfig.value("project_name").toString();
    json["display_name"] = m_currentProjectConfig.value("display_name").toString();
    json["version"] = m_currentProjectConfig.value("version").toString();
    json["created_date"] = m_currentProjectConfig.value("created_date").toString();
    json["modified_date"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    // 编译配置
    if (m_currentProjectConfig.contains("compile")) {
        QVariantMap compileConfig = m_currentProjectConfig.value("compile").toMap();
        QJsonObject compileJson;
        compileJson["compiler"] = compileConfig.value("compiler").toString();
        compileJson["cpp_standard"] = compileConfig.value("cpp_standard").toString();
        compileJson["optimization"] = compileConfig.value("optimization").toString();
        compileJson["debug_info"] = compileConfig.value("debug_info").toBool();
        compileJson["warnings"] = compileConfig.value("warnings").toString();
        json["compile"] = compileJson;
    }

    // 源文件列表
    if (m_currentProjectConfig.contains("source_files")) {
        QVariantList sourceFiles = m_currentProjectConfig.value("source_files").toList();
        QJsonArray sourceFilesJson;
        for (const QVariant& value : sourceFiles) {
            sourceFilesJson << value.toString();
        }
        json["source_files"] = sourceFilesJson;
    }

    // 包含路径
    if (m_currentProjectConfig.contains("include_paths")) {
        QVariantList includePaths = m_currentProjectConfig.value("include_paths").toList();
        QJsonArray includePathsJson;
        for (const QVariant& value : includePaths) {
            includePathsJson << value.toString();
        }
        json["include_paths"] = includePathsJson;
    }

    // 写入文件
    QJsonDocument doc(json);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

void ProjectManager::loadRecentProjects()
{
    m_recentProjects.clear();

    int count = m_settings->beginReadArray("recent_projects");
    for (int i = 0; i < count; ++i) {
        m_settings->setArrayIndex(i);
        QString path = m_settings->value("path").toString();
        QString dateStr = m_settings->value("date").toString();
        QDateTime date = QDateTime::fromString(dateStr, Qt::ISODate);
        if (!path.isEmpty() && date.isValid()) {
            m_recentProjects[path] = date;
        }
    }
    m_settings->endArray();
}

void ProjectManager::saveRecentProjects() const
{
    m_settings->beginWriteArray("recent_projects");
    int index = 0;
    for (auto it = m_recentProjects.constBegin(); it != m_recentProjects.constEnd(); ++it) {
        m_settings->setArrayIndex(index++);
        m_settings->setValue("path", it.key());
        m_settings->setValue("date", it.value().toString(Qt::ISODate));
    }
    m_settings->endArray();
}

bool ProjectManager::isValidProjectName(const QString& projectName) const
{
    // 检查项目名称是否符合C++标识符规则
    if (projectName.isEmpty()) {
        return false;
    }

    // 第一个字符必须是字母或下划线
    QChar firstChar = projectName.at(0);
    if (!firstChar.isLetter() && firstChar != '_') {
        return false;
    }

    // 后续字符可以是字母、数字或下划线
    for (int i = 1; i < projectName.length(); ++i) {
        QChar ch = projectName.at(i);
        if (!ch.isLetterOrNumber() && ch != '_') {
            return false;
        }
    }

    // 不能是C++关键字
    static const QStringList cppKeywords = {
        "auto", "break", "case", "char", "const", "continue", "default",
        "do", "double", "else", "enum", "extern", "float", "for", "goto",
        "if", "int", "long", "register", "return", "short", "signed",
        "sizeof", "static", "struct", "switch", "typedef", "union",
        "unsigned", "void", "volatile", "while"
    };

    return !cppKeywords.contains(projectName.toLower());
}

void ProjectManager::updateRecentProjects(const QString& projectPath)
{
    // 更新最近使用时间
    m_recentProjects[projectPath] = QDateTime::currentDateTime();

    // 最多保留10个最近项目
    if (m_recentProjects.size() > 10) {
        // 找到最早的项目
        QString earliestPath;
        QDateTime earliestDate;
        for (auto it = m_recentProjects.constBegin(); it != m_recentProjects.constEnd(); ++it) {
            if (earliestPath.isEmpty() || it.value() < earliestDate) {
                earliestPath = it.key();
                earliestDate = it.value();
            }
        }
        // 删除最早的项目
        m_recentProjects.remove(earliestPath);
    }

    // 保存到设置
    saveRecentProjects();
}

bool ProjectManager::initProjectStructure(const QString& projectPath, int projectType)
{
    QDir projectDir(projectPath);

    // 创建基本目录结构
    QStringList directories = {
        "src",
        "include",
        "bin",
        "docs"
    };

    for (const QString& dir : directories) {
        if (!projectDir.mkdir(dir)) {
            qWarning() << "Failed to create directory:" << dir;
            return false;
        }
    }

    // 根据项目类型创建特定目录
    switch (projectType) {
    case 1: // 简单游戏
        projectDir.mkdir("assets");
        break;
    case 2: // GUI应用
        projectDir.mkdir("ui");
        projectDir.mkdir("resources");
        break;
    default:
        break;
    }

    // 生成项目模板文件
    if (!generateProjectTemplate(projectPath, projectType)) {
        qWarning() << "Failed to generate project template";
        return false;
    }

    return true;
}

bool ProjectManager::generateProjectTemplate(const QString& projectPath, int projectType)
{
    QDir projectDir(projectPath);

    // 生成main.cpp
    QString mainCppContent;
    switch (projectType) {
    case 0: // 控制台应用
        mainCppContent = R"(
#include <iostream>

int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
)";
        break;
    case 1: // 简单游戏
        mainCppContent = R"(
#include <iostream>
#include <cstdlib>
#include <ctime>

int main() {
    srand(time(nullptr));
    int secretNumber = rand() % 100 + 1;
    int guess;
    int attempts = 0;

    std::cout << "猜数字游戏！" << std::endl;
    std::cout << "我已经想好了一个1到100之间的数字。" << std::endl;

    do {
        std::cout << "请输入你的猜测：";
        std::cin >> guess;
        attempts++;

        if (guess < secretNumber) {
            std::cout << "太小了！再试一次。" << std::endl;
        } else if (guess > secretNumber) {
            std::cout << "太大了！再试一次。" << std::endl;
        } else {
            std::cout << "恭喜你，猜对了！你用了" << attempts << "次尝试。" << std::endl;
        }
    } while (guess != secretNumber);

    return 0;
}
)";
        break;
    case 2: // GUI应用
        mainCppContent = R"(
#include <QApplication>
#include <QMainWindow>
#include <QLabel>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle("SprintIDE - GUI应用");
    window.setGeometry(100, 100, 400, 300);

    QLabel *label = new QLabel("Hello, GUI World!", &window);
    label->setGeometry(100, 100, 200, 50);

    window.show();

    return app.exec();
}
)";
        break;
    default: // 空项目
        mainCppContent = R"(
int main() {
    return 0;
}
)";
        break;
    }

    // 写入文件
    QFile mainCppFile(projectDir.absoluteFilePath("src/main.cpp"));
    if (!mainCppFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to create main.cpp";
        return false;
    }
    mainCppFile.write(mainCppContent.toUtf8());
    mainCppFile.close();

    // 生成README.md
    QString readmeContent = QString(R"(
# %1

这是一个使用 SprintIDE 创建的项目。

## 项目结构

- src/：源代码目录
- include/：头文件目录
- bin/：编译输出目录
- docs/：文档目录

## 编译和运行

1. 在 SprintIDE 中打开该项目
2. 点击编译按钮
3. 点击运行按钮
)").arg(m_currentProjectName);

    QFile readmeFile(projectDir.absoluteFilePath("README.md"));
    if (readmeFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        readmeFile.write(readmeContent.toUtf8());
        readmeFile.close();
    }

    return true;
}



} // namespace SprintIDE
