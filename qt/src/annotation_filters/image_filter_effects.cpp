#include "annotation_filters/image_filter_effects.h"
#include "image_bridge.h"
#include <QColorSpace>
namespace markshot::annotation_filters
{
namespace
{
/**
 * 【标注】【Rust滤镜】转换 Qt 像素格式并调用 Rust 图像核心
 * @param source 输入图像
 * @param kind 滤镜编号
 * @param amount 模糊半径或提亮比例
 * @return Qt 所有的预乘图像；接口失败返回空图像
 */
QImage filtered(const QImage &source, int kind, qreal amount)
{
    if (source.isNull())
    {
        return {};
    }
    const QImage input =
        source.convertToFormat(kind == 0 ? QImage::Format_ARGB32_Premultiplied : QImage::Format_ARGB32);
    QImage result(source.size(), QImage::Format_ARGB32_Premultiplied);
    if (result.isNull() || !rust::succeeded(shot_image_filter(rust::imageView(input), kind, amount,
                                                              reinterpret_cast<uint32_t *>(result.bits()),
                                                              static_cast<size_t>(result.sizeInBytes() / 4))))
    {
        return {};
    }
    result.setDevicePixelRatio(source.devicePixelRatio());
    result.setDotsPerMeterX(source.dotsPerMeterX());
    result.setDotsPerMeterY(source.dotsPerMeterY());
    result.setColorSpace(source.colorSpace());
    return result;
}
} // namespace
/** 对 source 应用 radius 半径的 Rust 模糊，返回预乘图像。 */
QImage gaussianBlurred(const QImage &source, int radius) { return filtered(source, 0, radius); }
/** 将 source 转为灰度，返回保留透明度的预乘图像。 */
QImage grayscaled(const QImage &source) { return filtered(source, 1, 0); }
/** 对 source 反色，返回保留透明度的预乘图像。 */
QImage inverted(const QImage &source) { return filtered(source, 2, 0); }
/** 按 amount 提亮 source，返回预乘图像。 */
QImage brightened(const QImage &source, qreal amount) { return filtered(source, 3, amount); }
/** 将 strength 转为 Rust 核心约束的模糊半径，返回像素数。 */
int blurRadiusForStrength(qreal strength) { return shot_blur_radius(strength); }
/** 将 strength 转为 Rust 核心约束的提亮比例，返回比例值。 */
qreal brightnessAmountForStrength(qreal strength) { return shot_brightness_amount(strength); }
} // namespace markshot::annotation_filters
