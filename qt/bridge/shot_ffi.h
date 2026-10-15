#pragma once
#include <stddef.h>
#include <stdint.h>
/** 宿主编译时使用的 ABI 版本；启动入口与契约测试必须共用该值。 */
#define SHOT_ABI_VERSION UINT32_C(2)
#ifdef __cplusplus
extern "C"
{
#endif
    /* 像素整数均为 0xAARRGGBB，stride、len 的单位为 uint32_t 像素，不是字节。
     * 宿主负责输入/输出缓冲区；Rust 不在返回后保留输入指针。
     * 指针必须对齐并指向声明长度的有效内存，输出不得与活动输入或句柄重叠。
     * 句柄由对应 create/destroy 配对管理，同一句柄不得并发访问。
     * 状态码：0 成功，1 参数/业务错误，2 已隔离的 Rust panic。
     */
    typedef struct
    {
        int32_t x, y, width, height;
    } ShotRect;
    typedef struct
    {
        int32_t width, height;
    } ShotSize;
    typedef struct
    {
        double x, y;
    } ShotPoint;
    typedef struct
    {
        double x, y, width, height;
    } ShotRectF;
    typedef struct
    {
        ShotRect geometry;
        double dpr;
    } ShotScreen;
    typedef struct
    {
        const uint32_t *pixels;
        size_t len;
        int32_t width, height;
        size_t stride;
    } ShotImageView;
    typedef struct ShotStitch ShotStitch;
    typedef struct ShotFixedRegions ShotFixedRegions;
    typedef struct
    {
        int32_t min_overlap;
        float accept_diff;
        int32_t min_append;
        float approx_diff;
    } ShotStitchConfig;
    typedef struct
    {
        int32_t status, added, edge, position, frame_length;
    } ShotStitchResult;
    typedef struct
    {
        int32_t width, height, frame_count, total_height, last_append, axis, axis_locked;
        uint64_t revision;
    } ShotStitchInfo;
    typedef struct
    {
        int32_t header, footer, valid, top, bottom;
    } ShotFixedObservation;
    /** 返回 ABI 版本，无参数。 */
    uint32_t shot_abi_version(void);
    /** 复制线程错误到 buffer，capacity 为字节容量，返回含终止符的所需容量。 */
    size_t shot_last_error(uint8_t *buffer, size_t capacity);
    /** 按 mode 换算 requested；source 为逻辑范围、size 为图像尺寸，返回矩形。 */
    ShotRect shot_geometry_transform(int32_t mode, ShotRect requested, ShotRect source, ShotSize size);
    /** 根据 screens 的 count 项布局定位 target，raw 为物理尺寸，返回裁剪范围。 */
    ShotRect shot_screen_crop(const ShotScreen *screens, size_t count, ShotRect target, ShotSize raw);
    /** 根据 anchor、pointer、ratio、size 返回比例选区。 */
    ShotRectF shot_selection_create(ShotPoint anchor, ShotPoint pointer, double ratio, ShotSize size);
    /** 根据 handle、adjusted、ratio 和 size 调整 before，返回新选区。 */
    ShotRectF shot_selection_adjust(ShotRectF before, int32_t handle, ShotRectF adjusted, double ratio,
                                    ShotSize size);
    /** 将 current 调整为 requested 尺寸并限制在 image 内，返回新选区。 */
    ShotRectF shot_selection_resize(ShotRectF current, ShotSize requested, ShotSize image);
    /** 根据 value、ratio 换算另一边，from_width 非零表示宽度，返回像素尺寸。 */
    int32_t shot_selection_side(int32_t value, double ratio, int32_t from_width);
    /** 把 start、end 构成的线段吸附到 45 度方向，返回终点。 */
    ShotPoint shot_line_end(ShotPoint start, ShotPoint end);
    /** 将 source 的 kind 滤镜结果写入 output，len 为目标像素容量，返回状态码。
     * kind: 0=模糊(预乘输入)、1=灰度、2=反色、3=提亮(后三者为直通输入)。
     * amount 为模糊半径或提亮比例；输出始终为预乘 ARGB。
     */
    int32_t shot_image_filter(ShotImageView source, int32_t kind, double amount, uint32_t *output,
                              size_t len);
    /** 将 strength 换算为模糊半径，返回 1 至 48。 */
    int32_t shot_blur_radius(double strength);
    /** 将 strength 换算为提亮比例，返回 0.05 至 0.9。 */
    double shot_brightness_amount(double strength);
    /** 按 config 创建会话，返回独占句柄，失败返回空指针。 */
    ShotStitch *shot_stitch_create(ShotStitchConfig config);
    /** 释放 handle 一次；空指针无操作，无返回值。 */
    void shot_stitch_destroy(ShotStitch *handle);
    /** 向 handle 提交预乘 frame 并写入 output，返回接口状态码。
     * output.status: 0=首帧、1=新增、2=无进展、3=无匹配；edge: 0=无、1=头部、2=尾部。
     */
    int32_t shot_stitch_push(ShotStitch *handle, ShotImageView frame, ShotStitchResult *output);
    /** 查询 handle 的统计到 output，返回接口状态码。 */
    int32_t shot_stitch_info(ShotStitch *handle, ShotStitchInfo *output);
    /** 设置 handle 的 axis，0=垂直、1=水平；首次增长后忽略切换，返回状态码。 */
    int32_t shot_stitch_set_axis(ShotStitch *handle, int32_t axis);
    /** 把 handle 的图像复制到 output 的 len 个像素空间，返回接口状态码。 */
    int32_t shot_stitch_copy_image(ShotStitch *handle, uint32_t *output, size_t len);
    /** 创建固定区域检测器，无参数，返回独占句柄。 */
    ShotFixedRegions *shot_fixed_create(void);
    /** 释放 handle 一次；空指针无操作，无返回值。 */
    void shot_fixed_destroy(ShotFixedRegions *handle);
    /** 重置 handle，返回接口状态码。 */
    int32_t shot_fixed_reset(ShotFixedRegions *handle);
    /** 在 handle 中比较 previous、current 的 offset 位移并写入 output，返回状态码。 */
    int32_t shot_fixed_observe(ShotFixedRegions *handle, ShotImageView previous, ShotImageView current,
                               int32_t offset, ShotFixedObservation *output);
#ifdef __cplusplus
}
static_assert(sizeof(ShotRect) == 16, "ShotRect ABI");
static_assert(sizeof(ShotScreen) == 24, "ShotScreen ABI");
static_assert(sizeof(ShotStitchConfig) == 16, "ShotStitchConfig ABI");
static_assert(sizeof(ShotStitchResult) == 20, "ShotStitchResult ABI");
#endif
