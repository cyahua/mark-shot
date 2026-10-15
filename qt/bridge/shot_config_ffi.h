#pragma once
#include "shot_bytes_ffi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int32_t boolean_valid, boolean_value, integer_valid, integer_value;
} ShotConfigScalar;

/** 根据 context JSON 生成默认配置到 output，返回状态码。 */
int32_t shot_config_defaults(ShotBytesView context, ShotBytes **output);
/** 从显式 UTF-8 path 读取配置到 output，返回状态码，不回退其他路径。 */
int32_t shot_config_read(ShotBytesView path, ShotBytes **output);
/** 将 root JSON 原子写入显式 UTF-8 path，返回状态码。 */
int32_t shot_config_write(ShotBytesView path, ShotBytesView root);
/** 仅在 path 缺失时写入 defaults JSON；created 返回是否新建，函数返回状态码。 */
int32_t shot_config_ensure(ShotBytesView path, ShotBytesView defaults, int32_t *created);
/** 根据 change JSON 的 path/value/remove 更新 path 文件，返回状态码。 */
int32_t shot_config_patch(ShotBytesView path, ShotBytesView change);
/** 在内存 root JSON 应用 change JSON，返回新 JSON 到 output，函数返回状态码。 */
int32_t shot_config_update(ShotBytesView root, ShotBytesView change, ShotBytes **output);
/** 读取 value JSON 的兼容布尔值与整数到 output，返回状态码。 */
int32_t shot_config_scalar(ShotBytesView value, ShotConfigScalar *output);
/** 归一化 UTF-8 key 到 output，输出为 UTF-8 原文，返回状态码。 */
int32_t shot_config_normalize_key(ShotBytesView key, ShotBytes **output);

#ifdef __cplusplus
}
static_assert(sizeof(ShotConfigScalar) == 16, "ShotConfigScalar ABI");
#endif
