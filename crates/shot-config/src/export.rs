use serde::Serialize;
use serde_json::Value;

use crate::validation;

#[derive(Clone, Copy, Debug, PartialEq, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct FrameSettings {
    pub enabled: bool,
    pub padding: i32,
    pub corner_radius: f64,
    pub shadow_radius: i32,
    pub shadow_offset_y: i32,
    pub shadow_opacity: f64,
}

impl Default for FrameSettings {
    /// 返回原应用的导出外框默认配置，无参数。
    fn default() -> Self {
        Self {
            enabled: false,
            padding: 112,
            corner_radius: 18.0,
            shadow_radius: 72,
            shadow_offset_y: 28,
            shadow_opacity: 0.32,
        }
    }
}

/// 从 root 的 keys 读取整数，按 maximum 夹紧，缺失时返回 fallback。
fn integer(root: &Value, keys: &[&str], fallback: i32, maximum: i32) -> i32 {
    validation::first_value(root, keys)
        .and_then(validation::integer)
        .map_or(fallback, |value| value.clamp(0, maximum))
}

/// 从 root 的 keys 读取有限实数，按 maximum 夹紧，缺失时返回 fallback。
fn real(root: &Value, keys: &[&str], fallback: f64, maximum: f64) -> f64 {
    validation::first_value(root, keys)
        .and_then(validation::real)
        .map_or(fallback, |value| value.clamp(0.0, maximum))
}

impl FrameSettings {
    /// 从应用 root 解析外框选项，返回支持旧别名与布尔开关的配置，不读取环境或文件。
    pub fn from_root(root: &Value) -> Self {
        let mut settings = Self::default();
        let frame = &root["export"]["imageFrame"];
        if let Some(enabled) = validation::boolean(frame) {
            settings.enabled = enabled;
            return settings;
        }
        if !frame.is_object() {
            return settings;
        }
        settings.enabled = validation::first_value(frame, &["enabled", "show", "visible"])
            .and_then(validation::boolean)
            .unwrap_or(settings.enabled);
        settings.padding = integer(frame, &["padding", "margin"], settings.padding, 256);
        settings.corner_radius = real(
            frame,
            &["cornerRadius", "radius"],
            settings.corner_radius,
            128.0,
        );
        settings.shadow_radius = integer(
            frame,
            &["shadowRadius", "shadowBlur"],
            settings.shadow_radius,
            128,
        );
        settings.shadow_offset_y = integer(
            frame,
            &["shadowOffsetY", "shadowYOffset"],
            settings.shadow_offset_y,
            128,
        );
        settings.shadow_opacity = real(
            frame,
            &["shadowOpacity", "shadowAlpha"],
            settings.shadow_opacity,
            1.0,
        );
        settings
    }
}
