#include "memory/image_allocation_policy.h"

#include <QtGlobal>

#if defined(Q_OS_LINUX) && defined(__GLIBC__)
#include <malloc.h>
#endif

namespace markshot::memory {

void configureImageAllocationPolicy()
{
#if defined(Q_OS_LINUX) && defined(__GLIBC__)
    // 1. 【内存】【图像分配】固定大缓冲映射阈值，避免整帧图像释放后仍占据堆页
    mallopt(M_MMAP_THRESHOLD, 1024 * 1024);
#endif
}

}  // namespace markshot::memory
