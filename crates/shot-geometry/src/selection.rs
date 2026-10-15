use crate::{Point, RectF, Size};

/// 与 Qt SelectionDrag 中选区手柄的稳定编号对应，其他编号不施加比例约束。
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Handle {
    Left,
    Right,
    Top,
    Bottom,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    Other,
}

impl From<i32> for Handle {
    /// 根据 Qt 手柄编号 value 返回领域手柄；未知编号返回 Other。
    fn from(value: i32) -> Self {
        match value {
            10 => Self::Left,
            11 => Self::Right,
            12 => Self::Top,
            13 => Self::Bottom,
            14 => Self::TopLeft,
            15 => Self::TopRight,
            16 => Self::BottomLeft,
            17 => Self::BottomRight,
            _ => Self::Other,
        }
    }
}

/// 按 anchor、pointer 创建宽高比为 ratio 的选区并限制在 size 内；非法比例返回自由矩形。
pub fn create(anchor: Point, pointer: Point, ratio: f64, size: Size) -> RectF {
    if !anchor.finite() || !pointer.finite() {
        return RectF::default();
    }
    if !ratio.is_finite() || ratio <= 0.0 {
        return RectF::from_corners(anchor, pointer);
    }
    let sx = if pointer.x >= anchor.x { 1.0 } else { -1.0 };
    let sy = if pointer.y >= anchor.y { 1.0 } else { -1.0 };
    let mut width = (pointer.x - anchor.x)
        .abs()
        .max((pointer.y - anchor.y).abs() * ratio);
    let mut height = width / ratio;
    let available_width = if sx > 0.0 {
        f64::from(size.width) - anchor.x
    } else {
        anchor.x
    };
    let available_height = if sy > 0.0 {
        f64::from(size.height) - anchor.y
    } else {
        anchor.y
    };
    if width > available_width {
        width = available_width.max(0.0);
        height = width / ratio;
    }
    if height > available_height {
        height = available_height.max(0.0);
        width = height * ratio;
    }
    RectF::from_corners(
        anchor,
        Point {
            x: anchor.x + sx * width,
            y: anchor.y + sy * height,
        },
    )
}

/// 按 handle 将 before 调整到 adjusted；ratio 为比例、size 为边界，结果过小时返回原选区。
pub fn adjust(before: RectF, handle: Handle, adjusted: RectF, ratio: f64, size: Size) -> RectF {
    if !before.finite() || !adjusted.finite() || size.width <= 0 || size.height <= 0 {
        return before;
    }
    if !ratio.is_finite() || ratio <= 0.0 || handle == Handle::Other {
        return adjusted;
    }
    let br = before.bottom_right();
    let ar = adjusted.bottom_right();
    let corners = match handle {
        Handle::TopLeft => Some((
            br,
            Point {
                x: adjusted.x,
                y: adjusted.y,
            },
        )),
        Handle::TopRight => Some((
            Point {
                x: before.x,
                y: br.y,
            },
            Point {
                x: ar.x,
                y: adjusted.y,
            },
        )),
        Handle::BottomLeft => Some((
            Point {
                x: br.x,
                y: before.y,
            },
            Point {
                x: adjusted.x,
                y: ar.y,
            },
        )),
        Handle::BottomRight => Some((
            Point {
                x: before.x,
                y: before.y,
            },
            ar,
        )),
        _ => None,
    };
    let result = if let Some((anchor, pointer)) = corners {
        create(anchor, pointer, ratio, size)
    } else if matches!(handle, Handle::Left | Handle::Right) {
        let mut width = adjusted.width;
        let mut height = width / ratio;
        if height > f64::from(size.height) {
            height = f64::from(size.height);
            width = height * ratio;
        }
        RectF {
            x: if handle == Handle::Right {
                before.x
            } else {
                br.x - width
            },
            y: (before.y + before.height / 2.0 - height / 2.0)
                .clamp(0.0, (f64::from(size.height) - height).max(0.0)),
            width,
            height,
        }
    } else {
        let mut height = adjusted.height;
        let mut width = height * ratio;
        if width > f64::from(size.width) {
            width = f64::from(size.width);
            height = width / ratio;
        }
        RectF {
            x: (before.x + before.width / 2.0 - width / 2.0)
                .clamp(0.0, (f64::from(size.width) - width).max(0.0)),
            y: if handle == Handle::Bottom {
                before.y
            } else {
                br.y - height
            },
            width,
            height,
        }
    };
    if !result.finite() || result.width < 8.0 || result.height < 8.0 {
        before
    } else {
        result
    }
}

/// 以 current 左上角设置 requested 尺寸并限制在 image 内；图像无效时返回空选区。
pub fn resize(current: RectF, requested: Size, image: Size) -> RectF {
    if !current.finite() || image.width <= 0 || image.height <= 0 {
        return RectF::default();
    }
    let width = f64::from(requested.width.clamp(1, image.width));
    let height = f64::from(requested.height.clamp(1, image.height));
    let current = current.normalized();
    RectF {
        x: current.x.clamp(0.0, f64::from(image.width) - width),
        y: current.y.clamp(0.0, f64::from(image.height) - height),
        width,
        height,
    }
}

/// 根据宽或高 value 计算另一边；ratio 是宽高比，from_width 指定输入类型，非法比例返回 0。
pub fn related_side(value: i32, ratio: f64, from_width: bool) -> i32 {
    if !ratio.is_finite() || ratio <= 0.0 {
        return 0;
    }
    let result = if from_width {
        f64::from(value) / ratio
    } else {
        f64::from(value) * ratio
    };
    (result.round() as i32).max(1)
}
