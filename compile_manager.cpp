#include "compile_manager.h"
#include "path_utils.h"
#include "project_config.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QCoreApplication>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSettings>
#include <QDirIterator>

namespace SprintIDE {

// ==================== Private 类定义 ====================

class CompileManager::Private {
public:
    Private(CompileManager* q) : q_ptr(q) {
        // 初始化进程管理器
        processManager = new ProcessManager(q);

        // 连接信号 - 使用lambda表达式处理参数不匹配
        QObject::connect(processManager, &ProcessManager::processStarted,
                         q, [q](const QString& processId) {
                             // 查找对应的任务，获取源文件信息
                             // 这里简化处理，直接发射信号
                             emit q->compileStarted(processId, "编译中...");
                         });

        QObject::connect(processManager, &ProcessManager::processFinished,
                         q, &CompileManager::onProcessFinished);
        QObject::connect(processManager, &ProcessManager::processError,
                         q, &CompileManager::compileError);
        QObject::connect(processManager, &ProcessManager::outputReceived,
                         q, &CompileManager::onOutputReceived);
        QObject::connect(processManager, &ProcessManager::progressUpdated,
                         q, &CompileManager::onProgressUpdated);
    }

    ~Private() {
        // 清理编译结果缓存
        compileResults.clear();
        // 进程管理器会在父对象销毁时自动清理
    }

    // 成员变量
    ProcessManager* processManager;                    ///< 进程管理器
    QMap<QString, CompileResult> compileResults;       ///< 编译结果缓存
    QMap<QString, CompileTask> compileTasks;           ///< 编译任务缓存
    QMap<QString, QStringList> compilerOutputs;        ///< 编译器输出缓存

    // 编译器检测结果
    QStringList availableCompilers;                    ///< 可用的编译器列表
    CompilerType defaultCompilerType = CompilerType::AutoDetect; ///< 默认编译器类型
    QString defaultCompilerPath;                       ///< 默认编译器路径

    // 互斥锁
    QMutex resultMutex;                               ///< 结果缓存互斥锁
    QMutex taskMutex;                                 ///< 任务缓存互斥锁

    // 编译配置
    QSettings* settings = nullptr;                     ///< 设置管理器

private:
    CompileManager* q_ptr;                            ///< 指向公共接口的指针
};

// ==================== 构造函数和析构函数 ====================

CompileManager::CompileManager(QObject* parent)
    : QObject(parent)
    , d(new Private(this))
{
    qDebug() << "编译管理器创建";
}

CompileManager::~CompileManager()
{
    qDebug() << "编译管理器销毁";
    // delete d; d是QScopedPointer<Private>对象， 智能指针，会自动管理内存，离开作用域时，会自动 delete 内部对象
    d.reset();//QScopedPointer<Private>对象手动释放资源语句
}

CompileManager& CompileManager::instance()
{
    static CompileManager instance;
    return instance;
}

// ==================== 初始化方法 ====================

bool CompileManager::initialize()
{
    qDebug() << "初始化编译管理器";

    // 初始化进程管理器
    d->processManager->clearHistory();

    // 加载编译器配置
    loadCompileConfig();

    // 检测可用编译器
    QStringList compilers = detectAvailableCompilers();
    if (!compilers.isEmpty()) {
        d->availableCompilers = compilers;
        emit compilersDetected(compilers);
        qDebug() << "检测到编译器:" << compilers;
    } else {
        qWarning() << "未检测到可用的C++编译器";
        // 对于儿童编程环境，可以尝试使用内置的简化编译器
        qDebug() << "将使用简化的编译模式";
    }

    // 创建编译日志目录
    QString logDir = PathUtils::joinPaths({PathUtils::platformDataPath(), "compile_logs"});
    PathUtils::ensureDirExists(logDir);

    qDebug() << "编译管理器初始化完成";
    return true;
}

// ==================== 编译方法 ====================

QString CompileManager::compileSingleFile(const QString& sourceFile,
                                          const QString& outputName,
                                          CompileMode compileMode)
{
    qDebug() << "开始编译单个文件:" << sourceFile;

    // 验证源文件
    if (!PathUtils::isValidFilePath(sourceFile, true)) {
        QString error = QString("源文件无效或不存在: %1").arg(sourceFile);
        qWarning() << error;
        emit compileError("", error);
        return QString();
    }

    // 创建编译任务
    CompileTask task;
    task.taskId = QString("single_%1_%2").arg(
        QFileInfo(sourceFile).fileName(),
        QString::number(QDateTime::currentMSecsSinceEpoch()));
    task.sourceFiles = QStringList(sourceFile);
    task.outputName = outputName.isEmpty() ?
                          PathUtils::fileNameWithoutExtension(sourceFile) : outputName;
    task.outputDir = "bin";
    task.targetType = TargetType::Executable;
    task.compileMode = compileMode;
    task.compilerType = d->defaultCompilerType;
    task.compilerPath = d->defaultCompilerPath;
    task.cppStandard = "c++17";
    task.enableDebugInfo = (compileMode == CompileMode::Debug);
    task.enableWarnings = true;
    task.treatWarningsAsErrors = false;
    task.optimizeForSpeed = (compileMode == CompileMode::Release);
    task.optimizeForSize = false;
    task.verboseOutput = true;

    // 添加默认包含路径
    task.includePaths << ".";
    task.includePaths << "include";

    // 验证编译任务
    QStringList errors;
    if (!validateCompileTask(task, errors)) {
        QString errorMsg = QString("编译任务验证失败:\n%1").arg(errors.join("\n"));
        qWarning() << errorMsg;
        emit compileError(task.taskId, errorMsg);
        return QString();
    }

    // 执行编译任务
    executeCompileTask(task);

    return task.taskId;
}

QString CompileManager::compileProject(const ProjectConfig& projectConfig,
                                       CompileMode compileMode)
{
    qDebug() << "开始编译项目:" << projectConfig.projectName();

    // 验证项目配置
    QStringList configErrors;
    if (!projectConfig.validate(configErrors)) {
        QString errorMsg = QString("项目配置验证失败:\n%1").arg(configErrors.join("\n"));
        qWarning() << errorMsg;
        emit compileError("", errorMsg);
        return QString();
    }

    // 创建编译任务
    CompileTask task;
    task.taskId = QString("project_%1_%2").arg(
        projectConfig.projectName(),
        QString::number(QDateTime::currentMSecsSinceEpoch()));
    task.projectPath = projectConfig.projectPath();
    task.sourceFiles = projectConfig.sourceFiles();
    task.headerFiles = projectConfig.headerFiles();
    task.outputName = projectConfig.projectName();
    task.outputDir = projectConfig.outputPath();
    task.targetType = TargetType::Executable;  // 默认可执行文件
    task.compileMode = compileMode;

    // 从项目配置获取编译器设置
    QString compilerTypeStr;
    switch (projectConfig.projectCompilerType()) {
    case ProjectCompilerType::GCC: task.compilerType = CompilerType::GCC; break;
    case ProjectCompilerType::Clang: task.compilerType = CompilerType::Clang; break;
    case ProjectCompilerType::MSVC: task.compilerType = CompilerType::MSVC; break;
    default: task.compilerType = d->defaultCompilerType; break;
    }

    task.compilerPath = projectConfig.compilerPath();
    task.cppStandard = projectConfig.cppStandard();
    task.enableDebugInfo = projectConfig.enableDebugInfo();
    task.enableWarnings = true;  // 默认启用警告

    // 优化设置
    QString optimizationLevel = projectConfig.optimizationLevel();
    if (optimizationLevel == "none") {
        task.optimizeForSpeed = false;
        task.optimizeForSize = false;
    } else if (optimizationLevel == "fast" || optimizationLevel == "faster" || optimizationLevel == "fastest") {
        task.optimizeForSpeed = true;
        task.optimizeForSize = false;
    } else if (optimizationLevel == "size") {
        task.optimizeForSpeed = false;
        task.optimizeForSize = true;
    }

    // 包含路径和库
    task.includePaths = projectConfig.includePaths();
    task.libraryPaths = projectConfig.libraryPaths();
    task.libraries = projectConfig.libraries();
    task.defines = projectConfig.defines();

    // 验证编译任务
    QStringList errors;
    if (!validateCompileTask(task, errors)) {
        QString errorMsg = QString("编译任务验证失败:\n%1").arg(errors.join("\n"));
        qWarning() << errorMsg;
        emit compileError(task.taskId, errorMsg);
        return QString();
    }

    // 执行编译任务
    executeCompileTask(task);

    return task.taskId;
}

QString CompileManager::compileCustomTask(const CompileTask& task)
{
    qDebug() << "开始自定义编译任务";

    // 验证编译任务
    QStringList errors;
    if (!validateCompileTask(task, errors)) {
        QString errorMsg = QString("编译任务验证失败:\n%1").arg(errors.join("\n"));
        qWarning() << errorMsg;
        emit compileError("", errorMsg);
        return QString();
    }

    // 复制任务并生成任务ID（如果未提供）
    CompileTask actualTask = task;
    if (actualTask.taskId.isEmpty()) {
        actualTask.taskId = QString("custom_%1").arg(
            QString::number(QDateTime::currentMSecsSinceEpoch()));
    }

    // 执行编译任务
    executeCompileTask(actualTask);

    return actualTask.taskId;
}

// ==================== 编译控制方法 ====================

bool CompileManager::cancelCompile(const QString& taskId)
{
    qDebug() << "取消编译任务:" << taskId;

    QMutexLocker locker(&d->taskMutex);

    if (!d->compileTasks.contains(taskId)) {
        qWarning() << "取消编译失败: 未找到任务ID" << taskId;
        return false;
    }

    // 停止对应的进程
    bool stopped = d->processManager->stopProcess(taskId);

    if (stopped) {
        qDebug() << "编译任务已取消:" << taskId;
        emit compileError(taskId, "编译已取消");
    }

    return stopped;
}

CompileResult CompileManager::getCompileResult(const QString& taskId) const
{
    QMutexLocker locker(&d->resultMutex);

    if (d->compileResults.contains(taskId)) {
        return d->compileResults[taskId];
    }

    // 返回默认结果
    CompileResult result;
    result.success = false;
    result.errorMessage = QString("未找到编译结果，任务ID: %1").arg(taskId);
    return result;
}

bool CompileManager::cleanBuild(const QString& projectPath, const QString& outputDir)
{
    qDebug() << "清理编译产物，项目路径:" << projectPath << "输出目录:" << outputDir;

    QString fullOutputPath = PathUtils::absolutePath(outputDir, projectPath);

    if (!PathUtils::isValidDirPath(fullOutputPath, true)) {
        qWarning() << "输出目录不存在:" << fullOutputPath;
        return false;
    }

    // 要删除的文件扩展名列表
    QStringList extensions = {
        // 可执行文件
        "", ".exe", ".app", ".out",
        // 目标文件
        ".o", ".obj",
        // 库文件
        ".a", ".so", ".dll", ".dylib", ".lib",
        // 编译中间文件
        ".gcno", ".gcda",
        // 其他文件
        ".ilk", ".pdb", ".idb", ".pch"
    };

    int deletedCount = 0;
    QDir outputDirObj(fullOutputPath);

    // 递归遍历目录
    QDirIterator it(fullOutputPath, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString filePath = it.next();
        QFileInfo fileInfo(filePath);

        // 检查文件扩展名
        bool shouldDelete = false;
        for (const QString& ext : extensions) {
            if (ext.isEmpty()) {
                // 无扩展名的文件（可能是Unix可执行文件）
                if (!fileInfo.suffix().isEmpty()) {
                    continue;
                }
                shouldDelete = true;
            } else if (fileInfo.suffix() == ext.mid(1)) {
                shouldDelete = true;
                break;
            }
        }

        if (shouldDelete) {
            if (QFile::remove(filePath)) {
                deletedCount++;
                qDebug() << "已删除文件:" << filePath;
            } else {
                qWarning() << "无法删除文件:" << filePath;
            }
        }
    }

    qDebug() << QString("清理完成，删除了 %1 个文件").arg(deletedCount);
    return deletedCount > 0;
}

// ==================== 编译器检测和配置 ====================

QStringList CompileManager::detectAvailableCompilers() const
{
    qDebug() << "检测可用编译器";

    QStringList compilers;
    QStringList testCommands;

    // 定义要测试的编译器命令
#ifdef Q_OS_WIN
    testCommands << "g++" << "clang++" << "cl.exe" << "gcc" << "clang";
#elif defined(Q_OS_MAC)
    testCommands << "clang++" << "g++" << "clang" << "gcc";
#else
    testCommands << "g++" << "clang++" << "gcc" << "clang";
#endif

    // 测试每个编译器
    for (const QString& cmd : testCommands) {
        ProcessManager::ProcessResult result = d->processManager->executeSync(
            cmd, QStringList() << "--version", QString(), 3000);

        if (result.success && !result.standardOutput.isEmpty()) {
            // 成功获取版本信息
            compilers << cmd;

            // 解析编译器信息
            QString info = result.standardOutput.split('\n').first();
            qDebug() << QString("检测到编译器: %1 - %2").arg(cmd).arg(info);
        }
    }

    // 如果未检测到任何编译器，尝试使用绝对路径
    if (compilers.isEmpty()) {
        QStringList commonPaths;

#ifdef Q_OS_WIN
        commonPaths << "C:/mingw64/bin/g++.exe"
                    << "C:/mingw/bin/g++.exe"
                    << "C:/Program Files/LLVM/bin/clang++.exe"
                    << "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.29.30133/bin/Hostx64/x64/cl.exe";
#elif defined(Q_OS_MAC)
        commonPaths << "/usr/bin/clang++"
                    << "/usr/local/bin/g++"
                    << "/opt/homebrew/bin/g++";
#else
        commonPaths << "/usr/bin/g++"
                    << "/usr/bin/clang++"
                    << "/usr/local/bin/g++"
                    << "/usr/local/bin/clang++";
#endif

        for (const QString& path : commonPaths) {
            if (QFile::exists(path)) {
                compilers << path;
                qDebug() << "检测到编译器(绝对路径):" << path;
            }
        }
    }

    return compilers;
}

QString CompileManager::getCompilerInfo(const QString& compilerPath)
{
    if (compilerPath.isEmpty()) {
        return "编译器路径为空";
    }

    ProcessManager::ProcessResult result = d->processManager->executeSync(
        compilerPath, QStringList() << "--version", QString(), 3000);

    if (result.success) {
        // 提取第一行作为基本信息
        QStringList lines = result.standardOutput.split('\n');
        if (!lines.isEmpty()) {
            return lines.first().trimmed();
        }
    }

    return QString("无法获取编译器信息: %1").arg(compilerPath);
}

bool CompileManager::setDefaultCompiler(CompilerType compilerType, const QString& compilerPath)
{
    d->defaultCompilerType = compilerType;

    if (!compilerPath.isEmpty()) {
        // 验证编译器路径
        if (!PathUtils::isValidFilePath(compilerPath, true)) {
            qWarning() << "无效的编译器路径:" << compilerPath;
            return false;
        }
        d->defaultCompilerPath = compilerPath;
    } else {
        // 自动选择编译器
        QStringList compilers = detectAvailableCompilers();
        if (compilers.isEmpty()) {
            qWarning() << "未找到可用的编译器";
            return false;
        }

        // 根据编译器类型选择
        QString selectedCompiler;
        for (const QString& compiler : compilers) {
            QString lowerCompiler = compiler.toLower();
            switch (compilerType) {
            case CompilerType::GCC:
            case CompilerType::GPP:
                if (lowerCompiler.contains("g++") || lowerCompiler.contains("gcc")) {
                    selectedCompiler = compiler;
                    break;
                }
                break;
            case CompilerType::Clang:
            case CompilerType::ClangPP:
                if (lowerCompiler.contains("clang++") || lowerCompiler.contains("clang")) {
                    selectedCompiler = compiler;
                    break;
                }
                break;
            case CompilerType::MSVC:
                if (lowerCompiler.contains("cl.exe")) {
                    selectedCompiler = compiler;
                    break;
                }
                break;
            default:
                // 使用第一个检测到的编译器
                selectedCompiler = compilers.first();
                break;
            }

            if (!selectedCompiler.isEmpty()) {
                break;
            }
        }

        if (selectedCompiler.isEmpty()) {
            selectedCompiler = compilers.first();
        }

        d->defaultCompilerPath = selectedCompiler;
    }

    qDebug() << QString("设置默认编译器: 类型=%1, 路径=%2")
                    .arg(static_cast<int>(compilerType))
                    .arg(d->defaultCompilerPath);

    // 保存配置
    saveCompileConfig();

    return true;
}

QString CompileManager::getCompilerCommand(CompilerType compilerType)
{
    switch (compilerType) {
    case CompilerType::GCC:
        return "gcc";
    case CompilerType::GPP:
        return "g++";
    case CompilerType::Clang:
        return "clang";
    case CompilerType::ClangPP:
        return "clang++";
    case CompilerType::MSVC:
        return "cl.exe";
    case CompilerType::MinGW:
#ifdef Q_OS_WIN
        return "g++.exe";
#else
        return "g++";
#endif
    default:
        // 自动检测，默认为g++
        return "g++";
    }
}

// ==================== 编译器参数生成 ====================

QStringList CompileManager::generateCompilerArgs(const CompileTask& task)
{
    QStringList args;

    // 确定编译器
    QString compiler = task.compilerPath;
    if (compiler.isEmpty()) {
        compiler = getCompilerCommand(task.compilerType);
    }

    // C++标准
    if (!task.cppStandard.isEmpty()) {
        args << "-std=" + task.cppStandard;
    }

    // 调试信息
    if (task.enableDebugInfo) {
        args << "-g";
    }

    // 警告设置
    if (task.enableWarnings) {
        args << "-Wall" << "-Wextra";

        if (task.treatWarningsAsErrors) {
            args << "-Werror";
        }
    }

    // 优化设置
    if (task.optimizeForSpeed) {
        args << "-O2";
    } else if (task.optimizeForSize) {
        args << "-Os";
    }

    // 包含路径
    for (const QString& includePath : task.includePaths) {
        QString fullPath = PathUtils::absolutePath(includePath, task.projectPath);
        args << "-I" + fullPath;
    }

    // 宏定义
    for (const QString& define : task.defines) {
        args << "-D" + define;
    }

    // 额外参数
    args << task.extraArgs;

    // 输出文件
    QString outputFile;
    QString outputDir = PathUtils::absolutePath(task.outputDir, task.projectPath);
    PathUtils::ensureDirExists(outputDir);

    switch (task.targetType) {
    case TargetType::Executable:
        outputFile = task.outputName;
#ifdef Q_OS_WIN
        if (!outputFile.endsWith(".exe")) {
            outputFile += ".exe";
        }
#endif
        break;
    case TargetType::StaticLibrary:
        outputFile = "lib" + task.outputName + ".a";
        break;
    case TargetType::DynamicLibrary:
#ifdef Q_OS_WIN
        outputFile = task.outputName + ".dll";
#elif defined(Q_OS_MAC)
        outputFile = "lib" + task.outputName + ".dylib";
#else
        outputFile = "lib" + task.outputName + ".so";
#endif
        break;
    case TargetType::ObjectFile:
        // 编译为单个目标文件（不链接）
        for (const QString& sourceFile : task.sourceFiles) {
            QString objectFile = PathUtils::fileNameWithoutExtension(sourceFile) + ".o";
            args << "-c" << sourceFile << "-o" << (outputDir + "/" + objectFile);
        }
        return args;
    }

    // 源文件
    for (const QString& sourceFile : task.sourceFiles) {
        QString fullPath = PathUtils::absolutePath(sourceFile, task.projectPath);
        args << fullPath;
    }

    // 库路径和库
    for (const QString& libPath : task.libraryPaths) {
        QString fullPath = PathUtils::absolutePath(libPath, task.projectPath);
        args << "-L" + fullPath;
    }

    for (const QString& lib : task.libraries) {
        args << "-l" + lib;
    }

    // 输出选项
    if (!outputFile.isEmpty()) {
        QString fullOutputPath = PathUtils::joinPaths({outputDir, outputFile});
        args << "-o" << fullOutputPath;
    }

    // 详细输出
    if (task.verboseOutput) {
        args << "-v";
    }

    return args;
}

// ==================== 编译器输出解析 ====================

CompileResult CompileManager::parseCompilerOutput(const QString& output, const QString& baseDir)
{
    CompileResult result;
    result.success = true;  // 默认成功，遇到错误时设为false

    QStringList lines = output.split('\n');
    QRegularExpression errorRegex(
        // 匹配错误格式: filename:line:column: error: message
        "(.*?):(\\d+):(\\d+):\\s+(fatal )?error:\\s+(.*)",
        QRegularExpression::CaseInsensitiveOption);

    QRegularExpression warningRegex(
        // 匹配警告格式: filename:line:column: warning: message
        "(.*?):(\\d+):(\\d+):\\s+warning:\\s+(.*)",
        QRegularExpression::CaseInsensitiveOption);

    QRegularExpression infoRegex(
        // 匹配信息格式: filename:line:column: note: message
        "(.*?):(\\d+):(\\d+):\\s+note:\\s+(.*)",
        QRegularExpression::CaseInsensitiveOption);

    // GCC/Clang的链接错误格式
    QRegularExpression linkerErrorRegex(
        "undefined reference to `(.*)'",
        QRegularExpression::CaseInsensitiveOption);

    // MSVC错误格式
    QRegularExpression msvcErrorRegex(
        "(.*)\\((\\d+)\\)\\s*:\\s*(fatal )?error\\s+C\\d+:\\s*(.*)",
        QRegularExpression::CaseInsensitiveOption);

    QRegularExpression msvcWarningRegex(
        "(.*)\\((\\d+)\\)\\s*:\\s*warning\\s+C\\d+:\\s*(.*)",
        QRegularExpression::CaseInsensitiveOption);

    for (const QString& line : lines) {
        if (line.trimmed().isEmpty()) {
            continue;
        }

        // 检查GCC/Clang错误
        QRegularExpressionMatch errorMatch = errorRegex.match(line);
        if (errorMatch.hasMatch()) {
            QString file = errorMatch.captured(1);
            int lineNum = errorMatch.captured(2).toInt();
            int col = errorMatch.captured(3).toInt();
            QString message = errorMatch.captured(5);

            // 如果是相对路径，转换为绝对路径
            if (!baseDir.isEmpty() && QDir::isRelativePath(file)) {
                file = PathUtils::absolutePath(file, baseDir);
            }

            QString errorMsg = QString("%1:%2:%3: 错误: %4")
                                   .arg(file)
                                   .arg(lineNum)
                                   .arg(col)
                                   .arg(message);
            result.errors.append(errorMsg);
            result.success = false;
            continue;
        }

        // 检查GCC/Clang警告
        QRegularExpressionMatch warningMatch = warningRegex.match(line);
        if (warningMatch.hasMatch()) {
            QString file = warningMatch.captured(1);
            int lineNum = warningMatch.captured(2).toInt();
            int col = warningMatch.captured(3).toInt();
            QString message = warningMatch.captured(4);

            if (!baseDir.isEmpty() && QDir::isRelativePath(file)) {
                file = PathUtils::absolutePath(file, baseDir);
            }

            QString warningMsg = QString("%1:%2:%3: 警告: %4")
                                     .arg(file)
                                     .arg(lineNum)
                                     .arg(col)
                                     .arg(message);
            result.warnings.append(warningMsg);
            continue;
        }

        // 检查GCC/Clang信息
        QRegularExpressionMatch infoMatch = infoRegex.match(line);
        if (infoMatch.hasMatch()) {
            QString file = infoMatch.captured(1);
            int lineNum = infoMatch.captured(2).toInt();
            int col = infoMatch.captured(3).toInt();
            QString message = infoMatch.captured(4);

            if (!baseDir.isEmpty() && QDir::isRelativePath(file)) {
                file = PathUtils::absolutePath(file, baseDir);
            }

            QString infoMsg = QString("%1:%2:%3: 信息: %4")
                                  .arg(file)
                                  .arg(lineNum)
                                  .arg(col)
                                  .arg(message);
            result.infos.append(infoMsg);
            continue;
        }

        // 检查链接错误
        QRegularExpressionMatch linkerMatch = linkerErrorRegex.match(line);
        if (linkerMatch.hasMatch()) {
            QString symbol = linkerMatch.captured(1);
            QString errorMsg = QString("链接错误: 未定义的引用 `%1'").arg(symbol);
            result.errors.append(errorMsg);
            result.success = false;
            continue;
        }

        // 检查MSVC错误
        QRegularExpressionMatch msvcErrorMatch = msvcErrorRegex.match(line);
        if (msvcErrorMatch.hasMatch()) {
            QString file = msvcErrorMatch.captured(1);
            int lineNum = msvcErrorMatch.captured(2).toInt();
            bool isFatal = !msvcErrorMatch.captured(3).isEmpty();
            QString message = msvcErrorMatch.captured(4);

            if (!baseDir.isEmpty() && QDir::isRelativePath(file)) {
                file = PathUtils::absolutePath(file, baseDir);
            }

            QString errorType = isFatal ? "致命错误" : "错误";
            QString errorMsg = QString("%1(%2): %3: %4")
                                   .arg(file)
                                   .arg(lineNum)
                                   .arg(errorType)
                                   .arg(message);
            result.errors.append(errorMsg);
            result.success = false;
            continue;
        }

        // 检查MSVC警告
        QRegularExpressionMatch msvcWarningMatch = msvcWarningRegex.match(line);
        if (msvcWarningMatch.hasMatch()) {
            QString file = msvcWarningMatch.captured(1);
            int lineNum = msvcWarningMatch.captured(2).toInt();
            QString message = msvcWarningMatch.captured(3);

            if (!baseDir.isEmpty() && QDir::isRelativePath(file)) {
                file = PathUtils::absolutePath(file, baseDir);
            }

            QString warningMsg = QString("%1(%2): 警告: %3")
                                     .arg(file)
                                     .arg(lineNum)
                                     .arg(message);
            result.warnings.append(warningMsg);
            continue;
        }

        // 其他未分类的输出
        if (line.contains("error", Qt::CaseInsensitive)) {
            result.errors.append(line.trimmed());
            result.success = false;
        } else if (line.contains("warning", Qt::CaseInsensitive)) {
            result.warnings.append(line.trimmed());
        } else {
            result.infos.append(line.trimmed());
        }
    }

    return result;
}

// ==================== 私有方法实现 ====================

void CompileManager::executeCompileTask(const CompileTask& task)
{
    qDebug() << "执行编译任务:" << task.taskId;

    // 保存任务
    {
        QMutexLocker locker(&d->taskMutex);
        d->compileTasks[task.taskId] = task;
    }

    // 初始化输出缓存
    d->compilerOutputs[task.taskId] = QStringList();

    // 构建编译命令
    QString compiler = task.compilerPath.isEmpty() ?
                           autoSelectCompiler(task) : task.compilerPath;
    QStringList args = generateCompilerArgs(task);

    // 构建完整命令字符串（用于日志和调试）
    QString fullCommand = compiler + " " + args.join(" ");
    qDebug() << "编译命令:" << fullCommand;

    // 确定工作目录
    QString workingDir = task.projectPath.isEmpty() ?
                             QDir::currentPath() : task.projectPath;

    // 异步执行编译
    bool started = d->processManager->executeAsync(
        compiler, args, workingDir, task.taskId);

    if (!started) {
        QString error = QString("无法启动编译器: %1").arg(compiler);
        qWarning() << error;

        // 更新结果
        CompileResult result;
        result.success = false;
        result.exitCode = -1;
        result.errorMessage = error;
        result.compileCommand = fullCommand;
        result.startTime = QDateTime::currentDateTime();
        result.endTime = result.startTime;

        // 保存结果
        {
            QMutexLocker locker(&d->resultMutex);
            d->compileResults[task.taskId] = result;
        }

        emit compileError(task.taskId, error);
        emit compileFinished(task.taskId, result);
    } else {
        // 发送编译开始信号
        QString sourceFile = task.sourceFiles.isEmpty() ?
                                 "未知文件" : task.sourceFiles.first();
        emit compileStarted(task.taskId, sourceFile);
    }
}

QString CompileManager::buildCompileCommand(const CompileTask& task) const
{
    QString compiler = task.compilerPath.isEmpty() ?
                           autoSelectCompiler(task) : task.compilerPath;
    QStringList args = generateCompilerArgs(task);

    // 对参数进行转义（如果有空格）
    QStringList escapedArgs;
    for (const QString& arg : args) {
        if (arg.contains(' ')) {
            escapedArgs << "\"" + arg + "\"";
        } else {
            escapedArgs << arg;
        }
    }

    return compiler + " " + escapedArgs.join(" ");
}

bool CompileManager::validateCompileTask(const CompileTask& task, QStringList& errors) const
{
    errors.clear();

    // 检查任务ID
    if (task.taskId.isEmpty()) {
        errors.append("任务ID不能为空");
    }

    // 检查源文件
    if (task.sourceFiles.isEmpty()) {
        errors.append("没有源文件需要编译");
    } else {
        for (const QString& sourceFile : task.sourceFiles) {
            QString fullPath = PathUtils::absolutePath(sourceFile, task.projectPath);
            if (!PathUtils::isValidFilePath(fullPath, true)) {
                errors.append(QString("源文件不存在: %1").arg(sourceFile));
            }
        }
    }

    // 检查头文件
    for (const QString& headerFile : task.headerFiles) {
        QString fullPath = PathUtils::absolutePath(headerFile, task.projectPath);
        if (!PathUtils::isValidFilePath(fullPath, true)) {
            errors.append(QString("头文件不存在: %1").arg(headerFile));
        }
    }

    // 检查输出目录
    if (!task.outputDir.isEmpty()) {
        // 允许输出目录不存在，会在编译时创建
        QString fullPath = PathUtils::absolutePath(task.outputDir, task.projectPath);
        if (!PathUtils::isValidPath(fullPath)) {
            errors.append(QString("输出目录无效: %1").arg(task.outputDir));
        }
    }

    // 检查包含路径
    for (const QString& includePath : task.includePaths) {
        QString fullPath = PathUtils::absolutePath(includePath, task.projectPath);
        if (!PathUtils::isValidPath(fullPath)) {
            errors.append(QString("包含路径无效: %1").arg(includePath));
        }
    }

    // 检查库路径
    for (const QString& libraryPath : task.libraryPaths) {
        QString fullPath = PathUtils::absolutePath(libraryPath, task.projectPath);
        if (!PathUtils::isValidPath(fullPath)) {
            errors.append(QString("库路径无效: %1").arg(libraryPath));
        }
    }

    // 检查C++标准
    static const QStringList validStandards = {"c++98", "c++11", "c++14", "c++17", "c++20", "c++23"};
    if (!validStandards.contains(task.cppStandard.toLower()) && !task.cppStandard.isEmpty()) {
        errors.append(QString("不支持的C++标准: %1").arg(task.cppStandard));
    }

    // 检查编译器（如果手动指定）
    if (!task.compilerPath.isEmpty()) {
        if (!PathUtils::isValidFilePath(task.compilerPath, true)) {
            errors.append(QString("编译器不存在: %1").arg(task.compilerPath));
        }
    }

    return errors.isEmpty();
}

QString CompileManager::autoSelectCompiler(const CompileTask& task) const
{
    // 如果有默认编译器，使用默认编译器
    if (!d->defaultCompilerPath.isEmpty()) {
        return d->defaultCompilerPath;
    }

    // 根据编译器类型选择
    QString compilerCmd = getCompilerCommand(task.compilerType);

    // 检查编译器是否可用
    ProcessManager::ProcessResult result = d->processManager->executeSync(
        compilerCmd, QStringList() << "--version", QString(), 3000);

    if (result.success) {
        return compilerCmd;
    }

    // 如果指定的编译器不可用，尝试其他编译器
    QStringList available = d->availableCompilers;
    if (available.isEmpty()) {
        available = detectAvailableCompilers();
    }

    if (!available.isEmpty()) {
        // 优先选择与请求类型匹配的编译器
        QString targetType;
        switch (task.compilerType) {
        case CompilerType::GCC:
        case CompilerType::GPP:
        case CompilerType::MinGW:
            targetType = "g++";
            break;
        case CompilerType::Clang:
        case CompilerType::ClangPP:
            targetType = "clang++";
            break;
        case CompilerType::MSVC:
            targetType = "cl.exe";
            break;
        default:
            targetType = "";
            break;
        }

        for (const QString& compiler : available) {
            if (!targetType.isEmpty() && compiler.contains(targetType, Qt::CaseInsensitive)) {
                return compiler;
            }
        }

        // 如果没有匹配的，返回第一个可用的
        return available.first();
    }

    // 没有可用的编译器，返回默认命令
    return compilerCmd;
}

void CompileManager::processCompilerOutput(const QString& taskId, const QString& output, bool isError)
{
    if (!d->compilerOutputs.contains(taskId)) {
        return;
    }

    // 添加到输出缓存
    d->compilerOutputs[taskId].append(output);

    // 转发信号
    emit compileOutput(taskId, output, isError);

    // 尝试解析输出中的进度信息
    if (output.contains("正在编译") || output.contains("Compiling")) {
        static QRegularExpression fileRegex("\\[.*?\\]\\s+(.*\\.cpp)");
        QRegularExpressionMatch match = fileRegex.match(output);
        if (match.hasMatch()) {
            QString currentFile = match.captured(1);

            // 计算进度（简化版）
            if (d->compileTasks.contains(taskId)) {
                const CompileTask& task = d->compileTasks[taskId];
                if (task.sourceFiles.contains(currentFile)) {
                    int index = task.sourceFiles.indexOf(currentFile);
                    int progress = (index + 1) * 100 / task.sourceFiles.size();
                    emit compileProgress(taskId, progress, currentFile);
                }
            }
        }
    }
}

void CompileManager::saveCompileLog(const QString& taskId, const CompileResult& result)
{
    // 创建日志目录
    QString logDir = PathUtils::joinPaths({PathUtils::platformDataPath(), "compile_logs"});
    PathUtils::ensureDirExists(logDir);

    // 创建日志文件
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString logFile = PathUtils::joinPaths({logDir, QString("compile_%1_%2.log")
                                                        .arg(taskId)
                                                        .arg(timestamp)});

    QFile file(logFile);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);

        stream << "=== 编译日志 ===" << "\n";
        stream << "任务ID: " << taskId << "\n";
        stream << "开始时间: " << result.startTime.toString(Qt::ISODate) << "\n";
        stream << "结束时间: " << result.endTime.toString(Qt::ISODate) << "\n";
        stream << "编译耗时: " << result.compileTime << " 毫秒" << "\n";
        stream << "是否成功: " << (result.success ? "是" : "否") << "\n";
        stream << "退出码: " << result.exitCode << "\n";
        stream << "使用编译器: " << result.compilerUsed << "\n";
        stream << "编译命令: " << result.compileCommand << "\n";
        stream << "\n";

        if (!result.errors.isEmpty()) {
            stream << "=== 错误信息 ===" << "\n";
            for (const QString& error : result.errors) {
                stream << error << "\n";
            }
            stream << "\n";
        }

        if (!result.warnings.isEmpty()) {
            stream << "=== 警告信息 ===" << "\n";
            for (const QString& warning : result.warnings) {
                stream << warning << "\n";
            }
            stream << "\n";
        }

        stream << "=== 完整输出 ===" << "\n";
        stream << result.output << "\n";

        file.close();
        qDebug() << "编译日志已保存:" << logFile;
    }
}

void CompileManager::loadCompileConfig()
{
    // 使用项目现有的ConfigManager或直接使用QSettings
    // 这里简化实现，直接使用QSettings
    if (!d->settings) {
        d->settings = new QSettings("SprintIDE", "CompileManager", this);
    }

    // 加载编译器设置
    d->defaultCompilerType = static_cast<CompilerType>(
        d->settings->value("DefaultCompilerType", static_cast<int>(CompilerType::AutoDetect)).toInt());
    d->defaultCompilerPath = d->settings->value("DefaultCompilerPath").toString();

    // 加载其他设置
    // ...

    qDebug() << "加载编译配置完成";
}

void CompileManager::saveCompileConfig()
{
    if (!d->settings) {
        d->settings = new QSettings("SprintIDE", "CompileManager", this);
    }

    // 保存编译器设置
    d->settings->setValue("DefaultCompilerType", static_cast<int>(d->defaultCompilerType));
    d->settings->setValue("DefaultCompilerPath", d->defaultCompilerPath);

    // 保存其他设置
    // ...

    d->settings->sync();
    qDebug() << "保存编译配置完成";
}

// ==================== 私有槽函数 ====================

void CompileManager::onProcessFinished(const QString& processId, const ProcessManager::ProcessResult& processResult)
{
    qDebug() << "编译进程完成:" << processId;

    // 查找对应的编译任务
    CompileTask task;
    {
        QMutexLocker locker(&d->taskMutex);
        if (d->compileTasks.contains(processId)) {
            task = d->compileTasks[processId];
        }
    }

    // 构建编译结果
    CompileResult result;
    result.success = processResult.success;
    result.exitCode = processResult.exitCode;
    result.output = processResult.standardOutput + "\n" + processResult.standardError;
    result.compilerUsed = processResult.success ? "编译器" : "未知编译器";
    result.compileTime = processResult.executionTime;
    result.startTime = QDateTime::currentDateTime().addMSecs(-result.compileTime);
    result.endTime = QDateTime::currentDateTime();
    result.errorMessage = processResult.errorMessage;

    // 构建编译命令
    if (!task.taskId.isEmpty()) {
        result.compileCommand = buildCompileCommand(task);
        result.compilerUsed = task.compilerPath.isEmpty() ?
                                  autoSelectCompiler(task) : task.compilerPath;

        // 确定可执行文件路径
        if (result.success && task.targetType == TargetType::Executable) {
            QString outputDir = PathUtils::absolutePath(task.outputDir, task.projectPath);
            QString outputFile = task.outputName;

#ifdef Q_OS_WIN
            if (!outputFile.endsWith(".exe")) {
                outputFile += ".exe";
            }
#endif

            result.executablePath = PathUtils::joinPaths({outputDir, outputFile});
        }
    }

    // 解析编译器输出
    CompileResult parsedResult = parseCompilerOutput(result.output, task.projectPath);
    result.errors = parsedResult.errors;
    result.warnings = parsedResult.warnings;
    result.infos = parsedResult.infos;

    // 保存编译结果
    {
        QMutexLocker locker(&d->resultMutex);
        d->compileResults[processId] = result;
    }

    // 保存编译日志
    saveCompileLog(processId, result);

    // 发射 compileFinished 信号
    emit compileFinished(processId, result);

    // 清理输出缓存
    d->compilerOutputs.remove(processId);
}

void CompileManager::onOutputReceived(const QString& processId, const QString& data, bool isError)
{
    // 处理编译器输出
    processCompilerOutput(processId, data, isError);
}

void CompileManager::onProcessStarted(const QString& processId)
{
    // 查找对应的编译任务
    CompileTask task;
    {
        QMutexLocker locker(&d->taskMutex);
        if (d->compileTasks.contains(processId)) {
            task = d->compileTasks[processId];
        }
    }

    // 确定源文件
    QString sourceFile;
    if (!task.sourceFiles.isEmpty()) {
        sourceFile = task.sourceFiles.first();
    } else {
        sourceFile = "未知文件";
    }

    // 发射 compileStarted 信号
    emit compileStarted(processId, sourceFile);
}

void CompileManager::onProcessError(const QString& processId, const QString& errorMessage)
{
    // 直接转发错误信号
    emit compileError(processId, errorMessage);
}

void CompileManager::onProgressUpdated(const QString& processId, qint64 elapsedTime)
{
    Q_UNUSED(elapsedTime);

    // 这里可以计算更精确的进度
    // 简化实现：每收到进度更新就发送一次
    emit compileProgress(processId, 50, "编译中...");
}

} // namespace SprintIDE
