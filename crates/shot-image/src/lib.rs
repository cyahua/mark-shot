//! 原生整数 ARGB 像素运算；不依赖 Qt，格式转换由边界适配层负责。

#![forbid(unsafe_code)]

pub mod blur;
pub mod buffer;
pub mod export;
pub mod filters;

pub use buffer::{Image, ImageError, ImageView, MAX_PIXELS};
