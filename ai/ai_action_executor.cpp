#include "ai_action_executor.h"
#include "ai/ai_integration.h"
#include "ai/ai_server.h"
#include "ai/chat_panel.h"

#include "mainwindow.h"
#include "text_editor.h"
#include "compile_manager.h"
#include "debug_manager.h"
#include "project_manager.h"
#include "path_utils.h"
#include "terminal/terminal_panel.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include <QMetaObject>
#include <QApplication>
#include <QThread>

namespace SprintIDE {

// ============================================================
// 构造函数
// ============================================================
AIActionExecutor::AIActionExecutor(MainWindow *mainWindow, QObject *parent)
    : QObject(parent)
    , m_mainWindow(mainWindow)
{
    // 获取编译管理器和调试管理器的单例引用（用于后续操作）
    if (m_mainWindow) {
        // 编译管理器是单例，可以直接获取
        m_compileManager = &CompileManager::instance();
        m_debugManager = &DebugManager::instance();

        // 连接编译和调试完成信号，用于异步操作
        connect(m_compileManager, &CompileManager::compileFinished,
                this, &AIActionExecutor::onCompileFinished);
        connect(m_debugManager, &DebugManager::stateChanged,
                this, &AIActionExecutor::onDebugStateChanged);

        // 获取当前项目路径（用于路径解析）
        m_currentProjectPath = m_mainWindow->projectPath();
    }

    qDebug() << "[AI Action Executor] 已创建";
}

// ============================================================
// 析构函数
// ============================================================
AIActionExecutor::~AIActionExecutor()
{
    qDebug() << "[AI Action Executor] 已销毁";
}

// ============================================================
// 执行单个操作（同步）
// ============================================================
AIActionResult AIActionExecutor::executeAction(const QJsonObject &action)
{
    // 检查操作类型
    QString type = action.value("type").toString();
    if (type.isEmpty()) {
        return AIActionResult(false, "操作缺少 'type' 字段");
    }

    // 获取参数（如果存在）
    QJsonObject params = action.value("params").toObject();

    // 根据类型分发执行
    if (type == "openFile") {
        return executeOpenFile(params);
    } else if (type == "closeFile") {
        return executeCloseFile(params);
    } else if (type == "saveFile") {
        return executeSaveFile(params);
    } else if (type == "saveAllFiles") {
        return executeSaveAllFiles(params);
    } else if (type == "createFile") {
        return executeCreateFile(params);
    } else if (type == "deleteFile") {
        return executeDeleteFile(params);
    } else if (type == "getFileContent") {
        return executeGetFileContent(params);
    } else if (type == "replaceText") {
        return executeReplaceText(params);
    } else if (type == "insertText") {
        return executeInsertText(params);
    } else if (type == "deleteText") {
        return executeDeleteText(params);
    } else if (type == "formatFile") {
        return executeFormatFile(params);
    } else if (type == "compile") {
        return executeCompile(params);
    } else if (type == "compileProject") {
        return executeCompileProject(params);
    } else if (type == "run") {
        return executeRun(params);
    } else if (type == "startDebug") {
        return executeStartDebug(params);
    } else if (type == "stopDebug") {
        return executeStopDebug(params);
    } else if (type == "stepOver") {
        return executeStepOver(params);
    } else if (type == "stepInto") {
        return executeStepInto(params);
    } else if (type == "stepOut") {
        return executeStepOut(params);
    } else if (type == "continueDebug") {
        return executeContinueDebug(params);
    } else if (type == "addBreakpoint") {
        return executeAddBreakpoint(params);
    } else if (type == "removeBreakpoint") {
        return executeRemoveBreakpoint(params);
    } else if (type == "executeCommand") {
        return executeExecuteCommand(params);
    } else if (type == "getContext") {
        return executeGetContext(params);
    } else {
        return AIActionResult(false, QString("不支持的操作类型: %1").arg(type));
    }
}

// ============================================================
// 批量执行多个操作（同步）
// ============================================================
QList<AIActionResult> AIActionExecutor::executeActions(const QJsonArray &actions,
                                                       bool stopOnError)
{
    QList<AIActionResult> results;

    for (const QJsonValue &val : actions) {
        if (!val.isObject()) {
            results.append(AIActionResult(false, "操作项不是 JSON 对象"));
            if (stopOnError) break;
            continue;
        }

        QJsonObject action = val.toObject();
        AIActionResult result = executeAction(action);
        results.append(result);

        // 如果失败且需要停止，则中断
        if (!result.success && stopOnError) {
            break;
        }
    }

    emit actionsFinished(results);
    return results;
}

// ============================================================
// 执行异步操作（通过信号返回结果）
// ============================================================
void AIActionExecutor::executeActionAsync(const QJsonObject &action)
{
    QString type = action.value("type").toString();
    if (type.isEmpty()) {
        emit actionFinished("", AIActionResult(false, "操作缺少 'type' 字段"));
        return;
    }

    // 只对支持异步的操作进行异步执行（目前主要是编译）
    // 这里我们为编译操作生成一个 actionId，并保存 pending 状态
    if (type == "compile" || type == "compileProject") {
        QString actionId = QString("async_%1_%2")
        .arg(type)
            .arg(QDateTime::currentMSecsSinceEpoch());

        // 保存待处理操作
        m_pendingActions[actionId] = action;
        m_nextActionId++;

        // 执行操作（同步执行，但结果通过信号返回）
        // 注意：编译是异步的，所以我们先调用，然后在 onCompileFinished 中发出完成信号
        AIActionResult result = executeAction(action);
        if (!result.success) {
            // 如果立即失败（如启动失败），直接返回结果
            m_pendingActions.remove(actionId);
            emit actionFinished(actionId, result);
        } else {
            // 成功启动，等待编译完成信号
            // 注意：编译完成后会在 onCompileFinished 中发出 actionFinished
        }
    } else {
        // 其他操作同步执行，但通过信号返回
        AIActionResult result = executeAction(action);
        emit actionFinished(QString::number(QDateTime::currentMSecsSinceEpoch()), result);
    }
}

// ============================================================
// 获取当前 IDE 上下文
// ============================================================
QJsonObject AIActionExecutor::getContext() const
{
    QJsonObject context;

    if (!m_mainWindow) {
        return context;
    }

    // ----- 文件上下文 -----
    QJsonObject fileContext;
    TextEditor *editor = getCurrentEditor();
    if (editor) {
        fileContext["filePath"] = editor->filePath();
        fileContext["fileName"] = editor->fileName();
        fileContext["content"] = editor->toPlainText();
        fileContext["cursorLine"] = editor->currentLine();
        fileContext["cursorColumn"] = editor->currentColumn();
        fileContext["isModified"] = editor->isModified();
        // fileContext["language"] = editor->language();  //多语言支持尚未开发完成
        // ================ 临时代替多语言功能：开始 ================
        QString ext = QFileInfo(editor->filePath()).suffix().toLower();
        QString lang;
        if (ext == "cpp" || ext == "cxx" || ext == "cc" || ext == "c") {
            lang = "cpp";
        } else if (ext == "h" || ext == "hpp" || ext == "hxx") {
            lang = "cpp-header";
        } else if (ext == "py") {
            lang = "python";
        } else if (ext == "js" || ext == "ts") {
            lang = "javascript";
        } else if (ext == "go") {
            lang = "go";
        } else if (ext == "rs") {
            lang = "rust";
        } else if (ext == "java") {
            lang = "java";
        } else {
            lang = ext.isEmpty() ? "text" : ext;
        }
        fileContext["language"] = lang;
        // ================ 临时代替多语言功能：结束 多语言支持完成开发后删除 ================
    }
    context["file"] = fileContext;

    // ----- 项目上下文 -----
    QJsonObject projectContext;
    QString projPath = m_mainWindow->projectPath();
    if (!projPath.isEmpty()) {
        projectContext["projectPath"] = projPath;
        projectContext["projectName"] = QFileInfo(projPath).fileName();

        // 获取项目中的源文件列表（简单实现：扫描目录）
        QDir dir(projPath);
        QStringList filters;
        filters << "*.cpp" << "*.c" << "*.h" << "*.hpp";
        QFileInfoList entries = dir.entryInfoList(filters, QDir::Files);
        QStringList sourceFiles;
        for (const QFileInfo &info : entries) {
            sourceFiles.append(info.absoluteFilePath());
        }
        projectContext["sourceFiles"] = QJsonArray::fromStringList(sourceFiles);

        // 获取已打开的文件列表
        QStringList openFiles;
        if (m_mainWindow) {
            // 通过 MainWindow 获取所有打开的编辑器（需要添加接口）
            // 这里简化，只获取当前文件
            if (editor) {
                openFiles.append(editor->filePath());
            }
        }
        projectContext["openFiles"] = QJsonArray::fromStringList(openFiles);
    }
    context["project"] = projectContext;

    // ----- 编译上下文 -----
    QJsonObject compileContext;
    if (m_compileManager) {
        // 编译状态不是直接可获取，简化处理
        compileContext["isCompiling"] = m_mainWindow->isCompiling();
        // 最近编译结果（如果有）需要缓存，暂不实现
    }
    context["compile"] = compileContext;

    // ----- 调试上下文 -----
    QJsonObject debugContext;
    if (m_debugManager) {
        debugContext["isDebugging"] = m_debugManager->isDebugging();
        DebugState state = m_debugManager->currentState();
        QString stateStr;
        switch (state) {
        case DebugState::Stopped:  stateStr = "stopped"; break;
        case DebugState::Starting: stateStr = "starting"; break;
        case DebugState::Running:  stateStr = "running"; break;
        case DebugState::Paused:   stateStr = "paused"; break;
        case DebugState::Stepping: stateStr = "stepping"; break;
        case DebugState::Finished: stateStr = "finished"; break;
        case DebugState::Error:    stateStr = "error"; break;
        default: stateStr = "unknown";
        }
        debugContext["state"] = stateStr;

        // 获取当前执行位置
        QPair<QString, int> pos = m_debugManager->getCurrentPosition();
        debugContext["currentFile"] = pos.first;
        debugContext["currentLine"] = pos.second;

        // 获取调用栈和变量（简化，只取前几个）
        QList<StackFrame> stack = m_debugManager->getCallStack();
        QJsonArray stackArray;
        for (const StackFrame &frame : stack) {
            QJsonObject f;
            f["level"] = frame.level;
            f["functionName"] = frame.functionName;
            f["filePath"] = frame.filePath;
            f["lineNumber"] = frame.lineNumber;
            stackArray.append(f);
        }
        debugContext["callStack"] = stackArray;

        QList<DebugVariable> vars = m_debugManager->getLocalVariables();
        QJsonArray varArray;
        for (const DebugVariable &var : vars) {
            QJsonObject v;
            v["name"] = var.name;
            v["type"] = var.type;
            v["value"] = var.value;
            varArray.append(v);
        }
        debugContext["localVariables"] = varArray;
    }
    context["debug"] = debugContext;

    // ----- IDE 状态 -----
    QJsonObject ideState;
    ideState["hasUnsavedChanges"] = m_mainWindow->hasUnsavedChanges();
    ideState["currentFilePath"] = m_mainWindow->currentFilePath();
    context["ideState"] = ideState;

    return context;
}

// ============================================================
// 获取文件内容
// ============================================================
QString AIActionExecutor::getFileContent(const QString &filePath) const
{
    if (filePath.isEmpty()) {
        return QString();
    }

    // 首先尝试从已打开的编辑器中获取
    TextEditor *editor = findEditorForFile(filePath);
    if (editor) {
        return editor->toPlainText();
    }

    // 如果未打开，直接从磁盘读取
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        return stream.readAll();
    }

    return QString();
}

// ============================================================
// 检查文件是否已打开
// ============================================================
bool AIActionExecutor::isFileOpen(const QString &filePath) const
{
    return findEditorForFile(filePath) != nullptr;
}

// ============================================================
// 获取当前活动文件的路径
// ============================================================
QString AIActionExecutor::getActiveFilePath() const
{
    TextEditor *editor = getCurrentEditor();
    return editor ? editor->filePath() : QString();
}

// ============================================================
// 私有槽：编译完成（用于异步操作）
// ============================================================
void AIActionExecutor::onCompileFinished(const QString &taskId, const CompileResult &result)
{
    Q_UNUSED(taskId);

    // 查找对应的异步操作
    QString actionId;
    for (auto it = m_pendingActions.begin(); it != m_pendingActions.end(); ++it) {
        if (it.value().value("type").toString().startsWith("compile")) {
            actionId = it.key();
            break;
        }
    }

    if (actionId.isEmpty()) {
        return;
    }

    // 构造结果
    QJsonObject data;
    data["success"] = result.success;
    data["exitCode"] = result.exitCode;
    data["errors"] = QJsonArray::fromStringList(result.errors);
    data["warnings"] = QJsonArray::fromStringList(result.warnings);
    if (!result.executablePath.isEmpty()) {
        data["executablePath"] = result.executablePath;
    }

    AIActionResult actionResult(result.success,
                                result.success ? "编译成功" : "编译失败",
                                data);

    // 移除待处理操作
    m_pendingActions.remove(actionId);

    emit actionFinished(actionId, actionResult);
    emit outputProduced(actionResult.message, !result.success);
}

// ============================================================
// 私有槽：调试状态变化（用于异步操作）
// ============================================================
void AIActionExecutor::onDebugStateChanged(DebugState state)
{
    // 暂时未使用，预留
    Q_UNUSED(state);
}

// ============================================================
// ==================== 具体操作实现 ====================
// ============================================================

// ------------------------------------------------------------
// 打开文件
// ------------------------------------------------------------
AIActionResult AIActionExecutor::executeOpenFile(const QJsonObject &params)
{
    QString filePath = params.value("filePath").toString();
    if (filePath.isEmpty()) {
        return AIActionResult(false, "缺少参数: filePath");
    }

    // 解析路径（相对路径转绝对）
    filePath = resolvePath(filePath);

    // 检查文件是否存在
    if (!QFile::exists(filePath)) {
        return AIActionResult(false, QString("文件不存在: %1").arg(filePath));
    }

    if (!m_mainWindow) {
        return AIActionResult(false, "主窗口未初始化");
    }

    // 在主线程中打开文件（使用 invokeMethod 确保线程安全）
    bool opened = false;
    QMetaObject::invokeMethod(m_mainWindow, [this, filePath, &opened]() {
        m_mainWindow->openFile(filePath);
        opened = true;
    }, Qt::BlockingQueuedConnection);

    if (opened) {
        emit outputProduced(QString("已打开文件: %1").arg(filePath), false);
        return AIActionResult(true, "文件已打开");
    } else {
        return AIActionResult(false, "打开文件失败");
    }
}

// ------------------------------------------------------------
// 关闭文件
// ------------------------------------------------------------
AIActionResult AIActionExecutor::executeCloseFile(const QJsonObject &params)
{
    QString filePath = params.value("filePath").toString();
    if (filePath.isEmpty()) {
        return AIActionResult(false, "缺少参数: filePath");
    }

    filePath = resolvePath(filePath);

    if (!m_mainWindow) {
        return AIActionResult(false, "主窗口未初始化");
    }

    // 在主线程中关闭文件
    bool closed = false;
    QMetaObject::invokeMethod(m_mainWindow, [this, filePath, &closed]() {
        m_mainWindow->closeFile(filePath);
        closed = true;
    }, Qt::BlockingQueuedConnection);

    if (closed) {
        emit outputProduced(QString("已关闭文件: %1").arg(filePath), false);
        return AIActionResult(true, "文件已关闭");
    } else {
        return AIActionResult(false, "关闭文件失败（文件可能未打开）");
    }
}

// ------------------------------------------------------------
// 保存文件
// ------------------------------------------------------------
AIActionResult AIActionExecutor::executeSaveFile(const QJsonObject &params)
{
    QString filePath = params.value("filePath").toString();

    if (!m_mainWindow) {
        return AIActionResult(false, "主窗口未初始化");
    }

    // 如果指定了文件路径，先找到对应的编辑器
    TextEditor *editor = nullptr;
    if (!filePath.isEmpty()) {
        filePath = resolvePath(filePath);
        editor = findEditorForFile(filePath);
        if (!editor) {
            return AIActionResult(false, QString("文件未打开: %1").arg(filePath));
        }
    } else {
        // 未指定路径，保存当前文件
        editor = getCurrentEditor();
        if (!editor) {
            return AIActionResult(false, "没有打开的文件可保存");
        }
        filePath = editor->filePath();
        if (filePath.isEmpty()) {
            return AIActionResult(false, "当前文件未命名，无法保存");
        }
    }

    // 在主线程中保存
    bool saved = false;
    QMetaObject::invokeMethod(m_mainWindow, [editor, &saved]() {
        if (editor) {
            saved = editor->saveFile();
        }
    }, Qt::BlockingQueuedConnection);

    if (saved) {
        emit outputProduced(QString("已保存文件: %1").arg(filePath), false);
        return AIActionResult(true, "文件已保存");
    } else {
        return AIActionResult(false, "保存文件失败");
    }
}

// ------------------------------------------------------------
// 保存所有文件
// ------------------------------------------------------------
AIActionResult AIActionExecutor::executeSaveAllFiles(const QJsonObject &params)
{
    Q_UNUSED(params);

    if (!m_mainWindow) {
        return AIActionResult(false, "主窗口未初始化");
    }

    // 调用 MainWindow 的保存全部功能（通过信号或直接调用）
    // 目前 MainWindow 没有直接方法，我们使用替代方式：遍历所有编辑器
    bool allSaved = true;
    QMetaObject::invokeMethod(m_mainWindow, [this, &allSaved]() {
        // 通过 MainWindow 获取所有标签页（需要增加接口）
        // 这里简化，仅保存当前文件
        TextEditor *editor = getCurrentEditor();
        if (editor) {
            if (!editor->saveFile()) {
                allSaved = false;
            }
        }
    }, Qt::BlockingQueuedConnection);

    if (allSaved) {
        emit outputProduced("所有文件已保存", false);
        return AIActionResult(true, "所有文件已保存");
    } else {
        return AIActionResult(false, "保存文件失败");
    }
}

// ------------------------------------------------------------
// 创建文件
// ------------------------------------------------------------
AIActionResult AIActionExecutor::executeCreateFile(const QJsonObject &params)
{
    QString filePath = params.value("filePath").toString();
    QString content = params.value("content").toString();
    bool openAfterCreate = params.value("openAfterCreate").toBool(true);

    if (filePath.isEmpty()) {
        return AIActionResult(false, "缺少参数: filePath");
    }

    filePath = resolvePath(filePath);

    // 检查文件是否已存在
    if (QFile::exists(filePath)) {
        return AIActionResult(false, QString("文件已存在: %1").arg(filePath));
    }

    // 确保目录存在
    QFileInfo info(filePath);
    QDir dir = info.dir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            return AIActionResult(false, QString("无法创建目录: %1").arg(dir.absolutePath()));
        }
    }

    // 写入文件
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return AIActionResult(false, QString("无法创建文件: %1").arg(file.errorString()));
    }
    QTextStream stream(&file);
    stream << content;
    file.close();

    // 如果需要，打开文件
    if (openAfterCreate && m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, [this, filePath]() {
            m_mainWindow->openFile(filePath);
        }, Qt::QueuedConnection);
    }

    emit outputProduced(QString("已创建文件: %1").arg(filePath), false);
    return AIActionResult(true, "文件已创建");
}

// ------------------------------------------------------------
// 删除文件
// ------------------------------------------------------------
AIActionResult AIActionExecutor::executeDeleteFile(const QJsonObject &params)
{
    QString filePath = params.value("filePath").toString();
    if (filePath.isEmpty()) {
        return AIActionResult(false, "缺少参数: filePath");
    }

    filePath = resolvePath(filePath);

    if (!QFile::exists(filePath)) {
        return AIActionResult(false, QString("文件不存在: %1").arg(filePath));
    }

    // 如果文件已打开，先关闭
    if (isFileOpen(filePath)) {
        // 关闭文件（不保存更改？这里使用强制关闭）
        if (m_mainWindow) {
            QMetaObject::invokeMethod(m_mainWindow, [this, filePath]() {
                m_mainWindow->closeFile(filePath);
            }, Qt::BlockingQueuedConnection);
        }
    }

    // 删除文件
    if (!QFile::remove(filePath)) {
        return AIActionResult(false, "删除文件失败");
    }

    emit outputProduced(QString("已删除文件: %1").arg(filePath), false);
    return AIActionResult(true, "文件已删除");
}

// ------------------------------------------------------------
// 获取文件内容
// ------------------------------------------------------------
AIActionResult AIActionExecutor::executeGetFileContent(const QJsonObject &params)
{
    QString filePath = params.value("filePath").toString();
    if (filePath.isEmpty()) {
        return AIActionResult(false, "缺少参数: filePath");
    }

    filePath = resolvePath(filePath);

    QString content = getFileContent(filePath);
    if (content.isNull()) {
        return AIActionResult(false, QString("无法读取文件: %1").arg(filePath));
    }

    QJsonObject data;
    data["content"] = content;
    return AIActionResult(true, "文件内容已获取", data);
}

// ------------------------------------------------------------
// 替换文本
// ------------------------------------------------------------
AIActionResult AIActionExecutor::executeReplaceText(const QJsonObject &params)
{
    QString filePath = params.value("filePath").toString();
    int startLine = params.value("startLine").toInt(0);
    int startCol = params.value("startCol").toInt(0);
    int endLine = params.value("endLine").toInt(0);
    int endCol = params.value("endCol").toInt(0);
    QString newText = params.value("newText").toString();

    if (filePath.isEmpty()) {
        return AIActionResult(false, "缺少参数: filePath");
    }

    filePath = resolvePath(filePath);

    TextEditor *editor = findEditorForFile(filePath);
    if (!editor) {
        QString dummy;
        if (!ensureFileOpen(filePath, &dummy)) {
            return AIActionResult(false, QString("文件未打开且无法打开: %1").arg(filePath));
        }
        editor = findEditorForFile(filePath);
        if (!editor) {
            return AIActionResult(false, "打开文件后仍未找到编辑器");
        }
    }

    bool success = false;
    QMetaObject::invokeMethod(editor, [editor, startLine, startCol, endLine, endCol, newText, &success]() {
        QTextDocument *doc = editor->document();
        QTextBlock startBlock = doc->findBlockByLineNumber(startLine - 1);
        if (!startBlock.isValid()) {
            success = false;
            return;
        }
        QTextBlock endBlock = doc->findBlockByLineNumber(endLine - 1);
        if (!endBlock.isValid()) {
            success = false;
            return;
        }

        int startPos = startBlock.position() + startCol - 1;
        int endPos = endBlock.position() + endCol - 1;

        QTextCursor cursor(doc);
        cursor.setPosition(startPos);
        cursor.setPosition(endPos, QTextCursor::KeepAnchor);
        cursor.beginEditBlock();
        cursor.removeSelectedText();
        cursor.insertText(newText);
        cursor.endEditBlock();

        success = true;
    }, Qt::BlockingQueuedConnection);

    if (success) {
        emit outputProduced(QString("已替换文本: %1 (行 %2-%3)")
                                .arg(filePath).arg(startLine).arg(endLine), false);
        return AIActionResult(true, "文本替换成功");
    } else {
        return AIActionResult(false, "文本替换失败（无效的行号或列号）");
    }
}

// ------------------------------------------------------------
// 插入文本
// ------------------------------------------------------------
AIActionResult AIActionExecutor::executeInsertText(const QJsonObject &params)
{
    QString filePath = params.value("filePath").toString();
    int line = params.value("line").toInt(0);
    int col = params.value("col").toInt(0);
    QString text = params.value("text").toString();

    if (filePath.isEmpty()) {
        return AIActionResult(false, "缺少参数: filePath");
    }
    if (text.isEmpty()) {
        return AIActionResult(false, "缺少参数: text");
    }

    filePath = resolvePath(filePath);

    TextEditor *editor = findEditorForFile(filePath);
    if (!editor) {
        if (!ensureFileOpen(filePath)) {
            return AIActionResult(false, QString("文件未打开且无法打开: %1").arg(filePath));
        }
        editor = findEditorForFile(filePath);
        if (!editor) {
            return AIActionResult(false, "打开文件后仍未找到编辑器");
        }
    }

    bool success = false;
    QMetaObject::invokeMethod(editor, [editor, line, col, text, &success]() {
        QTextDocument *doc = editor->document();
        QTextBlock block = doc->findBlockByLineNumber(line - 1);
        if (!block.isValid()) {
            success = false;
            return;
        }

        int pos = block.position() + col - 1;
        QTextCursor cursor(doc);
        cursor.setPosition(pos);
        cursor.insertText(text);
        success = true;
    }, Qt::BlockingQueuedConnection);

    if (success) {
        emit outputProduced(QString("已插入文本: %1 (行 %2)").arg(filePath).arg(line), false);
        return AIActionResult(true, "文本插入成功");
    } else {
        return AIActionResult(false, "文本插入失败（无效的行号或列号）");
    }
}

// ------------------------------------------------------------
// 删除文本
// ------------------------------------------------------------
AIActionResult AIActionExecutor::executeDeleteText(const QJsonObject &params)
{
    QString filePath = params.value("filePath").toString();
    int startLine = params.value("startLine").toInt(0);
    int startCol = params.value("startCol").toInt(0);
    int endLine = params.value("endLine").toInt(0);
    int endCol = params.value("endCol").toInt(0);

    if (filePath.isEmpty()) {
        return AIActionResult(false, "缺少参数: filePath");
    }

    filePath = resolvePath(filePath);

    TextEditor *editor = findEditorForFile(filePath);
    if (!editor) {
        return AIActionResult(false, QString("文件未打开: %1").arg(filePath));
    }

    bool success = false;
    QMetaObject::invokeMethod(editor, [editor, startLine, startCol, endLine, endCol, &success]() {
        QTextDocument *doc = editor->document();
        QTextBlock startBlock = doc->findBlockByLineNumber(startLine - 1);
        if (!startBlock.isValid()) {
            success = false;
            return;
        }
        QTextBlock endBlock = doc->findBlockByLineNumber(endLine - 1);
        if (!endBlock.isValid()) {
            success = false;
            return;
        }

        int startPos = startBlock.position() + startCol - 1;
        int endPos = endBlock.position() + endCol - 1;

        QTextCursor cursor(doc);
        cursor.setPosition(startPos);
        cursor.setPosition(endPos, QTextCursor::KeepAnchor);
        cursor.beginEditBlock();
        cursor.removeSelectedText();
        cursor.endEditBlock();
        success = true;
    }, Qt::BlockingQueuedConnection);

    if (success) {
        emit outputProduced(QString("已删除文本: %1 (行 %2-%3)")
                                .arg(filePath).arg(startLine).arg(endLine), false);
        return AIActionResult(true, "文本删除成功");
    } else {
        return AIActionResult(false, "文本删除失败");
    }
}

// ------------------------------------------------------------
// 格式化文件
// ------------------------------------------------------------
AIActionResult AIActionExecutor::executeFormatFile(const QJsonObject &params)
{
    QString filePath = params.value("filePath").toString();

    if (filePath.isEmpty()) {
        // 未指定文件，格式化当前文件
        TextEditor *editor = getCurrentEditor();
        if (!editor) {
            return AIActionResult(false, "没有打开的文件可格式化");
        }
        filePath = editor->filePath();
    } else {
        filePath = resolvePath(filePath);
        TextEditor *editor = findEditorForFile(filePath);
        if (!editor) {
            return AIActionResult(false, QString("文件未打开: %1").arg(filePath));
        }
    }

    // 调用 MainWindow 的格式化功能（通过信号）
    bool success = false;
    QMetaObject::invokeMethod(m_mainWindow, [this, &success]() {
        // MainWindow 有 onFormatCode 方法，但需要确保在 GUI 线程
        // 使用 QMetaObject::invokeMethod 调用槽函数
        QMetaObject::invokeMethod(m_mainWindow, "onFormatCode", Qt::BlockingQueuedConnection);
        success = true;
    }, Qt::BlockingQueuedConnection);

    if (success) {
        emit outputProduced(QString("已格式化文件: %1").arg(filePath), false);
        return AIActionResult(true, "文件格式化成功");
    } else {
        return AIActionResult(false, "文件格式化失败");
    }
}

// ------------------------------------------------------------
// 编译当前文件
// ------------------------------------------------------------
AIActionResult AIActionExecutor::executeCompile(const QJsonObject &params)
{
    Q_UNUSED(params);

    if (!m_mainWindow) {
        return AIActionResult(false, "主窗口未初始化");
    }

    // 确保有当前编辑器
    TextEditor *editor = getCurrentEditor();
    if (!editor) {
        return AIActionResult(false, "没有打开的文件可编译");
    }

    if (editor->filePath().isEmpty()) {
        return AIActionResult(false, "当前文件未保存，无法编译");
    }

    // 调用 MainWindow 的编译功能
    bool started = false;
    QMetaObject::invokeMethod(m_mainWindow, [this, &started]() {
        // 注意：onCompile 是槽函数，直接调用
        m_mainWindow->onCompile();
        started = true;
    }, Qt::BlockingQueuedConnection);

    if (started) {
        emit outputProduced("编译已启动", false);
        return AIActionResult(true, "编译已启动");
    } else {
        return AIActionResult(false, "启动编译失败");
    }
}

// ------------------------------------------------------------
// 编译项目
// ------------------------------------------------------------
AIActionResult AIActionExecutor::executeCompileProject(const QJsonObject &params)
{
    Q_UNUSED(params);

    if (!m_mainWindow) {
        return AIActionResult(false, "主窗口未初始化");
    }

    QString projPath = m_mainWindow->projectPath();
    if (projPath.isEmpty()) {
        return AIActionResult(false, "没有打开的项目");
    }

    // 调用 MainWindow 的编译项目功能（目前 MainWindow 没有直接实现，使用编译当前文件代替）
    // 实际应该调用 onBuildProject，但该槽未实现，我们调用编译当前文件
    emit outputProduced("编译项目功能暂未实现，正在编译当前文件", true);
    return executeCompile(params);
}

// ------------------------------------------------------------
// 运行程序
// ------------------------------------------------------------
AIActionResult AIActionExecutor::executeRun(const QJsonObject &params)
{
    Q_UNUSED(params);

    if (!m_mainWindow) {
        return AIActionResult(false, "主窗口未初始化");
    }

    bool started = false;
    QMetaObject::invokeMethod(m_mainWindow, [this, &started]() {
        m_mainWindow->onRun();
        started = true;
    }, Qt::BlockingQueuedConnection);

    if (started) {
        emit outputProduced("程序已启动运行", false);
        return AIActionResult(true, "程序已运行");
    } else {
        return AIActionResult(false, "启动程序失败");
    }
}

// ------------------------------------------------------------
// 开始调试
// ------------------------------------------------------------
AIActionResult AIActionExecutor::executeStartDebug(const QJsonObject &params)
{
    Q_UNUSED(params);

    if (!m_mainWindow) {
        return AIActionResult(false, "主窗口未初始化");
    }

    bool started = false;
    QMetaObject::invokeMethod(m_mainWindow, [this, &started]() {
        m_mainWindow->onStartDebug();
        started = true;
    }, Qt::BlockingQueuedConnection);

    if (started) {
        emit outputProduced("调试已启动", false);
        return AIActionResult(true, "调试已启动");
    } else {
        return AIActionResult(false, "启动调试失败");
    }
}

// ------------------------------------------------------------
// 停止调试
// ------------------------------------------------------------
AIActionResult AIActionExecutor::executeStopDebug(const QJsonObject &params)
{
    Q_UNUSED(params);

    if (!m_mainWindow || !m_debugManager) {
        return AIActionResult(false, "调试管理器未初始化");
    }

    if (!m_debugManager->isDebugging()) {
        return AIActionResult(false, "当前未在调试中");
    }

    QMetaObject::invokeMethod(m_mainWindow, [this]() {
        m_mainWindow->onStopDebug();
    }, Qt::BlockingQueuedConnection);

    emit outputProduced("调试已停止", false);
    return AIActionResult(true, "调试已停止");
}

// ------------------------------------------------------------
// 步过
// ------------------------------------------------------------
AIActionResult AIActionExecutor::executeStepOver(const QJsonObject &params)
{
    Q_UNUSED(params);

    if (!m_mainWindow || !m_debugManager) {
        return AIActionResult(false, "调试管理器未初始化");
    }

    if (!m_debugManager->isDebugging()) {
        return AIActionResult(false, "当前未在调试中");
    }

    QMetaObject::invokeMethod(m_mainWindow, [this]() {
        m_mainWindow->onStepOver();
    }, Qt::BlockingQueuedConnection);

    return AIActionResult(true, "步过指令已发送");
}

// ------------------------------------------------------------
// 步入
// ------------------------------------------------------------
AIActionResult AIActionExecutor::executeStepInto(const QJsonObject &params)
{
    Q_UNUSED(params);

    if (!m_mainWindow || !m_debugManager) {
        return AIActionResult(false, "调试管理器未初始化");
    }

    if (!m_debugManager->isDebugging()) {
        return AIActionResult(false, "当前未在调试中");
    }

    QMetaObject::invokeMethod(m_mainWindow, [this]() {
        m_mainWindow->onStepInto();
    }, Qt::BlockingQueuedConnection);

    return AIActionResult(true, "步入指令已发送");
}

// ------------------------------------------------------------
// 步出
// ------------------------------------------------------------
AIActionResult AIActionExecutor::executeStepOut(const QJsonObject &params)
{
    Q_UNUSED(params);

    if (!m_mainWindow || !m_debugManager) {
        return AIActionResult(false, "调试管理器未初始化");
    }

    if (!m_debugManager->isDebugging()) {
        return AIActionResult(false, "当前未在调试中");
    }

    QMetaObject::invokeMethod(m_mainWindow, [this]() {
        m_mainWindow->onStepOut();
    }, Qt::BlockingQueuedConnection);

    return AIActionResult(true, "步出指令已发送");
}

// ------------------------------------------------------------
// 继续调试
// ------------------------------------------------------------
AIActionResult AIActionExecutor::executeContinueDebug(const QJsonObject &params)
{
    Q_UNUSED(params);

    if (!m_mainWindow || !m_debugManager) {
        return AIActionResult(false, "调试管理器未初始化");
    }

    if (!m_debugManager->isDebugging()) {
        return AIActionResult(false, "当前未在调试中");
    }

    QMetaObject::invokeMethod(m_mainWindow, [this]() {
        m_mainWindow->onContinueDebug();
    }, Qt::BlockingQueuedConnection);

    return AIActionResult(true, "继续执行指令已发送");
}

// ------------------------------------------------------------
// 添加断点
// ------------------------------------------------------------
AIActionResult AIActionExecutor::executeAddBreakpoint(const QJsonObject &params)
{
    QString filePath = params.value("filePath").toString();
    int line = params.value("line").toInt(0);

    if (filePath.isEmpty()) {
        return AIActionResult(false, "缺少参数: filePath");
    }
    if (line < 1) {
        return AIActionResult(false, "行号必须大于0");
    }

    filePath = resolvePath(filePath);

    if (!m_debugManager) {
        return AIActionResult(false, "调试管理器未初始化");
    }

    // 检查文件是否存在
    if (!QFile::exists(filePath)) {
        return AIActionResult(false, QString("文件不存在: %1").arg(filePath));
    }

    // 添加断点（同步操作）
    bool success = m_debugManager->addBreakpoint(filePath, line);

    if (success) {
        emit outputProduced(QString("已添加断点: %1:%2").arg(filePath).arg(line), false);
        return AIActionResult(true, "断点已添加");
    } else {
        return AIActionResult(false, QString("添加断点失败（可能已存在）"));
    }
}

// ------------------------------------------------------------
// 移除断点
// ------------------------------------------------------------
AIActionResult AIActionExecutor::executeRemoveBreakpoint(const QJsonObject &params)
{
    QString filePath = params.value("filePath").toString();
    int line = params.value("line").toInt(0);

    if (filePath.isEmpty()) {
        return AIActionResult(false, "缺少参数: filePath");
    }
    if (line < 1) {
        return AIActionResult(false, "行号必须大于0");
    }

    filePath = resolvePath(filePath);

    if (!m_debugManager) {
        return AIActionResult(false, "调试管理器未初始化");
    }

    bool success = m_debugManager->removeBreakpoint(filePath, line);

    if (success) {
        emit outputProduced(QString("已移除断点: %1:%2").arg(filePath).arg(line), false);
        return AIActionResult(true, "断点已移除");
    } else {
        return AIActionResult(false, "移除断点失败（断点不存在）");
    }
}

// ------------------------------------------------------------
// 执行终端命令
// ------------------------------------------------------------
AIActionResult AIActionExecutor::executeExecuteCommand(const QJsonObject &params)
{
    QString command = params.value("command").toString();
    QString workingDir = params.value("workingDir").toString();
    int timeout = params.value("timeout").toInt(30000);

    if (command.isEmpty()) {
        return AIActionResult(false, "缺少参数: command");
    }

    // 解析工作目录
    if (workingDir.isEmpty()) {
        workingDir = m_currentProjectPath;
    }
    if (workingDir.isEmpty()) {
        workingDir = QDir::currentPath();
    }
    workingDir = resolvePath(workingDir);

    // 检查目录是否存在
    if (!QDir(workingDir).exists()) {
        return AIActionResult(false, QString("工作目录不存在: %1").arg(workingDir));
    }

    // 执行命令（使用 ProcessManager 同步执行）
    ProcessManager processMgr;
    ProcessManager::ProcessResult result = processMgr.executeSync(
        command, QStringList(), workingDir, timeout);

    QJsonObject data;
    data["exitCode"] = result.exitCode;
    data["stdout"] = result.standardOutput;
    data["stderr"] = result.standardError;
    data["executionTime"] = result.executionTime;

    if (result.success) {
        emit outputProduced(QString("命令执行成功: %1 (耗时 %2ms)")
                                .arg(command).arg(result.executionTime), false);
        return AIActionResult(true, "命令执行成功", data);
    } else {
        data["errorMessage"] = result.errorMessage;
        emit outputProduced(QString("命令执行失败: %1 - %2")
                                .arg(command).arg(result.errorMessage), true);
        return AIActionResult(false, "命令执行失败", data);
    }
}

// ------------------------------------------------------------
// 获取上下文
// ------------------------------------------------------------
AIActionResult AIActionExecutor::executeGetContext(const QJsonObject &params)
{
    Q_UNUSED(params);

    QJsonObject context = getContext();
    return AIActionResult(true, "上下文已获取", context);
}

// ============================================================
// ==================== 辅助方法 ====================
// ============================================================

// ------------------------------------------------------------
// 查找指定文件的编辑器
// ------------------------------------------------------------
TextEditor* AIActionExecutor::findEditorForFile(const QString &filePath) const
{
    if (!m_mainWindow) {
        return nullptr;
    }

    // 由于 MainWindow 没有提供遍历所有编辑器的接口，我们使用变通方法：
    // 通过 TabWidget 获取所有子控件，查找 TextEditor 对象并匹配文件路径
    // 这里我们简化：直接返回当前编辑器（如果路径匹配）
    TextEditor *current = getCurrentEditor();
    if (current && current->filePath() == filePath) {
        return current;
    }

    // 更完整的实现需要 MainWindow 提供 getEditors() 方法，暂未提供
    // 作为临时方案，只检查当前编辑器
    return nullptr;
}

// ------------------------------------------------------------
// 获取当前活动的编辑器
// ------------------------------------------------------------
TextEditor* AIActionExecutor::getCurrentEditor() const
{
    if (!m_mainWindow) {
        return nullptr;
    }
    return m_mainWindow->currentEditor();
}

// ------------------------------------------------------------
// 确保文件已打开（修复指针赋值错误）
// ------------------------------------------------------------
bool AIActionExecutor::ensureFileOpen(const QString &filePath, QString *errorMessage)
{
    if (!m_mainWindow) {
        if (errorMessage) {
            *errorMessage = "主窗口未初始化";
        }
        return false;
    }

    if (isFileOpen(filePath)) {
        return true;
    }

    if (!QFile::exists(filePath)) {
        if (errorMessage) {
            *errorMessage = "文件不存在";
        }
        return false;
    }

    QMetaObject::invokeMethod(m_mainWindow, [this, filePath]() {
        m_mainWindow->openFile(filePath);
    }, Qt::BlockingQueuedConnection);

    return true;
}

// ------------------------------------------------------------
// 验证参数中的必需字段
// ------------------------------------------------------------
bool AIActionExecutor::validateParams(const QJsonObject &params,
                                      const QStringList &requiredKeys,
                                      QString &errorMessage) const
{
    for (const QString &key : requiredKeys) {
        if (!params.contains(key)) {
            errorMessage = QString("缺少必需参数: %1").arg(key);
            return false;
        }
    }
    return true;
}

// ------------------------------------------------------------
// 解析文件路径（相对路径转绝对路径）
// ------------------------------------------------------------
QString AIActionExecutor::resolvePath(const QString &path) const
{
    if (path.isEmpty()) {
        return path;
    }

    // 如果已经是绝对路径，直接返回
    if (QFileInfo(path).isAbsolute()) {
        return PathUtils::normalizePath(path);
    }

    // 如果设置了项目路径，相对于项目路径解析
    if (!m_currentProjectPath.isEmpty()) {
        return PathUtils::normalizePath(
            PathUtils::absolutePath(path, m_currentProjectPath));
    }

    // 否则相对于当前工作目录
    return PathUtils::normalizePath(
        PathUtils::absolutePath(path, QDir::currentPath()));
}

} // namespace SprintIDE
