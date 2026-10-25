// terminal/pty/unix_pty_process.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
#include "unix_pty_process.h"

#ifndef Q_OS_WIN

#include <QSocketNotifier>
#include <QFile>
#include <QList>

#include <cerrno>
#include <cstring>
#include <csignal>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
#  include <util.h>          // forkpty / openpty
#elif defined(Q_OS_LINUX)
#  include <pty.h>
#else
#  error "Unsupported Unix platform"
#endif

namespace SprintIDE {

// ============================================================================
// 构造 / 析构
// ============================================================================
UnixPtyProcess::UnixPtyProcess(QObject* parent)
    : IPtyProcess(parent)
{
}

UnixPtyProcess::~UnixPtyProcess()
{
    // 1. 杀子进程并同步等待
    if (m_running && m_childPid > 0) {
        const pid_t childPid = static_cast<pid_t>(m_childPid);
        ::kill(childPid, SIGTERM);
        int status = 0;
        ::waitpid(childPid, &status, 0);
        m_running = false;
    }

    // 2. 直接 delete notifier
    //    析构里事件循环可能已停，deleteLater 不会被执行
    if (m_notifier) {
        QObject::disconnect(m_notifier, nullptr, this, nullptr);
        delete m_notifier;
        m_notifier = nullptr;
    }

    // 3. 关闭 fd（不调 closeMaster，因为它在槽里用了 deleteLater 是给
    //    正常运行时用的，析构里要同步释放）
    if (m_masterFd >= 0) {
        ::close(m_masterFd);
        m_masterFd = -1;
    }
}

// ============================================================================
// start() —— forkpty + execvp
// ============================================================================
bool UnixPtyProcess::start(const QString& program,
                           const QStringList& args,
                           const QString& workingDir,
                           const QProcessEnvironment& env)
{
    if (m_running) {
        emit errorOccurred(QStringLiteral("UnixPtyProcess already running"));
        return false;
    }

    struct winsize ws{};
    ws.ws_row = 24;
    ws.ws_col = 80;

    int masterFd = -1;
    pid_t pid = ::forkpty(&masterFd, nullptr, nullptr, &ws);
    if (pid < 0) {
        emit errorOccurred(QString::fromLocal8Bit(std::strerror(errno)));
        return false;
    }

    if (pid == 0) {
        // ---- 子进程：只用 async-signal-safe 调用 ----
        if (!workingDir.isEmpty()) {
            const QByteArray dir = QFile::encodeName(workingDir);
            ::chdir(dir.constData());
        }

        // 注入环境变量（PATH / VIRTUAL_ENV / SDK 变量等）
        for (const QString& kv : env.toStringList()) {
            const int eq = kv.indexOf(QLatin1Char('='));
            if (eq <= 0) continue;
            const QByteArray k = kv.left(eq).toUtf8();
            const QByteArray v = kv.mid(eq + 1).toUtf8();
            ::setenv(k.constData(), v.constData(), 1);
        }
        // 终端标识：让 ls / grep / vim 显示颜色
        ::setenv("TERM",      "xterm-256color", 1);
        ::setenv("COLORTERM", "truecolor",      1);

        // 组装 argv
        const QByteArray prog = QFile::encodeName(program);
        QList<QByteArray> argBytes;
        argBytes.reserve(args.size() + 2);
        argBytes.append(prog);
        for (const QString& a : args) {
            argBytes.append(a.toUtf8());
        }
        argBytes.append(QByteArray());

        QList<char*> argv;
        argv.reserve(argBytes.size() + 1);
        for (auto& b : argBytes) argv.append(b.data());
        argv.append(nullptr);

        ::execvp(prog.constData(), argv.data());

        // exec 失败
        ::_exit(127);
    }

    // ---- 父进程 ----
    m_masterFd = masterFd;
    m_childPid = static_cast<qint64>(pid);
    m_running  = true;

    // masterFd 设非阻塞（read 返回 EAGAIN 时才交给 Qt 事件循环）
    int flags = ::fcntl(m_masterFd, F_GETFL);
    ::fcntl(m_masterFd, F_SETFL, flags | O_NONBLOCK);

    m_notifier = new QSocketNotifier(m_masterFd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated,
            this, &UnixPtyProcess::onReadable);

    return true;
}

// ============================================================================
// onReadable() —— master fd 可读：读数据 + 检查子进程是否退出
// ============================================================================
void UnixPtyProcess::onReadable()
{
    if (m_masterFd < 0) return;

    char buf[8192];
    for (;;) {
        const ssize_t n = ::read(m_masterFd, buf, sizeof(buf));
        if (n > 0) {
            emit readyRead(QByteArray(buf, int(n)));
            continue;
        }
        if (n == 0) break;                                   // EOF
        if (errno == EAGAIN || errno == EWOULDBLOCK) break;  // 暂无更多数据
        if (errno == EINTR) continue;
        break;                                               // 其他错误
    }

    // 子进程结束检查（非阻塞）
    const pid_t childPid = static_cast<pid_t>(m_childPid);
    int status = 0;
    pid_t r = ::waitpid(childPid, &status, WNOHANG);
    if (r == childPid && !m_finishedEmitted) {
        m_running = false;
        m_finishedEmitted = true;
        if (m_notifier) m_notifier->setEnabled(false);
        closeMaster();

        int exitCode = -1;
        PtyExitStatus es = PtyExitStatus::NormalExit;
        if (WIFEXITED(status)) {
            exitCode = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            exitCode = 128 + WTERMSIG(status);
            es = PtyExitStatus::CrashExit;
        }
        emit finished(exitCode, es);
    }
}

// ============================================================================
// write() —— 用户键盘输入 → 子进程 stdin
// ============================================================================
void UnixPtyProcess::write(const QByteArray& data)
{
    if (m_masterFd < 0) return;

    const char* p = data.constData();
    qsizetype left = data.size();
    while (left > 0) {
        const ssize_t n = ::write(m_masterFd, p, static_cast<size_t>(left));
        if (n > 0) { p += n; left -= n; continue; }
        if (errno == EINTR) continue;
        if (errno == EAGAIN) break;   // 缓冲满，丢弃剩余
        break;
    }
}

// ============================================================================
// resize() —— 窗口尺寸变化（vim / top / htop 依赖）
// ============================================================================
void UnixPtyProcess::resize(const PtySize& size)
{
    if (m_masterFd < 0) return;

    struct winsize ws{};
    ws.ws_row = static_cast<unsigned short>(size.rows);
    ws.ws_col = static_cast<unsigned short>(size.cols);
    ::ioctl(m_masterFd, TIOCSWINSZ, &ws);

    // 通知前台进程组尺寸变了（vim / htop 会重绘）
    if (m_childPid > 0) {
        ::kill(static_cast<pid_t>(m_childPid), SIGWINCH);
    }
}

// ============================================================================
// 信号发送
// ============================================================================
void UnixPtyProcess::sendSignal(int sig)
{
    if (m_childPid > 0) {
        ::kill(static_cast<pid_t>(m_childPid), sig);
    }
}

void UnixPtyProcess::sendInterrupt()
{
    // 注意：对 ssh 场景，写 0x03 更准确（会在远端触发 Ctrl+C），
    //      这里的 SIGINT 只中断本地前台进程，TerminalPanel 里
    //      Ctrl+C 应该走 write("\x03") 而不是 sendInterrupt()。
    sendSignal(SIGINT);
}

void UnixPtyProcess::sendTerminate()
{
    sendSignal(SIGTERM);
}

void UnixPtyProcess::sendKill()
{
    sendSignal(SIGKILL);
}

// ============================================================================
// 状态查询
// ============================================================================
bool UnixPtyProcess::isRunning() const
{
    return m_running;
}

qint64 UnixPtyProcess::pid() const
{
    return m_childPid;
}

// ============================================================================
// closeMaster() —— 幂等关闭 master fd 和 notifier
// ============================================================================
void UnixPtyProcess::closeMaster()
{
    if (m_masterFd >= 0) {
        ::close(m_masterFd);
        m_masterFd = -1;
    }
    if (m_notifier) {
        m_notifier->deleteLater();
        m_notifier = nullptr;
    }
}

} // namespace SprintIDE

#endif // !Q_OS_WIN

// ============================================================================
// AutoMoc 兜底：
//   unix_pty_process.h 里的 Q_OBJECT 类通过这个 include 生成 moc 代码。
//   必须放在 namespace 外、#endif 外。
//   unix_pty_process.moc 是 AutoMoc 运行时生成到 build 目录的虚拟文件，
//   不是仓库里的真实文件。
// ============================================================================
#include "unix_pty_process.moc"
