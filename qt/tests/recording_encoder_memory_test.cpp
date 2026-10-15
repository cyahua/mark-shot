#include "recording/libav/libav_video_encoder_setup.h"
#include "recording/recording_memory_policy.h"

#include <QThread>
#include <QtTest/QtTest>

#include <memory>

extern "C" {
#include <libavutil/opt.h>
}

using namespace markshot::recording;

namespace {

/** 在 ctx 所属模块中释放编码上下文，无返回值。 */
void releaseCodec(AVCodecContext *ctx)
{
    avcodec_free_context(&ctx);
}

using CodecOwner = std::unique_ptr<AVCodecContext, decltype(&releaseCodec)>;

/** 按 quality、size 和 fps 打开实际 x264 编码器，失败写入 error，返回拥有上下文的句柄。 */
CodecOwner openSoftwareEncoder(RecordingQuality quality, QString *error,
                              QSize size = QSize(1920, 1080), int fps = 30)
{
    LibavVideoEncoderRequest request;
    request.encoder = {QStringLiteral("libx264"), QStringLiteral("libx264"), false};
    request.encodedSize = size;
    request.fps = fps;
    request.quality = recordingQualityProfile(quality, request.fps);
    LibavVideoEncoderResult result;
    if (!openVideoEncoder(request, nullptr, &result, error)) return {nullptr, &releaseCodec};
    return {result.codecContext, &releaseCodec};
}

/** 读取 ctx 中名为 name 的字符串选项，返回独立字节数组。 */
QByteArray stringOption(AVCodecContext *ctx, const char *name)
{
    uint8_t *value = nullptr;
    if (av_opt_get(ctx->priv_data, name, 0, &value) < 0) return {};
    const QByteArray result(reinterpret_cast<const char *>(value));
    av_free(value);
    return result;
}

}  // namespace

class RecordingEncoderMemoryTest final : public QObject {
    Q_OBJECT

private slots:
    /** 每个用例从默认线程策略开始，不读取外部覆盖值，无参数和返回值。 */
    void init()
    {
        qunsetenv("MARK_SHOT_RECORDING_ENCODER_THREADS");
    }

    /** 实际编码器必须采用分辨率对应的并发上限，而非仅测试策略函数，无参数和返回值。 */
    void boundsProductionEncoderThreads()
    {
        if (!avcodec_find_encoder_by_name("libx264")) QSKIP("libx264 is not available");
        for (const int fps : {30, 60}) {
            for (const QSize size : {QSize(1920, 1080), QSize(3840, 2160)}) {
                QString error;
                const auto codec = openSoftwareEncoder(RecordingQuality::Balanced, &error, size, fps);
                QVERIFY2(codec != nullptr, qPrintable(error));
                QCOMPARE(codec->thread_count, recordingSoftwareEncoderThreads(size, QThread::idealThreadCount(), fps));
            }
        }
    }

    /** 显式线程数和自动模式都应传递到编码器，无参数和返回值。 */
    void appliesProductionThreadOverrides()
    {
        if (!avcodec_find_encoder_by_name("libx264")) QSKIP("libx264 is not available");
        for (const int threads : {0, 2}) {
            qputenv("MARK_SHOT_RECORDING_ENCODER_THREADS", QByteArray::number(threads));
            QString error;
            const auto codec = openSoftwareEncoder(RecordingQuality::Balanced, &error);
            QVERIFY2(codec != nullptr, qPrintable(error));
            QCOMPARE(codec->thread_count, threads);
        }
    }

    /** 高质量实际编码器保留原自动线程策略，不能用降低实时吞吐量换取内存下降，无参数和返回值。 */
    void keepsHighQualityConcurrency()
    {
        if (!avcodec_find_encoder_by_name("libx264")) QSKIP("libx264 is not available");
        for (const int fps : {30, 60}) {
            QString error;
            const auto codec = openSoftwareEncoder(RecordingQuality::High, &error, QSize(3840, 2160), fps);
            QVERIFY2(codec != nullptr, qPrintable(error));
            QCOMPARE(codec->thread_count, 0);
        }
    }

    /** 保持原质量档位、CRF 和码率前瞻配置，无参数和返回值。 */
    void keepsQualitySettings()
    {
        if (!avcodec_find_encoder_by_name("libx264")) QSKIP("libx264 is not available");
        for (const int fps : {30, 60}) {
            for (auto quality : {RecordingQuality::Efficient, RecordingQuality::Balanced, RecordingQuality::High}) {
                QString error;
                const auto codec = openSoftwareEncoder(quality, &error, QSize(1920, 1080), fps);
                QVERIFY2(codec != nullptr, qPrintable(error));
                const auto expected = recordingQualityProfile(quality, fps);
                QCOMPARE(stringOption(codec.get(), "preset"), expected.softwarePreset.toUtf8());
                QVERIFY(stringOption(codec.get(), "x264-params").isEmpty());
                double crf = 0;
                QCOMPARE(av_opt_get_double(codec->priv_data, "crf", 0, &crf), 0);
                QCOMPARE(crf, static_cast<double>(expected.constantQuality));
                int64_t lookahead = 0;
                QCOMPARE(av_opt_get_int(codec->priv_data, "rc-lookahead", 0, &lookahead), 0);
                QCOMPARE(lookahead, int64_t(-1));
                QCOMPARE(codec->max_b_frames, 0);
            }
        }
    }
};

QTEST_APPLESS_MAIN(RecordingEncoderMemoryTest)
#include "recording_encoder_memory_test.moc"
