use std::path::PathBuf;

use serde::{Serialize, de::DeserializeOwned};

use crate::boundary::{self, Result};

#[repr(C)]
#[derive(Clone, Copy)]
pub struct ShotBytesView {
    pub data: *const u8,
    pub len: usize,
}

/// Rust 独占的不可变输出；宿主只能借用内容并调用对应释放函数。
pub struct ShotBytes(pub(crate) Vec<u8>);

/// 借用 view 的字节，limit 限制输入大小，返回已检查的切片。
/// # Safety
/// view 必须引用同步调用期间有效、不可变的连续内存。
pub(crate) unsafe fn input<'a>(view: ShotBytesView, limit: usize) -> Result<&'a [u8]> {
    if view.len > limit {
        return Err("FFI byte input exceeds size limit".into());
    }
    // SAFETY: 【Rust接口】【字节借用】宿主保证生命周期，长度上限已检查
    unsafe { boundary::input(view.data, view.len) }
}

/// 将 view 解码为 JSON 类型 T，limit 为字节上限，返回数据或解码错误。
/// # Safety
/// view 必须满足 input 的有效内存契约。
pub(crate) unsafe fn json<T: DeserializeOwned>(view: ShotBytesView, limit: usize) -> Result<T> {
    // SAFETY: 【Rust接口】【JSON解码】只在当前调用期间读取宿主输入
    let bytes = unsafe { input(view, limit)? };
    serde_json::from_slice(bytes).map_err(|error| error.to_string())
}

/// 解析 view 中的 UTF-8 显式路径，返回独立路径；不展开环境变量或 HOME。
/// # Safety
/// view 必须满足 input 的有效内存契约。
pub(crate) unsafe fn path(view: ShotBytesView) -> Result<PathBuf> {
    // SAFETY: 【Rust接口】【配置路径】限制路径输入长度并在返回前复制
    let bytes = unsafe { input(view, 1024 * 1024)? };
    let text = std::str::from_utf8(bytes).map_err(|_| "configuration path is not UTF-8")?;
    if text.is_empty() || text.contains('\0') {
        return Err("configuration path is empty or contains NUL".into());
    }
    Ok(PathBuf::from(text))
}

/// 将 value 编码到独占输出，返回尚未移交给宿主的缓冲区。
pub(crate) fn encode(value: &impl Serialize) -> Result<Box<ShotBytes>> {
    serde_json::to_vec(value)
        .map(|bytes| Box::new(ShotBytes(bytes)))
        .map_err(|error| error.to_string())
}

/// 把 bytes 的所有权移交到 output，返回指针检查结果；失败时自动释放 bytes。
/// # Safety
/// output 必须指向独立可写的句柄存储，且不能覆盖仍未释放的句柄。
pub(crate) unsafe fn publish(output: *mut *mut ShotBytes, bytes: Box<ShotBytes>) -> Result<()> {
    boundary::validate(output)?;
    // SAFETY: 【Rust接口】【输出移交】输出地址已检查，只有成功写入后才交出所有权
    unsafe { output.write(Box::into_raw(bytes)) };
    Ok(())
}

/// 借用 handle 的内容写入 output，返回状态码；视图在 handle 释放前有效。
/// # Safety
/// handle 必须是有效的独占输出句柄，output 必须指向独立可写视图。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_bytes_view(
    handle: *const ShotBytes,
    output: *mut ShotBytesView,
) -> i32 {
    boundary::run(|| {
        boundary::validate(handle)?;
        // SAFETY: 【Rust接口】【输出借用】句柄存活且不可变，宿主保证输出存储独立
        unsafe {
            let bytes = &(*handle).0;
            boundary::write(
                output,
                ShotBytesView {
                    data: bytes.as_ptr(),
                    len: bytes.len(),
                },
            )
        }
    })
}

/// 释放 handle 一次，空指针无操作，无返回值。
/// # Safety
/// 非空 handle 必须来自本库且尚未释放；释放后所有借用视图失效。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_bytes_destroy(handle: *mut ShotBytes) {
    if !handle.is_null() {
        boundary::run(|| {
            boundary::validate(handle)?;
            // SAFETY: 【Rust接口】【输出释放】宿主交回唯一的有效句柄
            unsafe { drop(Box::from_raw(handle)) };
            Ok(())
        });
    }
}
