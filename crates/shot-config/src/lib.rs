//! 配置默认值、兼容读取与显式路径存储；不查询 HOME 或平台配置目录。

#![forbid(unsafe_code)]

mod atomic;
pub mod defaults;
mod error;
pub mod export;
pub mod store;
pub mod validation;

pub use error::{Error, Result};
pub const MAX_JSON_BYTES: usize = 8 * 1024 * 1024;
