#include "memory/idle_memory_trimmer.h"
#include "memory/process_memory_trim.h"

#include <QtTest/QtTest>
#include <QScopeGuard>
#include <QSemaphore>
#include <QThreadPool>

#include <memory>

using markshot::memory::IdleMemoryTrimmer;

class IdleMemoryTrimmerTest : public QObject {
    Q_OBJECT

private:
    /**
     * 创建回收器，计数回收次数并可控制忙碌状态。
     * @param trims 回收计数输出。
     * @param busy 忙碌状态。
     * @return 回收器实例。
     */
    std::unique_ptr<IdleMemoryTrimmer> createTrimmer(int *trims, bool *busy)
    {
        IdleMemoryTrimmer::Options options;
        options.idleDelayMs = 20;
        options.busy = [busy] { return *busy; };
        options.trim = [trims] { ++(*trims); };
        auto trimmer = std::make_unique<IdleMemoryTrimmer>(std::move(options));
        trimmer->handleApplicationStateChanged(Qt::ApplicationActive);
        return trimmer;
    }

private slots:
    /** 验证后台图像分析进行时不回收工作集，无参数和返回值。 */
    void backgroundWorkDefersTrim()
    {
        QSemaphore release;
        const auto cleanup = qScopeGuard([&release] {
            release.release();
            QThreadPool::globalInstance()->waitForDone();
        });
        QThreadPool::globalInstance()->start([&release] { release.acquire(); });
        int trims = 0;
        bool busy = false;
        auto trimmer = createTrimmer(&trims, &busy);
        trimmer->handleApplicationStateChanged(Qt::ApplicationInactive);
        QTest::qWait(80);
        QCOMPARE(trims, 0);
        release.release();
        QTRY_COMPARE_WITH_TIMEOUT(trims, 1, 1000);
    }

    /** 验证较短的空闲间隔不会舍入成零并立即触发，无参数和返回值。 */
    void respectsQuietDelay()
    {
        int trims = 0;
        IdleMemoryTrimmer::Options options;
        options.idleDelayMs = 150;
        options.trim = [&trims] { ++trims; };
        IdleMemoryTrimmer trimmer(std::move(options));
        trimmer.handleApplicationStateChanged(Qt::ApplicationActive);
        trimmer.handleApplicationStateChanged(Qt::ApplicationInactive);
        QTest::qWait(40);
        QCOMPARE(trims, 0);
        QTRY_COMPARE_WITH_TIMEOUT(trims, 1, 1000);
    }

    /**
     * 验证非活动持续到期后只回收一次，回到前台后可再次回收。
     * @return 无返回值。
     */
    void trimsOncePerIdlePeriod()
    {
        int trims = 0;
        bool busy = false;
        auto trimmer = createTrimmer(&trims, &busy);
        QVERIFY(!trimmer->pending());

        // 1. 进入非活动后开始计时，到期回收一次
        trimmer->handleApplicationStateChanged(Qt::ApplicationInactive);
        QVERIFY(trimmer->pending());
        QTRY_COMPARE_WITH_TIMEOUT(trims, 1, 1000);

        // 2. 同一空闲期内的状态切换不再重复回收
        trimmer->handleApplicationStateChanged(Qt::ApplicationHidden);
        QVERIFY(!trimmer->pending());
        QTest::qWait(60);
        QCOMPARE(trims, 1);

        // 3. 回到前台后再次空闲时重新回收
        trimmer->handleApplicationStateChanged(Qt::ApplicationActive);
        trimmer->handleApplicationStateChanged(Qt::ApplicationInactive);
        QTRY_COMPARE_WITH_TIMEOUT(trims, 2, 1000);
    }

    /**
     * 验证计时期间回到前台会取消回收。
     * @return 无返回值。
     */
    void activationCancelsPendingTrim()
    {
        int trims = 0;
        bool busy = false;
        auto trimmer = createTrimmer(&trims, &busy);
        trimmer->handleApplicationStateChanged(Qt::ApplicationInactive);
        trimmer->handleApplicationStateChanged(Qt::ApplicationActive);
        QVERIFY(!trimmer->pending());
        QTest::qWait(60);
        QCOMPARE(trims, 0);
    }

    /**
     * 验证忙碌期间推迟回收，忙碌结束后执行。
     * @return 无返回值。
     */
    void busyDefersTrim()
    {
        int trims = 0;
        bool busy = true;
        auto trimmer = createTrimmer(&trims, &busy);
        trimmer->handleApplicationStateChanged(Qt::ApplicationInactive);
        QTest::qWait(80);
        QCOMPARE(trims, 0);
        QVERIFY(trimmer->pending());

        busy = false;
        QTRY_COMPARE_WITH_TIMEOUT(trims, 1, 1000);
    }

    /**
     * 验证平台回收调用安全且常驻内存读数有效。
     * @return 无返回值。
     */
    void platformTrimIsSafe()
    {
        markshot::memory::trimProcessMemory();
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
        QVERIFY(markshot::memory::residentMemoryBytes() > 0);
#endif
    }
};

QTEST_MAIN(IdleMemoryTrimmerTest)
#include "idle_memory_trimmer_test.moc"
