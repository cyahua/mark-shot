#include "recording/recording_bgra_buffer_pool.h"

#include <QtTest/QtTest>

#include <cstring>

class RecordingBgraBufferPoolTest : public QObject {
    Q_OBJECT

private slots:
    /**
     * 验证缓冲池返回指定大小的可写缓冲。
     * @return 无返回值。
     */
    void acquiresRequestedSize()
    {
        markshot::recording::RecordingBgraBufferPool pool;
        QByteArray &buffer = pool.acquire(64);
        QCOMPARE(buffer.size(), qsizetype(64));
        std::memset(buffer.data(), 0x5a, 64);
        QCOMPARE(buffer.at(63), char(0x5a));
    }

    /**
     * 验证下游释放引用后槽位内存被复用。
     * @return 无返回值。
     */
    void reusesReleasedSlot()
    {
        markshot::recording::RecordingBgraBufferPool pool;
        const char *firstData = nullptr;
        {
            QByteArray &buffer = pool.acquire(128);
            firstData = buffer.constData();
        }
        // 未被下游持有的槽位应在后续 acquire 中复用同一块内存
        bool reused = false;
        for (int i = 0; i < 4; ++i) {
            QByteArray &buffer = pool.acquire(128);
            if (buffer.constData() == firstData) {
                reused = true;
                break;
            }
        }
        QVERIFY(reused);
    }

    /**
     * 验证被下游持有的缓冲不会被后续 acquire 覆写。
     * @return 无返回值。
     */
    void doesNotOverwriteHeldBuffers()
    {
        markshot::recording::RecordingBgraBufferPool pool;
        QByteArray &slot = pool.acquire(16);
        std::memset(slot.data(), 0x11, 16);
        const QByteArray held = slot;

        // 连续获取超过槽位数量的缓冲并写入不同内容
        for (int i = 0; i < 8; ++i) {
            QByteArray &buffer = pool.acquire(16);
            std::memset(buffer.data(), 0x22 + i, 16);
        }
        QCOMPARE(held.at(0), char(0x11));
        QCOMPARE(held.at(15), char(0x11));
    }

    /** 突发积压消退后回收长期闲置槽位，避免一直保留峰值容量，无参数和返回值。 */
    void releasesIdleSlotsAfterBurst()
    {
        markshot::recording::RecordingBgraBufferPool pool;
        std::array<QByteArray, 4> held;
        for (QByteArray &frame : held) frame = pool.acquire(4096);
        QVERIFY(pool.retainedCapacity() >= 4 * 4096);
        held = {};

        qsizetype activeCapacity = 0;
        for (int i = 0; i < 128; ++i) activeCapacity = pool.acquire(4096).capacity();
        QCOMPARE(pool.retainedCapacity(), activeCapacity);
    }

    /** 短时间负载波动保留缓冲，避免每帧反复分配大块内存，无参数和返回值。 */
    void retainsSlotsAcrossShortReusePeriods()
    {
        markshot::recording::RecordingBgraBufferPool pool;
        std::array<QByteArray, 4> held;
        for (QByteArray &frame : held) frame = pool.acquire(4096);
        const qsizetype retained = pool.retainedCapacity();
        held = {};

        for (int i = 0; i < 63; ++i) pool.acquire(4096);
        QCOMPARE(pool.retainedCapacity(), retained);
        const qsizetype activeCapacity = pool.acquire(4096).capacity();
        QCOMPARE(pool.retainedCapacity(), activeCapacity);
    }

    /** 超过池容量时仍保留外部快照内容，回收不能覆盖在途帧，无参数和返回值。 */
    void preservesEveryHeldSlotDuringReclamation()
    {
        markshot::recording::RecordingBgraBufferPool pool;
        std::array<QByteArray, 4> held;
        for (int i = 0; i < 4; ++i) {
            QByteArray &buffer = pool.acquire(4096);
            buffer.fill(char(i + 1));
            held[i] = buffer;
        }
        for (int i = 0; i < 128; ++i) pool.acquire(4096).fill(char(0x7f));
        for (int i = 0; i < 4; ++i) QCOMPARE(held[i], QByteArray(4096, char(i + 1)));
    }

    /** 分辨率显著降低时不保留旧帧的大容量分配，无参数和返回值。 */
    void shrinksAfterResolutionChange()
    {
        markshot::recording::RecordingBgraBufferPool pool;
        const qsizetype largeCapacity = pool.acquire(1024 * 1024).capacity();
        const QByteArray &small = pool.acquire(1024);
        QCOMPARE(small.size(), qsizetype(1024));
        QVERIFY(small.capacity() < largeCapacity / 2);
    }
};

QTEST_APPLESS_MAIN(RecordingBgraBufferPoolTest)
#include "recording_bgra_buffer_pool_test.moc"
