#include "memory/process_memory_trim.h"

#include "debug_log.h"

#include <QElapsedTimer>

#if defined(Q_OS_WIN)
#include <malloc.h>
#include <windows.h>
#include <psapi.h>
#elif defined(Q_OS_LINUX)
#include <QFile>
#include <unistd.h>
#if defined(__GLIBC__)
#include <malloc.h>
#endif
#endif

namespace markshot::memory {

quint64 residentMemoryBytes()
{
#if defined(Q_OS_WIN)
    PROCESS_MEMORY_COUNTERS counters{};
    counters.cb = sizeof(counters);
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters))) {
        return 0;
    }
    return static_cast<quint64>(counters.WorkingSetSize);
#elif defined(Q_OS_LINUX)
    // 1. statm 第二列为常驻页数，按页大小换算字节
    QFile statm(QStringLiteral("/proc/self/statm"));
    if (!statm.open(QIODevice::ReadOnly)) {
        return 0;
    }
    const QList<QByteArray> fields = statm.readAll().simplified().split(' ');
    if (fields.size() < 2) {
        return 0;
    }
    const long pageSize = sysconf(_SC_PAGESIZE);
    return fields.at(1).toULongLong() * static_cast<quint64>(pageSize > 0 ? pageSize : 4096);
#else
    return 0;
#endif
}

void trimProcessMemory()
{
#if defined(Q_OS_WIN)
    // 1. 归还 CRT 堆尾部空闲块，再让系统把可换入页面移出工作集
    _heapmin();
    SetProcessWorkingSetSize(GetCurrentProcess(), static_cast<SIZE_T>(-1), static_cast<SIZE_T>(-1));
#elif defined(Q_OS_LINUX) && defined(__GLIBC__)
    // 1. glibc 不会自动归还 arena 中间的空闲页，需要显式整理
    malloc_trim(0);
#endif
}

void trimProcessMemoryWithLog()
{
    const quint64 before = residentMemoryBytes();
    QElapsedTimer elapsed;
    elapsed.start();
    trimProcessMemory();
    const quint64 after = residentMemoryBytes();
    markshot::debugLog("memory",
                       "【内存】【空闲回收】resident_before_kb=%llu resident_after_kb=%llu duration_ms=%lld",
                       static_cast<unsigned long long>(before / 1024),
                       static_cast<unsigned long long>(after / 1024),
                       static_cast<long long>(elapsed.elapsed()));
}

}  // namespace markshot::memory
