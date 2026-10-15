use shot_image::{
    Image, ImageError, ImageView, blur,
    filters::{self, Filter},
};

#[test]
/// 验证行尾填充不会进入输出，也不会修改输入，无参数、无返回值。
fn strided_input_is_not_modified() {
    let data = [0xff112233, 0xdeadbeef, 0xff445566];
    let view = ImageView::new(&data, 1, 2, 2).unwrap();
    let output = filters::apply(view, Filter::Invert).unwrap();
    assert_eq!(output.pixels(), &[0xffeeddcc, 0xffbbaa99]);
    assert_eq!(data[1], 0xdeadbeef);
}

#[test]
/// 验证透明像素与半透明像素使用正确的预乘舍入，无参数、无返回值。
fn color_filters_preserve_premultiplied_alpha() {
    let data = [0x80000000, 0x00123456, 0xffc86432];
    let view = ImageView::new(&data, 3, 1, 3).unwrap();
    assert_eq!(
        filters::apply(view, Filter::Invert).unwrap().pixels(),
        &[0x80808080, 0, 0xff379bcd]
    );
    assert_eq!(
        filters::apply(view, Filter::Brighten(0.5))
            .unwrap()
            .pixels()[2],
        0xffe4b299
    );
    let gray = filters::apply(view, Filter::Grayscale).unwrap().pixels()[2];
    assert_eq!((gray >> 16) & 255, (200 * 11 + 100 * 16 + 50 * 5) / 32);
}

#[test]
/// 验证透明边界模糊不会产生大于 alpha 的颜色通道，无参数、无返回值。
fn blur_respects_transparent_edges() {
    let mut pixels = vec![0; 40 * 10];
    for row in pixels.chunks_mut(40) {
        row[20..].fill(0xffffffff);
    }
    let input = Image::from_pixels(40, 10, pixels).unwrap();
    let output = blur::gaussian(input.view(), 6).unwrap();
    let edge = output.row(5)[20];
    assert!((edge >> 24) > 40 && (edge >> 24) < 215);
    for &pixel in output.pixels() {
        assert_eq!((pixel >> 16) & 255, pixel >> 24);
        assert_eq!((pixel >> 8) & 255, pixel >> 24);
    }
    assert_eq!(blur::gaussian(input.view(), 0).unwrap(), input);
}

#[test]
/// 验证尺寸乘法、短缓冲区、非法步长和非有限参数不会进入像素循环，无参数、无返回值。
fn malformed_buffers_are_rejected() {
    assert!(ImageView::new(&[], i32::MAX, i32::MAX, usize::MAX).is_err());
    assert!(ImageView::new(&[0; 4], 2, 2, 1).is_err());
    assert!(ImageView::new(&[0; 3], 2, 2, 2).is_err());
    let image = Image::from_pixels(2, 2, vec![0; 4]).unwrap();
    assert_eq!(
        blur::gaussian(image.view(), 4097),
        Err(ImageError::Parameter)
    );
    assert_eq!(
        filters::apply(image.view(), Filter::Brighten(f64::NAN)),
        Err(ImageError::Parameter)
    );
    assert_eq!(filters::blur_radius(f64::NAN), 1);
}

#[test]
/// 验证分块转置在非方形边缘仍可逆，无参数、无返回值。
fn transpose_is_an_involution() {
    let image = Image::from_pixels(35, 17, (0..35 * 17).collect()).unwrap();
    let transposed = image.transpose().unwrap();
    assert_eq!((transposed.width(), transposed.height()), (17, 35));
    assert_eq!(transposed.transpose().unwrap(), image);
}
