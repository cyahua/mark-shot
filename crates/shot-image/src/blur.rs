use crate::buffer::allocate;
use crate::{Image, ImageError, ImageView};

/// 根据高斯标准差 sigma 计算三次盒式模糊半径，返回长度为 3 的半径数组。
pub(crate) fn radii(sigma: f64) -> [i32; 3] {
    let mut lower = (4.0 * sigma * sigma + 1.0).sqrt().floor() as i32;
    if lower % 2 == 0 {
        lower -= 1;
    }
    lower = lower.max(1);
    let count =
        ((12.0 * sigma * sigma - 3.0 * f64::from(lower).powi(2) - 12.0 * f64::from(lower) - 9.0)
            / (-4.0 * f64::from(lower) - 4.0))
            .round() as i32;
    std::array::from_fn(|i| ((if (i as i32) < count { lower } else { lower + 2 }) - 1) / 2)
}

/// 在 source 与 target 之间执行一次盒式模糊；size 为宽高，radius 为半径，horizontal 指定方向，无返回值。
fn box_pass(source: &[u32], target: &mut [u32], size: (i32, i32), radius: i32, horizontal: bool) {
    let (width, height) = size;
    let (lines, length) = if horizontal {
        (height, width)
    } else {
        (width, height)
    };
    let window = i64::from(radius * 2 + 1);
    let index = |line: i32, position: i32| -> usize {
        if horizontal {
            line as usize * width as usize + position as usize
        } else {
            position as usize * width as usize + line as usize
        }
    };
    for line in 0..lines {
        // 1. 【图像核心】【盒式模糊】边缘按夹取扩展，通道在预乘空间内累加
        let mut sum = [0_i64; 4];
        for offset in -radius..=radius {
            let pixel = source[index(line, offset.clamp(0, length - 1))];
            for (channel, total) in sum.iter_mut().enumerate() {
                *total += i64::from((pixel >> (channel * 8)) & 255);
            }
        }
        // 2. 【图像核心】【盒式模糊】每次只移入和移出一个像素，避免重复遍历窗口
        for position in 0..length {
            target[index(line, position)] =
                sum.iter().enumerate().fold(0, |pixel, (channel, total)| {
                    pixel | ((*total / window) as u32) << (channel * 8)
                });
            let incoming = source[index(line, (position + radius + 1).min(length - 1))];
            let outgoing = source[index(line, (position - radius).max(0))];
            for (channel, total) in sum.iter_mut().enumerate() {
                *total += i64::from((incoming >> (channel * 8)) & 255)
                    - i64::from((outgoing >> (channel * 8)) & 255);
            }
        }
    }
}

/// 【图像核心】【高斯模糊】模糊预乘 ARGB source；radius 小于 1 时复制原图，返回结果或参数/分配错误。
pub fn gaussian(source: ImageView<'_>, radius: i32) -> Result<Image, ImageError> {
    if radius > 4096 {
        return Err(ImageError::Parameter);
    }
    let image = source.to_image()?;
    if radius < 1 || source.width() < 2 || source.height() < 2 {
        return Ok(image);
    }
    let mut buffer = allocate(image.pixels().len())?;
    buffer.copy_from_slice(image.pixels());
    let mut scratch = allocate(buffer.len())?;
    for radius in radii((f64::from(radius) / 2.0).max(1.0)) {
        if radius < 1 {
            continue;
        }
        box_pass(
            &buffer,
            &mut scratch,
            (source.width(), source.height()),
            radius,
            true,
        );
        box_pass(
            &scratch,
            &mut buffer,
            (source.width(), source.height()),
            radius,
            false,
        );
    }
    Image::from_pixels(source.width(), source.height(), buffer)
}
