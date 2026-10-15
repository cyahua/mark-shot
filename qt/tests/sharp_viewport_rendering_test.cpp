#include "shot_window_internal.h"

#include <QtTest/QtTest>

namespace markshot::test {
/** 使用旧版算法将 source 的 sourceRect 重采样为 targetSize，返回参考图像。 */
QImage renderSharpViewportReference(const QImage &source, const QRectF &sourceRect, const QSize &targetSize);
}

class SharpViewportRenderingTest final : public QObject {
    Q_OBJECT

private slots:
    /** 组合透明度、像素格式、分数裁剪和跨分块尺寸，无参数和返回值。 */
    void preservesReferencePixels_data()
    {
        QTest::addColumn<int>("format");
        QTest::addColumn<QSize>("sourceSize");
        QTest::addColumn<QRectF>("sourceRect");
        QTest::addColumn<QSize>("targetSize");
        const QList<QSize> sources{{37, 29}, {37, 29}, {37, 29}, {37, 29}, {641, 487}, {173, 257}};
        const QList<QRectF> crops{{0, 0, 37, 29}, {2.25, 1.75, 21.4, 20.1},
                                   {-5, -4, 43, 36}, {3.15, 4.05, 1.7, 1.3},
                                   {0, 0, 641, 487}, {1.25, 2.75, 160.3, 249.8}};
        const QList<QSize> targets{{257, 193}, {191, 145}, {11, 8}, {21, 19}, {331, 259}, {121, 129}};
        for (const auto format : {QImage::Format_RGB32, QImage::Format_ARGB32,
                                  QImage::Format_ARGB32_Premultiplied, QImage::Format_RGBA8888,
                                  QImage::Format_Grayscale8}) {
            for (int crop = 0; crop < crops.size(); ++crop) {
                const QByteArray name = QByteArray::number(format) + '-' + QByteArray::number(crop);
                QTest::newRow(name.constData()) << int(format) << sources[crop] << crops[crop] << targets[crop];
            }
        }
    }

    /** 逐像素比较旧版输出，分块、并行和格式复用不能改变颜色或透明度，无参数和返回值。 */
    void preservesReferencePixels()
    {
        QFETCH(int, format);
        QFETCH(QSize, sourceSize);
        QFETCH(QRectF, sourceRect);
        QFETCH(QSize, targetSize);
        QImage input(sourceSize, QImage::Format_ARGB32);
        for (int y = 0; y < input.height(); ++y) {
            for (int x = 0; x < input.width(); ++x) {
                input.setPixel(x, y, qRgba((x * 17 + y * 3) & 255, (x * 7 + y * 13) & 255,
                                           (x * 11 + y * 5) & 255, (x * 9 + y * 31) & 255));
            }
        }
        input = input.convertToFormat(static_cast<QImage::Format>(format));
        const QImage original = input.copy();
        const QImage expected = markshot::test::renderSharpViewportReference(input, sourceRect, targetSize);
        QCOMPARE(markshot::shot::renderSharpViewport(input, sourceRect, targetSize), expected);
        QCOMPARE(input, original);
    }

    /** 空图、空尺寸和完全越界裁剪应返回空图，无参数和返回值。 */
    void rejectsInvalidGeometry()
    {
        const QImage input(16, 16, QImage::Format_RGB32);
        QVERIFY(markshot::shot::renderSharpViewport({}, QRectF(0, 0, 10, 10), QSize(20, 20)).isNull());
        QVERIFY(markshot::shot::renderSharpViewport(input, {}, QSize(20, 20)).isNull());
        QVERIFY(markshot::shot::renderSharpViewport(input, QRectF(0, 0, 10, 10), {}).isNull());
        QVERIFY(markshot::shot::renderSharpViewport(input, QRectF(20, 20, 10, 10), QSize(20, 20)).isNull());
    }
};

QTEST_GUILESS_MAIN(SharpViewportRenderingTest)
#include "sharp_viewport_rendering_test.moc"
