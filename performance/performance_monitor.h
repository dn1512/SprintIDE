// src/performance/performance_monitor.h
#ifndef SPRINTIDE_PERFORMANCE_MONITOR_H
#define SPRINTIDE_PERFORMANCE_MONITOR_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QMetaType>

namespace SprintIDE {

/**
 * @brief 一次性能采样快照。
 *
 * 约定：
 *  - processCpuPercent 采用 Activity Monitor / top -H 风格，
 *    单核跑满 = 100%，双核跑满 = 200%，最大 = 逻辑核心数 * 100。
 *  - systemCpuPercent 是整机归一化百分比，0~100。
 *  - 所有内存字段单位是字节。
 */
struct PerformanceSnapshot {
    double   processCpuPercent     = 0.0;
    double   systemCpuPercent      = 0.0;
    quint64  processMemoryBytes    = 0;
    quint64  systemTotalMemoryBytes= 0;
    quint64  systemUsedMemoryBytes = 0;
    double   systemMemoryPercent   = 0.0;
    qint64   timestampMs           = 0;
};

/**
 * @brief 轻量级跨平台性能采样器（macOS / Linux / Windows）。
 *
 * 不依赖任何外部进程，不阻塞 UI 线程，不引入额外 Qt 模块。
 * 采样失败时静默返回 0，不影响 IDE 主流程。
 */
class PerformanceMonitor : public QObject
{
    Q_OBJECT
public:
    explicit PerformanceMonitor(QObject* parent = nullptr);
    ~PerformanceMonitor() override;

    void start(int intervalMs = 1000);
    void stop();
    void setInterval(int intervalMs);
    bool isRunning() const;

    PerformanceSnapshot lastSnapshot() const;

signals:
    void snapshotUpdated(const PerformanceSnapshot& snapshot);

private slots:
    void sample();

private:
    QTimer                m_timer;
    QElapsedTimer         m_wallTimer;
    PerformanceSnapshot   m_last;
    bool                  m_hasLast = false;

    // 差分状态
    quint64 m_lastProcessCpuUs       = 0;
    quint64 m_lastSystemTotalTicks   = 0;
    quint64 m_lastSystemIdleTicks    = 0;
};

} // namespace SprintIDE

Q_DECLARE_METATYPE(SprintIDE::PerformanceSnapshot)

#endif // SPRINTIDE_PERFORMANCE_MONITOR_H
