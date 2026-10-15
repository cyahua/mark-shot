#include "recording/recording_memory_policy.h"

#include <QtTest/QtTest>
#include <limits>

using namespace markshot::recording;

class RecordingMemoryPolicyTest final : public QObject {
    Q_OBJECT

private slots:
    /** 每个用例使用确定的默认配置，不读取外部覆盖值，无参数和返回值。 */
    void init()
    {
        qunsetenv("MARK_SHOT_RECORDING_QUEUE_MIB");
        qunsetenv("MARK_SHOT_RECORDING_ENCODER_THREADS");
    }

    /** 保留既有队列容量和背压阈值，无参数和返回值。 */
    void preservesQueueCapacity()
    {
        QCOMPARE(recordingQueueMemoryBudget(), quint64(96 * 1024 * 1024));
        QCOMPARE(recordingQueueFrameCapacity(QSize(1920, 1080), 30), 4);
        QCOMPARE(recordingQueueFrameCapacity(QSize(1920, 1080), 60), 3);
        QCOMPARE(recordingQueueFrameCapacity(QSize(3840, 2160), 30), 3);
        QCOMPARE(recordingQueueFrameCapacity(QSize(3840, 2160), 60), 3);
        QCOMPARE(recordingQueueFrameCapacity(QSize(7680, 4320), 30), 1);
        qputenv("MARK_SHOT_RECORDING_QUEUE_MIB", "12");
        QCOMPARE(recordingQueueFrameCapacity(QSize(1920, 1080), 30), 1);
    }

    /** 大分辨率减少帧级并发，较低核数仍按硬件限制，无参数和返回值。 */
    void boundsConcurrentFrameStorage()
    {
        QCOMPARE(recordingSoftwareEncoderThreads(QSize(1920, 1080), 16), 8);
        QCOMPARE(recordingSoftwareEncoderThreads(QSize(3840, 2160), 16), 4);
        QCOMPARE(recordingSoftwareEncoderThreads(QSize(7680, 4320), 16), 1);
        QCOMPARE(recordingSoftwareEncoderThreads(QSize(1920, 1080), 2), 2);
        QCOMPARE(recordingSoftwareEncoderThreads(QSize(1920, 1080), -1), 1);
    }

    /** 高帧率保留必要的并发余量，不用编码赶不上采集的策略降低内存，无参数和返回值。 */
    void preservesHighFrameRateHeadroom()
    {
        QCOMPARE(recordingSoftwareEncoderThreads(QSize(1920, 1080), 16, 60), 8);
        QCOMPARE(recordingSoftwareEncoderThreads(QSize(3840, 2160), 16, 60), 8);
        QCOMPARE(recordingSoftwareEncoderThreads(QSize(3840, 2160), 6, 60), 6);
        QCOMPARE(recordingSoftwareEncoderThreads(QSize(7680, 4320), 16, 60), 2);
        QCOMPARE(recordingSoftwareEncoderThreads(QSize(3840, 2160), 16, 47), 4);
        QCOMPARE(recordingSoftwareEncoderThreads(QSize(3840, 2160), 16, 48), 8);
    }

    /** 复杂预设不自动减少并发，用户仍可显式覆盖，无参数和返回值。 */
    void keepsComplexPresetsAutomatic()
    {
        QCOMPARE(recordingSoftwareEncoderThreads(QSize(3840, 2160), 16, 30, true), 0);
        QCOMPARE(recordingSoftwareEncoderThreads(QSize(3840, 2160), 16, 60, true), 0);
        qputenv("MARK_SHOT_RECORDING_ENCODER_THREADS", "6");
        QCOMPARE(recordingSoftwareEncoderThreads(QSize(3840, 2160), 16, 60, true), 6);
    }

    /** 显式自动模式可用于对比，异常配置回退默认值，无参数和返回值。 */
    void validatesThreadOverrides()
    {
        qputenv("MARK_SHOT_RECORDING_ENCODER_THREADS", "0");
        QCOMPARE(recordingSoftwareEncoderThreads(QSize(3840, 2160), 16), 0);
        qputenv("MARK_SHOT_RECORDING_ENCODER_THREADS", "6");
        QCOMPARE(recordingSoftwareEncoderThreads(QSize(3840, 2160), 16), 6);
        for (const char *invalid : {"-1", "65", "invalid", "999999999999"}) {
            qputenv("MARK_SHOT_RECORDING_ENCODER_THREADS", invalid);
            QCOMPARE(recordingSoftwareEncoderThreads(QSize(3840, 2160), 16), 4);
        }
    }

    /** 异常尺寸不会溢出或变成无界线程数，无参数和返回值。 */
    void handlesInvalidAndExtremeDimensions()
    {
        QCOMPARE(recordingSoftwareEncoderThreads(QSize(), 16), 1);
        const int maximum = std::numeric_limits<int>::max();
        QCOMPARE(recordingSoftwareEncoderThreads(QSize(maximum, maximum), 16), 1);
        QCOMPARE(recordingQueueFrameCapacity(QSize(maximum, maximum), 30), 1);
    }
};

QTEST_APPLESS_MAIN(RecordingMemoryPolicyTest)
#include "recording_memory_policy_test.moc"
