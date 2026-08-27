#include "compile_output_parser.h"
#include "compile_manager.h"
#include "path_utils.h"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QCoreApplication>

namespace SprintIDE {

CompileOutputParser::CompileOutputParser(QObject* parent)
    : QObject(parent)
    , m_currentCompilerType(CompilerType::Unknown)
    , m_verbose(false)
    , m_treatWarningsAsErrors(false)
{
    initializeRegexes();
    qDebug() << "编译器输出解析器初始化完成";
}

CompileOutputParser::~CompileOutputParser()
{
    qDebug() << "编译器输出解析器销毁";
}

void CompileOutputParser::initializeRegexes()
{
    // GCC/Clang正则表达式
    CompilerRegex gccClangRegex;

    // 错误格式: filename:line:column: error: message
    // 例如: main.cpp:12:5: error: 'cout' was not declared in this scope
    gccClangRegex.error = QRegularExpression(
        R"((.+):(\d+):(\d+):\s+(fatal\s+)?error:\s+(.+))",
        QRegularExpression::CaseInsensitiveOption);

    // 警告格式: filename:line:column: warning: message
    gccClangRegex.warning = QRegularExpression(
        R"((.+):(\d+):(\d+):\s+warning:\s+(.+))",
        QRegularExpression::CaseInsensitiveOption);

    // 说明格式: filename:line:column: note: message
    gccClangRegex.note = QRegularExpression(
        R"((.+):(\d+):(\d+):\s+note:\s+(.+))",
        QRegularExpression::CaseInsensitiveOption);

    // 位置提取
    gccClangRegex.location = QRegularExpression(
        R"((.+):(\d+):(\d+))");

    m_compilerRegexes[CompilerType::GCC] = gccClangRegex;
    m_compilerRegexes[CompilerType::Clang] = gccClangRegex;
    m_compilerRegexes[CompilerType::MinGW] = gccClangRegex;

    // MSVC正则表达式
    CompilerRegex msvcRegex;

    // 错误格式: filename(line) : error CXXXX: message
    // 例如: main.cpp(12): error C2065: 'cout': undeclared identifier
    msvcRegex.error = QRegularExpression(
        R"((.+)\((\d+)\)\s*:\s*(fatal\s+)?error\s+(C\d+):\s+(.+))",
        QRegularExpression::CaseInsensitiveOption);

    // 警告格式: filename(line) : warning CXXXX: message
    msvcRegex.warning = QRegularExpression(
        R"((.+)\((\d+)\)\s*:\s*warning\s+(C\d+):\s+(.+))",
        QRegularExpression::CaseInsensitiveOption);

    // MSVC的note通常与错误/警告在同一行
    msvcRegex.note = QRegularExpression(
        R"((.+)\((\d+)\)\s*:\s*note:\s+(.+))",
        QRegularExpression::CaseInsensitiveOption);

    // 位置提取
    msvcRegex.location = QRegularExpression(
        R"((.+)\((\d+)\))");

    m_compilerRegexes[CompilerType::MSVC] = msvcRegex;
}

QList<CompilerOutputItem> CompileOutputParser::parseOutput(const QString& output, const QString& baseDir)
{
    qDebug() << "开始解析编译器输出，长度:" << output.length();

    // 清空统计信息
    clear();

    // 自动检测编译器类型
    m_currentCompilerType = detectCompilerType(output);
    emit compilerTypeDetected(m_currentCompilerType);

    qDebug() << "检测到编译器类型:" << static_cast<int>(m_currentCompilerType);

    // 根据编译器类型调用相应的解析方法
    QList<CompilerOutputItem> items;

    switch (m_currentCompilerType) {
    case CompilerType::GCC:
    case CompilerType::Clang:
    case CompilerType::MinGW:
        items = parseGCCClangOutput(output, baseDir);
        break;
    case CompilerType::MSVC:
        items = parseMSVCOutput(output, baseDir);
        break;
    default:
        // 未知编译器，尝试通用解析
        items = parseGCCClangOutput(output, baseDir);
        if (items.isEmpty()) {
            items = parseMSVCOutput(output, baseDir);
        }
        break;
    }

    // 解析链接器输出（跨编译器通用）
    QList<CompilerOutputItem> linkerItems = parseLinkerOutput(output);
    items.append(linkerItems);

    // 更新统计信息
    for (const CompilerOutputItem& item : items) {
        updateStats(item.type);
    }

    m_stats.totalItems = items.size();

    qDebug() << QString("解析完成: 共 %1 项，错误: %2，警告: %3，信息: %4")
                .arg(m_stats.totalItems)
                .arg(m_stats.errorCount + m_stats.fatalErrorCount)
                .arg(m_stats.warningCount)
                .arg(m_stats.infoCount + m_stats.noteCount);

    emit parseFinished(items, m_stats);

    return items;
}

QList<CompilerOutputItem> CompileOutputParser::parseOutputForCompiler(
    const QString& output, CompilerType compilerType, const QString& baseDir)
{
    m_currentCompilerType = compilerType;
    return parseOutput(output, baseDir);
}

CompileOutputParser::CompilerType CompileOutputParser::detectCompilerType(const QString& output) const
{
    // 检测编译器特征字符串
    QStringList lines = output.split('\n');

    for (const QString& line : lines) {
        QString trimmedLine = line.trimmed();

        // 检测GCC
        if (trimmedLine.contains("gcc") || trimmedLine.contains("g++")) {
            if (trimmedLine.contains("version") || trimmedLine.contains("Copyright")) {
                return CompilerType::GCC;
            }
        }

        // 检测Clang
        if (trimmedLine.contains("clang") || trimmedLine.contains("LLVM")) {
            if (trimmedLine.contains("version") || trimmedLine.contains("Apple")) {
                return CompilerType::Clang;
            }
        }

        // 检测MSVC
        if (trimmedLine.contains("Microsoft") || trimmedLine.contains("Visual C++")) {
            if (trimmedLine.contains("Compiler") || trimmedLine.contains("版本")) {
                return CompilerType::MSVC;
            }
        }

        // 检测MinGW
        if (trimmedLine.contains("MinGW") || trimmedLine.contains("mingw")) {
            return CompilerType::MinGW;
        }

        // 通过错误格式检测
        if (isMSVCLine(trimmedLine)) {
            return CompilerType::MSVC;
        }

        if (isGCCClangLine(trimmedLine)) {
            // 进一步区分GCC和Clang
            if (output.contains("clang")) {
                return CompilerType::Clang;
            } else {
                return CompilerType::GCC;
            }
        }
    }

    // 默认返回GCC（最常见）
    return CompilerType::GCC;
}

CompilerOutputStats CompileOutputParser::getStats() const
{
    return m_stats;
}

void CompileOutputParser::clear()
{
    m_stats = CompilerOutputStats();
    m_currentCompilerType = CompilerType::Unknown;
}

void CompileOutputParser::setVerbose(bool verbose)
{
    m_verbose = verbose;
}

void CompileOutputParser::setTreatWarningsAsErrors(bool treatWarningsAsErrors)
{
    m_treatWarningsAsErrors = treatWarningsAsErrors;
}

QString CompileOutputParser::getCompilerFriendlyName(CompilerType compilerType)
{
    switch (compilerType) {
    case CompilerType::GCC: return "GCC编译器";
    case CompilerType::Clang: return "Clang编译器";
    case CompilerType::MSVC: return "Visual C++编译器";
    case CompilerType::MinGW: return "MinGW编译器";
    default: return "未知编译器";
    }
}

QString CompileOutputParser::formatOutputItem(const CompilerOutputItem& item, bool includeLocation)
{
    QString formatted;

    // 添加类型前缀
    QString typePrefix = getOutputTypeFriendlyName(item.type) + ": ";

    // 添加位置信息
    if (includeLocation && !item.filePath.isEmpty()) {
        QString location;

        if (item.lineNumber > 0 && item.column > 0) {
            location = QString("%1:%2:%3").arg(item.filePath).arg(item.lineNumber).arg(item.column);
        } else if (item.lineNumber > 0) {
            location = QString("%1:%2").arg(item.filePath).arg(item.lineNumber);
        } else {
            location = item.filePath;
        }

        formatted = QString("[%1] %2").arg(location).arg(item.message);
    } else {
        formatted = item.message;
    }

    // 添加错误代码（如果有）
    if (!item.errorCode.isEmpty()) {
        formatted = QString("(%1) %2").arg(item.errorCode).arg(formatted);
    }

    return typePrefix + formatted;
}

QString CompileOutputParser::getOutputTypeFriendlyName(CompilerOutputType type)
{
    switch (type) {
    case CompilerOutputType::Info: return "信息";
    case CompilerOutputType::Warning: return "警告";
    case CompilerOutputType::Error: return "错误";
    case CompilerOutputType::FatalError: return "致命错误";
    case CompilerOutputType::Note: return "说明";
    case CompilerOutputType::Progress: return "进度";
    case CompilerOutputType::Success: return "成功";
    case CompilerOutputType::Command: return "命令";
    default: return "未知";
    }
}

QString CompileOutputParser::getOutputTypeColor(CompilerOutputType type)
{
    switch (type) {
    case CompilerOutputType::Error:
    case CompilerOutputType::FatalError:
        return "#FF3333";  // 红色
    case CompilerOutputType::Warning:
        return "#FF9900";  // 橙色
    case CompilerOutputType::Info:
        return "#3399FF";  // 蓝色
    case CompilerOutputType::Note:
        return "#666666";  // 灰色
    case CompilerOutputType::Success:
        return "#33CC33";  // 绿色
    case CompilerOutputType::Progress:
        return "#9966FF";  // 紫色
    default:
        return "#000000";  // 黑色
    }
}

QString CompileOutputParser::getOutputTypeIcon(CompilerOutputType type)
{
    switch (type) {
    case CompilerOutputType::Error:
    case CompilerOutputType::FatalError:
        return "error.png";
    case CompilerOutputType::Warning:
        return "warning.png";
    case CompilerOutputType::Info:
        return "info.png";
    case CompilerOutputType::Note:
        return "note.png";
    case CompilerOutputType::Success:
        return "success.png";
    default:
        return "";
    }
}

QList<CompilerOutputItem> CompileOutputParser::parseGCCClangOutput(const QString& output, const QString& baseDir)
{
    QList<CompilerOutputItem> items;
    QStringList lines = output.split('\n');

    const CompilerRegex& regex = getCompilerRegex(m_currentCompilerType);

    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();
        if (line.isEmpty()) {
            continue;
        }

        // 发送进度信号
        if (i % 10 == 0) {
            int progress = (i * 100) / lines.size();
            emit parseProgress(progress);
        }

        CompilerOutputItem item;

        // 检查是否为错误
        QRegularExpressionMatch errorMatch = regex.error.match(line);
        if (errorMatch.hasMatch()) {
            QString filePath = errorMatch.captured(1);
            int lineNumber = errorMatch.captured(2).toInt();
            int column = errorMatch.captured(3).toInt();
            bool isFatal = !errorMatch.captured(4).isEmpty();
            QString message = errorMatch.captured(5);

            filePath = formatFilePath(filePath, baseDir);

            item = createOutputItem(
                isFatal ? CompilerOutputType::FatalError : CompilerOutputType::Error,
                message,
                filePath,
                lineNumber,
                column
            );

            items.append(item);
            continue;
        }

        // 检查是否为警告
        QRegularExpressionMatch warningMatch = regex.warning.match(line);
        if (warningMatch.hasMatch()) {
            QString filePath = warningMatch.captured(1);
            int lineNumber = warningMatch.captured(2).toInt();
            int column = warningMatch.captured(3).toInt();
            QString message = warningMatch.captured(4);

            filePath = formatFilePath(filePath, baseDir);

            item = createOutputItem(
                m_treatWarningsAsErrors ? CompilerOutputType::Error : CompilerOutputType::Warning,
                message,
                filePath,
                lineNumber,
                column
            );

            items.append(item);
            continue;
        }

        // 检查是否为说明
        QRegularExpressionMatch noteMatch = regex.note.match(line);
        if (noteMatch.hasMatch()) {
            QString filePath = noteMatch.captured(1);
            int lineNumber = noteMatch.captured(2).toInt();
            int column = noteMatch.captured(3).toInt();
            QString message = noteMatch.captured(4);

            filePath = formatFilePath(filePath, baseDir);

            item = createOutputItem(
                CompilerOutputType::Note,
                message,
                filePath,
                lineNumber,
                column
            );
            item.isPrimary = false;  // 说明通常是补充信息

            items.append(item);
            continue;
        }

        // 检查是否为进度信息
        if (isProgressLine(line)) {
            item = createOutputItem(CompilerOutputType::Progress, line);
            items.append(item);
            continue;
        }

        // 检查是否为成功信息
        if (isSuccessLine(line)) {
            item = createOutputItem(CompilerOutputType::Success, line);
            items.append(item);
            continue;
        }

        // 其他信息
        if (m_verbose || line.contains("正在编译") || line.contains("Compiling")) {
            item = createOutputItem(CompilerOutputType::Info, line);
            items.append(item);
        }
    }

    return items;
}

QList<CompilerOutputItem> CompileOutputParser::parseMSVCOutput(const QString& output, const QString& baseDir)
{
    QList<CompilerOutputItem> items;
    QStringList lines = output.split('\n');

    const CompilerRegex& regex = getCompilerRegex(CompilerType::MSVC);

    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();
        if (line.isEmpty()) {
            continue;
        }

        // 发送进度信号
        if (i % 10 == 0) {
            int progress = (i * 100) / lines.size();
            emit parseProgress(progress);
        }

        CompilerOutputItem item;

        // 检查是否为错误
        QRegularExpressionMatch errorMatch = regex.error.match(line);
        if (errorMatch.hasMatch()) {
            QString filePath = errorMatch.captured(1);
            int lineNumber = errorMatch.captured(2).toInt();
            bool isFatal = !errorMatch.captured(3).isEmpty();
            QString errorCode = errorMatch.captured(4);
            QString message = errorMatch.captured(5);

            filePath = formatFilePath(filePath, baseDir);

            item = createOutputItem(
                isFatal ? CompilerOutputType::FatalError : CompilerOutputType::Error,
                message,
                filePath,
                lineNumber,
                0,  // MSVC通常没有列号
                errorCode
            );

            items.append(item);
            continue;
        }

        // 检查是否为警告
        QRegularExpressionMatch warningMatch = regex.warning.match(line);
        if (warningMatch.hasMatch()) {
            QString filePath = warningMatch.captured(1);
            int lineNumber = warningMatch.captured(2).toInt();
            QString errorCode = warningMatch.captured(3);
            QString message = warningMatch.captured(4);

            filePath = formatFilePath(filePath, baseDir);

            item = createOutputItem(
                m_treatWarningsAsErrors ? CompilerOutputType::Error : CompilerOutputType::Warning,
                message,
                filePath,
                lineNumber,
                0,
                errorCode
            );

            items.append(item);
            continue;
        }

        // 检查是否为说明
        QRegularExpressionMatch noteMatch = regex.note.match(line);
        if (noteMatch.hasMatch()) {
            QString filePath = noteMatch.captured(1);
            int lineNumber = noteMatch.captured(2).toInt();
            QString message = noteMatch.captured(3);

            filePath = formatFilePath(filePath, baseDir);

            item = createOutputItem(
                CompilerOutputType::Note,
                message,
                filePath,
                lineNumber
            );
            item.isPrimary = false;

            items.append(item);
            continue;
        }

        // 其他信息
        if (m_verbose || line.contains("正在生成代码") || line.contains("Generating Code")) {
            item = createOutputItem(CompilerOutputType::Info, line);
            items.append(item);
        }
    }

    return items;
}

QList<CompilerOutputItem> CompileOutputParser::parseLinkerOutput(const QString& output)
{
    QList<CompilerOutputItem> items;
    QStringList lines = output.split('\n');

    // 链接器错误的正则表达式
    QRegularExpression undefinedRefRegex(
        R"(undefined reference to `(.+)')",
        QRegularExpression::CaseInsensitiveOption);

    QRegularExpression duplicateSymbolRegex(
        R"(duplicate symbol `(.+)' in:)",
        QRegularExpression::CaseInsensitiveOption);

    QRegularExpression cannotFindRegex(
        R"(cannot find -l(.+))",
        QRegularExpression::CaseInsensitiveOption);

    for (const QString& line : lines) {
        QString trimmedLine = line.trimmed();
        if (trimmedLine.isEmpty()) {
            continue;
        }

        CompilerOutputItem item;

        // 检查未定义引用
        QRegularExpressionMatch undefinedMatch = undefinedRefRegex.match(trimmedLine);
        if (undefinedMatch.hasMatch()) {
            QString symbol = undefinedMatch.captured(1);
            QString message = QString("未定义的引用: %1").arg(symbol);

            item = createOutputItem(CompilerOutputType::Error, message);
            items.append(item);
            continue;
        }

        // 检查重复符号
        QRegularExpressionMatch duplicateMatch = duplicateSymbolRegex.match(trimmedLine);
        if (duplicateMatch.hasMatch()) {
            QString symbol = duplicateMatch.captured(1);
            QString message = QString("重复的符号: %1").arg(symbol);

            item = createOutputItem(CompilerOutputType::Error, message);
            items.append(item);
            continue;
        }

        // 检查找不到库
        QRegularExpressionMatch cannotFindMatch = cannotFindRegex.match(trimmedLine);
        if (cannotFindMatch.hasMatch()) {
            QString lib = cannotFindMatch.captured(1);
            QString message = QString("找不到库文件: -l%1").arg(lib);

            item = createOutputItem(CompilerOutputType::Error, message);
            items.append(item);
            continue;
        }

        // 其他链接器错误
        if (trimmedLine.contains("ld returned") ||
            trimmedLine.contains("linker command failed") ||
            trimmedLine.contains("LNK")) {
            item = createOutputItem(CompilerOutputType::Error, trimmedLine);
            items.append(item);
        }
    }

    return items;
}

bool CompileOutputParser::extractGCCClangLocation(const QString& line, QString& filePath, int& lineNumber, int& column) const
{
    const CompilerRegex& regex = getCompilerRegex(m_currentCompilerType);
    QRegularExpressionMatch locationMatch = regex.location.match(line);

    if (locationMatch.hasMatch()) {
        filePath = locationMatch.captured(1);
        lineNumber = locationMatch.captured(2).toInt();
        column = locationMatch.captured(3).toInt();
        return true;
    }

    return false;
}

bool CompileOutputParser::extractMSVCLocation(const QString& line, QString& filePath, int& lineNumber) const
{
    const CompilerRegex& regex = getCompilerRegex(CompilerType::MSVC);
    QRegularExpressionMatch locationMatch = regex.location.match(line);

    if (locationMatch.hasMatch()) {
        filePath = locationMatch.captured(1);
        lineNumber = locationMatch.captured(2).toInt();
        return true;
    }

    return false;
}

QString CompileOutputParser::formatFilePath(const QString& filePath, const QString& baseDir) const
{
    if (filePath.isEmpty()) {
        return QString();
    }

    QString formattedPath = filePath;

    // 如果是相对路径且提供了基础目录，转换为绝对路径
    if (!baseDir.isEmpty() && QDir::isRelativePath(filePath)) {
        formattedPath = PathUtils::absolutePath(filePath, baseDir);
    }

    // 规范化路径
    formattedPath = PathUtils::normalizePath(formattedPath);

    return formattedPath;
}

CompilerOutputItem CompileOutputParser::createOutputItem(
    CompilerOutputType type,
    const QString& message,
    const QString& filePath,
    int lineNumber,
    int column,
    const QString& errorCode) const
{
    CompilerOutputItem item;
    item.type = type;
    item.message = message;
    item.filePath = filePath;
    item.lineNumber = lineNumber;
    item.column = column;
    item.errorCode = errorCode;
    item.isPrimary = true;

    // 根据编译器类型设置编译器标识
    switch (m_currentCompilerType) {
    case CompilerType::GCC: item.compilerId = "gcc"; break;
    case CompilerType::Clang: item.compilerId = "clang"; break;
    case CompilerType::MSVC: item.compilerId = "msvc"; break;
    case CompilerType::MinGW: item.compilerId = "mingw"; break;
    default: item.compilerId = "unknown"; break;
    }

    // 生成格式化消息
    item.formattedMessage = formatOutputItem(item);

    return item;
}

void CompileOutputParser::updateStats(CompilerOutputType type)
{
    switch (type) {
    case CompilerOutputType::Info:
        m_stats.infoCount++;
        break;
    case CompilerOutputType::Warning:
        m_stats.warningCount++;
        break;
    case CompilerOutputType::Error:
        m_stats.errorCount++;
        break;
    case CompilerOutputType::FatalError:
        m_stats.fatalErrorCount++;
        break;
    case CompilerOutputType::Note:
        m_stats.noteCount++;
        break;
    default:
        // 其他类型不计入统计
        break;
    }
}

bool CompileOutputParser::isGCCClangLine(const QString& line) const
{
    // 检查是否为GCC/Clang格式: file:line:column: type: message
    static QRegularExpression gccClangPattern(
        R"(^.+\:\d+\:\d+\:.+(error|warning|note)\:.+$)");

    return gccClangPattern.match(line).hasMatch();
}

bool CompileOutputParser::isMSVCLine(const QString& line) const
{
    // 检查是否为MSVC格式: file(line) : type CXXXX: message
    static QRegularExpression msvcPattern(
        R"(^.+\s*\(\d+\)\s*\:.+(error|warning)\s+C\d+\:.+$)");

    return msvcPattern.match(line).hasMatch();
}

bool CompileOutputParser::isLinkerLine(const QString& line) const
{
    return line.contains("undefined reference") ||
           line.contains("duplicate symbol") ||
           line.contains("cannot find") ||
           line.contains("ld returned") ||
           line.contains("linker command failed") ||
           line.startsWith("LNK");
}

bool CompileOutputParser::isProgressLine(const QString& line) const
{
    return line.contains("正在编译") ||
           line.contains("Compiling") ||
           line.contains("正在链接") ||
           line.contains("Linking") ||
           line.contains("Generating Code");
}

bool CompileOutputParser::isSuccessLine(const QString& line) const
{
    return line.contains("编译成功") ||
           line.contains("Build succeeded") ||
           line.contains("生成成功") ||
           (line.contains("error") && line.contains("0") && line.contains("warning") && line.contains("0"));
}

const CompileOutputParser::CompilerRegex& CompileOutputParser::getCompilerRegex(CompilerType compilerType) const
{
    auto it = m_compilerRegexes.find(compilerType);
    if (it != m_compilerRegexes.end()) {
        return it.value();
    }

    // 返回GCC作为默认
    return m_compilerRegexes[CompilerType::GCC];
}

} // namespace SprintIDE
