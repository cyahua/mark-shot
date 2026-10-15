#pragma once

namespace markshot::memory {

/**
 * 【内存】【图像分配】配置大型图像缓冲的释放策略
 *
 * glibc 默认会动态提高 mmap 阈值，导致释放后的整帧图像留在堆中；
 * 固定为 1 MiB，使大图像使用独立映射，释放时立即归还系统。
 * 其他平台保持原有分配器策略。
 * @return 无返回值
 */
void configureImageAllocationPolicy();

}  // namespace markshot::memory
