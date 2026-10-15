use crate::{Rect, Screen, Size};

/// 【截图核心】【坐标换算】把逻辑选区 requested 映射到 source 对应的图像 size，返回裁剪像素矩形。
pub fn scaled_crop(source: Rect, requested: Rect, size: Size) -> Rect {
    let overlap = requested.intersect(source);
    if !overlap.valid() || !size.bounds().valid() {
        return Rect::default();
    }
    let sx = f64::from(size.width) / f64::from(source.width);
    let sy = f64::from(size.height) / f64::from(source.height);
    let x = ((i64::from(overlap.x) - i64::from(source.x)) as f64 * sx).round() as i32;
    let y = ((i64::from(overlap.y) - i64::from(source.y)) as f64 * sy).round() as i32;
    let right = ((i64::from(overlap.x) + i64::from(overlap.width) - i64::from(source.x)) as f64
        * sx)
        .round() as i32;
    let bottom = ((i64::from(overlap.y) + i64::from(overlap.height) - i64::from(source.y)) as f64
        * sy)
        .round() as i32;
    Rect {
        x,
        y,
        width: (right - x).max(1),
        height: (bottom - y).max(1),
    }
    .intersect(size.bounds())
}

/// 将逻辑 geometry 映射为像素范围；source 无效时直接与图像 size 求交，返回像素矩形。
pub fn image_rect(geometry: Rect, source: Rect, size: Size) -> Rect {
    if source.valid() {
        scaled_crop(source, geometry, size)
    } else {
        geometry.intersect(size.bounds())
    }
}

/// 将图像选区 image 映射回 source 的逻辑坐标；size 为图像尺寸，返回受源范围约束的矩形。
pub fn logical_rect(image: Rect, source: Rect, size: Size) -> Rect {
    let clipped = image.intersect(size.bounds());
    if !clipped.valid() || !source.valid() {
        return Rect::default();
    }
    let sx = f64::from(source.width) / f64::from(size.width);
    let sy = f64::from(source.height) / f64::from(size.height);
    let x = (f64::from(clipped.x) * sx).round() as i64 + i64::from(source.x);
    let y = (f64::from(clipped.y) * sy).round() as i64 + i64::from(source.y);
    let right = ((i64::from(clipped.x) + i64::from(clipped.width)) as f64 * sx).round() as i64
        + i64::from(source.x);
    let bottom = ((i64::from(clipped.y) + i64::from(clipped.height)) as f64 * sy).round() as i64
        + i64::from(source.y);
    let (Ok(x), Ok(y)) = (i32::try_from(x), i32::try_from(y)) else {
        return Rect::default();
    };
    Rect {
        x,
        y,
        width: (right - i64::from(x)).max(1) as i32,
        height: (bottom - i64::from(y)).max(1) as i32,
    }
    .intersect(source)
}

/// 【截图核心】【混合缩放】按 screens 的行布局恢复物理拼接位置；target 为目标屏幕，返回与 raw 匹配的裁剪范围。
pub fn per_screen_crop(screens: &[Screen], target: Rect, raw: Size) -> Rect {
    let Some(index) = screens.iter().position(|s| s.geometry == target) else {
        return Rect::default();
    };
    if !target.valid()
        || !raw.bounds().valid()
        || !screens
            .iter()
            .any(|s| (s.dpr - screens[0].dpr).abs() > 0.01)
    {
        return Rect::default();
    }
    // 1. 【截图核心】【混合缩放】按逻辑垂直交集分组，保持原捕获后端的行拼接规则
    let mut rows: Vec<(i64, i64, Vec<usize>)> = Vec::new();
    for (i, screen) in screens.iter().enumerate() {
        if !screen.geometry.valid() || !screen.dpr.is_finite() || screen.dpr <= 0.0 {
            continue;
        }
        let top = i64::from(screen.geometry.y);
        let bottom = top + i64::from(screen.geometry.height);
        if let Some(row) = rows.iter_mut().find(|row| top < row.1 && row.0 < bottom) {
            row.0 = row.0.min(top);
            row.1 = row.1.max(bottom);
            row.2.push(i);
        } else {
            rows.push((top, bottom, vec![i]));
        }
    }
    rows.sort_by_key(|row| row.0);
    let mut rects = vec![Rect::default(); screens.len()];
    let mut y = 0_i64;
    let mut max_width = 0_i64;
    // 2. 【截图核心】【混合缩放】累计每屏物理尺寸，拒绝无法由输入图像解释的布局
    for (_, _, mut indices) in rows {
        indices.sort_by_key(|&i| screens[i].geometry.x);
        let mut x = 0_i64;
        let mut height = 0_i64;
        for i in indices {
            let screen = screens[i];
            let w = (f64::from(screen.geometry.width) * screen.dpr).round();
            let h = (f64::from(screen.geometry.height) * screen.dpr).round();
            if w < 1.0 || h < 1.0 || w > f64::from(i32::MAX) || h > f64::from(i32::MAX) {
                return Rect::default();
            }
            if x + w as i64 > i64::from(i32::MAX) || y + h as i64 > i64::from(i32::MAX) {
                return Rect::default();
            }
            rects[i] = Rect {
                x: x as i32,
                y: y as i32,
                width: w as i32,
                height: h as i32,
            };
            x += w as i64;
            height = height.max(h as i64);
        }
        max_width = max_width.max(x);
        y += height;
    }
    if (max_width - i64::from(raw.width)).abs() > 2 || (y - i64::from(raw.height)).abs() > 2 {
        return Rect::default();
    }
    rects[index].intersect(raw.bounds())
}
