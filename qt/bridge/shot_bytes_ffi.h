#pragma once
#include "shot_ffi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const uint8_t *data;
    size_t len;
} ShotBytesView;
typedef struct ShotBytes ShotBytes;

/** 借用 handle 内容到 output，返回状态码；视图在 destroy 前有效且禁止修改。 */
int32_t shot_bytes_view(const ShotBytes *handle, ShotBytesView *output);
/** 释放 handle 一次，空指针无操作，无返回值。 */
void shot_bytes_destroy(ShotBytes *handle);

#ifdef __cplusplus
}
#endif
