// SPDX-License-Identifier: GPL-3.0-or-later
/*
* SprintIDE - SprintIDE is a cross-platform IDE built from scratch with Qt 6 and C++20. Its core capability is to provide AI Agents with an executable, verifiable code execution environment.
* Copyright (C) 2026  Liu Tengjun (刘腾骏)
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#pragma once

#include <QWidget>
#include <QProcessEnvironment>
#include <QString>
#include <QStringList>

#include <memory>

class QStringDecoder;
class QPushButton;
class QTimer;

namespace SprintIDE {

class IPtyProcess;
class TerminalGridModel;
class TerminalView;
class TerminalSearchBar;
class TerminalOutputOverlay;

class TerminalPanel : public QWidget
{
    Q_OBJECT
public:
    explicit TerminalPanel(QWidget* parent = nullptr);
    ~TerminalPanel() override;

    void    setWorkingDirectory(const QString& dir);
    QString workingDirectory() const;

    void executeCommand(const QString& command);

    // 直接写原始字节到 PTY，不自动加回车
    // 用途：外部模块（AI、编辑器"发送选中"）注入文本，用户自己按回车
    void writeRaw(const QByteArray& data);

    bool isCommandRunning() const;

    void clearOutput();
    void focusPanel();
    void stopCurrentCommand();

    void restartShell();

    // 显示 / 隐藏搜索栏（Ctrl+F 自动调用；也可由外部 panel 调用）
    void showSearchBar();

signals:
    void commandStarted(const QString& command);
    void commandFinished(const QString& command, int exitCode);
    void outputReceived(const QString& output, bool isError);
    void workingDirectoryChanged(const QString& dir);

    // 用户在终端点击了 path:line:col 超链接
    // 由 MainWindow 处理成跳转到编辑器
    void openFileAtLine(const QString& path, int line, int column);

    // shell 通过 OSC 0/2 设置了标题（供 TabManager 更新 tab 文本）
    void shellTitleChanged(const QString& title);

    // shell 运行状态变化（供 TerminalTabManager 更新停止按钮 enable）
    void shellRunningChanged(bool running);

protected:
    void showEvent(QShowEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;

private slots:
    void onPtyReadyRead(const QByteArray& data);
    void onPtyFinished(int exitCode, int exitStatus);
    void onPtyError(const QString& message);
    void onViewInput(const QByteArray& bytes);

    // 用户回车提交了一条命令 → 记录到历史
    void onCommandSubmitted(const QString& command);

    void onSearchRequested();
    void onSearchChanged(const QString& query, bool caseSensitive);
    void onSearchStateChanged(int current, int total);
    void onSearchClose();

    void applyThemeColors();

    // 输出结构化
    void onCommandOutputIdle();                              // 命令输出静默 500ms
    void onStructuredAction(const QString& action);          // 用户点击结构化项

private:
    void setupUI();
    void startShell();
    void stopShell();

    // 从 ConfigManager 读 Editor/FontFamily + Editor/FontSize
    // 应用到 m_view，然后 syncPtySize()
    void applyEditorFontFromConfig();

    QString            shellProgram() const;
    QStringList        shellArgs()    const;
    QProcessEnvironment buildChildEnvironment() const;
    void               updateVenvForDirectory(const QString& dir);
    void               syncPtySize();

    // 把累积的 PTY 数据一次性 feed 给 model
    void flushPtyData();

    // SSH 断线检测与重连
    bool    isSshDisconnectOutput(const QString& recentOutput) const;
    void    promptSshReconnect(const QString& sshCommand);

    void parseLinkAndEmit(const QString& link);

    // ---- UI ----
    TerminalGridModel* m_model      = nullptr;
    TerminalView*      m_view       = nullptr;
    TerminalSearchBar* m_searchBar  = nullptr;

    // 输出结构化
    TerminalOutputOverlay* m_outputOverlay     = nullptr;
    QString                m_pendingCommand;
    QTimer*                m_commandIdleTimer  = nullptr;

    // 大输出量节流
    QByteArray m_pendingPtyData;
    QTimer*    m_feedTimer      = nullptr;

    // 命令耗时追踪
    qint64 m_pendingCommandStartMs = 0;

    // 命令提交时的项目根（从 cwd 向上找）
    QString m_pendingCommandProjectPath;

    // 白名单命令的原始输出累积（含 ANSI，解析前剥离）
    QString m_commandOutputBuffer;

    // SSH 断线检测
    QString m_lastSshCommand;
    QString m_recentOutputBuffer;
    bool    m_sshReconnectDialogOpen = false;

    // ---- PTY ----
    IPtyProcess* m_pty = nullptr;

    // ---- UTF-8 流式解码 ----
    std::unique_ptr<QStringDecoder> m_decoder;

    // ---- 状态 ----
    QString m_workingDirectory;
    QString m_currentVenvPath;
    QString m_currentPythonInterpreter;

    bool m_shellRunning      = false;
    bool m_shellStartPending = false;
    bool m_shellInitialized  = false;
};

} // namespace SprintIDE