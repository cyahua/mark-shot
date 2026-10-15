use shot_ffi::{
    geometry::shot_screen_crop,
    image::{ShotImageView, shot_image_filter},
    shot_abi_version, shot_last_error,
    stitch::*,
};
use shot_geometry::{Rect, Size};

/// 为 pixels 创建 width、height 的连续只读 ABI 视图，返回不持有内存的值。
fn view(pixels: &[u32], width: i32, height: i32) -> ShotImageView {
    ShotImageView {
        pixels: pixels.as_ptr(),
        len: pixels.len(),
        width,
        height,
        stride: width as usize,
    }
}

#[test]
/// 验证空指针、短缓冲区和巨大长度均返回错误，输出保持不变，无参数、无返回值。
fn invalid_arguments_never_write_outputs() {
    let pixels = [0xff123456; 4];
    let mut output = [0xfeedface; 4];
    // SAFETY: 【接口测试】【非法参数】非空指针指向本地有效数组，故意构造的空指针和巨大长度应在借用前被拒绝
    unsafe {
        assert_eq!(
            shot_image_filter(view(&pixels, 2, 2), 2, 0.0, output.as_mut_ptr(), 3),
            1
        );
        assert_eq!(output, [0xfeedface; 4]);
        let mut bad = view(&pixels, 2, 2);
        bad.pixels = std::ptr::null();
        assert_eq!(shot_image_filter(bad, 0, 2.0, output.as_mut_ptr(), 4), 1);
        bad = view(&pixels, 2, 2);
        bad.len = usize::MAX;
        assert_eq!(shot_image_filter(bad, 0, 2.0, output.as_mut_ptr(), 4), 1);
        assert_eq!(
            shot_screen_crop(std::ptr::null(), 1, Rect::default(), Size::default()),
            Rect::default()
        );
        let mut info = ShotStitchInfo::default();
        assert_eq!(shot_stitch_info(std::ptr::null_mut(), &mut info), 1);
        let needed = shot_last_error(std::ptr::null_mut(), 0);
        assert!(needed > 1);
        let mut error = [255_u8; 1];
        assert_eq!(shot_last_error(error.as_mut_ptr(), 1), needed);
        assert_eq!(error, [0]);
    }
}

#[test]
/// 验证实际 C ABI 的创建、帧提交、图像导出与独立释放，无参数、无返回值。
fn handle_lifecycle_and_pixel_ownership() {
    assert_eq!(shot_abi_version(), 2);
    let config = ShotStitchConfig {
        min_overlap: 32,
        accept_diff: 1.0,
        min_append: 10,
        approx_diff: 0.1,
    };
    let handle = shot_stitch_create(config);
    assert!(!handle.is_null());
    let mut pixels = vec![0xff123456; 64 * 100];
    let mut result = ShotStitchResult::default();
    // SAFETY: 【接口测试】【生命周期】句柄由 create 创建并只释放一次，输入输出均为本地独立数组
    unsafe {
        assert_eq!(
            shot_stitch_push(handle, view(&pixels, 64, 100), &mut result),
            0
        );
        assert_eq!(result.status, 0);
        pixels.fill(0);
        let mut info = ShotStitchInfo::default();
        assert_eq!(shot_stitch_info(handle, &mut info), 0);
        assert_eq!((info.width, info.height, info.frame_count), (64, 100, 1));
        let mut output = vec![0; 64 * 100];
        assert_eq!(
            shot_stitch_copy_image(handle, output.as_mut_ptr(), output.len()),
            0
        );
        assert!(output.iter().all(|&pixel| pixel == 0xff123456));
        assert_eq!(
            shot_stitch_push(handle, view(&pixels, 64, 100), std::ptr::null_mut()),
            1
        );
        assert_eq!(shot_stitch_info(handle, &mut info), 0);
        assert_eq!(info.frame_count, 1);
        shot_stitch_destroy(handle);
        shot_stitch_destroy(std::ptr::null_mut());
    }
}

#[test]
/// 验证线程内错误不会泄露到其他线程，无参数、无返回值。
fn error_storage_is_thread_local() {
    // SAFETY: 【接口测试】【错误隔离】只使用可直接拒绝的空句柄和零容量查询
    unsafe {
        assert_eq!(shot_stitch_set_axis(std::ptr::null_mut(), 0), 1);
        assert!(shot_last_error(std::ptr::null_mut(), 0) > 1);
    }
    let clean = std::thread::spawn(|| {
        // SAFETY: 【接口测试】【错误隔离】零容量查询不会读取空指针
        unsafe { shot_last_error(std::ptr::null_mut(), 0) }
    })
    .join()
    .unwrap();
    assert_eq!(clean, 1);
}
