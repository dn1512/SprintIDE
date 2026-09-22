#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QElapsedTimer>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <atomic>

#include "project_config.h"
#include "process_manager.h"
#include "builder/build_task.h"
#include "builder/build_result.h"
#include "builder/builder_factory.h"


namespace SprintIDE {

/**
 * @brief 编译器类型枚举
 *
 * 定义支持的编译器类型，支持自动检测和手动指定
 */
enum class CompilerType {
    AutoDetect,     ///< 自动检测 - IDE自动选择最佳编译器
    GCC,           ///< GNU编译器集合 - Linux/Windows常用
    GPP,           ///< GNU C++编译器 (g++)
    Clang,         ///< Clang编译器 - macOS/跨平台
    ClangPP,       ///< Clang C++编译器 (clang++)
    MSVC,          ///< Microsoft Visual C++ - Windows专用
    MinGW          ///< MinGW编译器 - Windows上的GCC端口
};

/**
 * @brief 编译模式枚举
 *
 * 定义不同的编译模式，适应不同学习阶段
 */
enum class CompileMode {
    Debug,          ///< 调试模式 - 包含调试信息，不优化
    Release,        ///< 发布模式 - 完全优化，去除调试信息
    Test,           ///< 测试模式 - 中等优化，包含测试信息
    Custom          ///< 自定义模式 - 用户自定义设置
};

/**
 * @brief 编译目标类型枚举
 *
 * 定义编译输出的目标类型
 */
enum class TargetType {
    Executable,     ///< 可执行文件
    StaticLibrary, ///< 静态库
    DynamicLibrary,///< 动态库
    ObjectFile     ///< 目标文件
};

/**
 * @brief 编译结果结构体
 *
 * 存储编译过程的详细结果信息
 */
struct CompileResult {
    bool success;                       ///< 编译是否成功
    int exitCode;                       ///< 编译器退出码
    QString output;                     ///< 编译器完整输出
    QString executablePath;             ///< 生成的可执行文件路径（如果适用）
    QStringList errors;                 ///< 错误信息列表
    QStringList warnings;               ///< 警告信息列表
    QStringList infos;                  ///< 信息列表
    qint64 compileTime;                 ///< 编译耗时（毫秒）
    QDateTime startTime;                ///< 编译开始时间
    QDateTime endTime;                  ///< 编译结束时间
    QString compilerUsed;               ///< 实际使用的编译器
    QString compileCommand;             ///< 执行的编译命令（便于调试）
    QString errorMessage;               ///< 错误信息（如果失败）<-- 添加这个

    CompileResult() : success(false), exitCode(-1), compileTime(0) {}
};

/**
 * @brief 编译任务结构体
 *
 * 定义一个编译任务的所有参数
 */
struct CompileTask {
    QString taskId;                     ///< 任务ID（唯一标识）
    QString projectPath;                ///< 项目路径
    QStringList sourceFiles;            ///< 源文件列表
    QStringList headerFiles;            ///< 头文件列表
    QString outputName;                 ///< 输出文件名
    QString outputDir;                  ///< 输出目录
    TargetType targetType;              ///< 目标类型
    CompileMode compileMode;            ///< 编译模式
    CompilerType compilerType;          ///< 编译器类型
    QString compilerPath;               ///< 编译器路径（如果手动指定）
    QString cppStandard;                ///< C++标准（如"c++17"）
    QStringList includePaths;           ///< 包含路径列表
    QStringList libraryPaths;           ///< 库路径列表
    QStringList libraries;              ///< 链接库列表
    QStringList defines;                ///< 预定义宏列表
    QStringList extraArgs;              ///< 额外编译参数
    bool enableDebugInfo;               ///< 是否启用调试信息
    bool enableWarnings;                ///< 是否启用警告
    bool treatWarningsAsErrors;         ///< 是否将警告视为错误
    bool optimizeForSpeed;              ///< 是否优化速度
    bool optimizeForSize;               ///< 是否优化大小
    bool verboseOutput;                 ///< 是否输出详细信息

    CompileTask() :
        targetType(TargetType::Executable),
        compileMode(CompileMode::Debug),
        compilerType(CompilerType::AutoDetect),
        enableDebugInfo(true),
        enableWarnings(true),
        treatWarningsAsErrors(false),
        optimizeForSpeed(false),
        optimizeForSize(false),
        verboseOutput(false) {}
};

/**
 * @brief 编译管理器类
 *
 * 负责管理C++代码的编译过程，支持单文件编译、项目编译和配置管理
 * 为儿童编程学习提供友好的编译体验，自动处理常见编译错误
 */
class CompileManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 获取CompileManager单例实例
     * @return CompileManager& 单例引用
     */
    static CompileManager& instance();

    /**
     * @brief 初始化编译管理器
     *
     * 检测系统可用的编译器，加载配置文件，准备编译环境
     *
     * @return bool 初始化成功返回true
     */
    bool initialize();

    /**
     * @brief 编译单个C++文件
     *
     * 快速编译单个文件，适合简单的代码测试
     *
     * @param sourceFile 源文件路径
     * @param outputName 输出文件名（可选）
     * @param compileMode 编译模式
     * @return QString 任务ID，用于跟踪编译状态
     */
    QString compileSingleFile(const QString& sourceFile,
                              const QString& outputName = QString(),
                              CompileMode compileMode = CompileMode::Debug);

    /**
     * @brief 编译项目
     *
     * 根据项目配置编译整个项目
     *
     * @param projectConfig 项目配置对象
     * @param compileMode 编译模式
     * @return QString 任务ID
     */
    QString compileProject(const ProjectConfig& projectConfig,
                           CompileMode compileMode = CompileMode::Debug);

    /**
     * @brief 编译自定义任务
     *
     * 使用自定义参数进行编译
     *
     * @param task 编译任务配置
     * @return QString 任务ID
     */
    QString compileCustomTask(const CompileTask& task);

    /**
     * @brief 运行构建产物
     * @param task 构建任务（需包含 language、projectPath、outputName 等）
     * @param buildResult 之前构建的结果（含 executablePath）
     * @param output 运行输出（返回）
     * @return bool 运行成功返回 true
     */
    bool runTask(const BuildTask& task, const BuildResult& buildResult, QString& output);

    /**
     * @brief 取消编译任务
     *
     * 停止正在进行的编译任务
     *
     * @param taskId 任务ID
     * @return bool 取消成功返回true
     */
    bool cancelCompile(const QString& taskId);

    /**
     * @brief 获取编译结果
     *
     * 根据任务ID获取编译结果
     *
     * @param taskId 任务ID
     * @return CompileResult 编译结果，如果任务不存在则返回默认结果
     */
    CompileResult getCompileResult(const QString& taskId) const;

    /**
     * @brief 清理编译产物
     *
     * 删除编译生成的文件（可执行文件、目标文件等）
     *
     * @param projectPath 项目路径
     * @param outputDir 输出目录
     * @return bool 清理成功返回true
     */
    bool cleanBuild(const QString& projectPath, const QString& outputDir = "bin");

    /**
     * @brief 检测系统可用的编译器
     *
     * 扫描系统环境，找出所有可用的C++编译器
     *
     * @return QStringList 可用的编译器路径列表
     */
    QStringList detectAvailableCompilers() const;

    /**
     * @brief 获取编译器信息
     *
     * 获取指定编译器的版本和功能信息
     *
     * @param compilerPath 编译器路径
     * @return QString 编译器信息字符串
     */
    QString getCompilerInfo(const QString& compilerPath);

    /**
     * @brief 设置默认编译器
     *
     * 设置优先使用的编译器
     *
     * @param compilerType 编译器类型
     * @param compilerPath 编译器路径（可选）
     * @return bool 设置成功返回true
     */
    bool setDefaultCompiler(CompilerType compilerType, const QString& compilerPath = QString());

    /**
     * @brief 获取编译器命令
     *
     * 根据编译器类型获取对应的可执行文件名
     *
     * @param compilerType 编译器类型
     * @return QString 编译器命令（如"g++"、"clang++"）
     */
    static QString getCompilerCommand(CompilerType compilerType);

    /**
     * @brief 获取编译器参数
     *
     * 根据编译模式和目标类型生成编译器参数
     *
     * @param task 编译任务
     * @return QStringList 编译器参数列表
     */
    static QStringList generateCompilerArgs(const CompileTask& task);

    /**
     * @brief 解析编译器输出
     *
     * 解析编译器输出，提取错误、警告和信息
     *
     * @param output 编译器原始输出
     * @param baseDir 基础目录（用于将相对路径转换为绝对路径）
     * @return CompileResult 解析后的结果
     */
    static CompileResult parseCompilerOutput(const QString& output, const QString& baseDir = QString());

    /**
     * @brief 执行通用构建任务（新接口）
     * @param task 构建任务
     * @return 任务ID（用于跟踪）
     */
    QString compileTask(const BuildTask& task);

    /**
     * @brief 设置项目 SDK 环境（供 AI 调用）
     * @param projectPath 项目路径
     * @param sdkEnv 环境变量键值对
     */
    void setProjectSdk(const QString& projectPath, const QVariantMap& sdkEnv);
    /**
     * @brief 清理构建产物（使用新的 BuildTask 体系）
     * @param task 构建任务
     * @return BuildResult 清理结果
     */
    BuildResult cleanTask(const BuildTask& task);
signals:
    /**
     * @brief 编译开始信号
     * @param taskId 任务ID
     * @param sourceFile 正在编译的源文件
     */
    void compileStarted(const QString& taskId, const QString& sourceFile);

    /**
     * @brief 编译进度信号
     * @param taskId 任务ID
     * @param progress 进度百分比（0-100）
     * @param currentFile 当前正在编译的文件
     */
    void compileProgress(const QString& taskId, int progress, const QString& currentFile);

    /**
     * @brief 编译输出信号
     * @param taskId 任务ID
     * @param output 编译器输出
     * @param isError 是否为错误输出
     */
    void compileOutput(const QString& taskId, const QString& output, bool isError);

    /**
     * @brief 编译完成信号
     * @param taskId 任务ID
     * @param result 编译结果
     */
    void compileFinished(const QString& taskId, const CompileResult& result);

    /**
     * @brief 编译器检测完成信号
     * @param compilers 检测到的编译器列表
     */
    void compilersDetected(const QStringList& compilers);

    /**
     * @brief 编译错误信号
     * @param taskId 任务ID
     * @param errorMessage 错误信息
     */
    void compileError(const QString& taskId, const QString& errorMessage);

private slots:
    void onProcessFinished(const QString& processId, const ProcessManager::ProcessResult& processResult);
    void onOutputReceived(const QString& processId, const QString& data, bool isError);
    void onProcessStarted(const QString& processId);
    void onProcessError(const QString& processId, const QString& errorMessage);
    void onProgressUpdated(const QString& processId, qint64 elapsedTime);

private:
    /**
     * @brief 构造函数（私有，单例模式）
     * @param parent 父对象
     */
    explicit CompileManager(QObject* parent = nullptr);
    ~CompileManager();

    CompileManager(const CompileManager&) = delete;
    CompileManager& operator=(const CompileManager&) = delete;

    /**
     * @brief 执行编译任务
     *
     * 在线程中执行实际的编译过程
     *
     * @param task 编译任务
     */
    void executeCompileTask(const CompileTask& task);

    /**
     * @brief 构建编译命令
     *
     * 根据编译任务构建完整的编译命令
     *
     * @param task 编译任务
     * @return QString 完整的编译命令字符串
     */
    QString buildCompileCommand(const CompileTask& task) const;

    /**
     * @brief 验证编译任务
     *
     * 检查编译任务的参数是否有效
     *
     * @param task 编译任务
     * @param errors 错误信息列表（输出参数）
     * @return bool 有效返回true
     */
    bool validateCompileTask(const CompileTask& task, QStringList& errors) const;

    /**
     * @brief 自动选择编译器
     *
     * 根据编译任务和系统环境自动选择最合适的编译器
     *
     * @param task 编译任务
     * @return QString 选择的编译器路径
     */
    QString autoSelectCompiler(const CompileTask& task) const;

    /**
     * @brief 处理编译器输出
     *
     * 实时处理编译器输出，解析并发送信号
     *
     * @param taskId 任务ID
     * @param output 编译器输出
     * @param isError 是否为错误输出
     */
    void processCompilerOutput(const QString& taskId, const QString& output, bool isError);

    /**
     * @brief 保存编译记录
     *
     * 将编译结果保存到日志文件，便于问题排查
     *
     * @param taskId 任务ID
     * @param result 编译结果
     */
    void saveCompileLog(const QString& taskId, const CompileResult& result);

    /**
     * @brief 加载编译配置
     *
     * 从配置文件加载编译器设置
     */
    void loadCompileConfig();

    /**
     * @brief 保存编译配置
     *
     * 将编译器设置保存到配置文件
     */
    void saveCompileConfig();

    /**
     * @brief 执行构建任务内部实现
     */
    QString executeBuildTask(const BuildTask& task);

    /**
     * @brief 将旧的 CompileResult 转换为 BuildResult（用于兼容）
     */
    BuildResult convertOldResultToNew(const CompileResult& oldResult) const;
    
    /**
     * @brief 处理异步编译完成后的信号分发
     * 从工作线程的 QFutureWatcher 回到 UI 线程后调用
     */
    void handleBuildFinished(const BuildTask& task,
                             bool started,
                             const BuildResult& result);

    class Private;
    QScopedPointer<Private> d;
    /// 正在运行的编译任务数（防并发）
    std::atomic<int> m_runningTaskCount{0};
};

} // namespace SprintIDE
