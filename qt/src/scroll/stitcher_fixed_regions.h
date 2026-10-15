#pragma once
#include "shot_ffi.h"
#include <QImage>
#include <memory>
namespace markshot::scroll
{
class FixedRegionDetector
{
  public:
    struct Observation
    {
        int header = 0;
        int footer = 0;
        bool valid = false;
    };
    /** 创建独立 Rust 检测器，无参数、无返回值。 */
    FixedRegionDetector();
    /** 清除观察状态，无参数、无返回值。 */
    void reset();
    /** 比较 prevFrame、curFrame 的 offset 位移，返回本次固定带候选。 */
    Observation observe(const QImage &prevFrame, const QImage &curFrame, int offset);
    /** 返回稳定顶部固定带高度，无参数。 */
    int matchTopIgnore() const { return m_latest.top; }
    /** 返回稳定底部固定带高度，无参数。 */
    int matchBottomIgnore() const { return m_latest.bottom; }

  private:
    struct Deleter
    {
        /** 释放本实例独占的 handle，无返回值。 */
        void operator()(ShotFixedRegions *handle) const { shot_fixed_destroy(handle); }
    };
    std::unique_ptr<ShotFixedRegions, Deleter> m_handle;
    ShotFixedObservation m_latest{};
};
} // namespace markshot::scroll
