#pragma once
#include "shot_ffi.h"
#include <QImage>
#include <memory>
namespace markshot::scroll
{
enum class ScrollAxis
{
    Vertical,
    Horizontal
};
struct StitchConfig
{
    int minOverlap;
    float acceptDiff;
    int minAppend;
    float approxDiff;
};
/** 返回与 Rust 核心一致的默认配置，无参数。 */
StitchConfig defaultConfig();
enum class StitchStatus
{
    FirstFrame,
    Appended,
    NoProgress,
    NoMatch
};
enum class StitchEdge
{
    None,
    Start,
    End
};
struct StitchResult
{
    StitchStatus status = StitchStatus::NoMatch;
    int added = 0;
    StitchEdge edge = StitchEdge::None;
    int position = 0;
    int frameLength = 0;
};
struct StitchStats
{
    int frameCount = 0;
    int totalHeight = 0;
    int lastAppend = 0;
};

/// 【长截图】【Qt适配】仅负责像素格式、句柄生命周期和显示缓存，算法状态全部位于 Rust
class Stitcher
{
  public:
    /** 根据 config 创建 Rust 拼接会话，无返回值。 */
    explicit Stitcher(StitchConfig config);
    /** 向核心提交 frame，返回业务拼接结果。 */
    StitchResult pushFrame(const QImage &frame);
    /** 返回 Qt 所有的长图快照，无参数。 */
    QImage fullImage() const;
    /** 返回核心统计，无参数。 */
    StitchStats stats() const;
    /** 设置 axis；首次增长后不再切换，无返回值。 */
    void setAxis(ScrollAxis axis);
    /** 返回当前滚动轴，无参数。 */
    ScrollAxis axis() const;
    /** 返回滚动轴是否锁定，无参数。 */
    bool axisLocked() const;

  private:
    struct Deleter
    {
        /** 释放 handle，必须为本实例拥有的唯一 Rust 句柄，无返回值。 */
        void operator()(ShotStitch *handle) const { shot_stitch_destroy(handle); }
    };
    /** 查询核心会话信息，无参数，失败返回零值。 */
    ShotStitchInfo info() const;
    std::unique_ptr<ShotStitch, Deleter> m_handle;
    mutable QImage m_cache;
    mutable uint64_t m_cacheRevision = 0;
    mutable int m_cacheAxis = -1;
};
} // namespace markshot::scroll
