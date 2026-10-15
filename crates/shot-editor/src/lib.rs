//! 可跨界面使用的标注数据、编辑历史及工具样式持久化。

#![forbid(unsafe_code)]

pub mod annotation;
pub mod colors;
pub mod history;
pub mod state;
pub mod style;

pub use annotation::{Annotation, Snapshot};
pub use history::History;
pub use style::AnnotationStyle;
