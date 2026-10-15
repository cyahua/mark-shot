#include "recording/backend/backend_recording_writer.h"
#include "recording/backend/recording_backend_loader.h"
#include "recording/recording_async_writer.h"

#include <QFile>
#include <QImageReader>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using namespace markshot::recording;

class RecordingBackendTest final : public QObject {
    Q_OBJECT

private slots:
    /** 未使用录制功能或已销毁写出器时不保留后端模块，无参数和返回值。 */
    void cleanup()
    {
#ifdef Q_OS_LINUX
        QFile maps(QStringLiteral("/proc/self/maps"));
        QVERIFY(maps.open(QIODevice::ReadOnly));
        QVERIFY2(!maps.readAll().contains("libmark-shot-recording-libav.so"),
                 "Recording backend remains mapped after its last writer is destroyed");
#endif
    }

    /** 固定使用软件编码，避免测试依赖显卡和驱动，无参数和返回值。 */
    void initTestCase()
    {
        qputenv("MARK_SHOT_RECORDING_SW_ENCODER", "1");
    }

    /** 缺少后端时返回明确错误而不是崩溃，无参数和返回值。 */
    void missingBackendIsReported()
    {
        QTemporaryDir directory;
        QString error;
        const auto writer = createBackendRecordingWriter({}, &error,
            directory.filePath(QStringLiteral("missing-backend")));
        QVERIFY(!writer);
        QVERIFY(!error.isEmpty());
    }

    /** 拒绝版本不匹配的后端，无参数和返回值。 */
    void incompatibleBackendIsRejected()
    {
        QString error;
        const auto library = RecordingBackendLibrary::load(&error,
            QString::fromUtf8(MARK_SHOT_BAD_BACKEND_PATH));
        QVERIFY(!library);
        QVERIFY(error.contains(QStringLiteral("version")));
    }

    /** 一个写出器销毁后另一个仍可继续使用，最后正常解码 GIF，无参数和返回值。 */
    void concurrentLeasesKeepBackendAlive()
    {
        QTemporaryDir directory;
        RecordingOptions options;
        options.outputPath = directory.filePath(QStringLiteral("sample.gif"));
        QString error;
        auto first = createBackendRecordingWriter(options, &error);
        QVERIFY2(first != nullptr, qPrintable(error));
        auto second = createBackendRecordingWriter(options, &error);
        QVERIFY2(second != nullptr, qPrintable(error));
        first.reset();

        QVERIFY2(second->start(QSize(32, 24), 10, &error), qPrintable(error));
        for (int index = 0; index < 3; ++index) {
            RecordingFrameSample sample;
            sample.image = QImage(32, 24, QImage::Format_ARGB32);
            sample.image.fill(QColor(50 + index * 60, 100, 140));
            sample.timestampMs = index * 100;
            QVERIFY2(second->writeFrame(sample, &error), qPrintable(error));
        }
        QVERIFY2(second->finish(&error), qPrintable(error));
        second.reset();
        QImageReader reader(options.outputPath);
        QCOMPARE(reader.read().size(), QSize(32, 24));
        QVERIFY(reader.imageCount() >= 2);
    }

    /** 后端错误不能在模块卸载后继续引用只读段，无参数和返回值。 */
    void errorOutlivesBackend()
    {
        QTemporaryDir directory;
        RecordingOptions options;
        options.outputPath = directory.filePath(QStringLiteral("invalid.gif"));
        QString error;
        auto writer = createBackendRecordingWriter(options, &error);
        QVERIFY2(writer != nullptr, qPrintable(error));
        QVERIFY(!writer->start({}, 10, &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(error.isDetached());
        const QString expected(error.constData(), error.size());
        writer.reset();
        QCOMPARE(error, expected);
    }

    /** 覆盖应用支持的两种视频容器，无参数和返回值。 */
    void asyncWriterProducesVideo_data()
    {
        QTest::addColumn<int>("container");
        QTest::addColumn<QString>("extension");
        QTest::newRow("mp4") << static_cast<int>(RecordingContainerFormat::Mp4) << QStringLiteral("mp4");
        QTest::newRow("mkv") << static_cast<int>(RecordingContainerFormat::Mkv) << QStringLiteral("mkv");
    }

    /** 通过生产异步线程入口写出视频并释放后端，无参数和返回值。 */
    void asyncWriterProducesVideo()
    {
        QFETCH(int, container);
        QFETCH(QString, extension);
        QTemporaryDir directory;
        RecordingOptions options;
        options.mode = RecordingMode::Video;
        options.container = static_cast<RecordingContainerFormat>(container);
        options.outputPath = directory.filePath(QStringLiteral("sample.") + extension);
        QString error;
        {
            RecordingAsyncWriter writer(options);
            QVERIFY2(writer.start(QSize(32, 24), 10, &error), qPrintable(error));
            for (int index = 0; index < 4; ++index) {
                RecordingFrameSample sample;
                sample.image = QImage(32, 24, QImage::Format_ARGB32);
                sample.image.fill(QColor(40 + index * 40, 90, 120));
                sample.timestampMs = index * 100;
                QVERIFY2(writer.writeFrame(sample, &error), qPrintable(error));
            }
            QVERIFY2(writer.finish(&error), qPrintable(error));
        }
        QFile output(options.outputPath);
        QVERIFY(output.open(QIODevice::ReadOnly));
        const QByteArray bytes = output.readAll();
        QVERIFY(bytes.size() > 100);
        if (options.container == RecordingContainerFormat::Mp4) {
            QVERIFY(bytes.contains("ftyp"));
            QVERIFY(bytes.contains("mdat"));
        } else {
            QVERIFY(bytes.startsWith(QByteArray::fromHex("1a45dfa3")));
        }
    }
};

QTEST_GUILESS_MAIN(RecordingBackendTest)
#include "recording_backend_test.moc"
