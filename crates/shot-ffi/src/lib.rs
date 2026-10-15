//! Qt 只链接此静态库；所有 unsafe 代码限定在这一边界内。

pub mod annotation_state;
mod boundary;
pub mod bytes;
pub mod config;
pub mod editor_history;
pub mod export;
pub mod fixed_regions;
pub mod geometry;
pub mod image;
pub mod stitch;

/// 返回 C ABI 版本；无参数，宿主必须在创建会话前验证版本。
#[unsafe(no_mangle)]
pub extern "C" fn shot_abi_version() -> u32 {
    2
}

/// 复制当前线程错误消息到 buffer；capacity 含终止符，返回所需容量，零容量可查询长度。
/// # Safety
/// buffer 必须在 capacity 非零时指向可写字节区；不得与其他活动借用重叠。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_last_error(buffer: *mut u8, capacity: usize) -> usize {
    boundary::ERROR.with(|error| {
        let error = error.borrow();
        let bytes = error.as_bytes();
        if !buffer.is_null() && capacity > 0 && capacity <= isize::MAX as usize {
            let count = bytes.len().min(capacity - 1);
            // SAFETY: 【Rust接口】【错误输出】宿主提供 capacity 字节可写空间，写入量不超过该范围
            unsafe {
                std::ptr::copy_nonoverlapping(bytes.as_ptr(), buffer, count);
                *buffer.add(count) = 0;
            }
        }
        bytes.len() + 1
    })
}
