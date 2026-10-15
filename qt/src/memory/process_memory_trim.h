#pragma once

#include <QtGlobal>

namespace markshot::memory {

/**
 * 【内存】【常驻统计】读取当前进程的常驻内存。
 * @return 常驻字节数，Windows 为工作集，Linux 为 RSS，平台不支持时返回 0。
 */
quint64 residentMemoryBytes();

/**
 * 【内存】【空闲回收】把分配器空闲页归还系统，并让系统回收进程工作集。
 *
 * 只释放空闲页和可重新换入的页面，不影响任何存活对象；
 * Windows 调用 _heapmin 与 SetProcessWorkingSetSize，glibc 调用 malloc_trim。
 * @return 无返回值。
 */
void trimProcessMemory();

/**
 * 【内存】【空闲回收】执行回收并在调试日志中记录前后常驻内存。
 * @return 无返回值。
 */
void trimProcessMemoryWithLog();

}  // namespace markshot::memory
