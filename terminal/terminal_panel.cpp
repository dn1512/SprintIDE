// terminal_panel.cpp
#include "terminal_panel.h"
#include <QScrollBar>
#include <QKeyEvent>
#include <QDir>
#include <QMessageBox>
#include <QApplication>
#include <QStyle>
#include <QProcessEnvironment>
#include <QRegularExpression>

#include "project_manager.h"
#include "python/python_virtual_env.h"
#include "python/python_project_model.h"
#include "theme_manager.h"
#include "Application.h"
#include "config_manager.h"

namespace SprintIDE {

TerminalPanel::TerminalPanel(QWidget* parent)
    : QWidget(parent)
    , m_process(nullptr)
    , m_isCommandRunning(false)
    , m_historyIndex(-1)
{
    setupUI();
    setObjectName("TerminalPanel");
    setMinimumHeight(0);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);

    // 主题切换时重染已有输出
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &TerminalPanel::recolorAllOutput);

    // 默认工作目录为当前目录
    m_workingDirectory = QDir::currentPath();
    // 自动检测并记录当前虚拟环境
    updateVenvForDirectory(m_workingDirectory);
}

TerminalPanel::~TerminalPanel()
{
    stopCurrentCommand();
    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }
}

void TerminalPanel::setupUI()
{
    setObjectName("TerminalPanel");

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);

    m_outputWidget = new QTextEdit(this);
    m_outputWidget->setObjectName("TerminalOutput");
    m_outputWidget->setReadOnly(true);
    m_outputWidget->setFont(QFont("Menlo", 10));   
    m_outputWidget->setMinimumHeight(100);
    m_outputWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(m_outputWidget);

    // 输入区域
    QHBoxLayout* inputLayout = new QHBoxLayout();
    inputLayout->setContentsMargins(0, 2, 0, 2);

    m_promptLabel = new QLabel("$ ", this);
    m_promptLabel->setObjectName("TerminalPrompt");
    m_promptLabel->setFixedWidth(20);
    inputLayout->addWidget(m_promptLabel);

    m_inputWidget = new QLineEdit(this);
    m_inputWidget->setObjectName("TerminalInput");
    m_inputWidget->setPlaceholderText(tr("输入命令..."));
    m_inputWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_inputWidget->setMinimumWidth(200);
    connect(m_inputWidget, &QLineEdit::returnPressed, this, &TerminalPanel::onInputEntered);
    inputLayout->addWidget(m_inputWidget);

    // 停止按钮
    m_stopButton = new QPushButton("■", this);
    m_stopButton->setObjectName("TerminalStopButton");
    m_stopButton->setMaximumWidth(30);
    m_stopButton->setToolTip(tr("停止当前命令"));
    m_stopButton->setEnabled(false);
    connect(m_stopButton, &QPushButton::clicked, this, &TerminalPanel::stopCurrentCommand);
    inputLayout->addWidget(m_stopButton);

    // 清空按钮
    m_clearButton = new QPushButton(tr("清空"), this);
    m_clearButton->setObjectName("TerminalClearButton");
    m_clearButton->setMaximumWidth(50);
    m_clearButton->setToolTip(tr("清空输出"));
    connect(m_clearButton, &QPushButton::clicked, this, &TerminalPanel::onClearClicked);
    inputLayout->addWidget(m_clearButton);

    layout->addLayout(inputLayout);

    // 初始提示
    appendOutput(tr("终端已准备就绪。输入命令并按 Enter 执行。"), false);
    updatePrompt();
    updateVenvDisplay();

    m_inputWidget->setVisible(true);
    m_inputWidget->setFocus();
    m_inputWidget->setFixedHeight(30);
}

// ========== 更新当前目录的虚拟环境 ==========
void TerminalPanel::updateVenvForDirectory(const QString& dir)
{
    m_currentVenvPath.clear();
    m_currentPythonInterpreter.clear();

    // 优先从 ProjectManager 获取 Python 项目模型
    auto model = ProjectManager::instance()->pythonProjectModel();
    if (model && !model->venvPath().isEmpty()) {
        m_currentVenvPath = model->venvPath();
        m_currentPythonInterpreter = model->interpreterPath();
        return;
    }

    // 如果 ProjectManager 没有模型，尝试手动检测
    QString detected = PythonVirtualEnv::detectVenv(dir);
    if (!detected.isEmpty()) {
        m_currentVenvPath = detected;
        m_currentPythonInterpreter = PythonVirtualEnv::interpreterPath(detected);
    }
}

// ========== 更新提示标签显示虚拟环境信息 ==========
void TerminalPanel::updateVenvDisplay()
{
    if (!m_currentVenvPath.isEmpty()) {
        QString venvName = QFileInfo(m_currentVenvPath).fileName();
        m_promptLabel->setText("(" + venvName + ") $ ");
    } else {
        m_promptLabel->setText("$ ");
    }
}

void TerminalPanel::setWorkingDirectory(const QString& dir)
{
    m_workingDirectory = dir;
    // 更新虚拟环境检测
    updateVenvForDirectory(dir);
    updateVenvDisplay();

    appendOutput(tr("工作目录已切换至: ") + dir, false);
    emit workingDirectoryChanged(dir);

    // 聚焦输入框
    m_inputWidget->setFocus();
}

QString TerminalPanel::workingDirectory() const
{
    return m_workingDirectory;
}

// ============================================================================
//  helper：把 dir 插到 PATH 头；若 PATH 为空，先用系统默认补齐
// ============================================================================
static QString prependToPath(const QProcessEnvironment& env, const QString& dir)
{
    QString path = env.value("PATH");

    // macOS GUI 进程的 systemEnvironment 里 PATH 可能为空
    // 补一个最小可用的默认值，避免后续 shell 找不到基本命令
    if (path.isEmpty()) {
        path = "/usr/local/bin:/opt/homebrew/bin:/usr/bin:/bin:"
               "/usr/sbin:/sbin";
    }

    // 去重：dir 已在 PATH 里先移除，避免重复累积
    QStringList parts = path.split(QDir::listSeparator(),
                                   Qt::SkipEmptyParts);
    parts.removeAll(dir);

    parts.prepend(dir);
    return parts.join(QDir::listSeparator());
}

// ============================================================================
//  构造终端子进程的环境
//
//  顺序（后面的覆盖前面的）：
//    1. 系统环境（systemEnvironment）
//    2. 项目 SDK 环境（Sdk/Environment 里的所有 key，原样注入）
//    3. PYTHON_HOME 特判：把它所在的 bin 目录加进 PATH 头部
//    4. 虚拟环境：把 venv/bin 加进 PATH 头部，并设置 VIRTUAL_ENV
//
//  这样 IDE 终端里的 `which python3` 会先命中用户配置的解释器。
// ============================================================================
QProcessEnvironment TerminalPanel::buildChildEnvironment() const
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    // ---------- 1. 注入项目 SDK 环境变量 ----------
    ConfigManager* cfg = Application::instance().configManager();
    if (cfg) {
        const QVariantMap sdkEnv = cfg->value("Sdk/Environment").toMap();
        for (auto it = sdkEnv.begin(); it != sdkEnv.end(); ++it) {
            const QString key = it.key();
            const QString val = it.value().toString();
            if (!key.isEmpty() && !val.isEmpty()) {
                env.insert(key, val);
            }
        }

        // ---------- 2. PYTHON_HOME 特判：加 PATH 头 ----------
        //   PYTHON_HOME 在本项目里的语义是"python 可执行文件完整路径"
        //   （例如 /usr/local/bin/python3 或 /path/to/venv/bin/python），
        //   所以取它的 absolutePath() 才是要加进 PATH 的 bin 目录。
        const QString pyHome = sdkEnv.value("PYTHON_HOME").toString();
        if (!pyHome.isEmpty()) {
            QFileInfo fi(pyHome);
            const QString binDir = fi.isDir() ? pyHome : fi.absolutePath();
            if (!binDir.isEmpty() && QDir(binDir).exists()) {
                env.insert("PATH", prependToPath(env, binDir));   // ← 用 helper
            }
        }
    }

    // ---------- 3. 虚拟环境优先（在 SDK 之上） ----------
    if (!m_currentVenvPath.isEmpty()) {
    #ifdef Q_OS_WIN
        const QString binDir = QDir(m_currentVenvPath).absoluteFilePath("Scripts");
    #else
        const QString binDir = QDir(m_currentVenvPath).absoluteFilePath("bin");
    #endif
        if (QDir(binDir).exists()) {
            env.insert("PATH", prependToPath(env, binDir));   // ← 用 helper
            env.insert("VIRTUAL_ENV", m_currentVenvPath);
        }
    }

    return env;
}

void TerminalPanel::executeCommand(const QString& command)
{
    if (command.trimmed().isEmpty()) {
        return;
    }

    if (m_isCommandRunning) {
        appendOutput(tr("命令已在运行中，请先停止"), true);
        return;
    }

    // 记录历史
    if (!m_commandHistory.isEmpty() && m_commandHistory.last() != command) {
        m_commandHistory.append(command);
    } else if (m_commandHistory.isEmpty()) {
        m_commandHistory.append(command);
    }
    m_historyIndex = m_commandHistory.size();

    m_currentCommand = command;

    // 显示命令
    appendOutput("> " + command, false);

    // 创建进程
    if (m_process) {
        m_process->deleteLater();
    }
    m_process = new QProcess(this);
    m_process->setWorkingDirectory(m_workingDirectory);
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    // 环境 = 系统 + Sdk/Environment（含 PYTHON_HOME）+ 虚拟环境
    QProcessEnvironment env = buildChildEnvironment();
    m_process->setProcessEnvironment(env);

    // 日志提示（保持原有可观测性）
    if (!m_currentVenvPath.isEmpty()) {
        appendOutput(tr("已自动激活虚拟环境: ") + m_currentVenvPath, false);
    }

    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &TerminalPanel::onProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &TerminalPanel::onProcessError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TerminalPanel::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &TerminalPanel::onProcessErrorOccurred);

    // 解析命令（支持带引号参数）
    QStringList args = QProcess::splitCommand(command);
    QString program = args.takeFirst();

    m_process->setProgram(program);
    m_process->setArguments(args);

    m_isCommandRunning = true;
    m_stopButton->setEnabled(true);
    m_inputWidget->setEnabled(false);
    m_promptLabel->setText(tr("[运行中] "));

    m_process->start();

    if (!m_process->waitForStarted(3000)) {
        appendOutput(tr("启动命令失败: ") + m_process->errorString(), true);
        onProcessFinished(-1, QProcess::NormalExit);
    } else {
        emit commandStarted(command);
    }
}

bool TerminalPanel::isCommandRunning() const
{
    return m_isCommandRunning;
}

void TerminalPanel::clearOutput()
{
    m_outputWidget->clear();
}

void TerminalPanel::stopCurrentCommand()
{
    if (m_isCommandRunning && m_process) {
        appendOutput(tr("正在停止命令..."), false);
        m_process->terminate();
        if (!m_process->waitForFinished(2000)) {
            m_process->kill();
            m_process->waitForFinished(500);
        }
    }
}

void TerminalPanel::onInputEntered()
{
    QString command = m_inputWidget->text().trimmed();
    m_inputWidget->clear();

    if (!command.isEmpty()) {
        executeCommand(command);
    }
}

void TerminalPanel::onProcessOutput()
{
    if (!m_process) return;
    QByteArray data = m_process->readAllStandardOutput();
    QString output = QString::fromLocal8Bit(data);
    if (!output.isEmpty()) {
        appendOutput(output, false);
        emit outputReceived(output, false);
    }
}

void TerminalPanel::onProcessError()
{
    if (!m_process) return;
    QByteArray data = m_process->readAllStandardError();
    QString output = QString::fromLocal8Bit(data);
    if (!output.isEmpty()) {
        appendOutput(output, true);
        emit outputReceived(output, true);
    }
}

void TerminalPanel::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    m_isCommandRunning = false;
    m_stopButton->setEnabled(false);
    m_inputWidget->setEnabled(true);
    m_inputWidget->setFocus();
    updatePrompt();

    if (status == QProcess::NormalExit) {
        appendOutput(tr("进程结束，退出码: ") + QString::number(exitCode), false);
        emit commandFinished(m_currentCommand, exitCode);
    } else {
        appendOutput(tr("进程异常结束"), true);
    }

    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }
}

void TerminalPanel::onProcessErrorOccurred(QProcess::ProcessError error)
{
    QString msg;
    switch (error) {
    case QProcess::FailedToStart: msg = tr("启动失败"); break;
    case QProcess::Crashed: msg = tr("进程崩溃"); break;
    case QProcess::Timedout: msg = tr("超时"); break;
    case QProcess::WriteError: msg = tr("写入错误"); break;
    case QProcess::ReadError: msg = tr("读取错误"); break;
    default: msg = tr("未知错误"); break;
    }
    appendOutput(tr("进程错误: ") + msg, true);
}

void TerminalPanel::onClearClicked()
{
    clearOutput();
}

void TerminalPanel::recolorAllOutput()
{
    if (!m_outputWidget) return;

    ThemeManager& tm = ThemeManager::instance();
    const QColor textColor  = tm.color(ThemeManager::UIElement::TextColor);
    const QColor errorColor = tm.color(ThemeManager::UIElement::ErrorColor);

    static const QRegularExpression errorRe(
        R"(\b(error|Error|ERROR|fatal|Fatal|FATAL|失败|错误|无法|not found|cannot|failed)\b)");

    QTextCursor cursor(m_outputWidget->document());
    cursor.beginEditBlock();

    QTextBlock block = m_outputWidget->document()->firstBlock();
    while (block.isValid()) {
        const QString lineText = block.text();
        const bool isErr = errorRe.match(lineText).hasMatch();

        QTextCursor blockCursor(block);
        blockCursor.select(QTextCursor::LineUnderCursor);

        QTextCharFormat fmt;
        fmt.setForeground(isErr ? errorColor : textColor);
        blockCursor.mergeCharFormat(fmt);   // merge 而非 set，保留其他格式

        block = block.next();
    }

    cursor.endEditBlock();
}

void TerminalPanel::appendOutput(const QString& text, bool /*isError*/)
{
    if (!m_outputWidget || text.isEmpty()) return;

    ThemeManager& tm = ThemeManager::instance();

    qDebug() << "[Terminal] TextColor token ="
             << tm.color(ThemeManager::UIElement::TextColor).name();

    // 逐行处理：含错误关键词的行用 ErrorColor，其余用 TextColor
    static const QRegularExpression errorRe(
        R"(\b(error|Error|ERROR|fatal|Fatal|FATAL|失败|错误|无法|not found|cannot|failed)\b)");

    QTextCursor cursor(m_outputWidget->document());
    cursor.movePosition(QTextCursor::End);

    const QStringList lines = text.split('\n', Qt::KeepEmptyParts);
    for (int i = 0; i < lines.size(); ++i) {
        const QString& line = lines.at(i);
        const bool isErr = errorRe.match(line).hasMatch();

        QTextCharFormat fmt;
        fmt.setForeground(isErr
                              ? tm.color(ThemeManager::UIElement::ErrorColor)
                              : tm.color(ThemeManager::UIElement::TextColor));
        cursor.setCharFormat(fmt);
        cursor.insertText(line);

        // 除最后一行外，插入换行
        if (i < lines.size() - 1) {
            cursor.insertText("\n");
        }
    }

    QScrollBar* sb = m_outputWidget->verticalScrollBar();
    if (sb) sb->setValue(sb->maximum());
}

void TerminalPanel::updatePrompt()
{
    if (m_isCommandRunning) {
        m_promptLabel->setText(tr("[运行中] "));
    } else {
        updateVenvDisplay();
    }
}

} // namespace SprintIDE
