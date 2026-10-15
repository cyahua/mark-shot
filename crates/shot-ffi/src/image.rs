use crate::boundary::{self, Result};
use shot_image::{
    Image, ImageView, MAX_PIXELS, blur,
    filters::{self, Filter},
};

#[repr(C)]
#[derive(Clone, Copy)]
pub struct ShotImageView {
    pub pixels: *const u32,
    pub len: usize,
    pub width: i32,
    pub height: i32,
    pub stride: usize,
}

impl ShotImageView {
    /// 校验并借用视图，返回领域图像视图或错误。
    /// # Safety
    /// pixels 必须指向 len 个有效 u32，调用期间内存不可修改或释放。
    pub(crate) unsafe fn borrow(&self) -> Result<ImageView<'_>> {
        if self.len > MAX_PIXELS * 2 {
            return Err("pixel buffer length exceeds limit".into());
        }
        // SAFETY: 【Rust接口】【图像借用】调用者保证内存有效，行布局随后由领域层检查
        let pixels = unsafe { boundary::input(self.pixels, self.len)? };
        ImageView::new(pixels, self.width, self.height, self.stride).map_err(|e| e.to_string())
    }
}

/// 把 image 紧凑像素复制到 output，len 为可用像素数，返回容量或指针错误。
/// # Safety
/// output 必须拥有 len 个可写 u32，且不与 image 内存重叠。
pub(crate) unsafe fn copy_image(image: &Image, output: *mut u32, len: usize) -> Result<()> {
    boundary::validate(output)?;
    if len < image.pixels().len() {
        return Err("output pixel buffer is too small".into());
    }
    // SAFETY: 【Rust接口】【像素输出】Rust 独立分配的图像与宿主目标缓冲区不重叠，容量已检查
    unsafe {
        std::ptr::copy_nonoverlapping(image.pixels().as_ptr(), output, image.pixels().len());
    }
    Ok(())
}

/// 对 source 应用 kind 滤镜，amount 为模糊半径或提亮比例，写入 output，len 为目标像素数；返回状态码。
/// # Safety
/// source 必须是有效视图；output 必须拥有 len 个可写像素。kind=0 使用预乘输入，其他种类使用直通 ARGB。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_image_filter(
    source: ShotImageView,
    kind: i32,
    amount: f64,
    output: *mut u32,
    len: usize,
) -> i32 {
    boundary::run(|| {
        boundary::validate(output)?;
        if !amount.is_finite() {
            return Err("filter amount is not finite".into());
        }
        // SAFETY: 【Rust接口】【像素滤镜】宿主提供有效只读图像，借用不跨越当前调用
        let view = unsafe { source.borrow()? };
        if len < view.width() as usize * view.height() as usize {
            return Err("output pixel buffer is too small".into());
        }
        let image = match kind {
            0 => blur::gaussian(view, amount.round() as i32),
            1 => filters::apply(view, Filter::Grayscale),
            2 => filters::apply(view, Filter::Invert),
            3 => filters::apply(view, Filter::Brighten(amount)),
            _ => return Err("unknown image filter".into()),
        }
        .map_err(|e| e.to_string())?;
        // SAFETY: 【Rust接口】【像素滤镜】输入借用已结束，独立结果可复制到宿主缓冲区
        unsafe { copy_image(&image, output, len) }
    })
}

/// 把 strength 转为模糊半径，返回 1 至 48。
#[unsafe(no_mangle)]
pub extern "C" fn shot_blur_radius(strength: f64) -> i32 {
    filters::blur_radius(strength)
}
/// 把 strength 转为提亮比例，返回 0.05 至 0.9。
#[unsafe(no_mangle)]
pub extern "C" fn shot_brightness_amount(strength: f64) -> f64 {
    filters::brightness_amount(strength)
}
