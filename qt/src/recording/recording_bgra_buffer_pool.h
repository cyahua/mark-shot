#pragma once

#include <QByteArray>

#include <array>

namespace markshot::recording {

/**
 * 录制帧 BGRA 缓冲池。
 *
 * 采集回调每帧需要一块与帧等大的连续缓冲，直接新建 QByteArray 会造成
 * 高频大块分配。池内最多保留固定数量的槽位，下游释放引用后槽位自动可复用。
 * 优先复用最近使用的槽位，持续闲置的槽位延迟释放，避免一直保留积压时的峰值容量。
 * 仅限单个采集线程使用，不做加锁。
 */
class RecordingBgraBufferPool final {
public:
    /**
     * 获取一块指定字节数的可写缓冲。
     * @param byteCount 需要的字节数。
     * @return 池内槽位引用，写入完成后按值拷出即共享数据；引用在下次
     *         acquire 前有效。
     */
    QByteArray &acquire(qsizetype byteCount);

    /** 返回池当前引用的总容量（字节），含下游共享槽位；无参数，不等于进程驻留内存。 */
    qsizetype retainedCapacity() const;

private:
    /** 回收长期闲置且未共享的槽位，activeSlot 为本次使用的索引，无返回值。 */
    void reclaimIdleSlots(int activeSlot);

    static constexpr int kSlotCount = 4;
    static constexpr int kIdleRetentionAcquisitions = 64;

    std::array<QByteArray, kSlotCount> m_slots;
    std::array<int, kSlotCount> m_idleAcquisitions{};
    int m_cursor = kSlotCount - 1;
};

}  // namespace markshot::recording
