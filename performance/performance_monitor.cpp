// src/performance/performance_monitor.cpp
#include "performance/performance_monitor.h"

#include <QDateTime>
#include <QtGlobal>

// ============================================================================
//  平台相关头文件
// ============================================================================
#ifdef Q_OS_MAC
#  include <mach/mach.h>
#  include <mach/mach_time.h>
#  include <mach/vm_statistics.h>
#  include <sys/sysctl.h>
#  include <unistd.h>
#endif

#ifdef Q_OS_LINUX
#  include <unistd.h>
#  include <QFile>
#  include <QByteArray>
#  include <QList>
#endif

#ifdef Q_OS_WIN
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <psapi.h>
#endif

namespace SprintIDE {

// ============================================================================
//  一、平台无关的采集函数（匿名命名空间）
// ============================================================================
namespace {

// ---------------------------------------------------------------------------
// 进程 CPU 累计时间（微秒）
// ---------------------------------------------------------------------------
quint64 processCpuTimeUs()
{
#ifdef Q_OS_MAC
    task_basic_info_data_t info;
    mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
    kern_return_t kr = task_info(mach_task_self(),
                                 TASK_BASIC_INFO,
                                 reinterpret_cast<task_info_t>(&info),
                                 &count);
    if (kr != KERN_SUCCESS) return 0;
    return quint64(info.user_time.seconds) * 1000000ULL
           + quint64(info.user_time.microseconds)
           + quint64(info.system_time.seconds) * 1000000ULL
           + quint64(info.system_time.microseconds);

#elif defined(Q_OS_LINUX)
    // /proc/self/stat 的第 14、15 个字段（1-based）是 utime / stime（单位 tick）
    QFile f(QStringLiteral("/proc/self/stat"));
    if (!f.open(QIODevice::ReadOnly)) return 0;
    const QByteArray content = f.readAll();

    // 跳过 "pid (comm)"，comm 本身可能含空格，因此找最后一个 ')'
    const int closeParen = content.lastIndexOf(')');
    if (closeParen < 0) return 0;

    const QByteArray rest = content.mid(closeParen + 2).trimmed();
    const QList<QByteArray> fields = rest.split(' ');
    // rest[0] = state, rest[11] = utime, rest[12] = stime
    if (fields.size() < 13) return 0;

    bool ok1 = false, ok2 = false;
    const quint64 utime = fields[11].toULongLong(&ok1);
    const quint64 stime = fields[12].toULongLong(&ok2);
    if (!ok1 || !ok2) return 0;

    const long hz = ::sysconf(_SC_CLK_TCK);
    if (hz <= 0) return 0;

    return (utime + stime) * 1000000ULL / quint64(hz);

#elif defined(Q_OS_WIN)
    FILETIME creation{}, exitT{}, kernel{}, user{};
    if (!::GetProcessTimes(::GetCurrentProcess(), &creation, &exitT, &kernel, &user))
        return 0;
    ULARGE_INTEGER k{}, u{};
    k.LowPart  = kernel.dwLowDateTime;  k.HighPart = kernel.dwHighDateTime;
    u.LowPart  = user.dwLowDateTime;    u.HighPart = user.dwHighDateTime;
    // FILETIME 单位 100ns → 微秒 = /10
    return (k.QuadPart + u.QuadPart) / 10ULL;

#else
    return 0;
#endif
}

// ---------------------------------------------------------------------------
// 进程内存（字节）
// ---------------------------------------------------------------------------
quint64 processMemoryBytes()
{
#ifdef Q_OS_MAC
    task_vm_info_data_t info;
    mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
    kern_return_t kr = task_info(mach_task_self(),
                                 TASK_VM_INFO,
                                 reinterpret_cast<task_info_t>(&info),
                                 &count);
    if (kr != KERN_SUCCESS) return 0;
    return quint64(info.phys_footprint);   // 与「活动监视器」口径一致

#elif defined(Q_OS_LINUX)
    QFile f(QStringLiteral("/proc/self/status"));
    if (!f.open(QIODevice::ReadOnly)) return 0;
    const QByteArray content = f.readAll();

    const int idx = content.indexOf("VmRSS:");
    if (idx < 0) return 0;
    int end = content.indexOf('\n', idx);
    if (end < 0) end = content.size();
    const QByteArray line = content.mid(idx, end - idx);

    QByteArray num;
    for (char c : line) {
        if (c >= '0' && c <= '9') {
            num.append(c);
        } else if (!num.isEmpty()) {
            break;
        }
    }
    bool ok = false;
    const quint64 kb = num.toULongLong(&ok);
    return ok ? kb * 1024ULL : 0;

#elif defined(Q_OS_WIN)
    PROCESS_MEMORY_COUNTERS pmc{};
    if (!::GetProcessMemoryInfo(::GetCurrentProcess(), &pmc, sizeof(pmc)))
        return 0;
    return quint64(pmc.WorkingSetSize);

#else
    return 0;
#endif
}

// ---------------------------------------------------------------------------
// 系统 CPU 累计 tick（总量 / 空闲量）
// ---------------------------------------------------------------------------
void systemCpuTicks(quint64& total, quint64& idle)
{
    total = 0;
    idle  = 0;

#ifdef Q_OS_MAC
    host_cpu_load_info_data_t info;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    if (host_statistics(mach_host_self(),
                        HOST_CPU_LOAD_INFO,
                        reinterpret_cast<host_info_t>(&info),
                        &count) != KERN_SUCCESS)
        return;

    const quint64 userT = info.cpu_ticks[CPU_STATE_USER];
    const quint64 sysT  = info.cpu_ticks[CPU_STATE_SYSTEM];
    const quint64 niceT = info.cpu_ticks[CPU_STATE_NICE];
    const quint64 idleT = info.cpu_ticks[CPU_STATE_IDLE];

    total = userT + sysT + niceT + idleT;
    idle  = idleT;

#elif defined(Q_OS_LINUX)
    QFile f(QStringLiteral("/proc/stat"));
    if (!f.open(QIODevice::ReadOnly)) return;
    const QByteArray line = f.readLine();
    if (!line.startsWith("cpu ")) return;

    const QList<QByteArray> parts = line.mid(4).trimmed().split(' ');
    QList<quint64> vals;
    vals.reserve(parts.size());
    for (const QByteArray& p : parts) {
        bool ok = false;
        const quint64 v = p.toULongLong(&ok);
        if (ok) vals.append(v);
    }
    if (vals.size() < 5) return;

    quint64 t = 0;
    for (quint64 v : vals) t += v;

    total = t;
    idle  = vals[3] + vals[4];   // idle + iowait

#elif defined(Q_OS_WIN)
    FILETIME idleFt{}, kernelFt{}, userFt{};
    if (!::GetSystemTimes(&idleFt, &kernelFt, &userFt)) return;

    ULARGE_INTEGER il{}, kl{}, ul{};
    il.LowPart = idleFt.dwLowDateTime;   il.HighPart = idleFt.dwHighDateTime;
    kl.LowPart = kernelFt.dwLowDateTime; kl.HighPart = kernelFt.dwHighDateTime;
    ul.LowPart = userFt.dwLowDateTime;   ul.HighPart = userFt.dwHighDateTime;

    // Windows 的 kernel 时间已经包含 idle
    total = kl.QuadPart + ul.QuadPart;
    idle  = il.QuadPart;
#endif
}

// ---------------------------------------------------------------------------
// 系统内存（总量 / 已用量，单位字节）
// ---------------------------------------------------------------------------
void systemMemory(quint64& totalBytes, quint64& usedBytes)
{
    totalBytes = 0;
    usedBytes  = 0;

#ifdef Q_OS_MAC
    // 总物理内存
    int mib[2] = { CTL_HW, HW_MEMSIZE };
    quint64 memSize = 0;
    size_t len = sizeof(memSize);
    if (sysctl(mib, 2, &memSize, &len, nullptr, 0) != 0) return;
    totalBytes = memSize;

    // 已用内存 ≈ active + wired + compressed（与活动监视器接近）
    vm_statistics64_data_t vmstat;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    if (host_statistics64(mach_host_self(),
                          HOST_VM_INFO64,
                          reinterpret_cast<host_info64_t>(&vmstat),
                          &count) != KERN_SUCCESS)
        return;

    vm_size_t pageSize = 0;
    if (host_page_size(mach_host_self(), &pageSize) != KERN_SUCCESS)
        pageSize = 4096;

    const quint64 active     = quint64(vmstat.active_count)         * pageSize;
    const quint64 wired      = quint64(vmstat.wire_count)           * pageSize;
    const quint64 compressed = quint64(vmstat.compressor_page_count)* pageSize;

    usedBytes = active + wired + compressed;

#elif defined(Q_OS_LINUX)
    QFile f(QStringLiteral("/proc/meminfo"));
    if (!f.open(QIODevice::ReadOnly)) return;
    const QByteArray content = f.readAll();

    auto parseField = [&content](const char* key) -> quint64 {
        const int idx = content.indexOf(key);
        if (idx < 0) return 0;
        int end = content.indexOf('\n', idx);
        if (end < 0) end = content.size();
        const QByteArray line = content.mid(idx, end - idx);
        QByteArray num;
        for (char c : line) {
            if (c >= '0' && c <= '9') num.append(c);
            else if (!num.isEmpty()) break;
        }
        bool ok = false;
        const quint64 kb = num.toULongLong(&ok);
        return ok ? kb * 1024ULL : 0;
    };

    totalBytes = parseField("MemTotal:");
    quint64 available = parseField("MemAvailable:");
    if (available == 0) {
        // 老内核回退
        available = parseField("MemFree:")
                    + parseField("Buffers:")
                    + parseField("Cached:");
    }
    usedBytes = totalBytes > available ? (totalBytes - available) : 0;

#elif defined(Q_OS_WIN)
    MEMORYSTATUSEX ms{};
    ms.dwLength = sizeof(ms);
    if (!::GlobalMemoryStatusEx(&ms)) return;
    totalBytes = ms.ullTotalPhys;
    usedBytes  = ms.ullTotalPhys - ms.ullAvailPhys;
#endif
}

} // anonymous namespace

// ============================================================================
//  二、PerformanceMonitor 实现
// ============================================================================
PerformanceMonitor::PerformanceMonitor(QObject* parent)
    : QObject(parent)
{
    m_timer.setTimerType(Qt::CoarseTimer);   // 降低唤醒精度，节省功耗
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &PerformanceMonitor::sample);
}

PerformanceMonitor::~PerformanceMonitor() = default;

void PerformanceMonitor::start(int intervalMs)
{
    if (intervalMs > 0) m_timer.setInterval(intervalMs);
    if (m_timer.isActive()) return;

    m_hasLast = false;
    m_lastProcessCpuUs     = 0;
    m_lastSystemTotalTicks = 0;
    m_lastSystemIdleTicks  = 0;

    m_wallTimer.start();
    m_timer.start();

    // 立刻采一次，让 UI 尽快有初值（CPU 显示 0，第二次开始有效）
    sample();
}

void PerformanceMonitor::stop()
{
    m_timer.stop();
}

void PerformanceMonitor::setInterval(int intervalMs)
{
    if (intervalMs > 0) m_timer.setInterval(intervalMs);
}

bool PerformanceMonitor::isRunning() const
{
    return m_timer.isActive();
}

PerformanceSnapshot PerformanceMonitor::lastSnapshot() const
{
    return m_last;
}

void PerformanceMonitor::sample()
{
    PerformanceSnapshot s;
    s.timestampMs = QDateTime::currentMSecsSinceEpoch();

    // ---- 原始数据 ----
    const quint64 procCpuUs = processCpuTimeUs();
    s.processMemoryBytes    = processMemoryBytes();

    quint64 sysTotalTicks = 0;
    quint64 sysIdleTicks  = 0;
    systemCpuTicks(sysTotalTicks, sysIdleTicks);
    systemMemory(s.systemTotalMemoryBytes, s.systemUsedMemoryBytes);

    if (s.systemTotalMemoryBytes > 0) {
        s.systemMemoryPercent =
            100.0 * double(s.systemUsedMemoryBytes) / double(s.systemTotalMemoryBytes);
    }

    // ---- 差分计算 ----
    if (m_hasLast) {
        const qint64 wallMs = m_wallTimer.restart();

        if (wallMs > 0 && procCpuUs >= m_lastProcessCpuUs) {
            const quint64 dProc = procCpuUs - m_lastProcessCpuUs;
            // 微秒 → 毫秒；单核跑满 = 100%
            s.processCpuPercent =
                100.0 * (double(dProc) / 1000.0) / double(wallMs);
        }

        if (sysTotalTicks > m_lastSystemTotalTicks) {
            const quint64 dTotal = sysTotalTicks - m_lastSystemTotalTicks;
            const quint64 dIdle  = sysIdleTicks >= m_lastSystemIdleTicks
                                      ? sysIdleTicks - m_lastSystemIdleTicks
                                      : 0;
            s.systemCpuPercent = 100.0 * double(dTotal - dIdle) / double(dTotal);
        }
    }

    m_lastProcessCpuUs     = procCpuUs;
    m_lastSystemTotalTicks = sysTotalTicks;
    m_lastSystemIdleTicks  = sysIdleTicks;
    m_hasLast              = true;

    m_last = s;
    emit snapshotUpdated(s);
}

} // namespace SprintIDE
