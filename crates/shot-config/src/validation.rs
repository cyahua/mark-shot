use crate::{Error, Result};
use serde_json::{Map, Value};

/// 从 value 读取兼容布尔值，返回识别结果；数字采用 Qt 的零值容差。
pub fn boolean(value: &Value) -> Option<bool> {
    match value {
        Value::Bool(value) => Some(*value),
        Value::Number(value) => value.as_f64().map(|v| v.abs() > 1e-12),
        Value::String(value) => match value.trim().to_lowercase().as_str() {
            "1" | "true" | "yes" | "on" | "enabled" => Some(true),
            "0" | "false" | "no" | "off" | "disabled" => Some(false),
            _ => None,
        },
        _ => None,
    }
}

/// 按 Qt JSON 规则读取 value，非整型数字返回 fallback，字符串不参与转换。
pub fn json_integer(value: &Value, fallback: i32) -> i32 {
    value
        .as_f64()
        .filter(|v| {
            v.is_finite()
                && v.fract() == 0.0
                && *v >= f64::from(i32::MIN)
                && *v <= f64::from(i32::MAX)
        })
        .map_or(fallback, |v| v as i32)
}

/// 从 value 读取整数或十进制整数字符串，返回兼容结果；非整数 JSON 数字按旧规则返回 0。
pub fn integer(value: &Value) -> Option<i32> {
    match value {
        Value::Number(_) => Some(json_integer(value, 0)),
        Value::String(value) => value.trim().parse().ok(),
        _ => None,
    }
}

/// 从 value 读取有限实数，支持字符串，返回解析结果。
pub fn real(value: &Value) -> Option<f64> {
    let parsed = match value {
        Value::Number(v) => v.as_f64(),
        Value::String(v) => v.trim().parse().ok(),
        _ => None,
    };
    parsed.filter(|v: &f64| v.is_finite())
}

/// 归一化 key 的大小写和兼容分隔符，返回用于查找的键名。
pub fn normalized_key(key: &str) -> String {
    key.trim()
        .to_lowercase()
        .chars()
        .filter(|c| !matches!(c, ' ' | '_' | '-' | '.'))
        .collect()
}

/// 按 keys 顺序查找 object 的首个已定义值，返回借用值或 None；null 不被跳过。
pub fn first_value<'a>(object: &'a Value, keys: &[&str]) -> Option<&'a Value> {
    keys.iter().find_map(|key| object.get(*key))
}

/// 在 root 内更新 path；value 为 None 时移除末级键，返回路径校验结果。
pub fn set_nested(root: &mut Value, path: &[String], value: Option<Value>) -> Result<()> {
    if path.is_empty() || path.len() > 64 || !root.is_object() {
        return Err(Error::Invalid("invalid configuration key path"));
    }
    let mut current = root;
    for key in &path[..path.len() - 1] {
        let object = current
            .as_object_mut()
            .ok_or(Error::Invalid("configuration root is not an object"))?;
        current = object
            .entry(key.clone())
            .or_insert_with(|| Value::Object(Map::new()));
        if !current.is_object() {
            *current = Value::Object(Map::new());
        }
    }
    let object = current
        .as_object_mut()
        .ok_or(Error::Invalid("configuration parent is not an object"))?;
    let key = path
        .last()
        .ok_or(Error::Invalid("empty configuration key path"))?;
    if let Some(value) = value {
        object.insert(key.clone(), value);
    } else {
        object.remove(key);
    }
    Ok(())
}
