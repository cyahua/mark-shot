use std::sync::Arc;

pub const MAX_PIXELS: usize = 128 * 1024 * 1024;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ImageError {
    Dimensions,
    Stride,
    BufferTooSmall,
    Allocation,
    Parameter,
}

impl std::fmt::Display for ImageError {
    /// 将错误 self 格式化到 f，返回格式化结果。
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(match self {
            Self::Dimensions => "invalid image dimensions or pixel limit exceeded",
            Self::Stride => "invalid image stride",
            Self::BufferTooSmall => "pixel buffer is too small",
            Self::Allocation => "image allocation failed",
            Self::Parameter => "invalid image parameter",
        })
    }
}
impl std::error::Error for ImageError {}

/// 借用像素，不拥有或延长宿主缓冲区的生命周期；stride 的单位是 u32 像素。
#[derive(Clone, Copy)]
pub struct ImageView<'a> {
    pixels: &'a [u32],
    width: i32,
    height: i32,
    stride: usize,
}

impl<'a> ImageView<'a> {
    /// 校验 pixels、width、height、stride 并创建只读视图；返回视图或布局错误。
    pub fn new(
        pixels: &'a [u32],
        width: i32,
        height: i32,
        stride: usize,
    ) -> Result<Self, ImageError> {
        pixel_count(width, height)?;
        if stride < width as usize {
            return Err(ImageError::Stride);
        }
        let needed = stride
            .checked_mul((height - 1) as usize)
            .and_then(|n| n.checked_add(width as usize))
            .ok_or(ImageError::Stride)?;
        if needed > pixels.len() {
            return Err(ImageError::BufferTooSmall);
        }
        Ok(Self {
            pixels,
            width,
            height,
            stride,
        })
    }

    /// 返回像素宽度；无参数。
    pub fn width(self) -> i32 {
        self.width
    }
    /// 返回像素高度；无参数。
    pub fn height(self) -> i32 {
        self.height
    }
    /// 读取第 y 行，不包含行尾填充；y 必须位于图像内，返回借用切片。
    pub fn row(self, y: i32) -> &'a [u32] {
        assert!(y >= 0 && y < self.height);
        let start = y as usize * self.stride;
        &self.pixels[start..start + self.width as usize]
    }

    /// 复制有效像素并去除行填充；无参数，返回独立图像或分配错误。
    pub fn to_image(self) -> Result<Image, ImageError> {
        let mut pixels = allocate(pixel_count(self.width, self.height)?)?;
        for y in 0..self.height {
            let start = y as usize * self.width as usize;
            pixels[start..start + self.width as usize].copy_from_slice(self.row(y));
        }
        Image::from_pixels(self.width, self.height, pixels)
    }
}

/// 不可变、可低成本共享的紧凑图像；像素整数布局为 0xAARRGGBB。
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Image {
    width: i32,
    height: i32,
    pixels: Arc<Vec<u32>>,
}

impl Image {
    /// 接管 pixels；width、height 必须恰好匹配，返回图像或尺寸错误。
    pub fn from_pixels(width: i32, height: i32, pixels: Vec<u32>) -> Result<Self, ImageError> {
        if pixel_count(width, height)? != pixels.len() {
            return Err(ImageError::BufferTooSmall);
        }
        Ok(Self {
            width,
            height,
            pixels: Arc::new(pixels),
        })
    }
    /// 返回图像宽度；无参数。
    pub fn width(&self) -> i32 {
        self.width
    }
    /// 返回图像高度；无参数。
    pub fn height(&self) -> i32 {
        self.height
    }
    /// 返回只读像素切片；无参数，生命周期与图像一致。
    pub fn pixels(&self) -> &[u32] {
        &self.pixels
    }
    /// 返回不含填充的只读视图；无参数。
    pub fn view(&self) -> ImageView<'_> {
        ImageView {
            pixels: &self.pixels,
            width: self.width,
            height: self.height,
            stride: self.width as usize,
        }
    }
    /// 读取第 y 行像素；y 必须位于图像内，返回只读切片。
    pub fn row(&self, y: i32) -> &[u32] {
        self.view().row(y)
    }

    /// 【图像核心】【转置】分块交换横纵坐标；无参数，返回独立图像或内存错误。
    pub fn transpose(&self) -> Result<Self, ImageError> {
        let mut output = allocate(self.pixels.len())?;
        for by in (0..self.height).step_by(32) {
            for bx in (0..self.width).step_by(32) {
                for y in by..(by + 32).min(self.height) {
                    for x in bx..(bx + 32).min(self.width) {
                        output[x as usize * self.height as usize + y as usize] =
                            self.row(y)[x as usize];
                    }
                }
            }
        }
        Self::from_pixels(self.height, self.width, output)
    }
}

/// 校验 width、height 及内存上限，返回安全像素总数或尺寸错误。
pub fn pixel_count(width: i32, height: i32) -> Result<usize, ImageError> {
    if width <= 0 || height <= 0 {
        return Err(ImageError::Dimensions);
    }
    let count = (width as usize)
        .checked_mul(height as usize)
        .ok_or(ImageError::Dimensions)?;
    if count > MAX_PIXELS {
        return Err(ImageError::Dimensions);
    }
    Ok(count)
}

/// 为 count 个像素分配零值缓冲区，返回缓冲区或显式分配错误。
pub fn allocate(count: usize) -> Result<Vec<u32>, ImageError> {
    if count > MAX_PIXELS {
        return Err(ImageError::Dimensions);
    }
    let mut pixels = Vec::new();
    pixels
        .try_reserve_exact(count)
        .map_err(|_| ImageError::Allocation)?;
    pixels.resize(count, 0);
    Ok(pixels)
}
