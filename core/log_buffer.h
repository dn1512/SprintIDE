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
#ifndef LOG_BUFFER_H
#define LOG_BUFFER_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QMutex>

namespace SprintIDE {

// ============================================================
// 日志分类（按"来源子系统"）
// ============================================================
enum class LogKind {
    Compile,    // 编译/构建
    Run,        // 运行程序
    Debug,      // 调试会话
    Terminal,   // 终端命令
    System,     // IDE 自身（打开文件、切主题等）
    UI,         // 【预留】UI 感知：截图、widget 树、事件流
    Unknown
};

// ============================================================
// 日志级别（按"严重程度"）
// ============================================================
enum class LogLevel {
    Info,
    Warning,
    Error
};

// ============================================================
// 单条日志
// ============================================================
struct LogEntry {
    qint64      timestamp  = 0;      // ms since epoch
    LogKind     kind       = LogKind::System;
    LogLevel    level      = LogLevel::Info;
    QString     source;              // "gcc" / "MainWindow" / "debugpy" ...
    QString     message;             // 纯文本（不带 HTML）
    QString     filePath;            // 关联文件（可选）
    int         lineNumber = -1;     // 关联行号（可选）
    int column     = -1;
    QStringList attachments;         // 附件相对路径（阶段1空跑，UI 感知时启用）

    LogEntry() = default;

    static LogEntry make(LogKind kind, LogLevel level,
                         const QString& source, const QString& message,
                         const QString& filePath = QString(),
                         int lineNumber = -1,
                         int column = -1);
};

// ============================================================
// 环形日志缓冲（线程安全）
// ============================================================
class LogBuffer
{
public:
    explicit LogBuffer(int capacity = 2000);

    // ---- 写入 ----
    void append(const LogEntry& entry);
    void append(LogKind kind, LogLevel level,
                const QString& source, const QString& message,
                const QString& filePath = QString(), int lineNumber = -1);

    /// 给"最近一条 timestamp 匹配"的日志挂附件（阶段1可不用）
    void attach(qint64 timestamp, const QString& relPath);

    void clear();

    // ---- 容量 ----
    int  size() const;
    int  capacity() const;
    void setCapacity(int capacity);

    // ---- 查询 ----
    QList<LogEntry> recent(int n = 50) const;
    QList<LogEntry> since(qint64 timestamp) const;
    QList<LogEntry> byKind(LogKind kind, int n = 50) const;
    QList<LogEntry> onlyErrors(int n = 50) const;

    // ---- 序列化辅助 ----
    static QString kindToString(LogKind kind);
    static QString levelToString(LogLevel level);
    static LogKind stringToKind(const QString& s);       // 供 RPC 解析用
    static LogLevel stringToLevel(const QString& s);

    /// 给 AI 看的纯文本（每条一行）
    static QString toText(const QList<LogEntry>& entries);

private:
    mutable QMutex   m_mutex;
    QList<LogEntry>  m_entries;
    int              m_capacity;
};

} // namespace SprintIDE

#endif // LOG_BUFFER_H
