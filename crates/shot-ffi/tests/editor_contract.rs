use shot_editor::Snapshot;
use shot_ffi::bytes::{ShotBytes, ShotBytesView, shot_bytes_destroy, shot_bytes_view};
use shot_ffi::editor_history::*;

/// 将 bytes 转为测试借用视图，返回不延长生命周期的 ABI 字段。
fn view(bytes: &[u8]) -> ShotBytesView {
    ShotBytesView {
        data: bytes.as_ptr(),
        len: bytes.len(),
    }
}

/// 复制并释放 output 的 JSON，返回快照；调用者传入有效的未释放输出句柄。
unsafe fn take(output: *mut ShotBytes) -> Snapshot {
    let mut bytes = ShotBytesView {
        data: std::ptr::null(),
        len: 0,
    };
    // SAFETY: 【接口测试】【输出所有权】调用者提供有效句柄，复制完成后只释放一次
    unsafe {
        assert_eq!(shot_bytes_view(output, &mut bytes), 0);
        let result = Snapshot::decode(std::slice::from_raw_parts(bytes.data, bytes.len)).unwrap();
        shot_bytes_destroy(output);
        result
    }
}

#[test]
/// 验证输出指针或输入 JSON 错误不会提交滚轮事务、消耗撤销记录，无参数、无返回值。
fn invalid_step_is_transactional_across_the_c_boundary() {
    let handle = shot_history_create(100);
    assert!(!handle.is_null());
    let before = Snapshot::default();
    let current = Snapshot {
        next_number: 9,
        ..Snapshot::default()
    };
    let before_json = serde_json::to_vec(&before).unwrap();
    let current_json = serde_json::to_vec(&current).unwrap();
    let mut output = std::ptr::null_mut();
    let mut info = ShotHistoryInfo::default();
    // SAFETY: 【接口测试】【历史事务】会话独占，所有输入和输出在调用期间有效且互不重叠
    unsafe {
        assert_eq!(shot_history_queue(handle, 3, view(&before_json)), 0);
        assert_eq!(
            shot_history_step(handle, view(&current_json), 0, std::ptr::null_mut()),
            1
        );
        assert_eq!(
            shot_history_step(handle, view(b"broken"), 0, &mut output),
            1
        );
        assert_eq!(
            shot_history_step(handle, view(&current_json), 2, &mut output),
            1
        );
        assert!(output.is_null());
        assert_eq!(shot_history_info(handle, &mut info), 0);
        assert_eq!(
            (
                info.undo_count,
                info.redo_count,
                info.pending_context,
                info.pending
            ),
            (0, 0, 3, 1)
        );

        assert_eq!(
            shot_history_step(handle, view(&current_json), 0, &mut output),
            0
        );
        assert_eq!(take(output), before);
        output = std::ptr::null_mut();
        assert_eq!(
            shot_history_step(handle, view(&before_json), 1, &mut output),
            0
        );
        assert_eq!(take(output), current);
        shot_history_destroy(handle);
    }
}

#[test]
/// 验证历史快照复制输入内容、会话隔离以及空句柄错误，无参数、无返回值。
fn history_does_not_retain_host_memory_or_share_other_sessions() {
    let first = shot_history_create(100);
    let second = shot_history_create(100);
    let mut input = serde_json::to_vec(&Snapshot {
        next_number: 7,
        ..Snapshot::default()
    })
    .unwrap();
    let current = serde_json::to_vec(&Snapshot::default()).unwrap();
    let mut output = std::ptr::null_mut();
    let mut info = ShotHistoryInfo::default();
    // SAFETY: 【接口测试】【会话隔离】句柄来自本库且互不共享，修改宿主内存发生在调用返回后
    unsafe {
        assert_eq!(shot_history_checkpoint(first, view(&input)), 0);
        input.fill(b'x');
        assert_eq!(shot_history_info(second, &mut info), 0);
        assert_eq!(info.undo_count, 0);
        assert_eq!(shot_history_step(first, view(&current), 0, &mut output), 0);
        assert_eq!(take(output).next_number, 7);
        assert_eq!(shot_history_info(std::ptr::null_mut(), &mut info), 1);
        assert_eq!(shot_history_clear(first), 0);
        shot_history_destroy(first);
        shot_history_destroy(second);
        shot_history_destroy(std::ptr::null_mut());
        shot_bytes_destroy(std::ptr::null_mut());
    }
}
