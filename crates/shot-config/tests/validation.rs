use serde_json::json;
use shot_config::{defaults, validation};

#[test]
/// 验证布尔值与整数沿用 Qt 对历史配置的转换规则，无参数、无返回值。
fn scalar_compatibility_matches_qt_rules() {
    for value in [json!(true), json!(" Enabled "), json!("YES"), json!(-2)] {
        assert_eq!(validation::boolean(&value), Some(true));
    }
    for value in [
        json!(false),
        json!("off"),
        json!(0),
        json!(1e-13),
        json!(1e-12),
    ] {
        assert_eq!(validation::boolean(&value), Some(false));
    }
    for value in [json!(null), json!([]), json!("maybe")] {
        assert_eq!(validation::boolean(&value), None);
    }
    assert_eq!(validation::integer(&json!(" +42 ")), Some(42));
    assert_eq!(validation::integer(&json!(3.5)), Some(0));
    assert_eq!(validation::integer(&json!(2147483648_i64)), Some(0));
    assert_eq!(validation::integer(&json!("2147483648")), None);
    assert_eq!(validation::integer(&json!("3.0")), None);
    assert_eq!(validation::integer(&json!(false)), None);
    assert_eq!(validation::real(&json!(" -2.5e2 ")), Some(-250.0));
    assert_eq!(validation::real(&json!("NaN")), None);
    assert_eq!(
        validation::normalized_key("  Font_Size-X.Y  "),
        "fontsizexy"
    );
}

#[test]
/// 验证默认配置只从传入上下文接收平台差异，不读取用户环境，无参数、无返回值。
fn defaults_keep_platform_and_ui_inputs_explicit() {
    let mut context = defaults::Context {
        window_detection_command: "window-helper".into(),
        pinned_border_color: "#123456".into(),
        windows: false,
    };
    let linux = defaults::application(&context);
    assert_eq!(linux["windows"]["tray"]["enabled"], false);
    assert_eq!(linux["pinnedWindow"]["borderColor"], "#123456");
    assert_eq!(linux["windowDetection"]["command"], "window-helper");
    assert_eq!(linux["export"]["imageFrame"]["padding"], 112);
    context.windows = true;
    assert_eq!(
        defaults::application(&context)["windows"]["tray"]["enabled"],
        true
    );
}

#[test]
/// 验证导出别名、开关及范围约束集中在配置核心，无参数、无返回值。
fn export_settings_preserve_aliases_and_clamp_ranges() {
    use shot_config::export::FrameSettings;

    let root = json!({"export": {"imageFrame": {"visible": "enabled", "margin": "-12",
        "radius": -7, "shadowBlur": 999, "shadowYOffset": "4", "shadowAlpha": "2.5"}}});
    let settings = FrameSettings::from_root(&root);
    assert!(settings.enabled);
    assert_eq!(settings.padding, 0);
    assert_eq!(settings.corner_radius, 0.0);
    assert_eq!(settings.shadow_radius, 128);
    assert_eq!(settings.shadow_offset_y, 4);
    assert_eq!(settings.shadow_opacity, 1.0);
    assert_eq!(
        FrameSettings::from_root(&json!({"export": {"imageFrame": "true"}})),
        FrameSettings {
            enabled: true,
            ..FrameSettings::default()
        }
    );
    let settings = FrameSettings::from_root(
        &json!({"export": {"imageFrame": {"padding": null, "margin": 42, "shadowOpacity": "NaN"}}}),
    );
    assert_eq!(settings.padding, 112);
    assert_eq!(settings.shadow_opacity, 0.32);
}
