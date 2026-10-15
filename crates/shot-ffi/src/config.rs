use serde::Deserialize;
use serde_json::Value;
use shot_config::{MAX_JSON_BYTES, defaults, store, validation};

use crate::boundary;
use crate::bytes::{self, ShotBytes, ShotBytesView};

#[derive(Deserialize)]
pub(crate) struct Change {
    path: Vec<String>,
    #[serde(default)]
    value: Value,
    #[serde(default)]
    remove: bool,
}

#[repr(C)]
#[derive(Default)]
pub struct ShotConfigScalar {
    pub boolean_valid: i32,
    pub boolean_value: i32,
    pub integer_valid: i32,
    pub integer_value: i32,
}

/// 根据 context 的平台与颜色 JSON 返回默认配置到 output，返回状态码。
/// # Safety
/// context 必须是有效字节视图，output 必须指向独立可写的空句柄。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_config_defaults(
    context: ShotBytesView,
    output: *mut *mut ShotBytes,
) -> i32 {
    boundary::run(|| {
        boundary::validate(output)?;
        // SAFETY: 【Rust接口】【默认配置】输入同步解码，结果通过独占句柄交回
        unsafe {
            let context: defaults::Context = bytes::json(context, MAX_JSON_BYTES)?;
            bytes::publish(output, bytes::encode(&defaults::application(&context))?)
        }
    })
}

/// 从 path 的显式 UTF-8 路径读取配置到 output，返回状态码，不尝试其他路径。
/// # Safety
/// path 必须是有效字节视图，output 必须指向独立可写的空句柄。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_config_read(path: ShotBytesView, output: *mut *mut ShotBytes) -> i32 {
    boundary::run(|| {
        boundary::validate(output)?;
        // SAFETY: 【Rust接口】【配置读取】宿主输入只在调用期间借用
        unsafe {
            let root = store::read(&bytes::path(path)?).map_err(|error| error.to_string())?;
            bytes::publish(output, bytes::encode(&root)?)
        }
    })
}

/// 把 root JSON 原子保存到 path，返回状态码。
/// # Safety
/// path、root 必须是调用期间有效的不可变字节视图。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_config_write(path: ShotBytesView, root: ShotBytesView) -> i32 {
    boundary::run(|| {
        // SAFETY: 【Rust接口】【配置保存】写入前完成所有输入解码和校验
        unsafe {
            store::write(&bytes::path(path)?, &bytes::json(root, MAX_JSON_BYTES)?)
                .map_err(|error| error.to_string())
        }
    })
}

/// 仅在 path 缺失时写入 defaults；created 接收是否新建，返回状态码。
/// # Safety
/// 输入视图必须有效，created 必须指向独立可写的整数。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_config_ensure(
    path: ShotBytesView,
    defaults: ShotBytesView,
    created: *mut i32,
) -> i32 {
    boundary::run(|| {
        boundary::validate(created)?;
        // SAFETY: 【Rust接口】【配置创建】输出地址在磁盘副作用发生前验证
        unsafe {
            let result =
                store::ensure(&bytes::path(path)?, &bytes::json(defaults, MAX_JSON_BYTES)?)
                    .map_err(|error| error.to_string())?;
            boundary::write(created, i32::from(result))
        }
    })
}

/// 按 change 的 path/value/remove 字段原子更新文件 path，返回状态码。
/// # Safety
/// path、change 必须是调用期间有效的不可变字节视图。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_config_patch(path: ShotBytesView, change: ShotBytesView) -> i32 {
    boundary::run(|| {
        // SAFETY: 【Rust接口】【配置更新】先解码请求，文件锁覆盖读取、修改和原子提交
        unsafe {
            let change: Change = bytes::json(change, MAX_JSON_BYTES)?;
            store::patch(
                &bytes::path(path)?,
                &change.path,
                (!change.remove).then_some(change.value),
            )
            .map_err(|error| error.to_string())
        }
    })
}

/// 在内存 root 中应用 change，返回新 JSON 到 output；不访问文件系统。
/// # Safety
/// 输入视图必须有效，output 必须指向独立可写的空句柄。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_config_update(
    root: ShotBytesView,
    change: ShotBytesView,
    output: *mut *mut ShotBytes,
) -> i32 {
    boundary::run(|| {
        boundary::validate(output)?;
        // SAFETY: 【Rust接口】【嵌套更新】只修改 Rust 拥有的解析结果
        unsafe {
            let mut root: Value = bytes::json(root, MAX_JSON_BYTES)?;
            let change: Change = bytes::json(change, MAX_JSON_BYTES)?;
            validation::set_nested(
                &mut root,
                &change.path,
                (!change.remove).then_some(change.value),
            )
            .map_err(|error| error.to_string())?;
            bytes::publish(output, bytes::encode(&root)?)
        }
    })
}

/// 按旧配置兼容规则读取 value JSON 的布尔值和整数到 output，返回状态码。
/// # Safety
/// value 必须是有效视图，output 必须指向独立可写结构。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_config_scalar(
    value: ShotBytesView,
    output: *mut ShotConfigScalar,
) -> i32 {
    boundary::run(|| {
        // SAFETY: 【Rust接口】【值校验】输入同步解码，输出指针由宿主保证有效
        unsafe {
            let value: Value = bytes::json(value, MAX_JSON_BYTES)?;
            let boolean = validation::boolean(&value);
            let integer = validation::integer(&value);
            boundary::write(
                output,
                ShotConfigScalar {
                    boolean_valid: i32::from(boolean.is_some()),
                    boolean_value: i32::from(boolean.unwrap_or(false)),
                    integer_valid: i32::from(integer.is_some()),
                    integer_value: integer.unwrap_or(0),
                },
            )
        }
    })
}

/// 归一化 UTF-8 key，将原始 UTF-8 文本写入 output，返回状态码。
/// # Safety
/// key 必须是有效视图，output 必须指向独立可写的空句柄。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_config_normalize_key(
    key: ShotBytesView,
    output: *mut *mut ShotBytes,
) -> i32 {
    boundary::run(|| {
        boundary::validate(output)?;
        // SAFETY: 【Rust接口】【键名兼容】UTF-8 校验在业务转换前完成
        unsafe {
            let key = std::str::from_utf8(bytes::input(key, MAX_JSON_BYTES)?)
                .map_err(|error| error.to_string())?;
            bytes::publish(
                output,
                Box::new(ShotBytes(validation::normalized_key(key).into_bytes())),
            )
        }
    })
}
