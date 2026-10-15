#include "annotation_filters/image_filter_effects.h"
#include "shot_ffi.h"
#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

namespace
{
/** 根据 size 创建固定 ARGB 样本，返回与原实现摘要使用相同输入的图像。 */
QImage fixtureImage(QSize size)
{
    QImage image(size, QImage::Format_ARGB32);
    for (int y = 0; y < size.height(); ++y)
    {
        for (int x = 0; x < size.width(); ++x)
        {
            image.setPixel(x, y,
                           qRgba((x * 71 + y * 13 + 3) % 256, (x * 19 + y * 89 + 17) % 256,
                                 (x * 47 + y * 7 + 93) % 256, (x * 37 + y * 53 + 3) % 256));
        }
    }
    return image;
}

/** 对 image 按大端 ARGB 顺序计算摘要，返回不依赖宿主端序的十六进制值。 */
QByteArray digest(const QImage &image)
{
    QByteArray bytes;
    for (int y = 0; y < image.height(); ++y)
    {
        for (int x = 0; x < image.width(); ++x)
        {
            const auto pixel = image.pixel(x, y);
            for (int shift : {24, 16, 8, 0})
            {
                bytes.append(static_cast<char>((pixel >> shift) & 255));
            }
        }
    }
    return QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex();
}
} // namespace

class RustImageContractTest final : public QObject
{
    Q_OBJECT
  private slots:
    /** 验证 Qt 真正链接预期 ABI，返回值为空。 */
    void abiVersion() { QCOMPARE(shot_abi_version(), uint32_t(SHOT_ABI_VERSION)); }

    /** 验证所有固定样本与旧 C++ 模糊算法逐像素一致，返回值为空。 */
    void matchesOriginalBlurFixtures()
    {
        QFile file(QStringLiteral(MARK_SHOT_FILTER_FIXTURE_PATH));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const auto cases = QJsonDocument::fromJson(file.readAll()).array();
        QCOMPARE(cases.size(), 15);
        for (const auto &value : cases)
        {
            const auto fixture = value.toObject();
            const QSize size(fixture.value("width").toInt(), fixture.value("height").toInt());
            const auto result = markshot::annotation_filters::gaussianBlurred(
                fixtureImage(size), fixture.value("radius").toInt());
            QCOMPARE(result.size(), size);
            QCOMPARE(digest(result), fixture.value("sha256").toString().toLatin1());
        }
    }

    /** 验证全部 alpha 值与 Qt 预乘规则一致，同时保留 DPR 与原图像素，返回值为空。 */
    void matchesQtPremultiplication()
    {
        QImage source(256, 1, QImage::Format_ARGB32);
        source.setDevicePixelRatio(2.0);
        for (int alpha = 0; alpha < 256; ++alpha)
        {
            source.setPixel(alpha, 0, qRgba(alpha, 91, 203, alpha));
        }
        const QImage original = source.copy();
        const auto result = markshot::annotation_filters::inverted(source);
        QCOMPARE(result.devicePixelRatio(), 2.0);
        for (int alpha = 0; alpha < 256; ++alpha)
        {
            QCOMPARE(result.pixel(alpha, 0), qPremultiply(qRgba(255 - alpha, 164, 52, alpha)));
        }
        QCOMPARE(source, original);
    }

    /** 验证非 ARGB 输入在 Qt 边界正确转换，返回值为空。 */
    void convertsOtherQtFormats()
    {
        for (auto format : {QImage::Format_RGBA8888, QImage::Format_RGB888, QImage::Format_Grayscale8})
        {
            const auto source = fixtureImage(QSize(13, 7)).convertToFormat(format);
            const auto straight = source.convertToFormat(QImage::Format_ARGB32);
            const auto result = markshot::annotation_filters::grayscaled(source);
            for (int y = 0; y < source.height(); ++y)
            {
                for (int x = 0; x < source.width(); ++x)
                {
                    const auto pixel = straight.pixel(x, y);
                    const int gray = qGray(pixel);
                    QCOMPARE(result.pixel(x, y), qPremultiply(qRgba(gray, gray, gray, qAlpha(pixel))));
                }
            }
        }
    }
};

QTEST_GUILESS_MAIN(RustImageContractTest)
#include "rust_image_contract_test.moc"
