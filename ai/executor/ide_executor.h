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
#ifndef IDE_EXECUTOR_H
#define IDE_EXECUTOR_H

#include <QObject>
#include <QString>
#include <QTimer>

#include "ai/executor/i_executor.h"

namespace SprintIDE {

class MainWindow;

namespace Protocol {

class IdeExecutor : public QObject, public IExecutor
{
    Q_OBJECT
public:
    explicit IdeExecutor(MainWindow* mw, QObject* parent = nullptr);

    QString domain() const override { return QStringLiteral("ide"); }
    ExecutionResult execute(const QJsonObject& item) override;

signals:
    void deferredResultReady(const QString& itemId,
                             const SprintIDE::Protocol::ExecutionResult& result);

private slots:
    void onBuildFinished(bool success, int exitCode,
                         const QString& log, int durationMs);
    void onRunFinished(int exitCode, const QString& log);
    void onRunStopped(int exitCode, const QString& log);

private:
    ExecutionResult handleBuild(const QJsonObject& item);
    ExecutionResult handleRun(const QJsonObject& item);
    ExecutionResult handleStopRun(const QJsonObject& item);

    MainWindow* m_mw = nullptr;
    QString     m_pendingBuildItemId;
    QString     m_pendingRunItemId;
    QString     m_pendingStopItemId;
};

} // namespace Protocol
} // namespace SprintIDE

#endif // IDE_EXECUTOR_H
