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
#ifndef PYTHON_DEBUGGER_H
#define PYTHON_DEBUGGER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include "debug_manager.h"  // 复用 DebugVariable, StackFrame

QT_BEGIN_NAMESPACE
class QTcpSocket;
QT_END_NAMESPACE

namespace SprintIDE {

class PythonDebugger : public QObject {
    Q_OBJECT
public:
    explicit PythonDebugger(QObject* parent = nullptr);
    ~PythonDebugger();

    bool startDebugging(const DebugConfig& config);
    void stopDebugging();

    void setBreakpoints(const QList<Breakpoint>& breakpoints);
    void continueExecution();
    void stepOver();
    void stepInto();
    void stepOut();
    void pause();

signals:
    void stopped(const QString& file, int line, const QString& reason);
    void continued();
    void exited(int exitCode);
    void terminated();
    void variablesUpdated(const QList<DebugVariable>& variables);
    void stackUpdated(const QList<StackFrame>& frames);
    void outputReceived(const QString& output, bool isError);
    void errorOccurred(const QString& error);

private:
    class Private;
    QScopedPointer<Private> d;
    DebugConfig config;   // 存储启动配置
};

} // namespace SprintIDE

#endif // PYTHON_DEBUGGER_H
