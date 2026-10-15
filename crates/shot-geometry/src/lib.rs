//! 截图与标注的纯几何计算，不依赖 Qt 或桌面平台。

#![forbid(unsafe_code)]

pub mod capture;
pub mod line;
pub mod selection;
pub mod types;

pub use types::{Point, Rect, RectF, Screen, Size};
