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
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QMap>

namespace SprintIDE {

/**
 * @brief 编译器输出类型枚举
 *
 * 定义编译器输出的不同类型，用于分类和显示
 */
enum class CompilerOutputType {
    Unknown,        ///< 未知类型
    Info,          ///< 普通信息
    Warning,       ///< 警告
    Error,         ///< 错误
    FatalError,    ///< 致命错误
    Note,          ///< 补充说明
    Progress,      ///< 进度信息
    Success,       ///< 成功信息
    Command        ///< 执行的命令
};

/**
 * @brief 编译器输出项
 *
 * 表示编译器输出的一个解析后的项
 */
struct CompilerOutputItem {
    CompilerOutputType type;      ///< 输出类型
    QString filePath;             ///< 相关文件路径
    int lineNumber;               ///< 行号（如果有）
    int column;                   ///< 列号（如果有）
    QString message;              ///< 原始消息
    QString formattedMessage;     ///< 格式化后的消息（适合显示）
    QString compilerId;           ///< 编译器标识（gcc/clang/msvc）
    QString errorCode;            ///< 错误代码（如C1234）
    bool isPrimary;               ///< 是否是主要消息（不是补充说明）

    CompilerOutputItem() :
        type(CompilerOutputType::Unknown),
        lineNumber(0),
        column(0),
        isPrimary(true) {}
};

/**
 * @brief 编译器输出统计
 *
 * 统计编译器输出的各类信息数量
 */
struct CompilerOutputStats {
    int totalItems;         ///< 总输出项数
    int infoCount;          ///< 信息数量
    int warningCount;       ///< 警告数量
    int errorCount;         ///< 错误数量
    int fatalErrorCount;    ///< 致命错误数量
    int noteCount;          ///< 说明数量

    CompilerOutputStats() :
        totalItems(0),
        infoCount(0),
        warningCount(0),
        errorCount(0),
        fatalErrorCount(0),
        noteCount(0) {}
};

/**
 * @brief 编译器输出解析器类
 *
 * 专门负责解析不同编译器（GCC、Clang、MSVC）的输出，
 * 提取错误、警告和信息，为儿童用户提供友好的中文提示
 */
class CompileOutputParser : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 编译器类型枚举
     */
    enum class CompilerType {
        Unknown,    ///< 未知编译器
        GCC,       ///< GNU编译器
        Clang,     ///< Clang编译器
        MSVC,      ///< Microsoft Visual C++
        MinGW      ///< MinGW编译器
    };

    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit CompileOutputParser(QObject* parent = nullptr);
    ~CompileOutputParser();

    /**
     * @brief 解析编译器输出
     *
     * 自动检测编译器类型并解析输出
     *
     * @param output 原始编译器输出
     * @param baseDir 基础目录（用于解析相对路径）
     * @return QList<CompilerOutputItem> 解析后的输出项列表
     */
    QList<CompilerOutputItem> parseOutput(const QString& output, const QString& baseDir = QString());

    /**
     * @brief 解析特定编译器输出
     *
     * 手动指定编译器类型进行解析
     *
     * @param output 原始编译器输出
     * @param compilerType 编译器类型
     * @param baseDir 基础目录
     * @return QList<CompilerOutputItem> 解析后的输出项列表
     */
    QList<CompilerOutputItem> parseOutputForCompiler(const QString& output,
                                                     CompilerType compilerType,
                                                     const QString& baseDir = QString());

    /**
     * @brief 自动检测编译器类型
     *
     * 根据输出内容自动检测使用的编译器类型
     *
     * @param output 编译器输出
     * @return CompilerType 检测到的编译器类型
     */
    CompilerType detectCompilerType(const QString& output) const;

    /**
     * @brief 获取解析统计信息
     * @return CompilerOutputStats 统计信息
     */
    CompilerOutputStats getStats() const;

    /**
     * @brief 清空解析器状态
     *
     * 重置统计信息和缓存
     */
    void clear();

    /**
     * @brief 设置是否启用详细输出
     * @param verbose 是否启用详细输出
     */
    void setVerbose(bool verbose);

    /**
     * @brief 设置是否将警告视为错误
     * @param treatWarningsAsErrors 是否将警告视为错误
     */
    void setTreatWarningsAsErrors(bool treatWarningsAsErrors);

    /**
     * @brief 获取编译器友好的名称
     * @param compilerType 编译器类型
     * @return QString 编译器名称（中文）
     */
    static QString getCompilerFriendlyName(CompilerType compilerType);

    /**
     * @brief 格式化输出项为字符串
     *
     * 将输出项格式化为适合显示的字符串
     *
     * @param item 输出项
     * @param includeLocation 是否包含位置信息
     * @return QString 格式化后的字符串
     */
    static QString formatOutputItem(const CompilerOutputItem& item, bool includeLocation = true);

    /**
     * @brief 获取输出类型的友好名称
     * @param type 输出类型
     * @return QString 类型名称（中文）
     */
    static QString getOutputTypeFriendlyName(CompilerOutputType type);

    /**
     * @brief 获取输出类型的颜色（用于界面显示）
     * @param type 输出类型
     * @return QString 颜色名称或十六进制值
     */
    static QString getOutputTypeColor(CompilerOutputType type);

    /**
     * @brief 获取输出类型的图标（用于界面显示）
     * @param type 输出类型
     * @return QString 图标名称或路径
     */
    static QString getOutputTypeIcon(CompilerOutputType type);

signals:
    /**
     * @brief 解析进度信号
     * @param progress 解析进度百分比（0-100）
     */
    void parseProgress(int progress);

    /**
     * @brief 解析完成信号
     * @param items 解析后的输出项列表
     * @param stats 解析统计信息
     */
    void parseFinished(const QList<CompilerOutputItem>& items, const CompilerOutputStats& stats);

    /**
     * @brief 编译器类型检测信号
     * @param compilerType 检测到的编译器类型
     */
    void compilerTypeDetected(CompilerType compilerType);

private:
    /**
     * @brief 解析GCC/Clang输出
     *
     * 解析GNU编译器集合和Clang编译器的输出
     *
     * @param output 原始输出
     * @param baseDir 基础目录
     * @return QList<CompilerOutputItem> 解析后的项
     */
    QList<CompilerOutputItem> parseGCCClangOutput(const QString& output, const QString& baseDir);

    /**
     * @brief 解析MSVC输出
     *
     * 解析Microsoft Visual C++编译器的输出
     *
     * @param output 原始输出
     * @param baseDir 基础目录
     * @return QList<CompilerOutputItem> 解析后的项
     */
    QList<CompilerOutputItem> parseMSVCOutput(const QString& output, const QString& baseDir);

    /**
     * @brief 解析链接器输出
     *
     * 解析链接器错误和警告
     *
     * @param output 原始输出
     * @return QList<CompilerOutputItem> 解析后的项
     */
    QList<CompilerOutputItem> parseLinkerOutput(const QString& output);

    /**
     * @brief 提取GCC/Clang错误位置
     *
     * 从GCC/Clang错误消息中提取文件、行号、列号
     *
     * @param line 原始行
     * @param filePath 文件路径（输出参数）
     * @param lineNumber 行号（输出参数）
     * @param column 列号（输出参数）
     * @return bool 是否成功提取
     */
    bool extractGCCClangLocation(const QString& line, QString& filePath, int& lineNumber, int& column) const;

    /**
     * @brief 提取MSVC错误位置
     *
     * 从MSVC错误消息中提取文件、行号
     *
     * @param line 原始行
     * @param filePath 文件路径（输出参数）
     * @param lineNumber 行号（输出参数）
     * @return bool 是否成功提取
     */
    bool extractMSVCLocation(const QString& line, QString& filePath, int& lineNumber) const;

    /**
     * @brief 格式化文件路径
     *
     * 将相对路径转换为绝对路径，并规范化路径格式
     *
     * @param filePath 原始文件路径
     * @param baseDir 基础目录
     * @return QString 格式化后的文件路径
     */
    QString formatFilePath(const QString& filePath, const QString& baseDir) const;

    /**
     * @brief 创建输出项
     *
     * 根据参数创建CompilerOutputItem对象
     *
     * @param type 输出类型
     * @param message 原始消息
     * @param filePath 文件路径
     * @param lineNumber 行号
     * @param column 列号
     * @param errorCode 错误代码
     * @return CompilerOutputItem 创建的输出项
     */
    CompilerOutputItem createOutputItem(CompilerOutputType type,
                                        const QString& message,
                                        const QString& filePath = QString(),
                                        int lineNumber = 0,
                                        int column = 0,
                                        const QString& errorCode = QString()) const;

    /**
     * @brief 更新统计信息
     * @param type 输出类型
     */
    void updateStats(CompilerOutputType type);

    /**
     * @brief 判断是否为GCC/Clang编译器
     * @param line 输出行
     * @return bool 如果是GCC/Clang输出返回true
     */
    bool isGCCClangLine(const QString& line) const;

    /**
     * @brief 判断是否为MSVC编译器
     * @param line 输出行
     * @return bool 如果是MSVC输出返回true
     */
    bool isMSVCLine(const QString& line) const;

    /**
     * @brief 判断是否为链接器输出
     * @param line 输出行
     * @return bool 如果是链接器输出返回true
     */
    bool isLinkerLine(const QString& line) const;

    /**
     * @brief 判断是否为进度信息
     * @param line 输出行
     * @return bool 如果是进度信息返回true
     */
    bool isProgressLine(const QString& line) const;

    /**
     * @brief 判断是否为成功信息
     * @param line 输出行
     * @return bool 如果是成功信息返回true
     */
    bool isSuccessLine(const QString& line) const;

private:
    CompilerOutputStats m_stats;                ///< 解析统计信息
    CompilerType m_currentCompilerType;         ///< 当前编译器类型
    bool m_verbose;                            ///< 是否启用详细输出
    bool m_treatWarningsAsErrors;              ///< 是否将警告视为错误

    // 正则表达式缓存
    QMap<QString, QRegularExpression> m_regexCache;

    // 编译器特定正则表达式
    struct CompilerRegex {
        QRegularExpression error;
        QRegularExpression warning;
        QRegularExpression note;
        QRegularExpression fatalError;
        QRegularExpression location;
    };

    QMap<CompilerType, CompilerRegex> m_compilerRegexes;

    /**
     * @brief 初始化正则表达式
     *
     * 初始化各种编译器输出的正则表达式
     */
    void initializeRegexes();

    /**
     * @brief 获取编译器正则表达式
     * @param compilerType 编译器类型
     * @return CompilerRegex 编译器正则表达式集合
     */
    const CompilerRegex& getCompilerRegex(CompilerType compilerType) const;
};

} // namespace SprintIDE
