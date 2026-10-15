use shot_geometry::{Point, Rect, RectF, Screen, Size, capture, line, selection};

#[test]
/// 验证负坐标屏幕上的非整数缩放与往返换算，无参数、无返回值。
fn negative_origin_and_fractional_scale() {
    let source = Rect {
        x: -1920,
        y: -200,
        width: 1920,
        height: 1080,
    };
    let selected = Rect {
        x: -1820,
        y: -120,
        width: 800,
        height: 400,
    };
    let size = Size {
        width: 2880,
        height: 1620,
    };
    let pixels = capture::scaled_crop(source, selected, size);
    assert_eq!(
        pixels,
        Rect {
            x: 150,
            y: 120,
            width: 1200,
            height: 600
        }
    );
    assert_eq!(capture::logical_rect(pixels, source, size), selected);
}

#[test]
/// 验证混合缩放拼接仅在物理图像布局吻合时生效，无参数、无返回值。
fn mixed_dpi_requires_matching_physical_layout() {
    let screens = [
        Screen {
            geometry: Rect {
                x: -1280,
                y: 0,
                width: 1280,
                height: 720,
            },
            dpr: 1.0,
        },
        Screen {
            geometry: Rect {
                x: 0,
                y: 0,
                width: 1920,
                height: 1080,
            },
            dpr: 1.5,
        },
    ];
    assert_eq!(
        capture::per_screen_crop(
            &screens,
            screens[1].geometry,
            Size {
                width: 4160,
                height: 1620
            }
        ),
        Rect {
            x: 1280,
            y: 0,
            width: 2880,
            height: 1620
        }
    );
    assert_eq!(
        capture::per_screen_crop(
            &screens,
            screens[1].geometry,
            Size {
                width: 3200,
                height: 1080
            }
        ),
        Rect::default()
    );
    assert_eq!(
        capture::per_screen_crop(
            &[],
            screens[0].geometry,
            Size {
                width: 1,
                height: 1
            }
        ),
        Rect::default()
    );
}

#[test]
/// 验证大坐标不发生整数溢出，空交集和零尺寸返回空矩形，无参数、无返回值。
fn malformed_geometry_is_bounded() {
    let a = Rect {
        x: i32::MAX - 10,
        y: i32::MIN,
        width: 100,
        height: 100,
    };
    assert_eq!(a.intersect(a), a);
    assert_eq!(
        capture::scaled_crop(
            a,
            a,
            Size {
                width: 100,
                height: 100
            }
        ),
        Rect {
            x: 0,
            y: 0,
            width: 100,
            height: 100
        }
    );
    assert_eq!(
        capture::logical_rect(
            Rect {
                x: 90,
                y: 0,
                width: 10,
                height: 10
            },
            a,
            Size {
                width: 100,
                height: 100
            }
        ),
        Rect::default()
    );
    assert_eq!(capture::scaled_crop(a, a, Size::default()), Rect::default());
}

#[test]
/// 验证四个拖动方向均保持宽高比且不越过图像边界，无参数、无返回值。
fn selection_creation_respects_all_quadrants() {
    let anchor = Point { x: 150.0, y: 100.0 };
    for x in [-500.0, 500.0] {
        for y in [-500.0, 500.0] {
            let rect = selection::create(
                anchor,
                Point { x, y },
                16.0 / 9.0,
                Size {
                    width: 300,
                    height: 200,
                },
            );
            assert!((rect.width / rect.height - 16.0 / 9.0).abs() < 1e-10);
            assert!(rect.x >= 0.0 && rect.y >= 0.0);
            assert!(rect.bottom_right().x <= 300.0 && rect.bottom_right().y <= 200.0);
        }
    }
}

#[test]
/// 验证选区缩放、最小尺寸、非法比例以及空图像处理，无参数、无返回值。
fn selection_resize_and_invalid_values() {
    let current = RectF {
        x: 90.0,
        y: 90.0,
        width: 10.0,
        height: 10.0,
    };
    assert_eq!(
        selection::resize(
            current,
            Size {
                width: 50,
                height: 60
            },
            Size {
                width: 100,
                height: 100
            }
        ),
        RectF {
            x: 50.0,
            y: 40.0,
            width: 50.0,
            height: 60.0
        }
    );
    assert_eq!(
        selection::resize(current, Size::default(), Size::default()),
        RectF::default()
    );
    assert_eq!(selection::related_side(100, f64::NAN, true), 0);
    let tiny = RectF {
        x: 90.0,
        y: 90.0,
        width: 1.0,
        height: 1.0,
    };
    assert_eq!(
        selection::adjust(
            current,
            selection::Handle::BottomRight,
            tiny,
            1.0,
            Size {
                width: 100,
                height: 100
            }
        ),
        current
    );
}

#[test]
/// 验证线段投影而不是径向缩放，并覆盖零长度输入，无参数、无返回值。
fn line_projection_and_zero_length() {
    let start = Point { x: 10.0, y: 10.0 };
    assert_eq!(line::constrained_end(start, start), start);
    let result = line::constrained_end(start, Point { x: 110.0, y: 20.0 });
    assert!((result.x - 110.0).abs() < 1e-10 && (result.y - 10.0).abs() < 1e-10);
}
