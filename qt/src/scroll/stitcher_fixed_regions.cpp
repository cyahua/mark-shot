#include "scroll/stitcher_fixed_regions.h"
#include "image_bridge.h"
namespace markshot::scroll
{
/** 创建 Rust 检测器，无参数；失败记录错误。 */
FixedRegionDetector::FixedRegionDetector() : m_handle(shot_fixed_create())
{
    if (!m_handle)
    {
        rust::succeeded(1);
    }
}
/** 清空 Rust 检测器与本地展示结果，无参数、无返回值。 */
void FixedRegionDetector::reset()
{
    if (m_handle && rust::succeeded(shot_fixed_reset(m_handle.get())))
    {
        m_latest = {};
    }
}
/** 比较 prevFrame、curFrame 的 offset 位移，返回 Rust 固定带候选。 */
FixedRegionDetector::Observation FixedRegionDetector::observe(const QImage &prevFrame, const QImage &curFrame,
                                                              int offset)
{
    if (!m_handle || prevFrame.isNull() || curFrame.isNull())
    {
        return {};
    }
    const auto previous = prevFrame.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    const auto current = curFrame.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    if (!rust::succeeded(shot_fixed_observe(m_handle.get(), rust::imageView(previous),
                                            rust::imageView(current), offset, &m_latest)))
    {
        return {};
    }
    return {m_latest.header, m_latest.footer, m_latest.valid != 0};
}
} // namespace markshot::scroll
