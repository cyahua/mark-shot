use serde_json::json;
use shot_ffi::bytes::{ShotBytes, ShotBytesView, shot_bytes_destroy, shot_bytes_view};
use shot_ffi::config::*;

/// 将 bytes 转为同步借用视图，返回 ABI 字节范围。
fn view(bytes: &[u8]) -> ShotBytesView {
    ShotBytesView {
        data: bytes.as_ptr(),
        len: bytes.len(),
    }
}

/// 复制并释放 output，返回独立字节；调用者保证句柄有效且尚未释放。
unsafe fn take(output: *mut ShotBytes) -> Vec<u8> {
    let mut bytes = ShotBytesView {
        data: std::ptr::null(),
        len: 0,
    };
    // SAFETY: 【接口测试】【字节所有权】输入句柄有效，复制先于唯一一次释放
    unsafe {
        assert_eq!(shot_bytes_view(output, &mut bytes), 0);
        let result = std::slice::from_raw_parts(bytes.data, bytes.len).to_vec();
        shot_bytes_destroy(output);
        result
    }
}

#[test]
/// 验证创建操作先检查输出地址，再执行磁盘写入，所有文件均位于临时目录，无参数、无返回值。
fn creation_validates_outputs_before_any_file_side_effect() {
    let directory = tempfile::tempdir().unwrap();
    let path = directory.path().join("nested/config.json");
    let encoded = path.to_str().unwrap().as_bytes();
    let mut created = -1;
    // SAFETY: 【接口测试】【配置创建】输入字节存活，空输出指针用于检查可恢复错误
    unsafe {
        assert_eq!(
            shot_config_ensure(view(encoded), view(br#"{"keep":1}"#), std::ptr::null_mut()),
            1
        );
        assert!(!path.exists());
        assert!(!path.parent().unwrap().exists());
        assert_eq!(
            shot_config_ensure(view(encoded), view(br#"{"keep":1}"#), &mut created),
            0
        );
        assert_eq!(created, 1);
        assert_eq!(
            shot_config_ensure(view(encoded), view(br#"{"keep":2}"#), &mut created),
            0
        );
        assert_eq!(created, 0);
        assert_eq!(shot_config_write(view(&[0xff]), view(b"{}")), 1);
    }
    assert_eq!(
        serde_json::from_slice::<serde_json::Value>(&std::fs::read(path).unwrap()).unwrap(),
        json!({"keep":1})
    );
}

#[test]
/// 验证配置输出与后续文件修改互不共享，错误更新不改变文件内容，无参数、无返回值。
fn json_output_ownership_and_patch_failures_are_isolated() {
    let directory = tempfile::tempdir().unwrap();
    let path = directory.path().join("config.json");
    let encoded = path.to_str().unwrap().as_bytes();
    let mut output = std::ptr::null_mut();
    // SAFETY: 【接口测试】【配置事务】路径属于临时目录，句柄和可写输出互不重叠
    unsafe {
        assert_eq!(shot_config_write(view(encoded), view(br#"{"keep":1}"#)), 0);
        assert_eq!(shot_config_read(view(encoded), &mut output), 0);
        assert_eq!(
            shot_config_patch(
                view(encoded),
                view(br#"{"path":["nested","value"],"value":true}"#)
            ),
            0
        );
        assert_eq!(take(output), br#"{"keep":1}"#);
        let before = std::fs::read(&path).unwrap();
        assert_eq!(
            shot_config_patch(view(encoded), view(br#"{"path":[],"value":4}"#)),
            1
        );
        assert_eq!(std::fs::read(&path).unwrap(), before);
        assert_eq!(shot_config_write(view(encoded), view(b"[]")), 1);
        assert_eq!(std::fs::read(&path).unwrap(), before);
    }
}

#[test]
/// 验证非法 JSON 和超长输入不修改标量输出，缺失值保留空解析标记，无参数、无返回值。
fn scalar_boundary_rejects_invalid_inputs_without_writes() {
    let mut scalar = ShotConfigScalar {
        boolean_valid: 9,
        ..ShotConfigScalar::default()
    };
    // SAFETY: 【接口测试】【标量边界】正常输入有效，超长空指针视图必须在解引用前拒绝
    unsafe {
        assert_eq!(shot_config_scalar(view(b"not-json"), &mut scalar), 1);
        assert_eq!(scalar.boolean_valid, 9);
        assert_eq!(
            shot_config_scalar(
                ShotBytesView {
                    data: std::ptr::null(),
                    len: usize::MAX
                },
                &mut scalar
            ),
            1
        );
        assert_eq!(scalar.boolean_valid, 9);
        assert_eq!(shot_config_scalar(view(b"null"), &mut scalar), 0);
        assert_eq!((scalar.boolean_valid, scalar.integer_valid), (0, 0));
    }
}
