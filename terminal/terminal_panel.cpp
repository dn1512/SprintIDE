// terminal_panel.cpp
#include "terminal_panel.h"

#include <QScrollBar>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QClipboard>
#include <QApplication>
#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <QFontMetricsF>
#include <QRegularExpression>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QDebug>

#include "terminal/pty/pty_types.h"
#include "terminal/pty/pty_factory.h"

#include "project_manager.h"
#include "python/python_virtual_env.h"
#include "python/python_project_model.h"
#include "theme_manager.h"
#include "Application.h"
#include "config_manager.h"

namespace SprintIDE {

    // ============================================================================
    // 终端 16 色调色板（xterm 默认色）
    // ============================================================================
    static const QColor kPalette[16] = {
        QColor(0x00, 0x00, 0x00), QColor(0xCD, 0x00, 0x00),
        QColor(0x00, 0xCD, 0x00), QColor(0xCD, 0xCD, 0x00),
        QColor(0x00, 0x00, 0xEE), QColor(0xCD, 0x00, 0xCD),
        QColor(0x00, 0xCD, 0xCD), QColor(0xE5, 0xE5, 0xE5),
        QColor(0x7F, 0x7F, 0x7F), QColor(0xFF, 0x00, 0x00),
        QColor(0x00, 0xFF, 0x00), QColor(0xFF, 0xFF, 0x00),
        QColor(0x5C, 0x5C, 0xFF), QColor(0xFF, 0x00, 0xFF),
        QColor(0x00, 0xFF, 0xFF), QColor(0xFF, 0xFF, 0xFF),
    };

    // ============================================================================
    // 构造 / 析构
    // ============================================================================
    TerminalPanel::TerminalPanel(QWidget* parent)
    : QWidget(parent)
    {
        setupUI();
        setObjectName("TerminalPanel");
        setMinimumHeight(0);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);

        connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &TerminalPanel::recolorAllOutput);

        m_workingDirectory = QDir::currentPath();
        updateVenvForDirectory(m_workingDirectory);

        m_currentFmt.setForeground(
            ThemeManager::instance().color(ThemeManager::UIElement::TextColor));

        appendSystemLine(tr("终端已准备就绪。输入命令并按 Enter 执行。"), false);
        printPrompt();
    }

    TerminalPanel::~TerminalPanel()
    {
        if (m_pty) {
            QObject::disconnect(m_pty, nullptr, this, nullptr);

            delete m_pty;
            m_pty = nullptr;
        }
    }

    // ============================================================================
    // UI
    // ============================================================================
    void TerminalPanel::setupUI()
    {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(2, 2, 2, 2);
        layout->setSpacing(2);

        m_outputWidget = new QTextEdit(this);
        m_outputWidget->setObjectName("TerminalOutput");
        m_outputWidget->setReadOnly(false);
        m_outputWidget->setAcceptRichText(false);
        m_outputWidget->setUndoRedoEnabled(false);
        m_outputWidget->setWordWrapMode(QTextOption::NoWrap);
        QFont font("Menlo", 10);
        font.setStyleHint(QFont::Monospace);
        m_outputWidget->setFont(font);
        m_outputWidget->setMinimumHeight(100);
        m_outputWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_outputWidget->installEventFilter(this);
        layout->addWidget(m_outputWidget, 1);

        QHBoxLayout* bottomBar = new QHBoxLayout();
        bottomBar->setContentsMargins(0, 0, 0, 0);
        bottomBar->addStretch();

        m_stopButton = new QPushButton(QStringLiteral("■"), this);
        m_stopButton->setObjectName("TerminalStopButton");
        m_stopButton->setFixedSize(24, 24);
        m_stopButton->setToolTip(tr("停止当前命令"));
        m_stopButton->setEnabled(false);
        connect(m_stopButton, &QPushButton::clicked,
            this, &TerminalPanel::stopCurrentCommand);
        bottomBar->addWidget(m_stopButton);

        layout->addLayout(bottomBar);
        m_outputWidget->setFocus();
    }

    // ============================================================================
    // 提示符
    // ============================================================================
    QString TerminalPanel::promptString() const
    {
        if (!m_currentVenvPath.isEmpty()) {
            const QString name = QFileInfo(m_currentVenvPath).fileName();
            return QStringLiteral("(%1) $ ").arg(name);
        }
        return QStringLiteral("$ ");
    }

    void TerminalPanel::printPrompt()
    {
        if (!m_outputWidget) return;

        ThemeManager& tm = ThemeManager::instance();

        QTextCursor c(m_outputWidget->document());
        c.movePosition(QTextCursor::End);

        if (c.position() > 0) {
            const QChar prev = m_outputWidget->document()
            ->characterAt(c.position() - 1);
            if (prev != QLatin1Char('\n')) c.insertText(QStringLiteral("\n"));
        }

        QTextCharFormat promptFmt;
        promptFmt.setForeground(tm.color(ThemeManager::UIElement::SuccessColor));
        promptFmt.setFontWeight(QFont::Bold);
        c.setCharFormat(promptFmt);
        c.insertText(promptString());

        m_promptStart = c.position();

        QTextCharFormat inputFmt;
        inputFmt.setForeground(tm.color(ThemeManager::UIElement::TextColor));
        inputFmt.setFontWeight(QFont::Normal);
        c.setCharFormat(inputFmt);

        c.movePosition(QTextCursor::End);
        m_outputWidget->setTextCursor(c);

        if (auto* sb = m_outputWidget->verticalScrollBar()) sb->setValue(sb->maximum());
    }

    // ============================================================================
    // 系统提示行
    // ============================================================================
    void TerminalPanel::appendSystemLine(const QString& text, bool isError)
    {
        if (!m_outputWidget) return;

        ThemeManager& tm = ThemeManager::instance();

        QTextCursor c(m_outputWidget->document());
        c.movePosition(QTextCursor::End);

        if (c.position() > 0) {
            const QChar prev = m_outputWidget->document()
            ->characterAt(c.position() - 1);
            if (prev != QLatin1Char('\n')) c.insertText(QStringLiteral("\n"));
        }

        QTextCharFormat fmt;
        fmt.setForeground(isError
            ? tm.color(ThemeManager::UIElement::ErrorColor)
            : tm.color(ThemeManager::UIElement::SecondaryTextColor));
        c.setCharFormat(fmt);
        c.insertText(text);
        c.insertText(QStringLiteral("\n"));

        m_outputWidget->setTextCursor(c);
        if (auto* sb = m_outputWidget->verticalScrollBar()) sb->setValue(sb->maximum());
    }

    // ============================================================================
    // 空闲态：从提示符到末尾取命令
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
        const int nl = cmd.indexOf(QLatin1Char('\n'));
        if (nl >= 0) cmd = cmd.left(nl);

        if (cmd.trimmed().isEmpty()) {
            printPrompt();
            return;
        }

        c.setPosition(endPos);
        m_outputWidget->setTextCursor(c);

        executeCommand(cmd);
    }

    // ============================================================================
    // 历史
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

        QTextCursor c(m_outputWidget->document());
        c.setPosition(m_promptStart);
        c.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        c.removeSelectedText();

        if (m_historyIndex < size) c.insertText(m_commandHistory[m_historyIndex]);

        c.movePosition(QTextCursor::End);
        m_outputWidget->setTextCursor(c);
    }

    // ============================================================================
    // 按键 → 写入 PTY 的字节
    // ============================================================================
    static QByteArray keyToPtyBytes(QKeyEvent* ke)
    {
        // Ctrl+字母 → 0x01-0x1A
        if ((ke->modifiers() & Qt::ControlModifier) && !ke->text().isEmpty()) {
            const int key = ke->key();
            if (key >= Qt::Key_A && key <= Qt::Key_Z) {
                return QByteArray(1, char(key - Qt::Key_A + 1));
            }
            if (key == Qt::Key_Space) return QByteArray(1, '\0');
            if (key == Qt::Key_BracketLeft)  return QByteArray(1, '\x1b');
            if (key == Qt::Key_Backslash)    return QByteArray(1, '\x1c');
            if (key == Qt::Key_BracketRight) return QByteArray(1, '\x1d');
            if (key == Qt::Key_AsciiCircum)  return QByteArray(1, '\x1e');
            if (key == Qt::Key_Underscore)   return QByteArray(1, '\x1f');
        }

        switch (ke->key()) {
            case Qt::Key_Return:
            case Qt::Key_Enter:     return "\r";
            case Qt::Key_Backspace: return "\x7F";
            case Qt::Key_Tab:       return "\t";
            case Qt::Key_Escape:    return "\x1b";
            case Qt::Key_Up:        return "\x1b[A";
            case Qt::Key_Down:      return "\x1b[B";
            case Qt::Key_Right:     return "\x1b[C";
            case Qt::Key_Left:      return "\x1b[D";
            case Qt::Key_Home:      return "\x1b[H";
            case Qt::Key_End:       return "\x1b[F";
            case Qt::Key_PageUp:    return "\x1b[5~";
            case Qt::Key_PageDown:  return "\x1b[6~";
            case Qt::Key_Delete:    return "\x1b[3~";
            case Qt::Key_Insert:    return "\x1b[2~";
            case Qt::Key_F1:        return "\x1bOP";
            case Qt::Key_F2:        return "\x1bOQ";
            case Qt::Key_F3:        return "\x1bOR";
            case Qt::Key_F4:        return "\x1bOS";
            case Qt::Key_F5:        return "\x1b[15~";
            case Qt::Key_F6:        return "\x1b[17~";
            case Qt::Key_F7:        return "\x1b[18~";
            case Qt::Key_F8:        return "\x1b[19~";
            case Qt::Key_F9:        return "\x1b[20~";
            case Qt::Key_F10:       return "\x1b[21~";
            case Qt::Key_F11:       return "\x1b[23~";
            case Qt::Key_F12:       return "\x1b[24~";
            default: break;
        }

        const QString text = ke->text();
        if (!text.isEmpty()) {
            QString filtered;
            for (QChar ch : text) {
                if (ch.unicode() >= 0x20 || ch == '\t') filtered.append(ch);
            }
            if (!filtered.isEmpty()) return filtered.toUtf8();
        }
        return {};
    }

    // ============================================================================
    // eventFilter：运行态把按键转 PTY；空闲态只限制编辑区
    // ============================================================================
    bool TerminalPanel::eventFilter(QObject* obj, QEvent* event)
    {
        if (obj != m_outputWidget) return QWidget::eventFilter(obj, event);

        if (event->type() == QEvent::KeyPress) {
            auto* ke = static_cast<QKeyEvent*>(event);

            // ---------- 运行态 ----------
            if (m_isCommandRunning && m_pty) {
                // Ctrl+C 有选区 → 复制
                if (ke->matches(QKeySequence::Copy)) {
                    if (m_outputWidget->textCursor().hasSelection()) return false;
                }

                // Ctrl+V → 粘贴，换行转 \r
                if (ke->matches(QKeySequence::Paste)) {
                    QString paste = QApplication::clipboard()->text();
                    paste.replace(QLatin1String("\r\n"), QLatin1String("\r"));
                    paste.replace(QLatin1Char('\n'), QLatin1Char('\r'));
                    if (!paste.isEmpty()) m_pty->write(paste.toUtf8());
                    return true;
                }

                QByteArray bytes = keyToPtyBytes(ke);
                if (!bytes.isEmpty()) {
                    m_pty->write(bytes);
                    return true;
                }
                return true;   // 其他按键吞掉
            }

            // ---------- 空闲态 ----------
            switch (ke->key()) {
                case Qt::Key_Return:
                case Qt::Key_Enter:    onCommandEntered(); return true;
                case Qt::Key_Up:       navigateHistory(-1); return true;
                case Qt::Key_Down:     navigateHistory(+1); return true;
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
                            c.setPosition(m_promptStart);
                            c.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
                            m_outputWidget->setTextCursor(c);
                        }
                        return false;
                    }
                    if (c.position() <= m_promptStart) return true;
                    return false;
                }
                case Qt::Key_Delete: return true;
                case Qt::Key_A:
                if (ke->modifiers() == Qt::ControlModifier) {
                    QTextCursor c(m_outputWidget->document());
                    c.setPosition(m_promptStart);
                    c.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
                    m_outputWidget->setTextCursor(c);
                    return true;
                }
                break;
                case Qt::Key_Z:
                if (ke->modifiers() == Qt::ControlModifier) return true;
                break;
                default: break;
            }
        }

        // 鼠标：运行态光标拉末尾（有选区不动，方便复制）；空闲态限制在提示符后
        if (event->type() == QEvent::MouseButtonRelease) {
            QTextCursor c = m_outputWidget->textCursor();
            if (m_isCommandRunning) {
                if (!c.hasSelection()) {
                    c.movePosition(QTextCursor::End);
                    m_outputWidget->setTextCursor(c);
                }
            } else {
                if (m_promptStart >= 0 && c.position() < m_promptStart) {
                    c.movePosition(QTextCursor::End);
                    m_outputWidget->setTextCursor(c);
                }
            }
        }
        return QWidget::eventFilter(obj, event);
    }

    // ============================================================================
    // 虚拟环境
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

    void TerminalPanel::setWorkingDirectory(const QString& dir)
    {
        if (m_isCommandRunning) return;

        m_workingDirectory = dir;
        updateVenvForDirectory(dir);

        m_outputWidget->clear();
        m_promptStart = -1;
        m_commandHistory.clear();
        m_historyIndex = 0;

        appendSystemLine(tr("工作目录: ") + dir, false);
        printPrompt();

        emit workingDirectoryChanged(dir);
        m_outputWidget->setFocus();
    }

    QString TerminalPanel::workingDirectory() const { return m_workingDirectory; }

    // ============================================================================
    // 子进程环境（保留原逻辑）
    // ============================================================================
    static QString prependToPath(const QProcessEnvironment& env, const QString& dir)
    {
        QString path = env.value("PATH");
        if (path.isEmpty()) path = "/usr/local/bin:/opt/homebrew/bin:/usr/bin:/bin:/usr/sbin:/sbin";
        QStringList parts = path.split(QDir::listSeparator(), Qt::SkipEmptyParts);
        parts.removeAll(dir);
        parts.prepend(dir);
        return parts.join(QDir::listSeparator());
    }

    QProcessEnvironment TerminalPanel::buildChildEnvironment() const
    {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("PAGER",     "cat");
        env.insert("MANPAGER",  "cat");
        env.insert("GIT_PAGER", "cat");

        ConfigManager* cfg = Application::instance().configManager();
        if (cfg) {
            const QVariantMap sdkEnv = cfg->value("Sdk/Environment").toMap();
            for (auto it = sdkEnv.begin(); it != sdkEnv.end(); ++it) {
                const QString key = it.key();
                const QString val = it.value().toString();
                if (!key.isEmpty() && !val.isEmpty()) env.insert(key, val);
            }
            const QString pyHome = sdkEnv.value("PYTHON_HOME").toString();
            if (!pyHome.isEmpty()) {
                QFileInfo fi(pyHome);
                const QString binDir = fi.isDir() ? pyHome : fi.absolutePath();
                if (!binDir.isEmpty() && QDir(binDir).exists())
                env.insert("PATH", prependToPath(env, binDir));
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
    // 执行命令 → 创建 PTY 子进程
    // ============================================================================
    void TerminalPanel::executeCommand(const QString& command)
    {
        if (command.trimmed().isEmpty()) { printPrompt(); return; }
        if (m_isCommandRunning) {
            appendSystemLine(tr("命令已在运行中，请先停止"), true);
            printPrompt();
            return;
        }

        // 光标移到末尾 + 换行（用户按的 Enter 视觉换行）
        {
            QTextCursor c(m_outputWidget->document());
            c.movePosition(QTextCursor::End);
            if (c.position() > 0) {
                const QChar prev = m_outputWidget->document()->characterAt(c.position() - 1);
                if (prev != QLatin1Char('\n')) c.insertText(QStringLiteral("\n"));
            }
            m_outputWidget->setTextCursor(c);
        }

        // 重置 ANSI / 当前行状态（避免上一条命令残留影响新命令）
        m_lineBuf.clear();
        m_cursorCol = 0;
        m_currentLineDocStart = -1;
        m_escapeState = EscapeState::Normal;
        m_escapeParams.clear();

        if (m_commandHistory.isEmpty() || m_commandHistory.last() != command)
        m_commandHistory.append(command);
        m_historyIndex = m_commandHistory.size();

        m_currentCommand = command;

        // 创建 PTY
        std::unique_ptr<IPtyProcess> pty = PtyFactory::create(this);
        if (!pty) {
            appendSystemLine(tr("当前系统不支持 PTY（Windows 需 10 1809+）"), true);
            printPrompt();
            return;
        }
        m_pty = pty.release();

        connect(m_pty, &IPtyProcess::readyRead,
            this, &TerminalPanel::onPtyReadyRead);
        connect(m_pty, &IPtyProcess::finished, this,
            [this](int exitCode, PtyExitStatus status) {
                onPtyFinished(exitCode, int(status));
            });
        connect(m_pty, &IPtyProcess::errorOccurred,
            this, &TerminalPanel::onPtyError);

        // 命令拆分
        QStringList args = QProcess::splitCommand(command);
        if (args.isEmpty()) {
            appendSystemLine(tr("命令为空"), true);
            m_pty->deleteLater(); m_pty = nullptr;
            printPrompt();
            return;
        }
        const QString program = args.takeFirst();

        if (!m_pty->start(program, args, m_workingDirectory, buildChildEnvironment())) {
            appendSystemLine(tr("启动命令失败"), true);
            m_pty->deleteLater(); m_pty = nullptr;
            printPrompt();
            return;
        }

        m_isCommandRunning = true;
        m_promptStart = -1;               // 运行态不显示本地提示符
        m_stopButton->setEnabled(true);
        syncPtySize();

        emit commandStarted(command);
    }

    bool TerminalPanel::isCommandRunning() const { return m_isCommandRunning; }

    void TerminalPanel::clearOutput()
    {
        if (!m_outputWidget) return;
        m_outputWidget->clear();
        m_lineBuf.clear();
        m_cursorCol = 0;
        m_currentLineDocStart = -1;
        m_escapeState = EscapeState::Normal;
        m_escapeParams.clear();
        m_promptStart = -1;
        printPrompt();
    }

    void TerminalPanel::stopCurrentCommand()
    {
        if (!m_isCommandRunning || !m_pty) return;

        appendSystemLine(tr("正在停止命令..."), false);
        m_pty->sendTerminate();

        QPointer<IPtyProcess> guard = m_pty;
        QTimer::singleShot(2000, this, [guard]() {
                if (guard && guard->isRunning()) guard->sendKill();
            });
    }

    // ============================================================================
    // PTY 输出 → ANSI 状态机
    // ============================================================================
    void TerminalPanel::onPtyReadyRead(const QByteArray& data)
    {
        if (data.isEmpty()) return;

        // 对 AI / 日志模块发原始字节（用 UTF-8 近似）
        emit outputReceived(QString::fromUtf8(data), false);

        feedAnsiBytes(data);

        if (auto* sb = m_outputWidget->verticalScrollBar()) sb->setValue(sb->maximum());
    }

    void TerminalPanel::onPtyFinished(int exitCode, int exitStatus)
    {
        Q_UNUSED(exitStatus);

        // 冲掉最后没换行的内容
        flushCurrentLine();

        m_isCommandRunning = false;
        m_stopButton->setEnabled(false);

        if (exitCode != 0) {
            appendSystemLine(QStringLiteral("[退出码 %1]").arg(exitCode), true);
        }

        if (m_pty) {
            QObject::disconnect(m_pty, nullptr, this, nullptr);
            m_pty->deleteLater();
            m_pty = nullptr;
        }

        emit commandFinished(m_currentCommand, exitCode);

        m_outputWidget->setFocus();
        printPrompt();
    }

    void TerminalPanel::onPtyError(const QString& message)
    {
        appendSystemLine(tr("进程错误: ") + message, true);
    }

    // ANSI 处理
    void TerminalPanel::feedAnsiBytes(const QByteArray& data)
    {
        const QString text = QString::fromUtf8(data);
        for (QChar ch : text) processAnsiChar(ch);

        // chunk 处理完后，如果当前行有未提交内容，立即画出来
        if (!m_lineBuf.isEmpty()) {
            renderCurrentLine();
        }
    }

    // 把"当前未完成行"实时渲染到 QTextEdit 末尾
    void TerminalPanel::renderCurrentLine()
    {
        if (!m_outputWidget) return;

        QTextCursor c(m_outputWidget->document());

        // 当前行尚未在文档中定位 → 从末尾开始
        if (m_currentLineDocStart < 0) {
            c.movePosition(QTextCursor::End);
            m_currentLineDocStart = c.position();
        }

        // 删除该行已画的内容（范围 [m_currentLineDocStart, 文档末尾)）
        c.setPosition(m_currentLineDocStart);
        c.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        c.removeSelectedText();

        // 按颜色分段重画 m_lineBuf
        int i = 0;
        while (i < m_lineBuf.size()) {
            const QTextCharFormat fmt = m_lineBuf[i].second;
            QString run;
            while (i < m_lineBuf.size() && m_lineBuf[i].second == fmt) {
                run.append(m_lineBuf[i].first);
                ++i;
            }
            c.insertText(run, fmt);
        }

        m_outputWidget->setTextCursor(c);
        if (auto* sb = m_outputWidget->verticalScrollBar())
        sb->setValue(sb->maximum());
    }

    void TerminalPanel::processAnsiChar(QChar ch)
    {
        const ushort u = ch.unicode();

        switch (m_escapeState) {
            case EscapeState::Normal:
            if (u == 0x1B) { m_escapeState = EscapeState::Escape; return; }
            if (u == '\n') { flushCurrentLine(); return; }
            if (u == '\r') { m_cursorCol = 0; return; }
            if (u == '\b') {
                if (m_cursorCol > 0) --m_cursorCol;
                return;
            }
            if (u == '\t') {
                const int stop = (m_cursorCol / 8 + 1) * 8;
                while (m_cursorCol < stop) {
                    if (m_cursorCol < m_lineBuf.size())
                    m_lineBuf[m_cursorCol] = { QChar(' '), m_currentFmt };
                    else
                    m_lineBuf.append({ QChar(' '), m_currentFmt });
                    ++m_cursorCol;
                }
                return;
            }
            if (u < 0x20) return;    // 其他控制字符忽略

            // 可打印：写入或覆盖
            if (m_cursorCol < m_lineBuf.size())
            m_lineBuf[m_cursorCol] = { ch, m_currentFmt };
            else {
                while (m_lineBuf.size() < m_cursorCol)
                m_lineBuf.append({ QChar(' '), m_currentFmt });
                m_lineBuf.append({ ch, m_currentFmt });
            }
            ++m_cursorCol;
            return;

            case EscapeState::Escape:
            if (u == '[') { m_escapeState = EscapeState::Csi; m_escapeParams.clear(); return; }
            if (u == ']') { m_escapeState = EscapeState::Osc; return; }
            m_escapeState = EscapeState::Normal;
            return;

            case EscapeState::Csi:
            // 终结字符：0x40 - 0x7E
            if (u >= 0x40 && u <= 0x7E) {
                handleCsi(ch);
                m_escapeState = EscapeState::Normal;
                m_escapeParams.clear();
            } else {
                m_escapeParams.append(ch);
            }
            return;

            case EscapeState::Osc:
            // ESC ] ... BEL 或 ESC \ 结束；这里简单处理：BEL 结束
            if (u == 0x07) m_escapeState = EscapeState::Normal;
            return;
        }
    }

    void TerminalPanel::handleCsi(QChar finalChar)
    {
        const QChar f = finalChar;

        // 去掉前导 '?', '>', '!' 等（如 \x1b[?25l）
        QString params = m_escapeParams;
        if (!params.isEmpty() && (params[0] == '?' || params[0] == '>' || params[0] == '!'))
        params = params.mid(1);

        if (f == 'm') {
            QStringList parts = params.split(';', Qt::SkipEmptyParts);
            if (parts.isEmpty()) parts << "0";
            handleSgr(parts);
            return;
        }
        if (f == 'K') {
            if (params.isEmpty() || params == "0") {
                // 从当前列清到末尾
                if (m_cursorCol < m_lineBuf.size()) m_lineBuf.resize(m_cursorCol);
            } else if (params == "2") {
                m_lineBuf.clear();
                m_cursorCol = 0;
            }
            return;
        }
        if (f == 'J') {
            if (params == "2" || params == "3") {
                m_lineBuf.clear();
                m_cursorCol = 0;
                if (m_outputWidget) m_outputWidget->clear();
                m_currentLineDocStart = -1;   // 文档已清空，起点必须重置
            }
            return;
        }
        if (f == 'H' || f == 'f') {
            // 光标定位：仅把列置 0（全屏程序会失败，但常规行式程序够用）
            m_cursorCol = 0;
            return;
        }
        // 其他 CSI 忽略
    }

    void TerminalPanel::handleSgr(const QStringList& params)
    {
        bool ok = false;
        int i = 0;
        while (i < params.size()) {
            int code = params[i].toInt(&ok);
            if (!ok) { ++i; continue; }

            switch (code) {
                case 0:  m_currentFmt = QTextCharFormat();
                m_currentFmt.setForeground(
                    ThemeManager::instance().color(ThemeManager::UIElement::TextColor));
                break;
                case 1:  m_currentFmt.setFontWeight(QFont::Bold); break;
                case 4:  m_currentFmt.setFontUnderline(true); break;
                case 22: m_currentFmt.setFontWeight(QFont::Normal); break;
                case 24: m_currentFmt.setFontUnderline(false); break;
                case 39: m_currentFmt.setForeground(
                    ThemeManager::instance().color(ThemeManager::UIElement::TextColor)); break;

                case 30: case 31: case 32: case 33:
                case 34: case 35: case 36: case 37:
                m_currentFmt.setForeground(kPalette[code - 30]);
                break;
                case 90: case 91: case 92: case 93:
                case 94: case 95: case 96: case 97:
                m_currentFmt.setForeground(kPalette[code - 90 + 8]);
                break;

                case 38: {
                    // 38;5;N → 256 色   38;2;R;G;B → truecolor
                    if (i + 1 < params.size()) {
                        int mode = params[i + 1].toInt(&ok);
                        if (ok && mode == 5 && i + 2 < params.size()) {
                            int idx = params[i + 2].toInt(&ok);
                            if (ok) {
                                if (idx < 16) m_currentFmt.setForeground(kPalette[idx]);
                                else if (idx >= 16 && idx <= 231) {
                                    int r = (idx - 16) / 36;
                                    int g = ((idx - 16) % 36) / 6;
                                    int b = (idx - 16) % 6;
                                    m_currentFmt.setForeground(QColor(r * 51, g * 51, b * 51));
                                } else m_currentFmt.setForeground(QColor(200, 200, 200));
                                i += 2;
                            }
                        } else if (ok && mode == 2 && i + 4 < params.size()) {
                            int r = params[i + 2].toInt(&ok);
                            int g = ok ? params[i + 3].toInt(&ok) : 0;
                            int b = ok ? params[i + 4].toInt(&ok) : 0;
                            if (ok) m_currentFmt.setForeground(QColor(r, g, b));
                            i += 4;
                        }
                    }
                    break;
                }

                default: break;   // 背景色 / 其他忽略
            }
            ++i;
        }
    }

    // 当前行提交
    void TerminalPanel::flushCurrentLine()
    {
        if (!m_outputWidget) return;

        // 1. 先把当前行（可能已经在画布上，也可能还没画）落实为最终内容
        renderCurrentLine();

        // 2. 插换行
        QTextCursor c(m_outputWidget->document());
        c.movePosition(QTextCursor::End);
        c.insertText(QStringLiteral("\n"));

        // 3. 重置当前行状态
        m_lineBuf.clear();
        m_cursorCol = 0;
        m_currentLineDocStart = -1;

        m_outputWidget->setTextCursor(c);
    }

    void TerminalPanel::clearCurrentLineBuf()
    {
        m_lineBuf.clear();
        m_cursorCol = 0;
    }

    // ============================================================================
    // 尺寸同步
    // ============================================================================
    void TerminalPanel::syncPtySize()
    {
        if (!m_pty || !m_outputWidget) return;

        const QFontMetricsF fm(m_outputWidget->font());
        const QMargins margins = m_outputWidget->contentsMargins();
        const int w = m_outputWidget->viewport()->width()  - margins.left() - margins.right();
        const int h = m_outputWidget->viewport()->height() - margins.top()  - margins.bottom();

        const qreal charW = fm.horizontalAdvance(QLatin1Char('M'));
        const qreal charH = fm.lineSpacing();

        int cols = charW > 0 ? int(w / charW) : 80;
        int rows = charH > 0 ? int(h / charH) : 24;
        cols = qBound(20, cols, 500);
        rows = qBound(5,  rows, 200);

        m_pty->resize(PtySize{rows, cols});
    }

    void TerminalPanel::resizeEvent(QResizeEvent* event)
    {
        QWidget::resizeEvent(event);
        syncPtySize();
    }

    // ============================================================================
    // 主题切换：清空终端（PTY 内容的颜色由远端决定，不跟随主题）
    // ============================================================================
    void TerminalPanel::recolorAllOutput()
    {
        if (!m_outputWidget) return;
        if (m_isCommandRunning) return;   // 运行中不清屏

        m_outputWidget->clear();
        m_lineBuf.clear();
        m_cursorCol = 0;
        m_currentLineDocStart = -1;
        m_escapeState = EscapeState::Normal;
        m_escapeParams.clear();
        m_promptStart = -1;

        m_currentFmt = QTextCharFormat();
        m_currentFmt.setForeground(
            ThemeManager::instance().color(ThemeManager::UIElement::TextColor));

        printPrompt();
    }

    void TerminalPanel::focusPanel()
    {
        if (m_outputWidget) {
            m_outputWidget->setFocus();
            QTextCursor c = m_outputWidget->textCursor();
            c.movePosition(QTextCursor::End);
            m_outputWidget->setTextCursor(c);
        }
    }

} // namespace SprintIDE