// SPDX-License-Identifier: GPL-3.0-or-later
#include "win_conpty_process.h"

#ifdef Q_OS_WIN

#include <QByteArray>
#include <QDebug>
#include <QMetaObject>
#include <QProcessEnvironment>
#include <QThread>
#include <QCoreApplication>

#include <winternl.h>   // RTL_OSVERSIONINFOW

#include <string>

// ---------------------------------------------------------------------------
// 老 SDK (<10.0.17763) 的头文件里没有 PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE
// 微软文档给的兜底宏，新 SDK 里已有定义，这里用 #ifndef 保护
// ---------------------------------------------------------------------------
#ifndef PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE
#  define PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE \
ProcThreadAttributeValue(22, FALSE, TRUE, FALSE)
#endif

namespace SprintIDE {

    // ============================================================================
    // ConPtyApi::instance()
    //   - 首次调用完成 GetProcAddress + RtlGetVersion 探测
    //   - 失败时 available = false，上层（PtyFactory / TerminalPanel）据此降级
    // ============================================================================
    ConPtyApi& ConPtyApi::instance()
    {
        static ConPtyApi api;
        static bool inited = false;
        if (inited) return api;
        inited = true;

        HMODULE k32 = ::GetModuleHandleW(L"kernel32.dll");
        if (!k32) return api;

        api.CreatePseudoConsole = reinterpret_cast<CreatePseudoConsoleFn>(
            ::GetProcAddress(k32, "CreatePseudoConsole"));
        api.ClosePseudoConsole = reinterpret_cast<ClosePseudoConsoleFn>(
            ::GetProcAddress(k32, "ClosePseudoConsole"));
        api.ResizePseudoConsole = reinterpret_cast<ResizePseudoConsoleFn>(
            ::GetProcAddress(k32, "ResizePseudoConsole"));

        api.available = api.CreatePseudoConsole
        && api.ClosePseudoConsole
        && api.ResizePseudoConsole;

        // OS build 检查：ConPTY 需要 Windows 10 1809 (build 17763) 及以上
        if (api.available) {
            using RtlGetVersionFn = LONG (WINAPI*)(PRTL_OSVERSIONINFOW);
            auto rtlGetVersion = reinterpret_cast<RtlGetVersionFn>(
                ::GetProcAddress(::GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion"));
            if (rtlGetVersion) {
                RTL_OSVERSIONINFOW vi{};
                vi.dwOSVersionInfoSize = sizeof(vi);
                if (rtlGetVersion(&vi) == 0 && vi.dwBuildNumber < 17763) {
                    api.available = false;
                }
            } else {
                // 拿不到版本号 → 保守起见禁用
                api.available = false;
            }
        }
        return api;
    }

    // ============================================================================
    // 内部辅助：Windows 命令行参数转义
    //
    //   CreateProcessW 的 lpCommandLine 解析规则（简化版）：
    //     - 空格 / 制表符 分隔参数
    //     - 双引号包裹含空格的参数
    //     - 反斜杠紧邻双引号时需要成对加倍（\\" → \\\"）
    //   这里只处理最常见的情况，够 ssh / cmd / powershell 用
    // ============================================================================
    namespace {

        QString quoteArg(const QString& arg)
        {
            if (!arg.contains(QLatin1Char(' '))
                && !arg.contains(QLatin1Char('\t'))
                && !arg.contains(QLatin1Char('"'))) {
                return arg;
            }

            QString escaped = arg;
            // 先把 \ 后面跟 " 的情况加倍反斜杠（简化处理）
            // 完整实现见 MSDN "Parsing C++ Command-Line Arguments"
            // 这里只做 " → \" 转义，足够 ssh -p 22 root@x 这类命令
            escaped.replace(QLatin1Char('"'), QLatin1String("\\\""));

            return QLatin1Char('"') + escaped + QLatin1Char('"');
        }

        QString buildCommandLine(const QString& program, const QStringList& args)
        {
            QString cmd = quoteArg(program);
            for (const QString& a : args) {
                cmd += QLatin1Char(' ');
                cmd += quoteArg(a);
            }
            return cmd;
        }

        // ============================================================================
        // 内部辅助：构造 Windows 环境块
        //
        //   CreateProcessW + CREATE_UNICODE_ENVIRONMENT 要求：
        //     - 环境块是 UTF-16 LE
        //     - 每个条目 "K=V\0"（L'\0' 是 2 字节）
        //     - 最后再跟一个 L'\0' 作为终止
        //   即整个块的最后 4 字节全 0
        // ============================================================================
        QByteArray buildEnvironmentBlock(const QProcessEnvironment& env)
        {
            QByteArray block;
            const QStringList pairs = env.toStringList();
            block.reserve(pairs.size() * 64);

            for (const QString& kv : pairs) {
                const std::wstring wkv = kv.toStdWString();
                // 追加 UTF-16 内容（含结尾的 \0，由 toStdWString 保证 wkv.size() 不含 \0，
                // 但 c_str() 之后是 \0 结尾，所以手动 append 两个 \0 字节 = L'\0'）
                block.append(reinterpret_cast<const char*>(wkv.c_str()),
                    int(wkv.size() * int(sizeof(wchar_t))));
                block.append('\0');
                block.append('\0');   // 组合成 L'\0'
            }

            // 终止：再加一个 L'\0'（两个字节全 0）
            if (block.isEmpty() || !block.endsWith(QByteArray(2, '\0'))) {
                block.append('\0');
                block.append('\0');
            }
            return block;
        }

    } // namespace

    // ============================================================================
    // WinConPtyProcess
    // ============================================================================
    WinConPtyProcess::WinConPtyProcess(QObject* parent)
    : IPtyProcess(parent)
    {
    }

    WinConPtyProcess::~WinConPtyProcess()
    {
        // 清掉排队事件 + 断开所有信号，防 UAF
        QObject::disconnect(this, nullptr, nullptr, nullptr);
        QCoreApplication::removePostedEvents(this);

        m_stopReader = true;

        if (m_hPC) {
            auto& api = ConPtyApi::instance();
            if (api.available && api.ClosePseudoConsole) {
                api.ClosePseudoConsole(m_hPC);
            }
            m_hPC = nullptr;
        }

        if (m_readerThread) {
            if (!m_readerThread->wait(3000)) {
                m_readerThread->terminate();
                m_readerThread->wait(1000);
            }
            delete m_readerThread;
            m_readerThread = nullptr;
        }

        closeAll();
    }

    // ============================================================================
    // start()
    // ============================================================================
    bool WinConPtyProcess::start(const QString& program,
        const QStringList& args,
        const QString& workingDir,
        const QProcessEnvironment& env)
    {
        auto& api = ConPtyApi::instance();
        if (!api.available) {
            emit errorOccurred(QStringLiteral(
                    "ConPTY 不可用（需要 Windows 10 1809 及以上）"));
            return false;
        }
        if (m_running) {
            emit errorOccurred(QStringLiteral("进程已在运行"));
            return false;
        }

        // ---------- 1. 创建两个管道（父子进程间通信） ----------
        //   inRead  : 子进程 stdin  →  ConPTY
        //   inWrite : 父进程写入
        //   outRead : 父进程读取  ←  ConPTY
        //   outWrite: ConPTY 写入 →  子进程 stdout/stderr
        HANDLE inRead = nullptr, inWrite = nullptr;
        HANDLE outRead = nullptr, outWrite = nullptr;

        SECURITY_ATTRIBUTES sa{};
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = nullptr;

        if (!::CreatePipe(&inRead, &inWrite, &sa, 0)) {
            emit errorOccurred(QStringLiteral("CreatePipe (input) failed: %1")
                .arg(::GetLastError()));
            return false;
        }
        if (!::CreatePipe(&outRead, &outWrite, &sa, 0)) {
            ::CloseHandle(inRead);
            ::CloseHandle(inWrite);
            emit errorOccurred(QStringLiteral("CreatePipe (output) failed: %1")
                .arg(::GetLastError()));
            return false;
        }

        // 父进程端的句柄不需要被子进程继承
        ::SetHandleInformation(inWrite, HANDLE_FLAG_INHERIT, 0);
        ::SetHandleInformation(outRead, HANDLE_FLAG_INHERIT, 0);

        // ---------- 2. CreatePseudoConsole ----------
        COORD size{ 80, 24 };
        HPCON hPC = nullptr;
        HRESULT hr = api.CreatePseudoConsole(size, inRead, outWrite, 0, &hPC);
        if (FAILED(hr)) {
            ::CloseHandle(inRead);
            ::CloseHandle(inWrite);
            ::CloseHandle(outRead);
            ::CloseHandle(outWrite);
            emit errorOccurred(QStringLiteral("CreatePseudoConsole failed: 0x%1")
                .arg(quint32(hr), 0, 16));
            return false;
        }

        // PTY 内部已复制 inRead / outWrite，父进程不再需要这两个句柄
        ::CloseHandle(inRead);
        ::CloseHandle(outWrite);

        m_hPC         = hPC;
        m_hInputWrite = inWrite;
        m_hOutputRead = outRead;

        // ---------- 3. 准备 STARTUPINFOEX + PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE ----------
        STARTUPINFOEXW si{};
        si.StartupInfo.cb = sizeof(si);
        // ConPTY 模式下不需要 STARTF_USESTDHANDLES（句柄通过 attribute list 传）

        SIZE_T attrSize = 0;
        ::InitializeProcThreadAttributeList(nullptr, 1, 0, &attrSize);
        if (attrSize == 0) {
            closeAll();
            emit errorOccurred(QStringLiteral(
                    "InitializeProcThreadAttributeList (size query) failed"));
            return false;
        }

        si.lpAttributeList = static_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(
            ::HeapAlloc(::GetProcessHeap(), 0, attrSize));
        if (!si.lpAttributeList) {
            closeAll();
            emit errorOccurred(QStringLiteral("HeapAlloc for attribute list failed"));
            return false;
        }

        if (!::InitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &attrSize)) {
            ::HeapFree(::GetProcessHeap(), 0, si.lpAttributeList);
            closeAll();
            emit errorOccurred(QStringLiteral(
                    "InitializeProcThreadAttributeList failed: %1").arg(::GetLastError()));
            return false;
        }

        if (!::UpdateProcThreadAttribute(si.lpAttributeList, 0,
                PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                m_hPC, sizeof(m_hPC), nullptr, nullptr)) {
            ::DeleteProcThreadAttributeList(si.lpAttributeList);
            ::HeapFree(::GetProcessHeap(), 0, si.lpAttributeList);
            closeAll();
            emit errorOccurred(QStringLiteral(
                    "UpdateProcThreadAttribute failed: %1").arg(::GetLastError()));
            return false;
        }

        // ---------- 4. 构造命令行 / 环境块 / 工作目录 ----------
        const QString cmdLine = buildCommandLine(program, args);
        std::wstring cmdLineW = cmdLine.toStdWString();
        //   CreateProcessW 要求 lpCommandLine 是【可写缓冲区】，
        //   所以必须用 std::wstring::data()（C++17 起非 const）

        const QByteArray envBlock = buildEnvironmentBlock(env);

        std::wstring wd;
        if (!workingDir.isEmpty()) {
            wd = workingDir.toStdWString();
        }

        // ---------- 5. CreateProcessW ----------
        BOOL ok = ::CreateProcessW(
            nullptr,                                                   // 用命令行第一个 token 作可执行文件
            cmdLineW.data(),                                           // 可写命令行
            nullptr, nullptr,
            FALSE,                                                     // 不继承句柄（ConPTY 句柄走 attribute list）
            EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT,
            const_cast<char*>(envBlock.constData()),                   // 环境块
            wd.empty() ? nullptr : wd.c_str(),                         // 工作目录
            &si.StartupInfo,
            &m_pi);

        // 无论成败，attribute list 用完就释放
        ::DeleteProcThreadAttributeList(si.lpAttributeList);
        ::HeapFree(::GetProcessHeap(), 0, si.lpAttributeList);

        if (!ok) {
            DWORD err = ::GetLastError();
            closeAll();
            emit errorOccurred(QStringLiteral("CreateProcessW failed: %1").arg(err));
            return false;
        }

        m_running    = true;
        m_stopReader = false;

        // ---------- 6. 启动输出读取线程 ----------
        //   ConPTY 的 ReadFile 是阻塞的，必须放到后台线程，
        //   否则主线程（UI）会被卡死。
        m_readerThread = QThread::create([this]() {
                char buf[8192];
                while (!m_stopReader) {
                    DWORD bytesRead = 0;
                    const BOOL r = ::ReadFile(m_hOutputRead, buf, sizeof(buf),
                        &bytesRead, nullptr);
                    if (!r || bytesRead == 0) {
                        // 管道关闭 / 子进程退出
                        break;
                    }
                    const QByteArray chunk(buf, int(bytesRead));
                    // 跨线程回到主线程 emit readyRead
                    QMetaObject::invokeMethod(this, [this, chunk]() {
                            emit readyRead(chunk);
                        }, Qt::QueuedConnection);
                }

                // 等子进程退出，拿退出码
                if (m_pi.hProcess) {
                    ::WaitForSingleObject(m_pi.hProcess, 5000);
                }
                DWORD exitCode = 0;
                if (m_pi.hProcess) {
                    ::GetExitCodeProcess(m_pi.hProcess, &exitCode);
                }

                m_running = false;
                QMetaObject::invokeMethod(this, [this, exitCode]() {
                        emit finished(int(exitCode), PtyExitStatus::NormalExit);
                    }, Qt::QueuedConnection);
            });
        m_readerThread->start();

        return true;
    }

    // ============================================================================
    // write() —— 用户键盘输入 → 子进程 stdin
    // ============================================================================
    void WinConPtyProcess::write(const QByteArray& data)
    {
        if (!m_hInputWrite || data.isEmpty()) return;

        DWORD written = 0;
        // 注：Windows pipe 默认阻塞模式；输入缓冲区通常足够大，
        //     少量键盘输入不会阻塞。若未来需要大段粘贴且不能阻塞，
        //     可改成 Overlapped I/O + IOCP。
        ::WriteFile(m_hInputWrite, data.constData(),
            DWORD(data.size()), &written, nullptr);
    }

    // ============================================================================
    // resize() —— 窗口尺寸变化（vim / top / htop 依赖）
    // ============================================================================
    void WinConPtyProcess::resize(const PtySize& size)
    {
        if (!m_hPC) return;
        auto& api = ConPtyApi::instance();
        if (!api.available || !api.ResizePseudoConsole) return;

        COORD sz{};
        sz.X = SHORT(qBound(1, size.cols, 32767));
        sz.Y = SHORT(qBound(1, size.rows, 32767));
        api.ResizePseudoConsole(m_hPC, sz);
    }

    // ============================================================================
    // sendInterrupt() —— 模拟 Ctrl+C
    //
    //   写 0x03 (ETX) 到 PTY 输入：
    //     - 远端 ssh 会转发到远端的 pty，远端命令收到 SIGINT
    //     - 本地 cmd.exe / powershell 收到 Ctrl+C
    //     - 本地 vim 会进入 normal 模式
    //   比 TerminateProcess 更温和，不破坏会话
    // ============================================================================
    void WinConPtyProcess::sendInterrupt()
    {
        write(QByteArray(1, '\x03'));
    }

    // ============================================================================
    // sendTerminate() —— 请求终止
    //
    //   GenerateConsoleCtrlEvent 在 ConPTY 场景下不可靠（要求进程共享 console），
    //   这里直接 TerminateProcess。想"温柔结束"的，先 sendInterrupt()。
    // ============================================================================
    void WinConPtyProcess::sendTerminate()
    {
        if (m_pi.hProcess) {
            ::TerminateProcess(m_pi.hProcess, 1);
        }
    }

    // ============================================================================
    // sendKill() —— 强杀（最后手段）
    // ============================================================================
    void WinConPtyProcess::sendKill()
    {
        if (m_pi.hProcess) {
            ::TerminateProcess(m_pi.hProcess, 1);
        }
    }

    bool WinConPtyProcess::isRunning() const
    {
        return m_running;
    }

    qint64 WinConPtyProcess::pid() const
    {
        return qint64(m_pi.dwProcessId);
    }

    // ============================================================================
    // closeAll() —— 幂等关闭所有句柄 / 资源
    // ============================================================================
    void WinConPtyProcess::closeAll()
    {
        auto& api = ConPtyApi::instance();

        if (m_hInputWrite) { ::CloseHandle(m_hInputWrite); m_hInputWrite = nullptr; }
        if (m_hOutputRead) { ::CloseHandle(m_hOutputRead); m_hOutputRead = nullptr; }

        if (m_hPC) {
            if (api.available && api.ClosePseudoConsole) {
                api.ClosePseudoConsole(m_hPC);
            }
            m_hPC = nullptr;
        }

        if (m_pi.hProcess) { ::CloseHandle(m_pi.hProcess); m_pi.hProcess = nullptr; }
        if (m_pi.hThread)  { ::CloseHandle(m_pi.hThread);  m_pi.hThread  = nullptr; }

        m_running = false;
    }

} // namespace SprintIDE

#endif // Q_OS_WIN

#include "win_conpty_process.moc"