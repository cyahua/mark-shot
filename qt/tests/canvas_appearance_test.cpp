#include "ui/transparency_background.h"
#include "ui/canvas_scrollbar_style.h"

#include <QPainter>
#include <QScrollBar>
#include <QtTest>

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
};

QTEST_MAIN(CanvasAppearanceTest)
#include "canvas_appearance_test.moc"
