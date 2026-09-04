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
    void variablesUpdated(const QList<DebugVariable>& variables);
    void stackUpdated(const QList<StackFrame>& frames);
    void outputReceived(const QString& output, bool isError);
    void errorOccurred(const QString& error);

private:
    class Private;
    QScopedPointer<Private> d;
};

} // namespace SprintIDE

#endif // PYTHON_DEBUGGER_H
