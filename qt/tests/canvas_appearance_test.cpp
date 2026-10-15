#include "ui/transparency_background.h"
#include "ui/canvas_scrollbar_style.h"

#include <QPainter>
#include <QScrollBar>
#include <QtTest>

namespace {

/// @brief 记录收到离开事件次数的画布替身
class LeaveCounter final : public QWidget {
public:
    int leaves = 0;

protected:
    /// @brief 统计离开事件后交给基类
    /// @param event 控件事件
    /// @return 基类处理结果
    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::Leave) {
            ++leaves;
        }
        return QWidget::event(event);
    }
};

/**
 * 统计滚动条截图中滑块的厚度，截图先叠加到黑底上以排除半透明轨道
 * @param bar 已显示的水平滚动条
 * @return 中间列上亮度达到滑块水平的行数
 */
int handleThickness(QScrollBar &bar)
{
    QImage image(bar.size(), QImage::Format_ARGB32);
    image.fill(Qt::black);
    QPainter painter(&image);
    painter.drawPixmap(0, 0, bar.grab());
    painter.end();
    const int column = image.width() / 2;
    int rows = 0;
    for (int y = 0; y < image.height(); ++y) {
        if (qRed(image.pixel(column, y)) > 90) {
            ++rows;
        }
    }
    return rows;
}

}  // namespace

/// @brief 图片编辑视图的透明背景与滚动条回归测试
class CanvasAppearanceTest : public QObject {
    Q_OBJECT

private slots:
    /** @brief 验证棋盘格颜色、周期和低对比度；无参数，无返回值 */
    void checkerboardRepeats()
    {
        QImage image(72, 72, QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        markshot::ui::paintTransparencyBackground(painter, image.rect());
        painter.end();
        QCOMPARE(image.pixelColor(1, 1), QColor(60, 64, 72));
        QCOMPARE(image.pixelColor(13, 1), QColor(51, 55, 62));
        QCOMPARE(image.pixelColor(25, 1), image.pixelColor(1, 1));
        QCOMPARE(image.pixelColor(13, 13), image.pixelColor(1, 1));
    }

    /** @brief 验证局部重绘网格与完整重绘对齐；无参数，无返回值 */
    void partialPaintKeepsOrigin()
    {
        QImage image(72, 72, QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        markshot::ui::paintTransparencyBackground(painter, QRect(13, 7, 30, 30));
        painter.end();
        QCOMPARE(image.pixelColor(13, 7), QColor(51, 55, 62));
        QCOMPARE(image.pixelColor(25, 7), QColor(60, 64, 72));
        QCOMPARE(image.pixelColor(0, 0).alpha(), 0);
    }

    /** @brief 验证细窄样式保留滚动条范围与数值操作；无参数，无返回值 */
    void scrollBarRemainsInteractive()
    {
        QScrollBar bar(Qt::Horizontal);
        bar.setStyleSheet(markshot::ui::canvasScrollBarStyleSheet(14));
        bar.resize(240, 14);
        bar.setRange(0, 100);
        bar.setPageStep(20);
        bar.setValue(50);
        bar.triggerAction(QAbstractSlider::SliderSingleStepAdd);
        QCOMPARE(bar.value(), 51);
        QCOMPARE(bar.minimum(), 0);
        QCOMPARE(bar.maximum(), 100);
    }

    /** @brief 验证画布滚动条的光标、悬停加宽及对画布悬停的结束；无参数，无返回值 */
    void scrollBarHoverEndsCanvasHover()
    {
        LeaveCounter canvas;
        canvas.setCursor(Qt::CrossCursor);
        canvas.resize(320, 120);
        auto *bar = new QScrollBar(Qt::Horizontal, &canvas);
        markshot::ui::setupCanvasScrollBar(bar, 14);
        bar->setGeometry(0, 106, 320, 14);
        bar->setRange(0, 10);
        bar->setPageStep(90);
        bar->setValue(5);
        canvas.show();
        QVERIFY(QTest::qWaitForWindowExposed(&canvas));

        // 1. 【画布滚动条】【光标】不继承画布十字光标
        QCOMPARE(bar->cursor().shape(), Qt::ArrowCursor);
        const int idle = handleThickness(*bar);
        QVERIFY(idle > 0);

        // 2. 【画布滚动条】【悬停】进入时加宽滑块，并让画布收到离开事件
        QEnterEvent enter(QPointF(160, 7), QPointF(160, 7), bar->mapToGlobal(QPointF(160, 7)));
        QCoreApplication::sendEvent(bar, &enter);
        QVERIFY(bar->property("canvasHover").toBool());
        QCOMPARE(canvas.leaves, 1);
        QVERIFY(handleThickness(*bar) > idle);

        // 3. 【画布滚动条】【悬停】离开后恢复细滑块
        QEvent leave(QEvent::Leave);
        QCoreApplication::sendEvent(bar, &leave);
        QVERIFY(!bar->property("canvasHover").toBool());
        QCOMPARE(handleThickness(*bar), idle);
    }
};

QTEST_MAIN(CanvasAppearanceTest)
#include "canvas_appearance_test.moc"
