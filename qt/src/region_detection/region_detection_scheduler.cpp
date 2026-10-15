#include "region_detection/region_detection_scheduler.h"

#include "region_detection/region_detector.h"

#include <QCoreApplication>
#include <QGlobalStatic>
#include <QThread>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <utility>

namespace markshot::region_detection {
namespace {

struct RegionDetectionScheduler {
    QThreadPool pool;

    /** 【选区识别】【并发预算】复用最多三个分析线程，兼顾常见多屏截图与临时内存预算，无参数和返回值。 */
    RegionDetectionScheduler()
    {
        pool.setMaxThreadCount(std::clamp(QThread::idealThreadCount(), 1, 3));
        // 1. 【选区识别】【退出收尾】在 Qt 清理全局资源前结束像素任务，正常窗口切换不清空线程池
        if (auto *application = QCoreApplication::instance()) {
            QObject::connect(application, &QCoreApplication::aboutToQuit, &pool,
                             [this] { pool.waitForDone(); }, Qt::DirectConnection);
        }
    }
};

Q_GLOBAL_STATIC(RegionDetectionScheduler, scheduler)

}  // namespace

QFuture<std::shared_ptr<RegionDetector>> scheduleRegionDetection(QImage image)
{
    return QtConcurrent::run(&scheduler()->pool, [image = std::move(image)] {
        return std::make_shared<RegionDetector>(image);
    });
}

bool regionDetectionBusy()
{
    return scheduler.exists() && scheduler->pool.activeThreadCount() > 0;
}

}  // namespace markshot::region_detection
