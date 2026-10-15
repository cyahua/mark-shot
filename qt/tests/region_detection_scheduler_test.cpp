#include "region_detection/region_detection_scheduler.h"
#include "region_detection/region_detector.h"

#include <QPainter>
#include <QElapsedTimer>
#include <QTimer>
#include <QtConcurrent/QtConcurrentRun>
#include <QtTest/QtTest>

class RegionDetectionSchedulerTest final : public QObject {
    Q_OBJECT

private slots:
    /** 【选区测量】【并发耗时】对比三个同时提交的原尺寸请求，不设依赖机器速度的测试门槛，无参数和返回值。 */
    void concurrentBatchLatency_data()
    {
        QTest::addColumn<bool>("dedicated");
        QTest::newRow("global") << false;
        QTest::newRow("dedicated") << true;
    }

    /** 【选区测量】【并发耗时】记录整批完成时间，并确认并发请求均返回正确图像尺寸，无参数和返回值。 */
    void concurrentBatchLatency()
    {
        using namespace markshot::region_detection;
        QFETCH(bool, dedicated);
        QImage input(3840, 2160, QImage::Format_RGB32);
        input.fill(QColor(225, 230, 237));
        {
            QPainter painter(&input);
            for (int y = 100; y < input.height(); y += 150) {
                for (int x = 40; x < input.width(); x += 260) {
                    painter.fillRect(x, y, 220, 110, Qt::white);
                    painter.fillRect(x + 12, y + 16, 120, 8, QColor(60, 80, 105));
                }
            }
        }
        QElapsedTimer timer;
        timer.start();
        QList<QFuture<std::shared_ptr<RegionDetector>>> jobs;
        for (int i = 0; i < 3; ++i) {
            jobs.append(dedicated ? scheduleRegionDetection(input)
                                  : QtConcurrent::run([input] { return std::make_shared<RegionDetector>(input); }));
        }
        for (auto &job : jobs) {
            job.waitForFinished();
            QCOMPARE(job.result()->bounds().size(), input.size());
        }
        qInfo("【选区测量】【并发耗时】queue=%s images=3 elapsed_ms=%lld",
              dedicated ? "dedicated" : "global", static_cast<long long>(timer.elapsed()));
    }

    /** 【选区测试】【排队分析】多个请求保留共享原图，结果与同步分析逐项一致，无参数和返回值。 */
    void preservesQueuedResultsAndImageLifetime()
    {
        using namespace markshot::region_detection;
        QTRY_VERIFY(!regionDetectionBusy());
        QImage input(640, 480, QImage::Format_RGB32);
        input.fill(Qt::white);
        {
            QPainter painter(&input);
            painter.fillRect(60, 70, 180, 90, QColor(50, 70, 90));
            painter.fillRect(300, 200, 230, 170, QColor(130, 150, 180));
        }
        const RegionDetector expected(input);
        QList<QFuture<std::shared_ptr<RegionDetector>>> jobs;
        for (int i = 0; i < 4; ++i) jobs.append(scheduleRegionDetection(input));
        input = {};
        for (auto &job : jobs) {
            job.waitForFinished();
            const auto result = job.result();
            QVERIFY(result);
            QCOMPARE(result->bounds(), expected.bounds());
            QCOMPARE(result->candidates().size(), expected.candidates().size());
            for (int i = 0; i < expected.candidates().size(); ++i) {
                QCOMPARE(result->candidates()[i].rect, expected.candidates()[i].rect);
                QCOMPARE(result->candidates()[i].quality, expected.candidates()[i].quality);
            }
        }
        QTRY_VERIFY(!regionDetectionBusy());
    }

    /** 【选区测试】【退出收尾】主事件循环退出前等待后台像素任务结束，无参数和返回值。 */
    void drainsBeforeApplicationQuit()
    {
        using namespace markshot::region_detection;
        QImage input(1280, 720, QImage::Format_RGB32);
        input.fill(Qt::white);
        QList<QFuture<std::shared_ptr<RegionDetector>>> jobs;
        for (int i = 0; i < 4; ++i) jobs.append(scheduleRegionDetection(input));
        QTimer::singleShot(0, QCoreApplication::instance(), &QCoreApplication::quit);
        QCoreApplication::exec();
        bool finished = true;
        for (auto &job : jobs) finished &= job.isFinished();
        for (auto &job : jobs) job.waitForFinished();
        QVERIFY(finished);
    }
};

QTEST_GUILESS_MAIN(RegionDetectionSchedulerTest)
#include "region_detection_scheduler_test.moc"
