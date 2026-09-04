// terminal_panel.h
#ifndef TERMINAL_PANEL_H
#define TERMINAL_PANEL_H

#include <QDockWidget>
#include <QProcess>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>

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

signals:
    void commandStarted(const QString& command);
    void commandFinished(const QString& command, int exitCode);
    void outputReceived(const QString& output, bool isError);
    void workingDirectoryChanged(const QString& dir);

private slots:
    void onInputEntered();
    void onProcessOutput();
    void onProcessError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessErrorOccurred(QProcess::ProcessError error);
    void onClearClicked();

private:
    void setupUI();
    void appendOutput(const QString& text, bool isError = false);
    void updatePrompt();
    void updateVenvForDirectory(const QString& dir);  // 新增
    void updateVenvDisplay();                         // 新增

    QTextEdit* m_outputWidget;
    QLineEdit* m_inputWidget;
    QPushButton* m_clearButton;
    QPushButton* m_stopButton;
    QLabel* m_promptLabel;

    QProcess* m_process;
    QString m_currentCommand;
    QString m_workingDirectory;
    bool m_isCommandRunning;

    QStringList m_commandHistory;
    int m_historyIndex;

    // ===== 新增：虚拟环境相关 =====
    QString m_currentVenvPath;
    QString m_currentPythonInterpreter;
};

} // namespace SprintIDE

#endif // TERMINAL_PANEL_H
