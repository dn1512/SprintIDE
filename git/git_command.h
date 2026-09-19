// src/git/git_command.h
#ifndef SPRINTIDE_GIT_COMMAND_H
#define SPRINTIDE_GIT_COMMAND_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

namespace SprintIDE {
namespace Git {

class GitCommand : public QObject
{
    Q_OBJECT
public:
    struct Result {
        int     exitCode = -1;
        QString stdOut;
        QString stdErr;
        bool ok() const { return exitCode == 0; }
    };

    explicit GitCommand(QObject* parent = nullptr);
    ~GitCommand() override;

    // ---------- 同步执行（仅用于极短命令：rev-parse / config / --version） ----------
    Result run(const QString& workingDir,
               const QStringList& args,
               int timeoutMs = 8000);

    // ---------- 异步执行（用于 status / commit / push / pull / fetch） ----------
    void runAsync(const QString& workingDir,
                  const QStringList& args,
                  const QString& tag = QString(),
                  int timeoutMs = 60000);

    void cancelAsync();
    bool isAsyncRunning() const;

    // ---------- 可执行文件解析 ----------
    static QString gitExecutable();
    static void    setGitExecutable(const QString& path);
    static bool    isGitAvailable();

signals:
    void finished(const QString& tag,
                  const SprintIDE::Git::GitCommand::Result& result);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);
    void onTimeout();

private:
    QProcessEnvironment buildEnvironment() const;

    QProcess* m_process         = nullptr;
    QString   m_currentTag;
    QTimer*   m_timer           = nullptr;
    bool      m_cancelled       = false;
};

} // namespace Git
} // namespace SprintIDE

Q_DECLARE_METATYPE(SprintIDE::Git::GitCommand::Result)

#endif // SPRINTIDE_GIT_COMMAND_H
