// terminal_panel.h
#ifndef TERMINAL_PANEL_H
#define TERMINAL_PANEL_H

#include <QWidget>
#include <QProcess>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProcessEnvironment>

namespace SprintIDE {

class TerminalPanel : public QWidget
{
    Q_OBJECT

public:
    explicit TerminalPanel(QWidget* parent = nullptr);
    ~TerminalPanel();

    void setWorkingDirectory(const QString& dir);
    QString workingDirectory() const;

    void executeCommand(const QString& command);
    bool isCommandRunning() const;

    void clearOutput();
    void stopCurrentCommand();
    void focusPanel();

signals:
    void commandStarted(const QString& command);
    void commandFinished(const QString& command, int exitCode);
    void outputReceived(const QString& output, bool isError);
    void workingDirectoryChanged(const QString& dir);

protected:
    // 拦截输入区键盘/鼠标事件，实现"伪终端"行为
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onProcessOutput();
    void onProcessError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessErrorOccurred(QProcess::ProcessError error);
    void recolorAllOutput();

private:
    void setupUI();

    // ---- 终端输出 API ----
    // 追加进程原始输出（原样，不加换行）
    void appendRaw(const QString& text);
    // 追加系统提示行（自动前后补换行 + 特定颜色）
    void appendSystemLine(const QString& text, bool isError = false);
    // 在末尾打印新提示符，并记录输入起点
    void printPrompt();

    // ---- 命令输入 ----
    // 提示符字符串（含虚拟环境前缀）
    QString promptString() const;
    // 用户按 Enter
    void onCommandEntered();
    // 上下键调历史（direction: -1=上 / +1=下）
    void navigateHistory(int direction);

    void updateVenvForDirectory(const QString& dir);

    // 构造终端子进程环境（含 SDK 变量 + 虚拟环境）
    QProcessEnvironment buildChildEnvironment() const;

    // ---- UI ----
    QTextEdit*   m_outputWidget = nullptr;
    QPushButton* m_stopButton   = nullptr;

    // ---- 运行状态 ----
    QProcess* m_process = nullptr;
    QString   m_currentCommand;
    QString   m_workingDirectory;
    bool      m_isCommandRunning = false;

    // ---- 输入区起点 ----
    // 提示符之后的位置，用户编辑被限制在此之后
    int m_promptStart = -1;

    // ---- 历史 ----
    QStringList m_commandHistory;
    int         m_historyIndex = 0;

    // ---- 虚拟环境 ----
    QString m_currentVenvPath;
    QString m_currentPythonInterpreter;
};

} // namespace SprintIDE

#endif // TERMINAL_PANEL_H