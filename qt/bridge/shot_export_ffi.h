#pragma once
#include "shot_bytes_ffi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int32_t enabled, padding, shadow_radius, shadow_offset_y;
    double corner_radius, shadow_opacity;
    uint32_t shadow_color, reserved;
} ShotExportConfig;

/** 将 root JSON 配置解析到 output，返回状态码；reserved 必须保留为零。 */
int32_t shot_export_config(ShotBytesView root, ShotExportConfig *output);
/** 按 source 物理像素尺寸和 config 计算 output 尺寸，返回状态码。 */
int32_t shot_export_size(ShotSize source, ShotExportConfig config, ShotSize *output);
/** 将预乘 source 的外框结果写入 output，len 为像素容量，返回状态码；输入输出不得重叠。 */
int32_t shot_image_export(ShotImageView source, ShotExportConfig config, uint32_t *output, size_t len);

#ifdef __cplusplus
}
static_assert(sizeof(ShotExportConfig) == 40, "ShotExportConfig ABI");
#endif
