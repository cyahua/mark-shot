use crate::{Error, MAX_JSON_BYTES, Result, atomic, validation};
use serde_json::Value;
use std::fs::File;
use std::io::Read;
use std::path::Path;

/// 从明确的 path 读取配置对象，返回对象或错误；不创建目录、不尝试任何替代路径。
pub fn read(path: &Path) -> Result<Value> {
    let path = atomic::target(path, false)?;
    let file = File::open(path)?;
    if !file.metadata()?.is_file() {
        return Err(Error::Invalid("configuration path is not a regular file"));
    }
    let mut bytes = Vec::new();
    file.take(MAX_JSON_BYTES as u64 + 1)
        .read_to_end(&mut bytes)?;
    if bytes.len() > MAX_JSON_BYTES {
        return Err(Error::Invalid("configuration exceeds size limit"));
    }
    let bytes = bytes.strip_prefix(&[0xef, 0xbb, 0xbf]).unwrap_or(&bytes);
    let root: Value = serde_json::from_slice(bytes)?;
    if !root.is_object() {
        return Err(Error::Invalid("configuration root must be an object"));
    }
    Ok(root)
}

/// 仅在 path 不存在时原子创建 defaults；返回是否创建，已存在的文件绝不被默认值覆盖。
pub fn ensure(path: &Path, defaults: &Value) -> Result<bool> {
    let path = atomic::target(path, true)?;
    let _lock = atomic::lock(&path)?;
    if path.try_exists()? {
        return Ok(false);
    }
    atomic::replace(&path, defaults)?;
    Ok(true)
}

/// 把完整 root 原子写入显式 path，返回保存结果。
pub fn write(path: &Path, root: &Value) -> Result<()> {
    let path = atomic::target(path, true)?;
    let _lock = atomic::lock(&path)?;
    atomic::replace(&path, root)
}

/// 【配置核心】【局部更新】锁定 path 后读取并更新 keys 对应的 value，返回提交结果；None 表示移除。
pub fn patch(path: &Path, keys: &[String], value: Option<Value>) -> Result<()> {
    if keys.is_empty() {
        return Err(Error::Invalid("configuration key path is empty"));
    }
    let path = atomic::target(path, true)?;
    let _lock = atomic::lock(&path)?;
    let mut root = read(&path)?;
    validation::set_nested(&mut root, keys, value)?;
    atomic::replace(&path, &root)
}
