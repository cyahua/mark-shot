use crate::{Error, MAX_JSON_BYTES, Result};
use serde_json::Value;
use std::fs::{self, File, OpenOptions};
use std::io::Write;
use std::path::{Path, PathBuf};
use std::time::{Duration, Instant};

/// 校验显式 path 并解析已有符号链接；create_parent 控制创建父目录，返回实际写入目标。
pub(crate) fn target(path: &Path, create_parent: bool) -> Result<PathBuf> {
    if !path.is_absolute() || path.file_name().is_none() {
        return Err(Error::Invalid(
            "configuration path must be an absolute file path",
        ));
    }
    let parent = path
        .parent()
        .ok_or(Error::Invalid("configuration parent is missing"))?;
    if create_parent {
        fs::create_dir_all(parent)?;
    }
    let target = parent.canonicalize()?.join(
        path.file_name()
            .ok_or(Error::Invalid("configuration filename is missing"))?,
    );
    match fs::symlink_metadata(&target) {
        Ok(metadata) if metadata.file_type().is_symlink() => {
            let resolved = target.canonicalize()?;
            if !fs::metadata(&resolved)?.is_file() {
                return Err(Error::Invalid("configuration target is not a regular file"));
            }
            Ok(resolved)
        }
        Ok(metadata) if metadata.is_file() => Ok(target),
        Ok(_) => Err(Error::Invalid("configuration target is not a regular file")),
        Err(error) if error.kind() == std::io::ErrorKind::NotFound => Ok(target),
        Err(error) => Err(error.into()),
    }
}

/// 【配置核心】【写入锁】锁定 path 的旁路锁文件，返回持锁文件；最多等待两秒，释放文件即解除锁。
pub(crate) fn lock(path: &Path) -> Result<File> {
    let mut name = path
        .file_name()
        .ok_or(Error::Invalid("configuration filename is missing"))?
        .to_os_string();
    name.push(".lock");
    let mut options = OpenOptions::new();
    options.read(true).write(true).create(true).truncate(false);
    #[cfg(unix)]
    {
        use std::os::unix::fs::OpenOptionsExt;
        options.mode(0o600);
    }
    let lock_path = path.with_file_name(name);
    if let Ok(metadata) = fs::symlink_metadata(&lock_path) {
        if !metadata.is_file() {
            return Err(Error::Invalid("configuration lock is not a regular file"));
        }
    }
    let lock = options.open(lock_path)?;
    let started = Instant::now();
    loop {
        match fs2::FileExt::try_lock_exclusive(&lock) {
            Ok(()) => return Ok(lock),
            Err(error) if error.raw_os_error() == fs2::lock_contended_error().raw_os_error() => {
                if started.elapsed() >= Duration::from_secs(2) {
                    return Err(std::io::Error::new(
                        std::io::ErrorKind::WouldBlock,
                        "configuration is locked by another writer",
                    )
                    .into());
                }
                std::thread::sleep(Duration::from_millis(10));
            }
            Err(error) => return Err(error.into()),
        }
    }
}

/// 【配置核心】【原子保存】把 root 写入 path；调用者必须持有旁路锁，返回提交结果。
pub(crate) fn replace(path: &Path, root: &Value) -> Result<()> {
    if !root.is_object() {
        return Err(Error::Invalid("configuration root must be an object"));
    }
    let mut encoded = serde_json::to_vec_pretty(root)?;
    encoded.push(b'\n');
    if encoded.len() > MAX_JSON_BYTES {
        return Err(Error::Invalid("configuration exceeds size limit"));
    }
    // 1. 【配置核心】【原子保存】在同一目录分配临时文件，保留原文件权限
    let mut temporary = tempfile::NamedTempFile::new_in(
        path.parent()
            .ok_or(Error::Invalid("configuration parent is missing"))?,
    )?;
    if let Ok(metadata) = fs::metadata(path) {
        temporary
            .as_file()
            .set_permissions(metadata.permissions())?;
    }
    // 2. 【配置核心】【原子保存】完整写入并同步后再原子替换；失败由临时文件析构清理
    temporary.write_all(&encoded)?;
    temporary.as_file().sync_all()?;
    temporary.persist(path).map_err(|error| error.error)?;
    Ok(())
}
