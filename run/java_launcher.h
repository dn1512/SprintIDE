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

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>
#include "run_configuration.h"

namespace SprintIDE {

class JavaLauncher : public QObject
{
    Q_OBJECT
public:
    explicit JavaLauncher(QObject* parent = nullptr);
    ~JavaLauncher() = default;

    /**
     * @brief 运行 Java 程序
     * @param config 运行配置
     * @param classpath 类路径列表
     * @param workDir 工作目录
     * @return 成功返回 true
     */
    bool run(const RunConfiguration& config,
             const QStringList& classpath,
             const QString& workDir = QString());

    /**
     * @brief 停止当前运行的程序
     */
    void stop();

    /**
     * @brief 是否正在运行
     */
    bool isRunning() const;

signals:
    /**
     * @brief 程序输出信号
     * @param output 输出内容
     * @param isError 是否为错误输出
     */
    void outputReceived(const QString& output, bool isError);

    /**
     * @brief 程序结束信号
     * @param exitCode 退出码
     */
    void programFinished(int exitCode);

    /**
     * @brief 程序启动信号
     */
    void programStarted();

private slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);

private:
    QString buildCommandLine(const RunConfiguration& config,
                             const QStringList& classpath) const;
    QString findJavaExecutable() const;

    QProcess* m_process = nullptr;
    bool m_isRunning = false;
};

} // namespace SprintIDE
