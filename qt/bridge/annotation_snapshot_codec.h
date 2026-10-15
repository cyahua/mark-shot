#pragma once

#include "shot_window_types.h"

#include <QByteArray>
#include <optional>

namespace markshot::rust {

/** 将 snapshot 的全部标注与选择状态编码成版本化 JSON，返回独立字节。 */
QByteArray encodeSnapshot(const shot::types::HistorySnapshot &snapshot);

/** 将 bytes 中的 Rust 快照转换为 Qt 数据，返回完整快照或协议错误空值。 */
std::optional<shot::types::HistorySnapshot> decodeSnapshot(const QByteArray &bytes);

} // namespace markshot::rust
