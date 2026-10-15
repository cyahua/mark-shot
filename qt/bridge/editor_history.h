#pragma once

#include "shot_editor_ffi.h"
#include "shot_window_types.h"

#include <memory>
#include <optional>

namespace markshot::rust {

/// Qt 仅转换当前画布数据；历史快照、容量与合并事务由 Rust 会话独占管理。
class EditorHistory final {
public:
    /** 创建独立历史会话，无参数、无返回值；容量沿用原实现的 100 项。 */
    EditorHistory();
    /** 读取当前历史数量和待提交上下文，无参数，返回状态结构。 */
    ShotHistoryInfo info() const;
    /** 清空会话及待提交事务，无参数、无返回值。 */
    void clear();
    /** 保存 snapshot 为编辑前状态，返回是否成功。 */
    bool checkpoint(const shot::types::HistorySnapshot &snapshot);
    /** 为 context 合并首个 snapshot，返回是否成功。 */
    bool queue(int context, const shot::types::HistorySnapshot &snapshot);
    /** 提交待合并事务，无参数、无返回值。 */
    void commit();
    /** 保存 current 并按 redo 方向切换，返回目标快照或空值。 */
    std::optional<shot::types::HistorySnapshot> step(const shot::types::HistorySnapshot &current, bool redo);

private:
    std::unique_ptr<ShotHistory, decltype(&shot_history_destroy)> m_handle;
};

} // namespace markshot::rust
