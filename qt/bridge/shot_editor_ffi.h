#pragma once
#include "shot_bytes_ffi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ShotHistory ShotHistory;
typedef struct {
    size_t undo_count, redo_count;
    int32_t pending_context, pending;
} ShotHistoryInfo;

/** 按 limit 创建独立历史，返回句柄；有效容量为 1 至 4096，失败时返回空指针。 */
ShotHistory *shot_history_create(size_t limit);
/** 释放 handle 一次，空指针无操作，无返回值。 */
void shot_history_destroy(ShotHistory *handle);
/** 将 handle 的栈数量和滚轮事务写入 output，返回状态码。 */
int32_t shot_history_info(ShotHistory *handle, ShotHistoryInfo *output);
/** 查询 handle 的 context 是否需要新快照到 needed，返回状态码，不修改会话。 */
int32_t shot_history_needs_snapshot(ShotHistory *handle, int32_t context, int32_t *needed);
/** 清除 handle 的全部历史和待提交事务，返回状态码。 */
int32_t shot_history_clear(ShotHistory *handle);
/** 保存 before 快照 JSON 到 handle，返回状态码。 */
int32_t shot_history_checkpoint(ShotHistory *handle, ShotBytesView before);
/** 合并 handle 中 context 的滚轮编辑，before 为首次修改前快照，返回状态码。 */
int32_t shot_history_queue(ShotHistory *handle, int32_t context, ShotBytesView before);
/** 提交 handle 的待合并滚轮事务，返回状态码。 */
int32_t shot_history_commit(ShotHistory *handle);
/** 保存 current 快照并切换 handle，redo 为 0 或 1；output 接收目标 JSON，返回状态码。
 * 无目标时 output 为空；错误时历史不变。输入视图只同步借用，输出用 shot_bytes_destroy 释放。
 */
int32_t shot_history_step(ShotHistory *handle, ShotBytesView current, int32_t redo, ShotBytes **output);
/** 从显式 UTF-8 path 读取完整样式 JSON 到 output，缺失或损坏时使用默认值，返回状态码。 */
int32_t shot_annotation_state_load(ShotBytesView path, ShotBytes **output);
/** 将完整 style JSON 原子写入显式 UTF-8 path，返回状态码。 */
int32_t shot_annotation_state_save(ShotBytesView path, ShotBytesView style);

#ifdef __cplusplus
}
#endif
