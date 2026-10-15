use std::fs;
use std::path::Path;
use std::sync::{Arc, Barrier};

use serde_json::json;
use shot_config::{MAX_JSON_BYTES, store};

#[test]
/// 验证仅在显式临时路径创建默认配置且重复初始化不覆盖用户字段，无参数、无返回值。
fn defaults_are_created_once_at_an_explicit_path() {
    let directory = tempfile::tempdir().unwrap();
    let path = directory.path().join("nested/config.json");
    assert!(!path.exists());
    assert!(store::ensure(&path, &json!({"first": true})).unwrap());
    assert!(!store::ensure(&path, &json!({"second": true})).unwrap());
    assert_eq!(store::read(&path).unwrap(), json!({"first": true}));
    assert!(store::write(Path::new("relative-config.json"), &json!({})).is_err());
    assert!(store::read(&directory.path().join("missing/other.json")).is_err());
    assert!(!directory.path().join("missing").exists());
}

#[test]
/// 验证 BOM、未知字段、null 和删除语义保持兼容，无参数、无返回值。
fn nested_patches_preserve_unknown_values_and_distinguish_null() {
    let directory = tempfile::tempdir().unwrap();
    let path = directory.path().join("config.json");
    fs::write(
        &path,
        b"\xef\xbb\xbf{\"other\": [1, 2], \"capture\": false}",
    )
    .unwrap();
    store::patch(
        &path,
        &["capture".into(), "mode".into()],
        Some(json!("copy")),
    )
    .unwrap();
    store::patch(&path, &["nullable".into()], Some(json!(null))).unwrap();
    assert_eq!(
        store::read(&path).unwrap(),
        json!({"other": [1, 2], "capture": {"mode": "copy"}, "nullable": null})
    );
    store::patch(&path, &["capture".into(), "mode".into()], None).unwrap();
    assert_eq!(store::read(&path).unwrap()["capture"], json!({}));
}

#[test]
/// 验证格式错误、容量超限及非法更新不会覆盖已有配置，无参数、无返回值。
fn rejected_writes_leave_existing_bytes_unchanged() {
    let directory = tempfile::tempdir().unwrap();
    let path = directory.path().join("config.json");
    store::write(&path, &json!({"keep": 42})).unwrap();
    let original = fs::read(&path).unwrap();
    assert!(store::write(&path, &json!([])).is_err());
    assert!(store::patch(&path, &[], None).is_err());
    assert!(store::patch(&path, &vec!["deep".into(); 65], Some(json!(true))).is_err());
    assert!(store::write(&path, &json!({"large": "x".repeat(MAX_JSON_BYTES)})).is_err());
    assert_eq!(fs::read(&path).unwrap(), original);
    fs::write(&path, b"broken").unwrap();
    assert!(store::patch(&path, &["keep".into()], Some(json!(1))).is_err());
    assert_eq!(fs::read(&path).unwrap(), b"broken");
}

#[test]
/// 验证并发局部更新共用文件锁，互不丢失其他字段，无参数、无返回值。
fn concurrent_patch_transactions_do_not_lose_updates() {
    let directory = tempfile::tempdir().unwrap();
    let path = directory.path().join("config.json");
    store::write(&path, &json!({"untouched": 7})).unwrap();
    let barrier = Arc::new(Barrier::new(12));
    std::thread::scope(|scope| {
        for index in 0..12 {
            let path = &path;
            let barrier = Arc::clone(&barrier);
            scope.spawn(move || {
                barrier.wait();
                store::patch(
                    path,
                    &["workers".into(), index.to_string()],
                    Some(json!(index)),
                )
                .unwrap();
            });
        }
    });
    let root = store::read(&path).unwrap();
    assert_eq!(root["untouched"], 7);
    for index in 0..12 {
        assert_eq!(root["workers"][index.to_string()], index);
    }
}

#[cfg(unix)]
#[test]
/// 验证原子更新保留符号链接及原文件权限，无参数、无返回值。
fn symbolic_links_and_private_permissions_survive_replacement() {
    use std::os::unix::fs::{PermissionsExt, symlink};

    let directory = tempfile::tempdir().unwrap();
    let target = directory.path().join("actual.json");
    let link = directory.path().join("config.json");
    store::write(&target, &json!({"value": 1})).unwrap();
    fs::set_permissions(&target, fs::Permissions::from_mode(0o640)).unwrap();
    symlink(&target, &link).unwrap();
    store::write(&link, &json!({"value": 2})).unwrap();
    assert!(
        fs::symlink_metadata(&link)
            .unwrap()
            .file_type()
            .is_symlink()
    );
    assert_eq!(store::read(&target).unwrap()["value"], 2);
    assert_eq!(
        fs::metadata(&target).unwrap().permissions().mode() & 0o777,
        0o640
    );
}

#[test]
/// 验证目录和非普通锁文件不会被配置写入替换，无参数、无返回值。
fn non_file_targets_are_rejected_without_replacement() {
    let directory = tempfile::tempdir().unwrap();
    let target = directory.path().join("config.json");
    fs::create_dir(&target).unwrap();
    assert!(store::read(&target).is_err());
    assert!(store::write(&target, &json!({})).is_err());
    assert!(target.is_dir());
    let blocked = directory.path().join("other.json");
    fs::create_dir(directory.path().join("other.json.lock")).unwrap();
    assert!(store::write(&blocked, &json!({})).is_err());
    assert!(!blocked.exists());
}

#[test]
/// 验证长时间占用的文件锁会返回错误，不永久阻塞界面且不改变原文件，无参数、无返回值。
fn contended_lock_times_out_without_changing_config() {
    let directory = tempfile::tempdir().unwrap();
    let path = directory.path().join("config.json");
    store::write(&path, &json!({"keep": 1})).unwrap();
    let before = fs::read(&path).unwrap();
    let lock = fs::OpenOptions::new()
        .read(true)
        .write(true)
        .open(directory.path().join("config.json.lock"))
        .unwrap();
    fs2::FileExt::lock_exclusive(&lock).unwrap();
    let error = store::write(&path, &json!({"keep": 2})).unwrap_err();
    assert!(error.to_string().contains("locked"));
    assert_eq!(fs::read(&path).unwrap(), before);
    drop(lock);
    store::write(&path, &json!({"keep": 3})).unwrap();
}
