#include "editor_history.h"

#include "annotation_snapshot_codec.h"
#include "image_bridge.h"
#include "json_bridge.h"

namespace markshot::rust {

EditorHistory::EditorHistory()
    : m_handle(shot_history_create(100), shot_history_destroy)
{
    if (!m_handle) {
        succeeded(1);
    }
}

ShotHistoryInfo EditorHistory::info() const
{
    ShotHistoryInfo result{};
    succeeded(shot_history_info(m_handle.get(), &result));
    return result;
}

void EditorHistory::clear()
{
    succeeded(shot_history_clear(m_handle.get()));
}

bool EditorHistory::checkpoint(const shot::types::HistorySnapshot &snapshot)
{
    return succeeded(shot_history_checkpoint(m_handle.get(), byteView(encodeSnapshot(snapshot))));
}

bool EditorHistory::queue(int context, const shot::types::HistorySnapshot &snapshot)
{
    int32_t needed = 0;
    if (!succeeded(shot_history_needs_snapshot(m_handle.get(), context, &needed))) {
        return false;
    }
    if (!needed) {
        return true;
    }
    return succeeded(shot_history_queue(m_handle.get(), context, byteView(encodeSnapshot(snapshot))));
}

void EditorHistory::commit()
{
    succeeded(shot_history_commit(m_handle.get()));
}

std::optional<shot::types::HistorySnapshot> EditorHistory::step(const shot::types::HistorySnapshot &current, bool redo)
{
    ShotBytes *output = nullptr;
    if (!succeeded(shot_history_step(m_handle.get(), byteView(encodeSnapshot(current)), redo ? 1 : 0, &output))
        || !output) {
        return std::nullopt;
    }
    const auto bytes = takeBytes(output);
    const auto snapshot = bytes ? decodeSnapshot(*bytes) : std::nullopt;
    if (!snapshot) {
        qCritical() << "【编辑核心】【历史恢复】Rust 快照与 Qt 协议不一致";
    }
    return snapshot;
}

} // namespace markshot::rust
