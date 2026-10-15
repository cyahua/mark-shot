#include "shot_window_line_constraint.h"
#include "geometry_bridge.h"
namespace markshot::shot
{
/** 将 start、end 交给 Rust 线段约束，返回吸附后的终点。 */
QPointF constrainedLineEnd(QPointF start, QPointF end)
{
    return rust::fromCore(shot_line_end(rust::toCore(start), rust::toCore(end)));
}
} // namespace markshot::shot
