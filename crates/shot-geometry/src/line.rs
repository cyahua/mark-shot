use crate::Point;

/// 将从 start 到 end 的线段投影到最近的 45 度方向；非有限坐标原样返回 end。
pub fn constrained_end(start: Point, end: Point) -> Point {
    if !start.finite() || !end.finite() {
        return end;
    }
    let dx = end.x - start.x;
    let dy = end.y - start.y;
    if !dx.is_finite() || !dy.is_finite() {
        return end;
    }
    if dx.abs() <= 1.0e-12 && dy.abs() <= 1.0e-12 {
        return end;
    }
    let step = std::f64::consts::FRAC_PI_4;
    let angle = (dy.atan2(dx) / step).round() * step;
    let (sine, cosine) = angle.sin_cos();
    let projected = dx * cosine + dy * sine;
    Point {
        x: start.x + cosine * projected,
        y: start.y + sine * projected,
    }
}
