#include "recording/recording_memory_policy.h"

#include <algorithm>

namespace markshot::recording {
namespace {
constexpr quint64 MiB = 1024 * 1024;

/** 计算 size 的 BGRA 帧字节数，非法尺寸返回 0；使用无符号宽整数避免乘法溢出。 */
quint64 frameBytes(QSize size)
{
    if (size.isEmpty()) return 0;
    return static_cast<quint64>(size.width()) * static_cast<quint64>(size.height()) * 4;
}
}  // namespace

quint64 recordingQueueMemoryBudget()
{
    bool ok = false;
    const int configured = qEnvironmentVariableIntValue("MARK_SHOT_RECORDING_QUEUE_MIB", &ok);
    return ok && configured > 0 ? static_cast<quint64>(configured) * MiB : 96 * MiB;
}

int recordingQueueFrameCapacity(QSize size, int fps)
{
    const quint64 bytes = std::max<quint64>(1, frameBytes(size));
    const int byMemory = static_cast<int>(std::clamp<quint64>(recordingQueueMemoryBudget() / bytes, 1, 4));
    return std::min(byMemory, fps >= 48 ? 3 : 4);
}

int recordingSoftwareEncoderThreads(QSize size, int availableThreads, int fps, bool keepAutomatic)
{
    bool ok = false;
    const int configured = qEnvironmentVariableIntValue("MARK_SHOT_RECORDING_ENCODER_THREADS", &ok);
    if (ok && configured >= 0 && configured <= 64) return configured;
    if (keepAutomatic) return 0;
    const quint64 bytes = frameBytes(size);
    if (!bytes) return 1;

    // 1. 【录制】【编码内存】按 BGRA 等效尺寸约束并发帧，高帧率保留两倍余量；不是编码器总内存上限
    const quint64 concurrentFrameBudget = (fps >= 48 ? 256 : 128) * MiB;
    const int byFrameSize = static_cast<int>(std::clamp<quint64>(concurrentFrameBudget / bytes, 1, 8));
    // 2. 【录制】【编码内存】不超额创建工作线程，保留显式自动模式供吞吐量对比
    return std::clamp(availableThreads, 1, byFrameSize);
}

}  // namespace markshot::recording
