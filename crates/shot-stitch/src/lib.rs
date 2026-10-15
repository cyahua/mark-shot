//! 双向滚动截图拼接；所有状态、采样与像素缓冲均由 Rust 管理。

#![forbid(unsafe_code)]

mod adjacent;
mod algorithm;
mod comparison;
pub mod fixed_regions;
mod line_matching;
mod long_image;
mod matching;
mod motion_mask;
pub mod profile;
mod session;
mod types;

pub use session::Stitcher;
pub use types::{Axis, Config, Edge, Stats, Status, StitchResult};
