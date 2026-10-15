use std::fs;

use serde_json::json;
use shot_editor::{AnnotationStyle, colors::Color, state};

#[test]
/// 验证缺失文件使用默认值、旧字段转换与枚举边界兼容，无参数、无返回值。
fn missing_and_legacy_state_keep_distinct_defaults() {
    let directory = tempfile::tempdir().unwrap();
    let path = directory.path().join("state.json");
    assert_eq!(state::load(&path), AnnotationStyle::default());
    assert!(!path.exists());
    assert_eq!(
        AnnotationStyle::from_legacy(&json!({})).highlighter_width,
        4.0
    );
    let style = AnnotationStyle::from_legacy(&json!({
        "penWidth": 8, "shapeWidth": 6, "strokeWidth": "wrong", "textSize": -30,
        "arrowStyle": 99, "arrowLineStyle": 2, "mosaicStyle": 4, "markerShape": 23,
        "magnifierScale": 9, "textFontWeight": 699.5, "shapeFilled": "true"
    }));
    assert_eq!(style.highlighter_width, 16.0);
    assert_eq!(style.stroke_width, 6.0);
    assert_eq!(style.text_size, -11.0);
    assert_eq!(style.arrow_style, 0);
    assert_eq!(style.arrow_line_style, 2);
    assert_eq!(style.mosaic_style, 4);
    assert_eq!(style.marker_shape, 23);
    assert_eq!(style.magnifier_scale, 6.0);
    assert_eq!(style.text_font_weight, 100);
    assert!(!style.shape_filled);
}

#[test]
/// 验证样式在临时目录原子保存且保留颜色透明度，损坏文件不会被读取过程覆盖，无参数、无返回值。
fn styles_round_trip_without_touching_other_files() {
    let directory = tempfile::tempdir().unwrap();
    let path = directory.path().join("nested/state.json");
    let style = AnnotationStyle {
        current_color: Color(0x80123456),
        text_background_color: Color(0x40112233),
        text_font_family: "思源黑体".into(),
        arrow_line_style: 2,
        ..AnnotationStyle::default()
    };
    state::save(&path, &style).unwrap();
    assert_eq!(state::load(&path), style);
    let json: serde_json::Value = serde_json::from_slice(&fs::read(&path).unwrap()).unwrap();
    assert_eq!(json["currentColor"], "#80123456");
    fs::write(&path, b"not-json").unwrap();
    assert_eq!(state::load(&path), AnnotationStyle::default());
    assert_eq!(fs::read(&path).unwrap(), b"not-json");
}

#[test]
/// 验证 Qt 标准颜色名称与各类十六进制精度格式，无参数、无返回值。
fn qt_colors_preserve_alpha_and_numeric_rounding() {
    for (text, argb) in [
        ("#abc", 0xffaabbcc),
        ("#123456", 0xff123456),
        ("#80123456", 0x80123456),
        ("#800000fff", 0xff8000ff),
        ("#80000000ffff", 0xff8000ff),
        ("ReD", 0xffff0000),
        ("transparent", 0),
    ] {
        assert_eq!(Color::parse(text), Some(Color(argb)), "{text}");
    }
    for text in ["#xy0000", "#1234", "none", "#12345", ""] {
        assert_eq!(Color::parse(text), None, "{text}");
    }
    assert_eq!(Color::parse("Light Grey"), Some(Color(0xffd3d3d3)));
    assert_eq!(Color::parse(" r e d\t"), Some(Color(0xffff0000)));
    assert_eq!(Color::parse("\nred"), None);
    assert_eq!(Color::parse(" #fff"), None);
}
