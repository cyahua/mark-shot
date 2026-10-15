use crate::colors::Color;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use shot_config::validation::json_integer;

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct AnnotationStyle {
    pub current_color: Color,
    pub stroke_width: f64,
    pub highlighter_width: f64,
    pub number_width: f64,
    pub text_size: f64,
    pub mosaic_block_size: f64,
    pub mosaic_style: i32,
    pub shape_filled: bool,
    pub rectangle_corner_radius: f64,
    pub rectangle_style: i32,
    pub marker_shape: i32,
    pub marker_filled: bool,
    pub magnifier_scale: f64,
    pub magnifier_shape: i32,
    pub arrow_style: i32,
    pub arrow_line_style: i32,
    pub highlighter_style: i32,
    pub number_style: i32,
    pub text_font_family: String,
    pub text_font_weight: i32,
    pub text_italic: bool,
    pub text_background_color: Color,
}

impl Default for AnnotationStyle {
    /// 返回没有状态文件时的工具默认值，无参数。
    fn default() -> Self {
        Self {
            current_color: Color(0xffff4d4d),
            stroke_width: 3.0,
            highlighter_width: 6.0,
            number_width: 3.0,
            text_size: 1.0,
            mosaic_block_size: 14.0,
            mosaic_style: 0,
            shape_filled: false,
            rectangle_corner_radius: 0.0,
            rectangle_style: 0,
            marker_shape: 0,
            marker_filled: true,
            magnifier_scale: 2.75,
            magnifier_shape: 0,
            arrow_style: 0,
            arrow_line_style: 0,
            highlighter_style: 1,
            number_style: 0,
            text_font_family: String::new(),
            text_font_weight: 600,
            text_italic: false,
            text_background_color: Color(0),
        }
    }
}

/// 从 root 的 key 读取 JSON 数字并夹紧，fallback 为缺失值，返回限定范围内的数值。
fn number(root: &Value, key: &str, fallback: f64, minimum: f64, maximum: f64) -> f64 {
    root.get(key)
        .and_then(Value::as_f64)
        .filter(|v| v.is_finite())
        .unwrap_or(fallback)
        .clamp(minimum, maximum)
}
/// 从 root 的 key 读取枚举编号，返回范围内编号或 fallback。
fn variant(root: &Value, key: &str, fallback: i32, maximum: i32) -> i32 {
    let parsed = root
        .get(key)
        .map_or(fallback, |v| json_integer(v, fallback));
    if (0..=maximum).contains(&parsed) {
        parsed
    } else {
        fallback
    }
}
/// 从 root 的 key 读取颜色，返回有效颜色或 fallback。
fn color(root: &Value, key: &str, fallback: Color) -> Color {
    root.get(key)
        .and_then(Value::as_str)
        .and_then(Color::parse)
        .unwrap_or(fallback)
}

impl AnnotationStyle {
    /// 【标注核心】【样式兼容】从已有 JSON root 加载工具样式；非法根返回默认值，兼容旧笔宽字段。
    pub fn from_legacy(root: &Value) -> Self {
        if !root.is_object() {
            return Self::default();
        }
        let defaults = Self::default();
        let pen = number(root, "penWidth", 2.0, 1.0, 24.0);
        let shape = number(root, "shapeWidth", 3.0, 1.0, 24.0);
        Self {
            current_color: color(root, "currentColor", defaults.current_color),
            text_background_color: color(
                root,
                "textBackgroundColor",
                defaults.text_background_color,
            ),
            stroke_width: number(root, "strokeWidth", shape, 1.0, 24.0),
            highlighter_width: number(root, "highlighterWidth", pen * 2.0, 1.0, 48.0),
            number_width: number(root, "numberWidth", defaults.number_width, 1.0, 72.0),
            text_size: number(root, "textSize", defaults.text_size, -11.0, 281.0),
            mosaic_block_size: number(
                root,
                "mosaicBlockSize",
                defaults.mosaic_block_size,
                4.0,
                48.0,
            ),
            shape_filled: root
                .get("shapeFilled")
                .and_then(Value::as_bool)
                .unwrap_or(defaults.shape_filled),
            rectangle_corner_radius: number(
                root,
                "rectangleCornerRadius",
                defaults.rectangle_corner_radius,
                0.0,
                96.0,
            ),
            rectangle_style: variant(root, "rectangleStyle", defaults.rectangle_style, 3),
            mosaic_style: variant(root, "mosaicStyle", defaults.mosaic_style, 4),
            marker_shape: variant(root, "markerShape", defaults.marker_shape, 23),
            marker_filled: root
                .get("markerFilled")
                .and_then(Value::as_bool)
                .unwrap_or(defaults.marker_filled),
            magnifier_scale: number(root, "magnifierScale", defaults.magnifier_scale, 1.25, 6.0),
            magnifier_shape: variant(root, "magnifierShape", defaults.magnifier_shape, 1),
            arrow_style: variant(root, "arrowStyle", defaults.arrow_style, 3),
            arrow_line_style: variant(root, "arrowLineStyle", defaults.arrow_line_style, 2),
            highlighter_style: variant(root, "highlighterStyle", defaults.highlighter_style, 1),
            number_style: variant(root, "numberStyle", defaults.number_style, 6),
            text_font_family: root
                .get("textFontFamily")
                .and_then(Value::as_str)
                .unwrap_or("")
                .to_owned(),
            text_font_weight: root
                .get("textFontWeight")
                .filter(|v| v.is_number())
                .map_or(defaults.text_font_weight, |v| {
                    json_integer(v, 0).clamp(100, 900)
                }),
            text_italic: root
                .get("textItalic")
                .and_then(Value::as_bool)
                .unwrap_or(defaults.text_italic),
        }
    }
}
