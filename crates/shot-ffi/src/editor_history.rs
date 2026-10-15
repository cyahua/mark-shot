use shot_editor::{History, Snapshot, annotation::MAX_SNAPSHOT_BYTES};

use crate::boundary::{self, Result};
use crate::bytes::{self, ShotBytes, ShotBytesView};

pub struct ShotHistory(History);

#[repr(C)]
#[derive(Default)]
pub struct ShotHistoryInfo {
    pub undo_count: usize,
    pub redo_count: usize,
    pub pending_context: i32,
    pub pending: i32,
}

/// 借用 handle 中的历史会话，返回独占引用或指针错误。
/// # Safety
/// handle 必须是本库创建的存活句柄，同一句柄禁止并发或重入。
unsafe fn history<'a>(handle: *mut ShotHistory) -> Result<&'a mut History> {
    boundary::validate(handle)?;
    // SAFETY: 【Rust接口】【历史借用】宿主保证会话独占且存活
    Ok(unsafe { &mut (*handle).0 })
}

/// 从 view 解码完整标注快照，返回已验证的独立数据。
/// # Safety
/// view 必须引用同步调用期间有效且不可变的字节。
unsafe fn snapshot(view: ShotBytesView) -> Result<Snapshot> {
    // SAFETY: 【Rust接口】【历史解码】长度校验后解码，Rust 不保留宿主指针
    Snapshot::decode(unsafe { bytes::input(view, MAX_SNAPSHOT_BYTES)? })
}

/// 按 limit 创建独立编辑历史，返回句柄；失败时返回空指针。
#[unsafe(no_mangle)]
pub extern "C" fn shot_history_create(limit: usize) -> *mut ShotHistory {
    boundary::value(std::ptr::null_mut(), || {
        Ok(Box::into_raw(Box::new(ShotHistory(History::new(limit)?))))
    })
}

/// 释放 handle 一次，空指针无操作，无返回值。
/// # Safety
/// 非空 handle 必须来自本库且尚未释放，释放期间不得持有借用。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_history_destroy(handle: *mut ShotHistory) {
    if !handle.is_null() {
        boundary::run(|| {
            boundary::validate(handle)?;
            // SAFETY: 【Rust接口】【历史释放】宿主交回唯一的有效会话
            unsafe { drop(Box::from_raw(handle)) };
            Ok(())
        });
    }
}

/// 读取 handle 的栈数量及滚轮事务到 output，返回状态码。
/// # Safety
/// handle 必须独占且存活，output 必须指向独立可写结构。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_history_info(
    handle: *mut ShotHistory,
    output: *mut ShotHistoryInfo,
) -> i32 {
    boundary::run(|| {
        // SAFETY: 【Rust接口】【历史状态】输入句柄与输出存储由宿主保证独立
        unsafe {
            let (undo_count, redo_count, context) = history(handle)?.info();
            boundary::write(
                output,
                ShotHistoryInfo {
                    undo_count,
                    redo_count,
                    pending_context: context.unwrap_or(0),
                    pending: i32::from(context.is_some()),
                },
            )
        }
    })
}

/// 查询 handle 的 context 是否需要新快照到 needed，返回状态码，不修改会话。
/// # Safety
/// handle 必须独占且存活，needed 必须指向独立可写整数。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_history_needs_snapshot(
    handle: *mut ShotHistory,
    context: i32,
    needed: *mut i32,
) -> i32 {
    boundary::run(|| {
        // SAFETY: 【Rust接口】【滚轮输入】核心决定是否需要完整快照，宿主只优化数据转换
        unsafe { boundary::write(needed, i32::from(history(handle)?.needs_snapshot(context))) }
    })
}

/// 清除 handle 的全部历史与待提交事务，返回状态码。
/// # Safety
/// handle 必须是独占且存活的历史句柄。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_history_clear(handle: *mut ShotHistory) -> i32 {
    boundary::run(|| {
        // SAFETY: 【Rust接口】【历史重置】只修改当前独占会话
        unsafe { history(handle)?.clear() };
        Ok(())
    })
}

/// 将 before 保存为 handle 的编辑前快照，返回状态码。
/// # Safety
/// handle 必须独占且存活，before 必须引用有效的快照 JSON 字节。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_history_checkpoint(
    handle: *mut ShotHistory,
    before: ShotBytesView,
) -> i32 {
    boundary::run(|| {
        // SAFETY: 【Rust接口】【历史记录】输入解码成功后才修改会话
        unsafe {
            history(handle)?
                .checkpoint(snapshot(before)?)
                .map_err(str::to_owned)
        }
    })
}

/// 在 handle 中合并 context 的滚轮编辑，before 为首次修改前快照，返回状态码。
/// # Safety
/// handle 必须独占且存活，before 必须引用有效的快照 JSON 字节。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_history_queue(
    handle: *mut ShotHistory,
    context: i32,
    before: ShotBytesView,
) -> i32 {
    boundary::run(|| {
        // SAFETY: 【Rust接口】【滚轮合并】只将解码完成的独立快照交给核心
        unsafe {
            history(handle)?
                .queue(context, snapshot(before)?)
                .map_err(str::to_owned)
        }
    })
}

/// 提交 handle 的待合并滚轮事务，返回状态码，无事务时不修改历史。
/// # Safety
/// handle 必须是独占且存活的历史句柄。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_history_commit(handle: *mut ShotHistory) -> i32 {
    boundary::run(|| {
        // SAFETY: 【Rust接口】【事务提交】会话仅由当前调用访问
        unsafe { history(handle)?.commit() };
        Ok(())
    })
}

/// 保存 current 并撤销或重做 handle，redo 只能为 0 或 1；output 接收目标 JSON，返回状态码。
/// 没有目标时 output 为空；输出校验或编码失败时不修改历史。
/// # Safety
/// handle 必须独占且存活，current 必须有效，output 必须指向独立可写的空句柄。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_history_step(
    handle: *mut ShotHistory,
    current: ShotBytesView,
    redo: i32,
    output: *mut *mut ShotBytes,
) -> i32 {
    boundary::run(|| {
        boundary::validate(output)?;
        if !matches!(redo, 0 | 1) {
            return Err("invalid history direction".into());
        }
        // SAFETY: 【Rust接口】【历史切换】输出地址和输入解码先于任何会话变更
        unsafe {
            let current = snapshot(current)?;
            let history = history(handle)?;
            // 1. 【编辑核心】【失败保护】先编码目标，避免输出失败消耗撤销记录
            let encoded = history
                .candidate(redo != 0)
                .map(bytes::encode)
                .transpose()?;
            // 2. 【编辑核心】【失败保护】已完成所有可失败转换，再提交栈切换和句柄输出
            history.step(current, redo != 0)?;
            output.write(encoded.map_or(std::ptr::null_mut(), Box::into_raw));
            Ok(())
        }
    })
}
