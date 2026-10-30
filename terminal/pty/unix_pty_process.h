// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QtGlobal>

// 只在 Unix 平台编译（Windows 走 WinConPtyProcess）
#ifndef Q_OS_WIN

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QProcessEnvironment>

#include "pty_types.h"

class QSocketNotifier;

namespace SprintIDE {

    // ===========================================================================
    // UnixPtyProcess —— macOS / Linux 的伪终端进程
    //
    //   基于 forkpty()：
    //     - 父进程拿到 master fd，读写它等价于读写终端
    //     - 子进程被自动放到新会话 + 新 PTY 的 slave 端，
    //       stdin/stdout/stderr 都接在 tty 上
    //   这正是 ssh / vim / top / htop 这些"必须要有 tty 才能正常工作"的程序
    //   所需要的环境。
    //
    //   输出通过 QSocketNotifier 监听 master fd 可读事件，
    //   在 Qt 事件循环内回调 onReadable()，不会阻塞 UI。
    // ===========================================================================
    class UnixPtyProcess final : public IPtyProcess
    {
        Q_OBJECT

        public:
        explicit UnixPtyProcess(QObject* parent = nullptr);
        ~UnixPtyProcess() override;

        // ---- IPtyProcess 接口实现 ----
        bool start(const QString& program,
            const QStringList& args,
            const QString& workingDir,
            const QProcessEnvironment& env) override;

        void write(const QByteArray& data) override;

        void resize(const PtySize& size) override;

        void sendInterrupt() override;   // SIGINT
        void sendTerminate() override;   // SIGTERM
        void sendKill()      override;   // SIGKILL

        bool   isRunning() const override;
        qint64 pid()       const override;

        private slots:
        // master fd 可读 读数据 + 检查子进程是否退出
        void onReadable();

        private:
        // 向子进程发信号（sig 取 <csignal> 里的常量）
        void sendSignal(int sig);

        // 关闭 master fd 和 notifier（幂等）
        void closeMaster();

        // ---- PTY ----
        int              m_masterFd  = -1;      // forkpty 返回的 master fd
        QSocketNotifier* m_notifier  = nullptr; // 监听 master fd 可读

        // ---- 子进程 ----
        // 用 qint64 存 pid，避免头文件包含 <sys/types.h>
        // .cpp 里用 static_cast<pid_t>() 转换
        qint64 m_childPid = -1;

        // ---- 状态 ----
        bool m_running         = false;   // start 成功且子进程未退出
        bool m_finishedEmitted = false;   // 防止 finished 信号重入
    };

} // namespace SprintIDE

#endif // !Q_OS_WIN