#include "region_detection/image_gray.h"

namespace markshot::region_detection {
namespace {

/** 检查 image 的每个像素是否完全不透明，返回能否直接读取预乘 RGB 通道。 */
bool opaquePremultipliedImage(const QImage &image)
{
    if (image.format() != QImage::Format_ARGB32_Premultiplied) return false;
    for (int y = 0; y < image.height(); ++y) {
        const auto *row = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(row[x]) != 255) return false;
        }
    }
    return true;
}

}  // namespace

QImage normalizedRgbImage(const QImage &image)
{
    if (image.isNull()) {
        return {};
    }
    // 1. 【截图】【分析内存】不透明截图的预乘通道与原始 RGB 相同，复用像素而不是复制整屏
    if (image.format() == QImage::Format_ARGB32 || image.format() == QImage::Format_RGB32
        || opaquePremultipliedImage(image)) {
        return image;
    }
    return image.convertToFormat(QImage::Format_ARGB32);
}

GrayImage grayscaleFromRgba(const QImage &image)
{
    const QImage source = normalizedRgbImage(image);
    GrayImage gray = GrayImage::create(source.width(), source.height());
    if (gray.width == 0 || gray.height == 0) {
        return gray;
    }

    // 1. 使用 image 库相同的 BT.601 定点权重，避免和参考实现出现阈值偏差
    quint8 *dst = gray.pixels.data();
    for (int y = 0; y < gray.height; ++y) {
        const QRgb *row = reinterpret_cast<const QRgb *>(source.constScanLine(y));
        for (int x = 0; x < gray.width; ++x) {
            const int r = qRed(row[x]);
            const int g = qGreen(row[x]);
            const int b = qBlue(row[x]);
            dst[y * gray.width + x] = static_cast<quint8>((19595 * r + 38470 * g + 7471 * b) >> 16);
        }
    }
    return gray;
}

} // namespace markshot::region_detection
