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
#ifndef TERMINAL_PANEL_H
#define TERMINAL_PANEL_H

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProcessEnvironment>
#include <QVector>
#include <QPair>
#include <QTextCharFormat>
#include <QPointer>

namespace SprintIDE {

    class IPtyProcess;   // 前置声明，避免头文件依赖 pty_types.h

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
        bool eventFilter(QObject* obj, QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;

        private slots:
        void onPtyReadyRead(const QByteArray& data);
        void onPtyFinished(int exitCode, int exitStatus);
        void onPtyError(const QString& message);
        void recolorAllOutput();

        private:
        void setupUI();

        // ---- 输出 API ----
        void appendSystemLine(const QString& text, bool isError = false);
        void printPrompt();

        // ---- 命令输入 ----
        QString promptString() const;
        void onCommandEntered();
        void navigateHistory(int direction);

        void updateVenvForDirectory(const QString& dir);
        QProcessEnvironment buildChildEnvironment() const;

        // 根据 QTextEdit 尺寸算 rows/cols m_pty->resize()
        void syncPtySize();

        // ---- ANSI 处理 ----
        void feedAnsiBytes(const QByteArray& data);
        void renderCurrentLine();           // 把当前行实时画到文档末尾
        void flushCurrentLine();
        void clearCurrentLineBuf();

        // 当前行缓冲：每个字符 + 它的颜色格式
        QVector<QPair<QChar, QTextCharFormat>> m_lineBuf;
        int              m_cursorCol = 0;       // 当前行光标列
        QTextCharFormat  m_currentFmt;          // 当前 SGR 状态
        int              m_currentLineDocStart = -1;  // 当前行在 QTextDocument 中的起点

        // CSI 状态机
        enum class EscapeState { Normal, Escape, Csi, Osc };
        EscapeState m_escapeState = EscapeState::Normal;
        QString     m_escapeParams;             // CSI 参数字符串（不含 '[' 和终结字符）

        void processAnsiChar(QChar ch);
        void handleCsi(QChar finalChar);
        void handleSgr(const QStringList& params);

        // ---- UI ----
        QTextEdit*   m_outputWidget = nullptr;
        QPushButton* m_stopButton   = nullptr;

        // ---- 进程 ----
        IPtyProcess* m_pty = nullptr;
        QString      m_currentCommand;
        QString      m_workingDirectory;
        bool         m_isCommandRunning = false;

        // ---- 空闲态输入起点 ----
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