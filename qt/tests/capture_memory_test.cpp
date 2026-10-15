#include "shot_window.h"
#include "support/isolated_config.h"

#include <QApplication>
#include <QPointer>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <atomic>
#include <memory>

namespace {

struct FrameAllocation {
    std::unique_ptr<uchar[]> pixels;
    std::shared_ptr<std::atomic_int> liveFrames;
};

/** 释放 context 中的原始帧并减少活动帧计数，无返回值。 */
void releaseFrame(void *context)
{
    const std::unique_ptr<FrameAllocation> allocation(static_cast<FrameAllocation *>(context));
    --*allocation->liveFrames;
}

/** 分配一帧 4K 图像，记录 liveFrames，返回持有自定义释放回调的图像。 */
QImage trackedFrame(const std::shared_ptr<std::atomic_int> &liveFrames)
{
    auto allocation = std::make_unique<FrameAllocation>();
    allocation->pixels = std::make_unique<uchar[]>(3840 * 2160 * 4);
    allocation->liveFrames = liveFrames;
    QImage image(allocation->pixels.get(), 3840, 2160, 3840 * 4,
                 QImage::Format_ARGB32_Premultiplied, &releaseFrame, allocation.get());
    ++*liveFrames;
    allocation.release();
    image.fill(QColor(65, 100, 145));
    return image;
}

}  // namespace

class CaptureMemoryTest final : public QObject {
    Q_OBJECT

private slots:
    /** 重复打开、关闭真实窗口，确认画布和后台识别不残留整帧引用，无参数和返回值。 */
    void repeatedCloseReleasesCapturedFrames()
    {
        const auto liveFrames = std::make_shared<std::atomic_int>(0);
        for (int iteration = 0; iteration < 6; ++iteration) {
            QPointer<ShotWindow> window = new ShotWindow(trackedFrame(liveFrames), QString());
            QCOMPARE(liveFrames->load(), 1);
            window->resize(480, 270);
            window->show();
            QVERIFY(QTest::qWaitForWindowExposed(window));
            window->close();
            QTRY_VERIFY_WITH_TIMEOUT(window.isNull(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(liveFrames->load(), 0, 10000);
        }
    }
};

/** 使用独立配置运行窗口内存测试；argc/argv 为测试参数，返回测试退出码。 */
int main(int argc, char **argv)
{
    QTemporaryDir home;
    if (!markshot::test::isolateConfig(home)) return 1;
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    CaptureMemoryTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "capture_memory_test.moc"
