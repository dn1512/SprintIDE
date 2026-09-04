// terminal_panel.cpp
#include "terminal_panel.h"
#include <QScrollBar>
#include <QKeyEvent>
#include <QDir>
#include <QMessageBox>
#include <QApplication>
#include <QStyle>
#include <QProcessEnvironment>

#include "project_manager.h"
#include "python/python_virtual_env.h"
#include "python/python_project_model.h"

namespace SprintIDE {

TerminalPanel::TerminalPanel(QWidget* parent)
    : QWidget(parent)
    , m_process(nullptr)
    , m_isCommandRunning(false)
    , m_historyIndex(-1)
{
    setupUI();
    setObjectName("TerminalPanel");

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
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);

    m_outputWidget = new QTextEdit(this);
    m_outputWidget->setReadOnly(true);
    m_outputWidget->setFont(QFont("Menlo", 10));
    m_outputWidget->setStyleSheet(
        "QTextEdit { background-color: #1E1E1E; color: #CCCCCC; border: 1px solid #3C3C3C; }"
        );
    m_outputWidget->setMinimumHeight(100);
    m_outputWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(m_outputWidget);

    // 输入区域
    QHBoxLayout* inputLayout = new QHBoxLayout();
    inputLayout->setContentsMargins(0, 2, 0, 2);

    m_promptLabel = new QLabel("$ ", this);
    m_promptLabel->setStyleSheet("color: #33CC33; font-weight: bold; background-color: #444444;");
    m_promptLabel->setFixedWidth(20);
    inputLayout->addWidget(m_promptLabel);

    m_inputWidget = new QLineEdit(this);
    m_inputWidget->setPlaceholderText("输入命令...");
    m_inputWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_inputWidget->setMinimumWidth(200);
    connect(m_inputWidget, &QLineEdit::returnPressed, this, &TerminalPanel::onInputEntered);
    inputLayout->addWidget(m_inputWidget);

    // 停止按钮
    m_stopButton = new QPushButton("■", this);
    m_stopButton->setMaximumWidth(30);
    m_stopButton->setToolTip("停止当前命令");
    m_stopButton->setEnabled(false);
    m_stopButton->setStyleSheet("QPushButton { background-color: #FF3333; color: white; border: none; }");
    connect(m_stopButton, &QPushButton::clicked, this, &TerminalPanel::stopCurrentCommand);
    inputLayout->addWidget(m_stopButton);

    // 清空按钮
    m_clearButton = new QPushButton("清空", this);
    m_clearButton->setMaximumWidth(50);
    m_clearButton->setToolTip("清空输出");
    connect(m_clearButton, &QPushButton::clicked, this, &TerminalPanel::onClearClicked);
    inputLayout->addWidget(m_clearButton);

    layout->addLayout(inputLayout);

    // 初始提示
    appendOutput("终端已准备就绪。输入命令并按 Enter 执行。", false);
    updatePrompt();
    updateVenvDisplay();

    m_inputWidget->setVisible(true);
    m_inputWidget->setFocus();
    m_inputWidget->setFixedHeight(30);
}

// ========== 新增：更新当前目录的虚拟环境 ==========
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
        m_promptLabel->setStyleSheet("color: #33CC33; font-weight: bold; background-color: #444444;");
    } else {
        m_promptLabel->setText("$ ");
        m_promptLabel->setStyleSheet("color: #33CC33; font-weight: bold; background-color: #444444;");
    }
}

void TerminalPanel::setWorkingDirectory(const QString& dir)
{
    m_workingDirectory = dir;
    // 更新虚拟环境检测
    updateVenvForDirectory(dir);
    updateVenvDisplay();

    appendOutput("工作目录已切换至: " + dir, false);
    emit workingDirectoryChanged(dir);

    // 聚焦输入框
    m_inputWidget->setFocus();
}

QString TerminalPanel::workingDirectory() const
{
    return m_workingDirectory;
}

void TerminalPanel::executeCommand(const QString& command)
{
    if (command.trimmed().isEmpty()) {
        return;
    }

    if (m_isCommandRunning) {
        appendOutput("命令已在运行中，请先停止", true);
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

    // ========== 关键修改：设置环境变量以激活虚拟环境 ==========
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    // 如果检测到虚拟环境，将虚拟环境的 bin 目录添加到 PATH 最前面
    if (!m_currentVenvPath.isEmpty()) {
#ifdef Q_OS_WIN
        QString binDir = QDir(m_currentVenvPath).absoluteFilePath("Scripts");
#else
        QString binDir = QDir(m_currentVenvPath).absoluteFilePath("bin");
#endif
        if (QDir(binDir).exists()) {
            QString path = env.value("PATH");
            env.insert("PATH", binDir + QDir::listSeparator() + path);

            // 设置 VIRTUAL_ENV 环境变量
            env.insert("VIRTUAL_ENV", m_currentVenvPath);

            appendOutput("已自动激活虚拟环境: " + m_currentVenvPath, false);
        }
    }

    m_process->setProcessEnvironment(env);

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
    m_promptLabel->setText("[运行中] ");

    m_process->start();

    if (!m_process->waitForStarted(3000)) {
        appendOutput("启动命令失败: " + m_process->errorString(), true);
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
        appendOutput("正在停止命令...", false);
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
        appendOutput("进程结束，退出码: " + QString::number(exitCode), false);
        emit commandFinished(m_currentCommand, exitCode);
    } else {
        appendOutput("进程异常结束", true);
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
    case QProcess::FailedToStart: msg = "启动失败"; break;
    case QProcess::Crashed: msg = "进程崩溃"; break;
    case QProcess::Timedout: msg = "超时"; break;
    case QProcess::WriteError: msg = "写入错误"; break;
    case QProcess::ReadError: msg = "读取错误"; break;
    default: msg = "未知错误"; break;
    }
    appendOutput("进程错误: " + msg, true);
}

void TerminalPanel::onClearClicked()
{
    clearOutput();
}

void TerminalPanel::appendOutput(const QString& text, bool isError)
{
    QString color = isError ? "#FF6B6B" : "#CCCCCC";
    m_outputWidget->append(
        QString("<span style='color:%1;'>%2</span>")
        .arg(color)
        .arg(text.toHtmlEscaped())
    );
    QScrollBar* scrollBar = m_outputWidget->verticalScrollBar();
    if (scrollBar) {
        scrollBar->setValue(scrollBar->maximum());
    }
}

void TerminalPanel::updatePrompt()
{
    if (m_isCommandRunning) {
        m_promptLabel->setText("[运行中] ");
    } else {
        updateVenvDisplay();
    }
}

} // namespace SprintIDE
