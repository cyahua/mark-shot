#include "region_detection/image_gray.h"

#include <QtTest/QtTest>

using namespace markshot::region_detection;

class RegionImageMemoryTest final : public QObject {
    Q_OBJECT

private slots:
    /** 不透明 4K 截图无需再分配整帧副本，无参数和返回值。 */
    void opaqueScreenshotSharesPixels()
    {
        QImage image(3840, 2160, QImage::Format_ARGB32_Premultiplied);
        image.fill(QColor(50, 100, 150));
        const QImage normalized = normalizedRgbImage(image);
        QCOMPARE(normalized.constBits(), image.constBits());
        QCOMPARE(normalized.size(), image.size());
    }

    /** 透明像素仍按原规则解除预乘，不能把存储通道直接当作颜色，无参数和返回值。 */
    void translucentPixelsKeepStraightColors()
    {
        QImage image(257, 2, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::white);
        for (int alpha = 0; alpha <= 255; ++alpha) {
            image.setPixelColor(alpha, 1, QColor(60, 130, 220, alpha));
        }
        const QImage expected = image.convertToFormat(QImage::Format_ARGB32);
        const QImage normalized = normalizedRgbImage(image);
        QCOMPARE(normalized, expected);
        QCOMPARE(grayscaleFromRgba(image).pixels, grayscaleFromRgba(expected).pixels);
        QCOMPARE(image.format(), QImage::Format_ARGB32_Premultiplied);
    }

    /** 非 32 位格式和空图继续使用安全转换，无参数和返回值。 */
    void otherFormatsRemainSupported()
    {
        QVERIFY(normalizedRgbImage({}).isNull());
        QImage image(8, 4, QImage::Format_RGB888);
        image.fill(QColor(40, 90, 150));
        const QImage normalized = normalizedRgbImage(image);
        QCOMPARE(normalized.format(), QImage::Format_ARGB32);
        QCOMPARE(normalized.pixelColor(0, 0), image.pixelColor(0, 0));
    }
};

QTEST_GUILESS_MAIN(RegionImageMemoryTest)
#include "region_image_memory_test.moc"
