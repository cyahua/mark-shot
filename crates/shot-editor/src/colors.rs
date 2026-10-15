use serde::{Deserialize, Deserializer, Serialize, Serializer};
use std::collections::BTreeMap;
use std::sync::OnceLock;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Color(pub u32);

impl Color {
    /// 解析 Qt 支持的颜色 text，返回 ARGB；颜色名称表来自 Qt 的标准 SVG 颜色目录。
    pub fn parse(text: &str) -> Option<Self> {
        if let Some(hex) = text.strip_prefix('#') {
            if !hex.is_ascii() || !hex.bytes().all(|b| b.is_ascii_hexdigit()) {
                return None;
            }
            if hex.len() == 8 {
                return u32::from_str_radix(hex, 16).ok().map(Self);
            }
            let digits = match hex.len() {
                3 => 1,
                6 => 2,
                9 => 3,
                12 => 4,
                _ => return None,
            };
            let maximum = (1_u32 << (digits * 4)) - 1;
            let mut argb = 0xff000000;
            for channel in 0..3 {
                let value =
                    u32::from_str_radix(&hex[channel * digits..(channel + 1) * digits], 16).ok()?;
                argb |= ((value * 255 + maximum / 2) / maximum) << (16 - channel * 8);
            }
            return Some(Self(argb));
        }
        static NAMES: OnceLock<BTreeMap<String, String>> = OnceLock::new();
        let names = NAMES.get_or_init(|| {
            serde_json::from_str(include_str!("color_names.json"))
                .expect("validated Qt color table")
        });
        let normalized: String = text
            .chars()
            .filter(|character| !matches!(character, ' ' | '\t'))
            .collect();
        names
            .get(&normalized.to_lowercase())
            .and_then(|hex| Self::parse(hex))
    }
}

impl Serialize for Color {
    /// 将颜色写入 serializer，返回兼容 Qt 的 #AARRGGBB 序列化结果。
    fn serialize<S: Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        serializer.serialize_str(&format!("#{:08x}", self.0))
    }
}
impl<'de> Deserialize<'de> for Color {
    /// 从 deserializer 读取颜色字符串，返回颜色或格式错误。
    fn deserialize<D: Deserializer<'de>>(deserializer: D) -> Result<Self, D::Error> {
        let text = String::deserialize(deserializer)?;
        Self::parse(&text).ok_or_else(|| serde::de::Error::custom("invalid Qt color"))
    }
}
