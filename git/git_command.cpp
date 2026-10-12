#include "git_command.h"

#include <QProcess>
#include <QProcessEnvironment>
#include <QTimer>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QMetaType>

namespace SprintIDE {
namespace Git {

// 文件作用域静态：用户可能在 Settings 里手动指定 git 路径
static QString g_gitExecutable;

// ============================================================
// 构造 / 析构
// ============================================================
GitCommand::GitCommand(QObject* parent)
    : QObject(parent)
{
    static bool metatypeRegistered = false;
    if (!metatypeRegistered) {
        qRegisterMetaType<SprintIDE::Git::GitCommand::Result>(
            "SprintIDE::Git::GitCommand::Result");
        metatypeRegistered = true;
    }
}

GitCommand::~GitCommand()
{
    // 析构时不调用 cancelAsync()：
    //   1. 对象正在析构，emit finished 会引发接收者访问已失效对象
    //   2. m_process / m_timer 是 this 的子对象，~QObject 会删除它们，
    //      手动 deleteLater 反而有双重删除风险
    // 只做：断开所有信号 + 同步 kill 掉正在运行的进程

    // 断开所有向外发的信号（防止 finished 在析构过程中被触发）
    disconnect(this, nullptr, nullptr, nullptr);

    if (m_process) {
        // 先断开 QProcess 到 this 的连接，防止 kill 时同步回调 onProcessFinished
        m_process->disconnect(this);
        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
            m_process->waitForFinished(1000);
        }
        // 不手动 delete：m_process 的 parent 是 this，~QObject 会作为析构的一部分删除它
    }
    if (m_timer) {
        m_timer->stop();
        m_timer->disconnect(this);
        // 同上，由 ~QObject 自动删除
    }
}

// ============================================================
// 可执行文件解析
// ============================================================
QString GitCommand::gitExecutable()
{
    if (!g_gitExecutable.isEmpty() && QFileInfo::exists(g_gitExecutable)) {
        return g_gitExecutable;
    }

    // 1) PATH
    const QString inPath = QStandardPaths::findExecutable("git");
    if (!inPath.isEmpty()) return inPath;

    // 2) 常见安装路径
    QStringList candidates;
#ifdef Q_OS_MAC
    candidates << "/opt/homebrew/bin/git"
               << "/usr/local/bin/git"
               << "/usr/bin/git";
#endif
#ifdef Q_OS_WIN
    candidates << "C:/Program Files/Git/bin/git.exe"
               << "C:/Program Files (x86)/Git/bin/git.exe"
               << "C:/Git/bin/git.exe";
#endif
#ifdef Q_OS_LINUX
    candidates << "/usr/bin/git"
               << "/usr/local/bin/git"
               << "/snap/bin/git";
#endif

    for (const QString& c : candidates) {
        if (QFileInfo::exists(c)) return c;
    }
    return QString();
}

void GitCommand::setGitExecutable(const QString& path)
{
    g_gitExecutable = path;
}

bool GitCommand::isGitAvailable()
{
    return !gitExecutable().isEmpty();
}

QProcessEnvironment GitCommand::buildEnvironment() const
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    // 关键：禁止 git 弹交互提示，否则 QProcess 会挂住（credential / passphrase）
    env.insert("GIT_TERMINAL_PROMPT", "0");
    // 禁止分页器（否则 stdout 永远读不完）
    env.insert("GIT_PAGER", "cat");
    // 输出统一英文，方便解析（用户系统若是中文，git status 会输出中文错误）
    env.insert("LC_ALL", "C");
    env.insert("LANG",   "C");
    return env;
}

// ============================================================
// 同步执行
// ============================================================
GitCommand::Result GitCommand::run(const QString& workingDir,
                                   const QStringList& args,
                                   int timeoutMs)
{
    Result r;
    const QString git = gitExecutable();
    if (git.isEmpty()) {
        r.stdErr = QStringLiteral("git executable not found in PATH");
        return r;
    }

    QProcess proc;
    proc.setProgram(git);
    proc.setArguments(args);
    proc.setWorkingDirectory(workingDir);
    proc.setProcessEnvironment(buildEnvironment());
    proc.setProcessChannelMode(QProcess::SeparateChannels);

    proc.start();
    if (!proc.waitForStarted(3000)) {
        r.stdErr = QStringLiteral("failed to start git: ") + proc.errorString();
        return r;
    }
    if (!proc.waitForFinished(timeoutMs)) {
        proc.kill();
        proc.waitForFinished(1000);
        r.stdErr = QStringLiteral("git command timed out after %1ms").arg(timeoutMs);
        return r;
    }

    r.exitCode = proc.exitCode();
    r.stdOut   = QString::fromUtf8(proc.readAllStandardOutput());
    r.stdErr   = QString::fromUtf8(proc.readAllStandardError());
    return r;
}

// ============================================================
// 异步执行
// ============================================================
void GitCommand::runAsync(const QString& workingDir,
                          const QStringList& args,
                          const QString& tag,
                          int timeoutMs)
{
    // 幂等：已有任务先杀掉
    cancelAsync();

    const QString git = gitExecutable();
    if (git.isEmpty()) {
        Result r;
        r.stdErr = QStringLiteral("git executable not found in PATH");
        // 延后发出，保证调用方 connect 已经建立
        QTimer::singleShot(0, this, [this, tag, r]() {
            emit finished(tag, r);
        });
        return;
    }

    m_process = new QProcess(this);
    m_process->setProgram(git);
    m_process->setArguments(args);
    m_process->setWorkingDirectory(workingDir);
    m_process->setProcessEnvironment(buildEnvironment());
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    m_currentTag = tag;
    m_cancelled  = false;

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &GitCommand::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &GitCommand::onProcessError);

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &GitCommand::onTimeout);
    m_timer->start(timeoutMs);

    m_process->start();
}

void GitCommand::cancelAsync()
{
    if (m_timer) {
        m_timer->stop();
        m_timer->deleteLater();
        m_timer = nullptr;
    }
    if (m_process) {
        m_cancelled = true;
        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
            // ⚠️ waitForFinished 会派发事件循环，
            //    QProcess::finished 同步触发 onProcessFinished，
            //    后者把 m_process 置为 nullptr。
            //    等待后必须重新检查 m_process，否则下一行 deleteLater 会 UAF。
            m_process->waitForFinished(1000);
        }
        // m_process 可能在 waitForFinished 里被 onProcessFinished 置空
        if (m_process) {
            m_process->deleteLater();
            m_process = nullptr;
        }
    }
}

bool GitCommand::isAsyncRunning() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

// ============================================================
// 槽
// ============================================================
void GitCommand::onTimeout()
{
    if (!m_process) return;
    m_cancelled = true;
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
    }
    // onProcessFinished 会被随后触发
}

void GitCommand::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    if (!m_process) return;

    Result r;
    if (m_cancelled) {
        r.exitCode = -1;
        r.stdErr   = QStringLiteral("cancelled");
    } else if (status == QProcess::CrashExit) {
        r.exitCode = exitCode;
        r.stdErr   = QStringLiteral("git process crashed");
    } else {
        r.exitCode = exitCode;
        r.stdOut   = QString::fromUtf8(m_process->readAllStandardOutput());
        r.stdErr   = QString::fromUtf8(m_process->readAllStandardError());
    }

    const QString tag = m_currentTag;

    if (m_timer) {
        m_timer->stop();
        m_timer->deleteLater();
        m_timer = nullptr;
    }
    m_process->deleteLater();
    m_process = nullptr;

    emit finished(tag, r);
}

void GitCommand::onProcessError(QProcess::ProcessError error)
{
    // 其他错误（crash / read / write）会通过 finished 发出，这里只处理 FailedToStart
    if (error != QProcess::FailedToStart) return;
    if (!m_process) return;

    Result r;
    r.exitCode = -1;
    r.stdErr   = QStringLiteral("failed to start git: ") + m_process->errorString();

    const QString tag = m_currentTag;

    if (m_timer) {
        m_timer->stop();
        m_timer->deleteLater();
        m_timer = nullptr;
    }
    m_process->deleteLater();
    m_process = nullptr;

    emit finished(tag, r);
}

} // namespace Git
} // namespace SprintIDE
