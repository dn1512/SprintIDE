// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QByteArray>
#include <QObject>
#include <QProcessEnvironment>
#include <QString>
#include <QStringList>

namespace SprintIDE {

    struct PtySize {
        int rows = 24;
        int cols = 80;
    };

    enum class PtyExitStatus {
        NormalExit,
        CrashExit
    };

    class IPtyProcess : public QObject
    {
        Q_OBJECT
        public:
        explicit IPtyProcess(QObject* parent = nullptr) : QObject(parent) {}
        ~IPtyProcess() override = default;

        virtual bool start(const QString& program,
            const QStringList& args,
            const QString& workingDir,
            const QProcessEnvironment& env) = 0;

        virtual void write(const QByteArray& data) = 0;
        virtual void resize(const PtySize& size) = 0;

        virtual void sendInterrupt() = 0;   // Ctrl+C
        virtual void sendTerminate() = 0;   // SIGTERM / TerminateProcess
        virtual void sendKill()      = 0;   // SIGKILL / 强杀

        virtual bool   isRunning() const = 0;
        virtual qint64 pid()       const = 0;

        signals:
        void readyRead(const QByteArray& data);
        void finished(int exitCode, PtyExitStatus status);
        void errorOccurred(const QString& message);
    };

} // namespace SprintIDE