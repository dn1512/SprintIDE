// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QtGlobal>

#ifdef Q_OS_WIN

// ---------------------------------------------------------------------------
// Windows 头包含顺序很重要：
//   - WIN32_LEAN_AND_MEAN 避免拉进 winsock.h 等一大堆东西
//   - NOMINMAX 防止 windows.h 定义 min/max 宏（Qt 的 std::min 会炸）
//   - windows.h 必须在 Qt 头之前 include（避免 qt_windows.h 的包含检查）
// ---------------------------------------------------------------------------
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>

#include <QObject>
#include <QThread>
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QProcessEnvironment>

#include <atomic>

#include "pty_types.h"

namespace SprintIDE {

    // ---------------------------------------------------------------------------
    // HPCON 是 Windows 10 1809 (build 17763) + SDK 10.0.17763 才有的类型
    // 老 SDK 没有声明，但本质就是 HANDLE；这里做兼容 typedef
    // ---------------------------------------------------------------------------
    #ifndef HPCON
    using HPCON = HANDLE;
    #endif

    // ===========================================================================
    // ConPtyApi —— CreatePseudoConsole / ClosePseudoConsole / ResizePseudoConsole
    //               的动态加载表
    //
    // 为什么动态加载：
    //   1. 老 SDK（< 10.0.17763）头文件里没有这三个函数的声明，
    //      直接链接会报 LNK2019，且老系统上进程启动就崩。
    //   2. 老系统（Win7/8/Server 2016）kernel32.dll 里没有这三个符号，
    //      GetProcAddress 返回 nullptr → available = false → 上层优雅降级。
    //   3. 不需要在 CMake 里链接额外库，纯运行时探测。
    // ===========================================================================
    struct ConPtyApi
    {
        // ---- 函数指针类型 ----
        using CreatePseudoConsoleFn = HRESULT (WINAPI*)(COORD size,
            HANDLE hInput,
            HANDLE hOutput,
            DWORD dwFlags,
            HPCON* phPC);
        using ClosePseudoConsoleFn  = void    (WINAPI*)(HPCON hPC);
        using ResizePseudoConsoleFn = HRESULT (WINAPI*)(HPCON hPC,
            COORD size);

        // ---- 已解析的函数指针（available == false 时可能为 nullptr） ----
        CreatePseudoConsoleFn  CreatePseudoConsole  = nullptr;
        ClosePseudoConsoleFn   ClosePseudoConsole   = nullptr;
        ResizePseudoConsoleFn  ResizePseudoConsole  = nullptr;

        // ---- 三个符号都解析成功、且 OS build >= 17763 时为 true ----
        bool available = false;

        // 进程内单例；首次调用时完成 GetProcAddress + RtlGetVersion 探测
        static ConPtyApi& instance();
    };

    // ===========================================================================
    // WinConPtyProcess —— Windows 10 1809+ 的伪终端进程
    //
    //   PTY 由 CreatePseudoConsole 创建；
    //   输入通过 m_hInputWrite 写入，输出通过后台线程从 m_hOutputRead 读出；
    //   子进程通过 STARTUPINFOEX + PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE 挂到 PTY 上。
    //
    //   本类不依赖 QProcess —— QProcess 的管道不支持 PTY，
    //   这是 ssh / vim / htop 在 Windows 上无法交互的根本原因。
    // ===========================================================================
    class WinConPtyProcess final : public IPtyProcess
    {
        Q_OBJECT

        public:
        explicit WinConPtyProcess(QObject* parent = nullptr);
        ~WinConPtyProcess() override;

        // ---- IPtyProcess 接口实现 ----
        bool start(const QString& program,
            const QStringList& args,
            const QString& workingDir,
            const QProcessEnvironment& env) override;

        void write(const QByteArray& data) override;

        void resize(const PtySize& size) override;

        void sendInterrupt() override;   // 写 0x03 到 PTY（远端 ssh / cmd 都认）
        void sendTerminate() override;   // GenerateConsoleCtrlEvent + 超时兜底
        void sendKill()      override;   // TerminateProcess（最后手段）

        bool   isRunning() const override;
        qint64 pid()       const override;

        private:
        // 关闭所有句柄 / ConPTY / 子进程句柄，幂等
        void closeAll();

        // ---- ConPTY 与管道 ----
        HPCON  m_hPC         = nullptr;   // 伪终端句柄
        HANDLE m_hInputWrite = nullptr;   // 我们写 → 子进程读
        HANDLE m_hOutputRead = nullptr;   // 子进程写 → 我们读

        // ---- 子进程 ----
        PROCESS_INFORMATION m_pi{};       // hProcess / hThread / dwProcessId 等

        // ---- 输出读取线程 ----
        QThread*           m_readerThread = nullptr;
        std::atomic<bool>  m_running{false};     // start 成功 → true；finished 后 → false
        std::atomic<bool>  m_stopReader{false};  // 析构时通知线程退出
    };

} // namespace SprintIDE

#endif // Q_OS_WIN