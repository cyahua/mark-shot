use crate::boundary;
use shot_geometry::{Point, Rect, RectF, Screen, Size, capture, line, selection};

/// 根据 mode 换算 requested；source 为逻辑范围、size 为像素尺寸，返回矩形，未知模式返回空矩形。
#[unsafe(no_mangle)]
pub extern "C" fn shot_geometry_transform(
    mode: i32,
    requested: Rect,
    source: Rect,
    size: Size,
) -> Rect {
    boundary::value(Rect::default(), || {
        Ok(match mode {
            0 => capture::scaled_crop(source, requested, size),
            1 => capture::image_rect(requested, source, size),
            2 => capture::logical_rect(requested, source, size),
            _ => return Err("unknown geometry transform".into()),
        })
    })
}

/// 根据 screens 的 count 项布局定位 target，raw 为物理图像尺寸，返回屏幕裁剪矩形。
/// # Safety
/// screens 必须指向 count 个有效 Screen，调用期间不可修改。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_screen_crop(
    screens: *const Screen,
    count: usize,
    target: Rect,
    raw: Size,
) -> Rect {
    boundary::value(Rect::default(), || {
        if count > 4096 {
            return Err("screen count exceeds limit".into());
        }
        // SAFETY: 【Rust接口】【屏幕布局】宿主保证数组有效，数量已限制
        let screens = unsafe { boundary::input(screens, count)? };
        Ok(capture::per_screen_crop(screens, target, raw))
    })
}

/// 根据 anchor、pointer、ratio、size 创建比例选区，返回归一化矩形。
#[unsafe(no_mangle)]
pub extern "C" fn shot_selection_create(
    anchor: Point,
    pointer: Point,
    ratio: f64,
    size: Size,
) -> RectF {
    boundary::value(RectF::default(), || {
        Ok(selection::create(anchor, pointer, ratio, size))
    })
}
/// 将 before 按 handle、adjusted、ratio、size 调整，返回受约束的选区。
#[unsafe(no_mangle)]
pub extern "C" fn shot_selection_adjust(
    before: RectF,
    handle: i32,
    adjusted: RectF,
    ratio: f64,
    size: Size,
) -> RectF {
    boundary::value(before, || {
        Ok(selection::adjust(
            before,
            handle.into(),
            adjusted,
            ratio,
            size,
        ))
    })
}
/// 将 current 调整到 requested 尺寸并限制在 image 内，返回新选区。
#[unsafe(no_mangle)]
pub extern "C" fn shot_selection_resize(current: RectF, requested: Size, image: Size) -> RectF {
    boundary::value(RectF::default(), || {
        Ok(selection::resize(current, requested, image))
    })
}
/// 根据 value、ratio 计算另一边，from_width 非零表示输入宽度，返回整数像素尺寸。
#[unsafe(no_mangle)]
pub extern "C" fn shot_selection_side(value: i32, ratio: f64, from_width: i32) -> i32 {
    selection::related_side(value, ratio, from_width != 0)
}
/// 将 start、end 构成的线段吸附到 45 度方向，返回约束终点。
#[unsafe(no_mangle)]
pub extern "C" fn shot_line_end(start: Point, end: Point) -> Point {
    line::constrained_end(start, end)
}
