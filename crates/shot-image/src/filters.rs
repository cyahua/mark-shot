use crate::buffer::{allocate, pixel_count};
use crate::{Image, ImageError, ImageView};

#[derive(Clone, Copy, Debug)]
pub enum Filter {
    Grayscale,
    Invert,
    Brighten(f64),
}

/// 将直通道 value 乘以 alpha，使用 Qt 相同的 8 位舍入公式，返回预乘通道。
fn premultiply(value: u32, alpha: u32) -> u32 {
    let product = value * alpha + 128;
    (product + (product >> 8)) >> 8
}

/// 【图像核心】【像素滤镜】对直通 ARGB source 应用 filter；返回预乘 ARGB 图像或参数错误。
pub fn apply(source: ImageView<'_>, filter: Filter) -> Result<Image, ImageError> {
    if matches!(filter, Filter::Brighten(amount) if !amount.is_finite()) {
        return Err(ImageError::Parameter);
    }
    let mut pixels = allocate(pixel_count(source.width(), source.height())?)?;
    for y in 0..source.height() {
        for (x, &pixel) in source.row(y).iter().enumerate() {
            let alpha = pixel >> 24;
            let rgb = [(pixel >> 16) & 255, (pixel >> 8) & 255, pixel & 255];
            let mapped = match filter {
                Filter::Grayscale => {
                    let gray = (rgb[0] * 11 + rgb[1] * 16 + rgb[2] * 5) / 32;
                    [gray; 3]
                }
                Filter::Invert => rgb.map(|c| 255 - c),
                Filter::Brighten(amount) => rgb.map(|c| {
                    (f64::from(c) + f64::from(255 - c) * amount.clamp(0.0, 1.0)).round() as u32
                }),
            }
            .map(|c| premultiply(c, alpha));
            pixels[y as usize * source.width() as usize + x] =
                (alpha << 24) | (mapped[0] << 16) | (mapped[1] << 8) | mapped[2];
        }
    }
    Image::from_pixels(source.width(), source.height(), pixels)
}

/// 把工具强度 strength 转为模糊半径，返回 1 至 48，非有限值使用最小值。
pub fn blur_radius(strength: f64) -> i32 {
    if strength.is_finite() {
        (strength.round() as i32).clamp(1, 48)
    } else {
        1
    }
}

/// 把工具强度 strength 转为提亮比例，返回 0.05 至 0.9，非有限值使用最小值。
pub fn brightness_amount(strength: f64) -> f64 {
    if strength.is_finite() {
        (strength / 60.0).clamp(0.05, 0.9)
    } else {
        0.05
    }
}
