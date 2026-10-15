use shot_ffi::export::*;
use shot_ffi::image::ShotImageView;
use shot_geometry::Size;

#[test]
/// 验证外框 ABI 布局与物理像素输出，并检查错误情况下宿主缓冲区保持不变，无参数、无返回值。
fn export_layout_and_pixels_obey_the_host_buffer_contract() {
    assert_eq!(std::mem::size_of::<ShotExportConfig>(), 40);
    let source = [0xff123456; 4];
    let view = ShotImageView {
        pixels: source.as_ptr(),
        len: 4,
        width: 2,
        height: 2,
        stride: 2,
    };
    let options = ShotExportConfig {
        enabled: 1,
        padding: 1,
        ..ShotExportConfig::default()
    };
    let mut size = Size {
        width: -1,
        height: -1,
    };
    let mut output = [0xdeadbeef; 16];
    // SAFETY: 【接口测试】【导出边界】输入输出是独立数组，声明长度均对应有效存储
    unsafe {
        assert_eq!(
            shot_export_size(
                Size {
                    width: 2,
                    height: 2
                },
                options,
                &mut size
            ),
            0
        );
        assert_eq!((size.width, size.height), (4, 4));
        assert_eq!(shot_image_export(view, options, output.as_mut_ptr(), 15), 1);
        assert_eq!(output, [0xdeadbeef; 16]);
        assert_eq!(
            shot_image_export(
                ShotImageView {
                    pixels: std::ptr::null(),
                    ..view
                },
                options,
                output.as_mut_ptr(),
                16
            ),
            1
        );
        assert_eq!(output, [0xdeadbeef; 16]);
        assert_eq!(shot_image_export(view, options, output.as_mut_ptr(), 16), 0);
        assert_eq!(
            output,
            [
                0, 0, 0, 0, 0, source[0], source[1], 0, 0, source[2], source[3], 0, 0, 0, 0, 0
            ]
        );
        let invalid = ShotExportConfig {
            padding: i32::MAX,
            ..options
        };
        assert_eq!(
            shot_export_size(
                Size {
                    width: 2,
                    height: 2
                },
                invalid,
                &mut size
            ),
            1
        );
        assert_eq!((size.width, size.height), (4, 4));
    }
}
