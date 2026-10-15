#include "recording/recording_bgra_buffer_pool.h"

namespace markshot::recording {

QByteArray &RecordingBgraBufferPool::acquire(qsizetype byteCount)
{
    // 1. 【录制】【帧缓冲】优先复用最近使用且未共享的槽位，使多余槽位能够持续闲置
    int selected = -1;
    for (int offset = 0; offset < kSlotCount; ++offset) {
        const int index = (m_cursor + offset) % kSlotCount;
        if (m_slots[index].isDetached()) {
            selected = index;
            break;
        }
    }

    // 2. 【录制】【帧缓冲】全部被共享时只解除池的引用，不复制或覆写下游仍持有的数据
    if (selected < 0) {
        selected = (m_cursor + 1) % kSlotCount;
        m_slots[selected] = QByteArray();
    }
    m_cursor = selected;
    QByteArray &buffer = m_slots[selected];
    // 3. 【录制】【帧缓冲】分辨率显著降低时重新分配，避免 resize 保留原来大帧的容量
    if (byteCount < buffer.capacity() / 2) buffer = QByteArray();
    buffer.resize(byteCount);
    reclaimIdleSlots(selected);
    return buffer;
}

void RecordingBgraBufferPool::reclaimIdleSlots(int activeSlot)
{
    for (int index = 0; index < kSlotCount; ++index) {
        if (index == activeSlot || !m_slots[index].isDetached()) {
            m_idleAcquisitions[index] = 0;
            continue;
        }
        // 【录制】【帧缓冲】跨过一段连续闲置期才回收，短时抖动仍能复用已有分配
        if (++m_idleAcquisitions[index] >= kIdleRetentionAcquisitions) {
            m_slots[index] = QByteArray();
            m_idleAcquisitions[index] = 0;
        }
    }
}

qsizetype RecordingBgraBufferPool::retainedCapacity() const
{
    qsizetype bytes = 0;
    for (const QByteArray &slot : m_slots) bytes += slot.capacity();
    return bytes;
}

}  // namespace markshot::recording
