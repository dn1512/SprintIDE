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
#include <QTextBlock>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QFileInfo>

#include "project_manager.h"
#include "python/python_virtual_env.h"
#include "python/python_project_model.h"
#include "theme_manager.h"
#include "Application.h"
#include "config_manager.h"

namespace SprintIDE {

// ============================================================================
//  构造 / 析构
// ============================================================================
TerminalPanel::TerminalPanel(QWidget* parent)
    : QWidget(parent)
    , m_process(nullptr)
    , m_isCommandRunning(false)
    , m_historyIndex(0)
{
    setupUI();
    setObjectName("TerminalPanel");
    setMinimumHeight(0);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);

    // 主题切换 → 重染已有输出
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &TerminalPanel::recolorAllOutput);

    m_workingDirectory = QDir::currentPath();
    updateVenvForDirectory(m_workingDirectory);

    // 初始欢迎 + 首个提示符
    appendSystemLine(tr("终端已准备就绪。输入命令并按 Enter 执行。"), false);
    printPrompt();
}

TerminalPanel::~TerminalPanel()
{
    stopCurrentCommand();
    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }
}

// ============================================================================
//  UI：只保留输出区 + 底部一个停止按钮
// ============================================================================
void TerminalPanel::setupUI()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);

    // ---------- 输出区（可编辑，光标被锁定在末尾输入区） ----------
    m_outputWidget = new QTextEdit(this);
    m_outputWidget->setObjectName("TerminalOutput");
    m_outputWidget->setReadOnly(false);                 // 可输入
    m_outputWidget->setAcceptRichText(false);           // 拒绝富文本粘贴
    m_outputWidget->setUndoRedoEnabled(false);          // 禁掉撤销，避免 Ctrl+Z 破坏输出
    m_outputWidget->setWordWrapMode(QTextOption::NoWrap);
    m_outputWidget->setFont(QFont("Menlo", 10));
    m_outputWidget->setMinimumHeight(100);
    m_outputWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_outputWidget->installEventFilter(this);
    layout->addWidget(m_outputWidget, 1);

    // ---------- 底部小工具条：仅保留停止按钮 ----------
    QHBoxLayout* bottomBar = new QHBoxLayout();
    bottomBar->setContentsMargins(0, 0, 0, 0);
    bottomBar->addStretch();

    m_stopButton = new QPushButton(QStringLiteral("■"), this);
    m_stopButton->setObjectName("TerminalStopButton");
    m_stopButton->setFixedSize(24, 24);
    m_stopButton->setToolTip(tr("停止当前命令 (Ctrl+C)"));
    m_stopButton->setEnabled(false);
    connect(m_stopButton, &QPushButton::clicked,
            this, &TerminalPanel::stopCurrentCommand);
    bottomBar->addWidget(m_stopButton);

    layout->addLayout(bottomBar);

    m_outputWidget->setFocus();
}

// ============================================================================
//  提示符
// ============================================================================
QString TerminalPanel::promptString() const
{
    if (!m_currentVenvPath.isEmpty()) {
        const QString name = QFileInfo(m_currentVenvPath).fileName();
        return QStringLiteral("(%1) $ ").arg(name);
    }
    return QStringLiteral("$ ");
}

// ============================================================================
//  打印提示符 → 用户后续输入都跟在它后面
// ============================================================================
void TerminalPanel::printPrompt()
{
    if (!m_outputWidget) return;

    ThemeManager& tm = ThemeManager::instance();

    QTextCursor c(m_outputWidget->document());
    c.movePosition(QTextCursor::End);

    // 确保前面是换行
    if (c.position() > 0) {
        const QChar prev = m_outputWidget->document()
                               ->characterAt(c.position() - 1);
        if (prev != QLatin1Char('\n')) {
            c.insertText(QStringLiteral("\n"));
        }
    }

    // 提示符样式：绿色加粗（终端惯例）
    QTextCharFormat promptFmt;
    promptFmt.setForeground(tm.color(ThemeManager::UIElement::SuccessColor));
    promptFmt.setFontWeight(QFont::Bold);
    c.setCharFormat(promptFmt);
    c.insertText(promptString());

    // ★ 记录输入起点
    m_promptStart = c.position();

    // 切回正文样式（用户输入用默认色）
    QTextCharFormat inputFmt;
    inputFmt.setForeground(tm.color(ThemeManager::UIElement::TextColor));
    inputFmt.setFontWeight(QFont::Normal);
    c.setCharFormat(inputFmt);

    // 光标落到输入起点
    c.movePosition(QTextCursor::End);
    m_outputWidget->setTextCursor(c);

    if (auto* sb = m_outputWidget->verticalScrollBar()) {
        sb->setValue(sb->maximum());
    }
}

// ============================================================================
//  追加进程原始输出（原样，不加换行）
// ============================================================================
void TerminalPanel::appendRaw(const QString& text)
{
    if (!m_outputWidget || text.isEmpty()) return;

    ThemeManager& tm = ThemeManager::instance();

    static const QRegularExpression errorRe(
        R"(\b(error|Error|ERROR|fatal|Fatal|FATAL|失败|错误|无法|not found|cannot|failed)\b)");

    QTextCursor c(m_outputWidget->document());
    c.movePosition(QTextCursor::End);

    const QStringList lines = text.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
    for (int i = 0; i < lines.size(); ++i) {
        const QString& line = lines.at(i);
        const bool isErr = errorRe.match(line).hasMatch();

        QTextCharFormat fmt;
        fmt.setForeground(isErr
                              ? tm.color(ThemeManager::UIElement::ErrorColor)
                              : tm.color(ThemeManager::UIElement::TextColor));
        c.setCharFormat(fmt);
        c.insertText(line);

        if (i < lines.size() - 1) {
            c.insertText(QStringLiteral("\n"));
        }
    }

    m_outputWidget->setTextCursor(c);
    if (auto* sb = m_outputWidget->verticalScrollBar()) {
        sb->setValue(sb->maximum());
    }
}

// ============================================================================
//  系统提示行（前后补换行）
// ============================================================================
void TerminalPanel::appendSystemLine(const QString& text, bool isError)
{
    if (!m_outputWidget) return;

    ThemeManager& tm = ThemeManager::instance();

    QTextCursor c(m_outputWidget->document());
    c.movePosition(QTextCursor::End);

    // 前面补换行
    if (c.position() > 0) {
        const QChar prev = m_outputWidget->document()
                               ->characterAt(c.position() - 1);
        if (prev != QLatin1Char('\n')) {
            c.insertText(QStringLiteral("\n"));
        }
    }

    QTextCharFormat fmt;
    fmt.setForeground(isError
                          ? tm.color(ThemeManager::UIElement::ErrorColor)
                          : tm.color(ThemeManager::UIElement::SecondaryTextColor));
    c.setCharFormat(fmt);
    c.insertText(text);
    c.insertText(QStringLiteral("\n"));   // ★ 尾部换行，避免与下一条粘连

    m_outputWidget->setTextCursor(c);
    if (auto* sb = m_outputWidget->verticalScrollBar()) {
        sb->setValue(sb->maximum());
    }
}

// ============================================================================
//  用户按 Enter：从输入区取命令
// ============================================================================
void TerminalPanel::onCommandEntered()
{
    if (!m_outputWidget || m_promptStart < 0) return;

    QTextCursor c(m_outputWidget->document());
    c.movePosition(QTextCursor::End);
    const int endPos = c.position();

    c.setPosition(m_promptStart);
    c.setPosition(endPos, QTextCursor::KeepAnchor);
    QString cmd = c.selectedText();
    cmd.replace(QChar::ParagraphSeparator, QLatin1Char('\n'));

    // 多行输入不常见：只取第一行
    const int nl = cmd.indexOf(QLatin1Char('\n'));
    if (nl >= 0) cmd = cmd.left(nl);

    // 空命令 → 新提示符，不执行
    if (cmd.trimmed().isEmpty()) {
        printPrompt();
        return;
    }

    // 光标移回末尾（executeCommand 会在末尾追加换行）
    c.setPosition(endPos);
    m_outputWidget->setTextCursor(c);

    executeCommand(cmd);
}

// ============================================================================
//  上下键调历史
// ============================================================================
void TerminalPanel::navigateHistory(int direction)
{
    if (!m_outputWidget || m_promptStart < 0) return;
    if (m_commandHistory.isEmpty()) return;

    const int size = m_commandHistory.size();
    int newIndex = m_historyIndex + direction;
    if (newIndex < 0)    newIndex = 0;
    if (newIndex > size) newIndex = size;
    if (newIndex == m_historyIndex) return;

    m_historyIndex = newIndex;

    // 替换输入区内容
    QTextCursor c(m_outputWidget->document());
    c.setPosition(m_promptStart);
    c.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    c.removeSelectedText();

    if (m_historyIndex < size) {
        c.insertText(m_commandHistory[m_historyIndex]);
    }

    c.movePosition(QTextCursor::End);
    m_outputWidget->setTextCursor(c);
}

// ============================================================================
//  键盘 / 鼠标拦截
// ============================================================================
bool TerminalPanel::eventFilter(QObject* obj, QEvent* event)
{
    if (obj != m_outputWidget) {
        return QWidget::eventFilter(obj, event);
    }

    if (event->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(event);

        // ---------- 命令运行中：拦截所有编辑 ----------
        if (m_isCommandRunning) {
            // Ctrl+C 且有选区 → 复制
            if (ke->matches(QKeySequence::Copy)) {
                QTextCursor c = m_outputWidget->textCursor();
                if (c.hasSelection()) return false;
            }
            // Ctrl+C 无选区 → 中断命令
            if (ke->key() == Qt::Key_C
                && ke->modifiers() == Qt::ControlModifier) {
                stopCurrentCommand();
                return true;
            }
            return true;   // 其他键全部吞掉
        }

        // ---------- 空闲：正常输入 ----------
        switch (ke->key()) {

        case Qt::Key_Return:
        case Qt::Key_Enter:
            onCommandEntered();
            return true;

        case Qt::Key_Up:
            navigateHistory(-1);
            return true;

        case Qt::Key_Down:
            navigateHistory(+1);
            return true;

        case Qt::Key_Left: {
            QTextCursor c = m_outputWidget->textCursor();
            if (c.position() <= m_promptStart) return true;
            return false;
        }

        case Qt::Key_Home: {
            QTextCursor c = m_outputWidget->textCursor();
            c.setPosition(m_promptStart);
            m_outputWidget->setTextCursor(c);
            return true;
        }

        case Qt::Key_Backspace: {
            QTextCursor c = m_outputWidget->textCursor();
            if (c.hasSelection()) {
                if (c.selectionStart() < m_promptStart) {
                    // 选区跨到 prompt 之前 → 裁到 prompt 之后
                    c.setPosition(m_promptStart);
                    c.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
                    m_outputWidget->setTextCursor(c);
                }
                return false;
            }
            if (c.position() <= m_promptStart) return true;
            return false;
        }

        case Qt::Key_Delete:
            // 光标永远在末尾附近，Delete 没什么可删的
            return true;

        case Qt::Key_A:
            if (ke->modifiers() == Qt::ControlModifier) {
                // Ctrl+A → 只选中输入区
                QTextCursor c(m_outputWidget->document());
                c.setPosition(m_promptStart);
                c.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
                m_outputWidget->setTextCursor(c);
                return true;
            }
            break;

        case Qt::Key_Z:
            if (ke->modifiers() == Qt::ControlModifier) {
                return true;   // 撤销禁用
            }
            break;

        case Qt::Key_V:
            // 允许粘贴（用户多行粘贴后面会被 onCommandEntered 截断到第一行）
            break;

        default:
            break;
        }
    }

    // 鼠标点击：光标落在 prompt 之前 → 拉回末尾
    if (event->type() == QEvent::MouseButtonRelease) {
        QTextCursor c = m_outputWidget->textCursor();
        if (m_promptStart >= 0 && c.position() < m_promptStart) {
            c.movePosition(QTextCursor::End);
            m_outputWidget->setTextCursor(c);
        }
    }

    return QWidget::eventFilter(obj, event);
}

// ============================================================================
//  虚拟环境
// ============================================================================
void TerminalPanel::updateVenvForDirectory(const QString& dir)
{
    m_currentVenvPath.clear();
    m_currentPythonInterpreter.clear();

    auto model = ProjectManager::instance()->pythonProjectModel();
    if (model && !model->venvPath().isEmpty()) {
        m_currentVenvPath = model->venvPath();
        m_currentPythonInterpreter = model->interpreterPath();
        return;
    }

    const QString detected = PythonVirtualEnv::detectVenv(dir);
    if (!detected.isEmpty()) {
        m_currentVenvPath = detected;
        m_currentPythonInterpreter = PythonVirtualEnv::interpreterPath(detected);
    }
}

// ============================================================================
//  工作目录切换：清空终端，作为新会话
// ============================================================================
void TerminalPanel::setWorkingDirectory(const QString& dir)
{
    if (m_isCommandRunning) return;   // 运行中不改

    m_workingDirectory = dir;
    updateVenvForDirectory(dir);

    // 清空 = 新终端
    m_outputWidget->clear();
    m_promptStart = -1;
    m_commandHistory.clear();
    m_historyIndex = 0;

    appendSystemLine(tr("工作目录: %1").arg(dir), false);
    printPrompt();

    emit workingDirectoryChanged(dir);

    m_outputWidget->setFocus();
}

QString TerminalPanel::workingDirectory() const
{
    return m_workingDirectory;
}

// ============================================================================
//  子进程环境
// ============================================================================
static QString prependToPath(const QProcessEnvironment& env, const QString& dir)
{
    QString path = env.value("PATH");
    if (path.isEmpty()) {
        path = "/usr/local/bin:/opt/homebrew/bin:/usr/bin:/bin:"
               "/usr/sbin:/sbin";
    }

    QStringList parts = path.split(QDir::listSeparator(),
                                   Qt::SkipEmptyParts);
    parts.removeAll(dir);
    parts.prepend(dir);
    return parts.join(QDir::listSeparator());
}

QProcessEnvironment TerminalPanel::buildChildEnvironment() const
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

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

        const QString pyHome = sdkEnv.value("PYTHON_HOME").toString();
        if (!pyHome.isEmpty()) {
            QFileInfo fi(pyHome);
            const QString binDir = fi.isDir() ? pyHome : fi.absolutePath();
            if (!binDir.isEmpty() && QDir(binDir).exists()) {
                env.insert("PATH", prependToPath(env, binDir));
            }
        }
    }

    if (!m_currentVenvPath.isEmpty()) {
#ifdef Q_OS_WIN
        const QString binDir = QDir(m_currentVenvPath).absoluteFilePath("Scripts");
#else
        const QString binDir = QDir(m_currentVenvPath).absoluteFilePath("bin");
#endif
        if (QDir(binDir).exists()) {
            env.insert("PATH", prependToPath(env, binDir));
            env.insert("VIRTUAL_ENV", m_currentVenvPath);
        }
    }

    return env;
}

// ============================================================================
//  执行命令
//
//  注意：命令本身已经在输出区了（用户自己敲的），这里只负责：
//    1. 追加换行（进入"输出模式"）
//    2. 记录历史
//    3. 启动 QProcess
// ============================================================================
void TerminalPanel::executeCommand(const QString& command)
{
    if (command.trimmed().isEmpty()) {
        printPrompt();
        return;
    }

    if (m_isCommandRunning) {
        appendSystemLine(tr("命令已在运行中，请先停止"), true);
        printPrompt();
        return;
    }

    // ---------- 进入"输出模式"：光标移到末尾 + 换行 ----------
    {
        QTextCursor c(m_outputWidget->document());
        c.movePosition(QTextCursor::End);
        c.insertText(QStringLiteral("\n"));
        m_outputWidget->setTextCursor(c);
    }

    // ---------- 历史记录 ----------
    if (m_commandHistory.isEmpty()
        || m_commandHistory.last() != command) {
        m_commandHistory.append(command);
    }
    m_historyIndex = m_commandHistory.size();

    m_currentCommand = command;

    // ---------- 创建进程 ----------
    if (m_process) {
        m_process->deleteLater();
    }
    m_process = new QProcess(this);
    m_process->setWorkingDirectory(m_workingDirectory);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    m_process->setProcessEnvironment(buildChildEnvironment());

    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &TerminalPanel::onProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &TerminalPanel::onProcessError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TerminalPanel::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &TerminalPanel::onProcessErrorOccurred);

    QStringList args = QProcess::splitCommand(command);
    const QString program = args.takeFirst();

    m_process->setProgram(program);
    m_process->setArguments(args);

    m_isCommandRunning = true;
    m_stopButton->setEnabled(true);

    m_process->start();

    if (!m_process->waitForStarted(3000)) {
        appendSystemLine(tr("启动命令失败: ") + m_process->errorString(), true);
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
    if (!m_outputWidget) return;

    m_outputWidget->clear();
    m_promptStart = -1;
    printPrompt();
}

void TerminalPanel::stopCurrentCommand()
{
    if (m_isCommandRunning && m_process) {
        appendSystemLine(tr("正在停止命令..."), false);
        m_process->terminate();
        if (!m_process->waitForFinished(2000)) {
            m_process->kill();
            m_process->waitForFinished(500);
        }
    }
}

// ============================================================================
//  进程输出
// ============================================================================
void TerminalPanel::onProcessOutput()
{
    if (!m_process) return;
    const QByteArray data = m_process->readAllStandardOutput();
    const QString output = QString::fromLocal8Bit(data);
    if (!output.isEmpty()) {
        appendRaw(output);
        emit outputReceived(output, false);
    }
}

void TerminalPanel::onProcessError()
{
    if (!m_process) return;
    const QByteArray data = m_process->readAllStandardError();
    const QString output = QString::fromLocal8Bit(data);
    if (!output.isEmpty()) {
        appendRaw(output);
        emit outputReceived(output, true);
    }
}

void TerminalPanel::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    m_isCommandRunning = false;
    m_stopButton->setEnabled(false);

    // 只在异常/非 0 时提示（避免每条命令都打"退出码 0"）
    if (status != QProcess::NormalExit) {
        appendSystemLine(tr("[进程异常结束]"), true);
    } else if (exitCode != 0) {
        appendSystemLine(QStringLiteral("[退出码 %1]").arg(exitCode), true);
    }

    // 新提示符
    printPrompt();

    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }

    m_outputWidget->setFocus();
}

void TerminalPanel::onProcessErrorOccurred(QProcess::ProcessError error)
{
    QString msg;
    switch (error) {
    case QProcess::FailedToStart: msg = tr("启动失败"); break;
    case QProcess::Crashed:       msg = tr("进程崩溃"); break;
    case QProcess::Timedout:      msg = tr("超时");     break;
    case QProcess::WriteError:    msg = tr("写入错误"); break;
    case QProcess::ReadError:     msg = tr("读取错误"); break;
    default:                      msg = tr("未知错误"); break;
    }
    appendSystemLine(tr("进程错误: ") + msg, true);
}

// ============================================================================
//  主题切换：按行重染
// ============================================================================
void TerminalPanel::recolorAllOutput()
{
    if (!m_outputWidget) return;

    ThemeManager& tm = ThemeManager::instance();
    const QColor textColor   = tm.color(ThemeManager::UIElement::TextColor);
    const QColor errorColor  = tm.color(ThemeManager::UIElement::ErrorColor);
    const QColor promptColor = tm.color(ThemeManager::UIElement::SuccessColor);

    static const QRegularExpression errorRe(
        R"(\b(error|Error|ERROR|fatal|Fatal|FATAL|失败|错误|无法|not found|cannot|failed)\b)");
    // 提示符行：以 "$ " 开头，或 "(venv) $ " 开头
    static const QRegularExpression promptRe(
        R"(^\s*(\(.+\)\s+)?\$\s)");

    QTextCursor cursor(m_outputWidget->document());
    cursor.beginEditBlock();

    QTextBlock block = m_outputWidget->document()->firstBlock();
    while (block.isValid()) {
        const QString lineText = block.text();

        QColor color;
        if (errorRe.match(lineText).hasMatch()) {
            color = errorColor;
        } else if (promptRe.match(lineText).hasMatch()) {
            color = promptColor;
        } else {
            color = textColor;
        }

        QTextCursor bc(block);
        bc.select(QTextCursor::LineUnderCursor);

        QTextCharFormat fmt;
        fmt.setForeground(color);
        bc.mergeCharFormat(fmt);

        block = block.next();
    }

    cursor.endEditBlock();
}

} // namespace SprintIDE