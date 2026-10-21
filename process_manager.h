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
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QElapsedTimer>
#include <QMap>
#include <QProcessEnvironment>

namespace SprintIDE {

/**
 * @brief 进程管理器类
 *
 * 封装QProcess，提供带超时控制、状态监控和错误处理的进程管理功能
 * 专门用于执行编译、调试等外部命令，确保IDE的稳定性和响应性
 */
class ProcessManager : public QObject {
    Q_OBJECT

public:
    using OutputCallback = std::function<void(const QString& chunk, bool isError)>;

    /**
     * @brief 进程执行结果
     */
    struct ProcessResult {
        bool success;               ///< 是否执行成功
        int exitCode;               ///< 退出码
        QString standardOutput;     ///< 标准输出
        QString standardError;      ///< 标准错误输出
        qint64 executionTime;       ///< 执行时间（毫秒）
        QString errorMessage;       ///< 错误信息（如果有）
    };

    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit ProcessManager(QObject* parent = nullptr);
    ~ProcessManager();

    /**
     * @brief 同步执行命令
     *
     * 执行命令并等待完成，适合短时间运行的命令
     *
     * @param command 要执行的命令（可以是可执行文件路径）
     * @param arguments 命令行参数
     * @param workingDirectory 工作目录（为空则使用当前目录）
     * @param timeoutMs 超时时间（毫秒），0表示无超时
     * @param encoding 输出编码（默认UTF-8）
     * @return ProcessResult 执行结果
     */
    ProcessResult executeSync(const QString& command,
                              const QStringList& arguments = QStringList(),
                              const QString& workingDirectory = QString(),
                              int timeoutMs = 30000,
                              const QString& encoding = "UTF-8",
                              OutputCallback onOutput = nullptr);

    /**
     * @brief 异步执行命令
     *
     * 启动命令后立即返回，通过信号获取结果
     *
     * @param command 要执行的命令
     * @param arguments 命令行参数
     * @param workingDirectory 工作目录
     * @param processId 进程ID（用于标识不同的进程）
     * @return bool 启动成功返回true
     */
    bool executeAsync(const QString& command,
                      const QStringList& arguments = QStringList(),
                      const QString& workingDirectory = QString(),
                      const QString& processId = QString());

    /**
     * @brief 停止正在运行的进程
     * @param processId 进程ID，为空则停止所有进程
     * @return bool 停止成功返回true
     */
    bool stopProcess(const QString& processId = QString());

    /**
     * @brief 获取进程状态
     * @param processId 进程ID
     * @return QProcess::ProcessState 进程状态
     */
    QProcess::ProcessState processState(const QString& processId) const;

    /**
     * @brief 获取进程执行结果
     * @param processId 进程ID
     * @return ProcessResult 执行结果（如果进程还在运行，返回部分结果）
     */
    ProcessResult processResult(const QString& processId) const;

    /**
     * @brief 设置环境变量
     * @param env 环境变量键值对
     */
    void setEnvironment(const QProcessEnvironment& env);

    /**
     * @brief 清空所有进程历史
     */
    void clearHistory();

signals:
    /**
     * @brief 进程启动信号
     * @param processId 进程ID
     */
    void processStarted(const QString& processId);

    /**
     * @brief 进程完成信号
     * @param processId 进程ID
     * @param result 执行结果
     */
    void processFinished(const QString& processId, const ProcessResult& result);

    /**
     * @brief 进程错误信号
     * @param processId 进程ID
     * @param errorMessage 错误信息
     */
    void processError(const QString& processId, const QString& errorMessage);

    /**
     * @brief 输出数据信号
     * @param processId 进程ID
     * @param data 输出数据
     * @param isError 是否为错误输出
     */
    void outputReceived(const QString& processId, const QString& data, bool isError);

    /**
     * @brief 进度更新信号
     * @param processId 进程ID
     * @param elapsedTime 已用时间（毫秒）
     */
    void progressUpdated(const QString& processId, qint64 elapsedTime);

private slots:
    void onProcessReadyRead();
    void onProcessReadyReadError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onTimeout();

private:
    /**
     * @brief 清理僵尸进程
     */
    void cleanupZombieProcesses();

    /**
     * @brief 验证命令是否可用
     * @param command 命令或可执行文件路径
     * @return bool 可用返回true
     */
    bool validateCommand(const QString& command) const;

    /**
     * @brief 创建进程ID
     * @param command 命令
     * @param arguments 参数
     * @return QString 唯一的进程ID
     */
    QString createProcessId(const QString& command, const QStringList& arguments) const;

    struct ProcessData {
        QProcess* process;
        QTimer* timeoutTimer;
        QElapsedTimer elapsedTimer;
        ProcessResult result;
        QString standardOutput;
        QString standardError;
        bool isFinished;
    };

    QMap<QString, ProcessData> m_processes; ///< 进程映射表
    QProcessEnvironment m_environment;      ///< 环境变量
    int m_nextProcessId;                    ///< 下一个进程ID编号

    void cleanupAll();
};

} // namespace SprintIDE
