// SPDX-License-Identifier: GPL-3.0-or-later
#include "terminal_panel.h"

#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFileInfo>
#include <QFontMetricsF>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMessageBox>
#include <QRegularExpression>
#include <QStringDecoder>
#include <QTimer>
#include <QVBoxLayout>
#include <QDebug>

#include "terminal/pty/pty_factory.h"
#include "terminal/pty/pty_types.h"
#include "terminal/terminal_grid_model.h"
#include "terminal/terminal_view.h"
#include "terminal/terminal_search_bar.h"
#include "terminal/terminal_history_store.h"
#include "terminal/terminal_output_parser.h"
#include "terminal/terminal_output_overlay.h"

#include "project_manager.h"
#include "python/python_virtual_env.h"
#include "python/python_project_model.h"
#include "theme_manager.h"
#include "Application.h"
#include "config_manager.h"

namespace SprintIDE {

static QString findProjectRoot(const QString& startDir)
{
    if (startDir.isEmpty()) return QString();

    QDir dir(startDir);
    if (!dir.exists()) return QString();

    for (int i = 0; i < 20; ++i) {
        if (dir.exists(QStringLiteral(".git"))
            || dir.exists(QStringLiteral("CMakeLists.txt"))
            || dir.exists(QStringLiteral("package.json"))
            || dir.exists(QStringLiteral("Cargo.toml"))
            || dir.exists(QStringLiteral("go.mod"))
            || dir.exists(QStringLiteral("pyproject.toml"))
            || dir.exists(QStringLiteral("pom.xml"))
            || dir.exists(QStringLiteral("build.gradle"))
            || dir.exists(QStringLiteral("Makefile"))) {
            return dir.absolutePath();
        }
        if (!dir.cdUp()) break;
    }
    return QString();
}

// ============================================================================
// 构造 / 析构
// ============================================================================
TerminalPanel::TerminalPanel(QWidget* parent)
    : QWidget(parent)
{
    m_decoder = std::make_unique<QStringDecoder>(QStringDecoder::Utf8);

    setupUI();
    setObjectName("TerminalPanel");
    setMinimumHeight(0);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
    setFocusPolicy(Qt::StrongFocus);

    m_workingDirectory = QDir::currentPath();
    updateVenvForDirectory(m_workingDirectory);
}

TerminalPanel::~TerminalPanel()
{
    if (m_pty) {
        // 断开所有信号：避免 delete 期间 onPtyFinished 重入
        QObject::disconnect(m_pty, nullptr, this, nullptr);

        // 删除 PTY —— ~UnixPtyProcess / ~WinConPtyProcess 内部负责 SIGTERM + 超时
        //   （不在这里再 sendTerminate，避免重复发信号 + 双重等待）
        delete m_pty;
        m_pty = nullptr;
    }
    m_shellRunning = false;
}

// ============================================================================
// UI
// ============================================================================
void TerminalPanel::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 搜索栏（顶部，默认隐藏）
    m_searchBar = new TerminalSearchBar(this);
    m_searchBar->setVisible(false);
    layout->addWidget(m_searchBar, 0);

    // 视图
    m_model = new TerminalGridModel(this);
    m_view  = new TerminalView(this);
    m_view->setModel(m_model);
    layout->addWidget(m_view, 1);

    // View 信号
    connect(m_view, &TerminalView::inputRequested,
            this, &TerminalPanel::onViewInput);
    connect(m_view, &TerminalView::searchRequested,
            this, &TerminalPanel::onSearchRequested);
    connect(m_view, &TerminalView::searchStateChanged,
            this, &TerminalPanel::onSearchStateChanged);
    connect(m_view, &TerminalView::linkClicked,
            this, &TerminalPanel::parseLinkAndEmit);
    connect(m_view, &TerminalView::clearRequested,
            this, &TerminalPanel::clearOutput);

    // 用户回车时提交一条命令
    connect(m_view, &TerminalView::commandSubmitted,
            this, &TerminalPanel::onCommandSubmitted);

    // resize 后同步 PTY 尺寸（发 SIGWINCH）
    connect(m_view, &TerminalView::modelSizeChanged,
            this, [this](int /*rows*/, int /*cols*/) {
                syncPtySize();
            });

    // 输出结构化浮层：parent = this（浮在整个终端面板右上角）
    m_outputOverlay = new TerminalOutputOverlay(this);
    connect(m_outputOverlay, &TerminalOutputOverlay::actionTriggered,
            this, &TerminalPanel::onStructuredAction);

    // 大输出量节流定时器（16ms 批量 feed，60fps 上限）
    m_feedTimer = new QTimer(this);
    m_feedTimer->setSingleShot(true);
    m_feedTimer->setInterval(16);
    connect(m_feedTimer, &QTimer::timeout,
            this, &TerminalPanel::flushPtyData);

    // GridModel 的 title 变化 -> 转发 shellTitleChanged
    connect(m_model, &TerminalGridModel::titleChanged,
            this, &TerminalPanel::shellTitleChanged);

    // OSC 52：远端写剪贴板，弹框确认
    connect(m_model, &TerminalGridModel::oscClipboardRequested,
            this, [this](const QString& text) {
                // 默认自动接受（和 iTerm2 行为一致）
                QApplication::clipboard()->setText(text);
            });

    // 搜索栏信号
    connect(m_searchBar, &TerminalSearchBar::searchChanged,
            this, &TerminalPanel::onSearchChanged);
    connect(m_searchBar, &TerminalSearchBar::findNext,
            m_view, &TerminalView::findNext);
    connect(m_searchBar, &TerminalSearchBar::findPrev,
            m_view, &TerminalView::findPrev);
    connect(m_searchBar, &TerminalSearchBar::closeRequested,
            this, &TerminalPanel::onSearchClose);

    // 主题适配：主题切换 -> 重设配色 -> View 全屏重绘
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &TerminalPanel::applyThemeColors);

    // 首次应用（构造时就同步当前主题）
    QTimer::singleShot(0, this, &TerminalPanel::applyThemeColors);

    // 字体跟随编辑器设置
    //   Settings -> 编辑器 -> 字体 / 字号，保存时 ThemeManager 会 emit fontChanged
    connect(&ThemeManager::instance(), &ThemeManager::fontChanged,
            this, [this]() { applyEditorFontFromConfig(); });

    // 首次应用也延迟（同 applyEditorFontFromConfig 内部逻辑一致，但这里
    // 用 100ms 比 0ms 更保险，确保布局 / 首次 resizeEvent 全部稳定后再应用）
    QTimer::singleShot(100, this, [this]() { applyEditorFontFromConfig(); });

}

// ============================================================================
// 搜索栏
// ============================================================================
void TerminalPanel::showSearchBar()
{
    if (!m_searchBar) return;
    m_searchBar->showAndFocus(m_searchBar->query());
}

void TerminalPanel::onSearchRequested()
{
    showSearchBar();
}

void TerminalPanel::onSearchChanged(const QString& query, bool caseSensitive)
{
    if (!m_view) return;
    m_view->setSearchQuery(query, caseSensitive);
}

void TerminalPanel::onSearchStateChanged(int current, int total)
{
    if (m_searchBar) m_searchBar->setMatchInfo(current, total);
}

void TerminalPanel::onSearchClose()
{
    if (!m_searchBar || !m_view) return;
    m_view->clearSearch();
    m_searchBar->setVisible(false);
    if (m_view) m_view->setFocus();
}

void TerminalPanel::applyThemeColors()
{
    if (!m_model || !m_view) return;

    auto& tm = ThemeManager::instance();
    using E = ThemeManager::UIElement;

    // 默认前景 / 背景
    const QColor fg = tm.color(E::TextColor);
    const QColor bg = tm.color(E::MainWindowBackground);
    m_model->setDefaultColors(fg, bg);

    // 选中 / 匹配 / 超链接 / 光标
    m_view->setThemeColors(
        tm.color(E::TreeSelectedBackground),
        tm.color(E::WarningColor).darker(140),
        tm.color(E::WarningColor),
        tm.color(E::PrimaryColor),
        fg);

    //   ANSI 16 色：顺序固定 SGR 30-37 → 90-97
    //   令牌名按你 theme_manager.h 实际命名改这 16 行，其余不动。
    const std::array<QColor, 16> ansi = {
        tm.color(E::TerminalAnsiBlack),
        tm.color(E::TerminalAnsiRed),
        tm.color(E::TerminalAnsiGreen),
        tm.color(E::TerminalAnsiYellow),
        tm.color(E::TerminalAnsiBlue),
        tm.color(E::TerminalAnsiMagenta),
        tm.color(E::TerminalAnsiCyan),
        tm.color(E::TerminalAnsiWhite),
        tm.color(E::TerminalAnsiBrightBlack),
        tm.color(E::TerminalAnsiBrightRed),
        tm.color(E::TerminalAnsiBrightGreen),
        tm.color(E::TerminalAnsiBrightYellow),
        tm.color(E::TerminalAnsiBrightBlue),
        tm.color(E::TerminalAnsiBrightMagenta),
        tm.color(E::TerminalAnsiBrightCyan),
        tm.color(E::TerminalAnsiBrightWhite),
    };
    m_model->setAnsiPalette(ansi);
}

void TerminalPanel::applyEditorFontFromConfig()
{
    if (!m_view) return;

    ConfigManager* cfg = Application::instance().configManager();
    if (!cfg) return;

    const QString family = cfg->value("Editor/FontFamily",
                                      QStringLiteral("JetBrains Mono")).toString();
    const int     size   = cfg->value("Editor/FontSize", 12).toInt();

    //   延迟到下一轮事件循环：构造期 viewport 尺寸未定，
    //   立即应用会导致 model 尺寸抖动。延迟后布局已完成，viewport 尺寸有效。
    QTimer::singleShot(0, this, [this, family, size]() {
        if (!m_view) return;
        m_view->applyEditorFont(family, size);
        syncPtySize();
    });
}

// ============================================================================
// showEvent / resizeEvent / keyPressEvent
// ============================================================================
void TerminalPanel::showEvent(QShowEvent* e)
{
    QWidget::showEvent(e);
    if (!m_shellInitialized) {
        m_shellInitialized = true;
        //   100ms 延迟：给 setVisible / resizeEvent / layout 全部稳定下来的时间
        //   0ms 会在 setVisible 同步流程刚结束时立即执行，此时布局可能还在调整
        QTimer::singleShot(100, this, &TerminalPanel::startShell);
    }
}

void TerminalPanel::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    QTimer::singleShot(0, this, &TerminalPanel::syncPtySize);
}

void TerminalPanel::keyPressEvent(QKeyEvent* e)
{
    // Ctrl+F 在主面板上也能触发搜索（当焦点在 stop button 等子控件时）
    if (e->key() == Qt::Key_F
        && (e->modifiers() & (Qt::ControlModifier | Qt::MetaModifier))) {
        showSearchBar();
        e->accept();
        return;
    }
    QWidget::keyPressEvent(e);
}

// ============================================================================
// shell
// ============================================================================
QString TerminalPanel::shellProgram() const
{
#ifdef Q_OS_WIN
    return QStringLiteral("cmd.exe");
#else
    QString sh = QProcessEnvironment::systemEnvironment().value(QStringLiteral("SHELL"));
    if (sh.isEmpty()) sh = QStringLiteral("/bin/bash");
    return sh;
#endif
}

QStringList TerminalPanel::shellArgs() const
{
    return {};
}

void TerminalPanel::startShell()
{
    if (m_shellRunning || m_shellStartPending) return;
    m_shellStartPending = true;

    if (m_pty) {
        QObject::disconnect(m_pty, nullptr, this, nullptr);
        m_pty->deleteLater();
        m_pty = nullptr;
    }

    auto pty = PtyFactory::create(this);
    if (!pty) {
        m_model->feed("\r\n\x1b[31m[当前系统不支持 PTY]\x1b[0m\r\n");
        m_shellStartPending = false;
        return;
    }
    m_pty = pty.release();

    connect(m_pty, &IPtyProcess::readyRead, this, &TerminalPanel::onPtyReadyRead);
    connect(m_pty, &IPtyProcess::finished, this,
            [this](int exitCode, PtyExitStatus status) {
                onPtyFinished(exitCode, int(status));
            });
    connect(m_pty, &IPtyProcess::errorOccurred,
            this, &TerminalPanel::onPtyError);

    const QString program = shellProgram();
    if (!m_pty->start(program, shellArgs(), m_workingDirectory,
                      buildChildEnvironment())) {
        m_model->feed("\r\n\x1b[31m[shell 启动失败]\x1b[0m\r\n");
        m_pty->deleteLater();
        m_pty = nullptr;
        m_shellStartPending = false;
        return;
    }

    m_shellRunning      = true;
    m_shellStartPending = false;

    if (m_view && m_model) {
        const int cols = m_view->visibleColCount();
        const int rows = m_view->visibleRowCount();
        if (rows >= 5 && rows <= 100 && cols >= 20 && cols <= 500) {
            m_model->setSize(rows, cols);
        }
    }

    // 允许后续 resize（拖动面板会触发 debounce → setSize → SIGWINCH）
    if (m_view) {
        m_view->setResizeAllowed(true);
    }

    emit commandStarted(program);
    emit shellRunningChanged(true);

    syncPtySize();

    // venv 自动激活
    if (!m_currentVenvPath.isEmpty() && m_pty) {
#ifdef Q_OS_WIN
        const QString activate = QStringLiteral("\"%1\\Scripts\\activate\"\r")
                                     .arg(QDir::toNativeSeparators(m_currentVenvPath));
#else
        const QString activate = QStringLiteral("source \"%1/bin/activate\"\r")
                                     .arg(m_currentVenvPath);
#endif
        const QByteArray bytes = activate.toUtf8();
        QTimer::singleShot(300, this, [this, bytes]() {
            if (m_pty && m_shellRunning) m_pty->write(bytes);
        });
    }
}

void TerminalPanel::stopShell()
{
    if (!m_pty) return;
    QObject::disconnect(m_pty, nullptr, this, nullptr);
    m_pty->sendTerminate();
    m_pty->deleteLater();
    m_pty = nullptr;
    m_shellRunning = false;
    emit shellRunningChanged(false);
}

void TerminalPanel::restartShell()
{
    stopShell();
    m_shellStartPending = false;
    QTimer::singleShot(0, this, &TerminalPanel::startShell);
}

// ============================================================================
// PTY
// ============================================================================
void TerminalPanel::onPtyReadyRead(const QByteArray& data)
{
    if (data.isEmpty() || !m_model) return;

    // ---- 1. 输出结构化：有新输出就重置 idle 定时器 ----
    if (!m_pendingCommand.isEmpty() && m_commandIdleTimer) {
        m_commandIdleTimer->start();
    }

    // ---- 2. UTF-8 流式解码 ----
    if (m_decoder) {
        const QString text = m_decoder->decode(data);
        if (!text.isEmpty()) {
            // ★ G1：累积最近输出用于 SSH 断线检测
            m_recentOutputBuffer.append(text);
            static constexpr int kBufferLimit = 8 * 1024;
            if (m_recentOutputBuffer.size() > kBufferLimit) {
                m_recentOutputBuffer.remove(
                    0, m_recentOutputBuffer.size() - kBufferLimit);
            }

            // ★ C2：白名单命令累积输出（含 ANSI，解析时统一剥离）
            if (!m_pendingCommand.isEmpty()
                && TerminalOutputParser::canParse(m_pendingCommand)) {
                m_commandOutputBuffer.append(text);

                // 上限 1MB，超了从头部丢掉一半（保留最近 512KB）
                static constexpr int kOutMax = 1024 * 1024;
                if (m_commandOutputBuffer.size() > kOutMax) {
                    m_commandOutputBuffer =
                        m_commandOutputBuffer.right(kOutMax / 2);
                }
            }

            emit outputReceived(text, false);
        }
    }

    // ---- 3. 累积到待 feed 缓冲区 ----
    m_pendingPtyData.append(data);

    static constexpr int kHardLimit = 32 * 1024;
    if (m_pendingPtyData.size() >= kHardLimit) {
        flushPtyData();
    } else {
        if (m_feedTimer && !m_feedTimer->isActive()) {
            m_feedTimer->start();
        }
    }
}

void TerminalPanel::flushPtyData()
{
    if (m_pendingPtyData.isEmpty() || !m_model) return;

    // 一次性喂给 libvterm
    m_model->feed(m_pendingPtyData);
    m_pendingPtyData.clear();

    if (m_feedTimer) m_feedTimer->stop();
}

void TerminalPanel::onPtyFinished(int exitCode, int exitStatus)
{
    Q_UNUSED(exitStatus);

    // shell 退出前把最后的数据 flush 出来（可能还有未处理完的）
    flushPtyData();

    m_shellRunning = false;
    emit shellRunningChanged(false);
    if (m_model) {
        m_model->feed(QStringLiteral("\r\n\x1b[33m[shell 已退出，退出码 %1]\x1b[0m\r\n")
                          .arg(exitCode).toUtf8());
    }
    if (m_pty) {
        QObject::disconnect(m_pty, nullptr, this, nullptr);
        m_pty->deleteLater();
        m_pty = nullptr;
    }
    emit commandFinished(QString(), exitCode);
}

void TerminalPanel::onPtyError(const QString& message)
{
    if (m_model) {
        m_model->feed(QStringLiteral("\r\n\x1b[31m[进程错误: %1]\x1b[0m\r\n")
                          .arg(message).toUtf8());
    }
}

void TerminalPanel::onViewInput(const QByteArray& bytes)
{
    if (!m_pty || !m_shellRunning) return;
    m_pty->write(bytes);
}

void TerminalPanel::onCommandSubmitted(const QString& command)
{
    if (command.trimmed().isEmpty()) return;

    // 如果上一条命令还没处理完，先强制 flush
    if (!m_pendingCommand.isEmpty()) {
        if (m_commandIdleTimer) m_commandIdleTimer->stop();
        onCommandOutputIdle();
    }

    // ---- 记录当前命令 + 开始时间 + 项目根 ----
    m_pendingCommand             = command;
    m_pendingCommandStartMs      = QDateTime::currentMSecsSinceEpoch();
    m_pendingCommandProjectPath  = findProjectRoot(m_workingDirectory);

    // 清空输出 buffer（只对白名单命令累积）
    m_commandOutputBuffer.clear();

    // 记录 ssh 命令
    if (command.trimmed().startsWith(QLatin1String("ssh "))) {
        m_lastSshCommand = command.trimmed();
        m_recentOutputBuffer.clear();
    } else {
        m_lastSshCommand.clear();
    }

    // ---- 启动 idle 检测 ----
    if (!m_commandIdleTimer) {
        m_commandIdleTimer = new QTimer(this);
        m_commandIdleTimer->setSingleShot(true);
        m_commandIdleTimer->setInterval(500);
        connect(m_commandIdleTimer, &QTimer::timeout,
                this, &TerminalPanel::onCommandOutputIdle);
    }
    m_commandIdleTimer->start();

    if (m_outputOverlay) m_outputOverlay->dismiss();
}

// ============================================================================
// 命令输出静默 -> 解析结构化
// ============================================================================
void TerminalPanel::onCommandOutputIdle()
{
    if (m_pendingCommand.isEmpty()) return;

    // ---- 0. 拷贝 + 清空输出 buffer（后续可能提前 return，避免残留） ----
    const QString rawOutput = m_commandOutputBuffer;
    m_commandOutputBuffer.clear();

    // ---- 1. 命令历史入库 ----
    const qint64 durationMs = (m_pendingCommandStartMs > 0)
        ? (QDateTime::currentMSecsSinceEpoch() - m_pendingCommandStartMs)
        : -1;

    auto* store = TerminalHistoryStore::instance();
    store->addCommand(
        m_pendingCommand,
        m_workingDirectory,
        m_pendingCommandProjectPath.isEmpty()
            ? m_workingDirectory
            : m_pendingCommandProjectPath,
        durationMs);

    // ---- 2. 输出结构化（用 PTY 流 buffer，而非可视区快照） ----
    if (m_outputOverlay
        && TerminalOutputParser::canParse(m_pendingCommand)) {
        ParsedOutputResult result =
            TerminalOutputParser::parse(m_pendingCommand, rawOutput);

        if (result.ok && !result.items.isEmpty()) {
            m_outputOverlay->showResult(result);
        } else {
            m_outputOverlay->dismiss();
        }
    }

    // ---- 3. SSH 断线检测 ----
    const bool wasSsh = m_pendingCommand.trimmed()
                            .startsWith(QLatin1String("ssh "));
    if (wasSsh && durationMs >= 3000
        && !m_lastSshCommand.isEmpty()
        && !m_sshReconnectDialogOpen
        && isSshDisconnectOutput(m_recentOutputBuffer)) {
        const QString cmdToReconnect = m_lastSshCommand;
        QTimer::singleShot(0, this, [this, cmdToReconnect]() {
            promptSshReconnect(cmdToReconnect);
        });
    }

    // ---- 4. 清理状态 ----
    m_pendingCommand.clear();
    m_pendingCommandStartMs     = 0;
    m_pendingCommandProjectPath.clear();
}

// ============================================================================
// 判断最近输出里是否有 SSH 断开的关键词
// ============================================================================
bool TerminalPanel::isSshDisconnectOutput(const QString& recentOutput) const
{
    if (recentOutput.isEmpty()) return false;

    // 各种 OpenSSH / 系统级断开提示（中英文都覆盖）
    static const QRegularExpression re(
        QStringLiteral(
            R"((Connection\s+(reset|closed|timed\s*out)\s+by\s+(peer|remote|foreign\s+host))|)"
            R"((Broken\s+pipe)|)"
            R"((client_loop:\s+send\s+disconnect)|)"
            R"((Disconnected\s+from\s+remote\s+host)|)"
            R"((Packet\s+corrupted)|)"
            R"((select:\s+Connection\s+reset\s+by\s+peer)|)"
            R"((Connection\s+to\s+\S+\s+closed\s+by\s+remote\s+host))"
        ),
        QRegularExpression::CaseInsensitiveOption
    );

    return re.match(recentOutput).hasMatch();
}

// ============================================================================
// 弹出 SSH 重连确认框
// ============================================================================
void TerminalPanel::promptSshReconnect(const QString& sshCommand)
{
    if (sshCommand.isEmpty()) return;
    if (m_sshReconnectDialogOpen) return;   // 防叠加

    m_sshReconnectDialogOpen = true;

    QMessageBox box(this);
    box.setWindowTitle(tr("SSH 会话已断开"));
    box.setIcon(QMessageBox::Question);
    box.setTextFormat(Qt::PlainText);
    box.setText(
        tr("SSH 会话已断开。\n\n"
           "命令：%1\n\n"
           "是否重新连接？").arg(sshCommand));
    box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    box.setDefaultButton(QMessageBox::Yes);

    const int ret = box.exec();

    m_sshReconnectDialogOpen = false;

    if (ret == QMessageBox::Yes) {
        // 回填原 ssh 命令并立即执行（用户按的"是"，语义明确）
        if (m_pty && m_shellRunning) {
            QByteArray bytes = sshCommand.toUtf8();
            if (!bytes.endsWith('\r')) bytes.append('\r');
            m_pty->write(bytes);

            // 把焦点给终端，方便用户输密码
            if (m_view) m_view->setFocus();
        }
    } else {
        // 用户拒绝：清空 ssh 状态，避免下次误触发
        m_lastSshCommand.clear();
        m_recentOutputBuffer.clear();
    }
}

// ============================================================================
// 用户点击结构化项 -> 执行动作
// ============================================================================
void TerminalPanel::onStructuredAction(const QString& action)
{
    if (action.isEmpty()) return;

    if (action.startsWith(QLatin1String("open:"))) {
        QString path = action.mid(5);
        if (QDir::isRelativePath(path)) {
            path = QDir(m_workingDirectory).absoluteFilePath(path);
        }
        // 复用 P1 阶段的 openFileAtLine 信号
        //   line = 1（跳到文件第一行）—— MainWindow::onDefinitionReceived 里
        //   line <= 0 会被拒绝，所以传 1
        emit openFileAtLine(path, 1, 0);
        m_outputOverlay->dismiss();
    } else if (action.startsWith(QLatin1String("run:"))) {
        QString cmd = action.mid(4);
        // 回填到终端（不自动执行）
        if (m_pty && m_shellRunning) {
            m_pty->write(cmd.toUtf8());
        }
        // 焦点回终端
        if (m_view) m_view->setFocus();
        m_outputOverlay->dismiss();
    } else if (action.startsWith(QLatin1String("copy:"))) {
        QApplication::clipboard()->setText(action.mid(5));
        m_outputOverlay->dismiss();
    }
}

// ============================================================================
// 命令 / 中断
// ============================================================================
void TerminalPanel::executeCommand(const QString& command)
{
    if (!m_pty || !m_shellRunning || command.isEmpty()) return;
    QByteArray bytes = command.toUtf8();
    bytes.replace('\n', '\r');
    if (!bytes.endsWith('\r')) bytes.append('\r');
    m_pty->write(bytes);
    emit commandStarted(command);
}

void TerminalPanel::writeRaw(const QByteArray& data)
{
    if (!m_pty || !m_shellRunning || data.isEmpty()) return;
    m_pty->write(data);
}

void TerminalPanel::stopCurrentCommand()
{
    if (!m_pty || !m_shellRunning) return;
    m_pty->write(QByteArray(1, '\x03'));
}

bool TerminalPanel::isCommandRunning() const { return m_shellRunning; }

void TerminalPanel::clearOutput()
{
    if (m_model) m_model->feed("\x1b[2J\x1b[H");
}

void TerminalPanel::focusPanel()
{
    if (m_view) m_view->setFocus();
}

// ============================================================================
// 超链接
// ============================================================================
void TerminalPanel::parseLinkAndEmit(const QString& link)
{
    // path:line[:col] -> 发 openFileAtLine
    static const QRegularExpression re(
        QStringLiteral(R"(^(.+):(\d+)(?::(\d+))?$)"));
    const auto m = re.match(link);
    if (m.hasMatch()) {
        const QString path = m.captured(1);
        const int line = m.captured(2).toInt();
        const int col  = m.captured(3).isEmpty() ? 0 : m.captured(3).toInt();
        emit openFileAtLine(path, line, col);
    }
    // URL 已经在 View 里 QDesktopServices::openUrl 处理了，不会到这里
}

// ============================================================================
// 工作目录 / venv / 环境
// ============================================================================
void TerminalPanel::setWorkingDirectory(const QString& dir)
{
    if (dir.isEmpty() || dir == m_workingDirectory) return;
    m_workingDirectory = dir;
    updateVenvForDirectory(dir);
    if (m_shellRunning) restartShell();
    emit workingDirectoryChanged(dir);
}

QString TerminalPanel::workingDirectory() const { return m_workingDirectory; }

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

static QString prependToPath(const QProcessEnvironment& env, const QString& dir)
{
    QString path = env.value(QStringLiteral("PATH"));
    if (path.isEmpty()) {
        path = QStringLiteral("/usr/local/bin:/opt/homebrew/bin:/usr/bin:/bin:"
                              "/usr/sbin:/sbin");
    }
    QStringList parts = path.split(QDir::listSeparator(), Qt::SkipEmptyParts);
    parts.removeAll(dir);
    parts.prepend(dir);
    return parts.join(QDir::listSeparator());
}

QProcessEnvironment TerminalPanel::buildChildEnvironment() const
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("PAGER"),     QStringLiteral("cat"));
    env.insert(QStringLiteral("MANPAGER"),  QStringLiteral("cat"));
    env.insert(QStringLiteral("GIT_PAGER"), QStringLiteral("cat"));

    ConfigManager* cfg = Application::instance().configManager();
    if (cfg) {
        const QVariantMap sdkEnv = cfg->value("Sdk/Environment").toMap();
        for (auto it = sdkEnv.begin(); it != sdkEnv.end(); ++it) {
            if (!it.key().isEmpty() && !it.value().toString().isEmpty()) {
                env.insert(it.key(), it.value().toString());
            }
        }
        const QString pyHome = sdkEnv.value(QStringLiteral("PYTHON_HOME")).toString();
        if (!pyHome.isEmpty()) {
            QFileInfo fi(pyHome);
            const QString binDir = fi.isDir() ? pyHome : fi.absolutePath();
            if (!binDir.isEmpty() && QDir(binDir).exists()) {
                env.insert(QStringLiteral("PATH"), prependToPath(env, binDir));
            }
        }
    }

    if (!m_currentVenvPath.isEmpty()) {
#ifdef Q_OS_WIN
        const QString binDir = QDir(m_currentVenvPath).absoluteFilePath(QStringLiteral("Scripts"));
#else
        const QString binDir = QDir(m_currentVenvPath).absoluteFilePath(QStringLiteral("bin"));
#endif
        if (QDir(binDir).exists()) {
            env.insert(QStringLiteral("PATH"), prependToPath(env, binDir));
            env.insert(QStringLiteral("VIRTUAL_ENV"), m_currentVenvPath);
        }
    }
    return env;
}

void TerminalPanel::syncPtySize()
{
    if (!m_pty || !m_model) return;
    const int rows = m_model->rows();
    const int cols = m_model->cols();
    if (rows <= 0 || cols <= 0) return;
    m_pty->resize(PtySize{rows, cols});
}

} // namespace SprintIDE