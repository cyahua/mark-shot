#pragma once

#include <QSize>

namespace markshot::recording {

/** 返回输入队列的字节预算；保留 MARK_SHOT_RECORDING_QUEUE_MIB 覆盖和 96 MiB 默认值，无参数。 */
quint64 recordingQueueMemoryBudget();

/** 根据 size 帧尺寸和 fps 帧率计算队列容量，返回至少一帧。 */
int recordingQueueFrameCapacity(QSize size, int fps);

/**
 * 【录制】【编码内存】限制同时编码的全分辨率帧，避免并发数仅随 CPU 核数增长。
 * @param size 编码帧尺寸。
 * @param availableThreads 可用逻辑处理器数量，非正数按一个处理。
 * @param fps 目标帧率，高帧率需要为编码吞吐量保留更多并发。
 * @param keepAutomatic 复杂预设保留自动并发，避免减少实时编码余量；显式环境配置仍优先。
 * @return 软件编码线程上限；返回 0 表示保留编码器自动选择。
 */
int recordingSoftwareEncoderThreads(QSize size, int availableThreads, int fps = 30,
                                     bool keepAutomatic = false);

}  // namespace markshot::recording
