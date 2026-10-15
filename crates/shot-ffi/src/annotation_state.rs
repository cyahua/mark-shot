use shot_config::MAX_JSON_BYTES;
use shot_editor::{AnnotationStyle, state};

use crate::boundary;
use crate::bytes::{self, ShotBytes, ShotBytesView};

/// 从显式 UTF-8 path 读取工具样式到 output，返回状态码；文件缺失或损坏时返回默认样式。
/// # Safety
/// path 必须引用有效字节，output 必须指向独立可写的空句柄。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_annotation_state_load(
    path: ShotBytesView,
    output: *mut *mut ShotBytes,
) -> i32 {
    boundary::run(|| {
        boundary::validate(output)?;
        // SAFETY: 【Rust接口】【样式读取】路径在调用期间复制，不查找其他配置目录
        unsafe {
            let style = state::load(&bytes::path(path)?);
            bytes::publish(output, bytes::encode(&style)?)
        }
    })
}

/// 将完整 style JSON 原子写入显式 UTF-8 path，返回状态码。
/// # Safety
/// path、style 必须引用调用期间有效且不可变的字节。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_annotation_state_save(
    path: ShotBytesView,
    style: ShotBytesView,
) -> i32 {
    boundary::run(|| {
        // SAFETY: 【Rust接口】【样式保存】完整解析类型后才允许文件写入
        unsafe {
            let style: AnnotationStyle = bytes::json(style, MAX_JSON_BYTES)?;
            state::save(&bytes::path(path)?, &style).map_err(|error| error.to_string())
        }
    })
}
