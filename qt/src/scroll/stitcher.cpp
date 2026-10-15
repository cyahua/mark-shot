#include "scroll/stitcher.h"
#include "debug_log.h"
#include "image_bridge.h"
namespace markshot::scroll
{
/** 返回默认拼接阈值，无参数。 */
StitchConfig defaultConfig() { return {100, 9.0f, 15, 1.0f}; }
/** 以 config 创建 Rust 句柄；失败记录错误，不退回旧算法。 */
Stitcher::Stitcher(StitchConfig config)
    : m_handle(
          shot_stitch_create({config.minOverlap, config.acceptDiff, config.minAppend, config.approxDiff}))
{
    if (!m_handle)
    {
        rust::succeeded(1);
    }
}
/** 将 frame 归一化为预乘像素后提交核心，返回拼接结果。 */
StitchResult Stitcher::pushFrame(const QImage &frame)
{
    if (frame.isNull() || !m_handle)
    {
        return {};
    }
    const QImage input = frame.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    ShotStitchResult result{};
    if (!rust::succeeded(shot_stitch_push(m_handle.get(), rust::imageView(input), &result)))
    {
        return {};
    }
    markshot::debugLog("stitch", "【长截图】【Rust拼接】status=%d added=%d edge=%d position=%d frame=%d",
                       result.status, result.added, result.edge, result.position, result.frame_length);
    return {static_cast<StitchStatus>(result.status), result.added, static_cast<StitchEdge>(result.edge),
            result.position, result.frame_length};
}
/** 查询核心状态，返回统计与缓存修订号，无参数。 */
ShotStitchInfo Stitcher::info() const
{
    ShotStitchInfo result{};
    if (m_handle)
    {
        rust::succeeded(shot_stitch_info(m_handle.get(), &result));
    }
    return result;
}
/** 按 Rust 修订号缓存 Qt 像素副本，返回长图，无参数。 */
QImage Stitcher::fullImage() const
{
    const auto state = info();
    if (state.width <= 0 || state.height <= 0)
    {
        return {};
    }
    if (m_cache.isNull() || state.revision != m_cacheRevision || state.axis != m_cacheAxis)
    {
        QImage image(state.width, state.height, QImage::Format_ARGB32_Premultiplied);
        if (image.isNull() || !rust::succeeded(shot_stitch_copy_image(
                                  m_handle.get(), reinterpret_cast<uint32_t *>(image.bits()),
                                  static_cast<size_t>(image.sizeInBytes() / 4))))
        {
            return {};
        }
        image.setDevicePixelRatio(1.0);
        m_cache = std::move(image);
        m_cacheRevision = state.revision;
        m_cacheAxis = state.axis;
    }
    return m_cache;
}
/** 返回 Rust 核心统计，无参数。 */
StitchStats Stitcher::stats() const
{
    const auto state = info();
    return {state.frame_count, state.total_height, state.last_append};
}
/** 设置核心 axis，方向锁定后保持原值，无返回值。 */
void Stitcher::setAxis(ScrollAxis axis)
{
    if (m_handle)
    {
        rust::succeeded(shot_stitch_set_axis(m_handle.get(), static_cast<int>(axis)));
    }
}
/** 返回核心当前滚动轴，无参数。 */
ScrollAxis Stitcher::axis() const { return static_cast<ScrollAxis>(info().axis); }
/** 返回核心滚动轴锁定状态，无参数。 */
bool Stitcher::axisLocked() const { return info().axis_locked != 0; }
} // namespace markshot::scroll
