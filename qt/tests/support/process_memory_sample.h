#pragma once

#include <QFile>
#include <QJsonObject>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <psapi.h>
#endif

namespace markshot::test {

struct ProcessMemorySample {
    quint64 resident = 0;
    quint64 peakResident = 0;
    quint64 privateBytes = 0;

    /** 返回字节单位的内存记录；Windows 私有值为提交量，Linux 私有值为驻留量，无参数。 */
    QJsonObject json() const
    {
        return {{QStringLiteral("resident_bytes"), static_cast<qint64>(resident)},
                {QStringLiteral("peak_resident_bytes"), static_cast<qint64>(peakResident)},
                {QStringLiteral("private_bytes"), static_cast<qint64>(privateBytes)}};
    }
};

/** 读取本进程内存，不整理分配器或回收工作集；返回当前值与系统维护的峰值，无参数。 */
inline ProcessMemorySample processMemorySample()
{
    ProcessMemorySample sample;
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS_EX counters{};
    counters.cb = sizeof(counters);
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                            reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&counters), sizeof(counters))) {
        sample.resident = counters.WorkingSetSize;
        sample.peakResident = counters.PeakWorkingSetSize;
        sample.privateBytes = counters.PrivateUsage;
    }
#elif defined(Q_OS_LINUX)
    QFile status(QStringLiteral("/proc/self/status"));
    if (status.open(QIODevice::ReadOnly)) {
        for (const QByteArray &line : status.readAll().split('\n')) {
            const auto fields = line.simplified().split(' ');
            if (fields.size() < 2) continue;
            const quint64 bytes = fields.at(1).toULongLong() * 1024;
            if (fields.first() == "VmRSS:") sample.resident = bytes;
            if (fields.first() == "VmHWM:") sample.peakResident = bytes;
        }
    }
    QFile mappings(QStringLiteral("/proc/self/smaps_rollup"));
    if (mappings.open(QIODevice::ReadOnly)) {
        for (const QByteArray &line : mappings.readAll().split('\n')) {
            const auto fields = line.simplified().split(' ');
            if (fields.size() >= 2 && (fields.first() == "Private_Clean:" || fields.first() == "Private_Dirty:")) {
                sample.privateBytes += fields.at(1).toULongLong() * 1024;
            }
        }
    }
#endif
    return sample;
}

}  // namespace markshot::test
