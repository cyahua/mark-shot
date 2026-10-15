use crate::boundary;
use crate::image::ShotImageView;
use shot_stitch::fixed_regions::FixedRegions;

pub struct ShotFixedRegions {
    detector: FixedRegions,
}
#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct ShotFixedObservation {
    pub header: i32,
    pub footer: i32,
    pub valid: i32,
    pub top: i32,
    pub bottom: i32,
}

/// 创建独立固定区域检测器；无参数，返回 Rust 句柄，失败为空指针。
#[unsafe(no_mangle)]
pub extern "C" fn shot_fixed_create() -> *mut ShotFixedRegions {
    boundary::value(std::ptr::null_mut(), || {
        Ok(Box::into_raw(Box::new(ShotFixedRegions {
            detector: FixedRegions::default(),
        })))
    })
}
/// 释放 handle；空指针无操作。
/// # Safety
/// handle 必须由 create 返回且只释放一次，不得与其他调用并发。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_fixed_destroy(handle: *mut ShotFixedRegions) {
    boundary::run(|| {
        if !handle.is_null() {
            boundary::validate(handle)?;
            // SAFETY: 【Rust接口】【检测器释放】回收创建函数转移给宿主的唯一所有权
            unsafe {
                drop(Box::from_raw(handle));
            }
        }
        Ok(())
    });
}
/// 清空 handle 的历史估计，返回接口状态码。
/// # Safety
/// handle 必须存活且独占。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_fixed_reset(handle: *mut ShotFixedRegions) -> i32 {
    boundary::run(|| {
        boundary::validate(handle)?;
        // SAFETY: 【Rust接口】【检测器重置】调用者保证句柄存活且没有并发借用
        unsafe { &mut *handle }.detector.reset();
        Ok(())
    })
}
/// 对 previous、current 按 offset 观察固定栏并写入 output，返回接口状态码。
/// # Safety
/// handle 必须存活且独占，两个图像视图必须有效，output 必须是独立可写对象。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_fixed_observe(
    handle: *mut ShotFixedRegions,
    previous: ShotImageView,
    current: ShotImageView,
    offset: i32,
    output: *mut ShotFixedObservation,
) -> i32 {
    boundary::run(|| {
        boundary::validate(handle)?;
        boundary::validate(output)?;
        // SAFETY: 【Rust接口】【固定栏观察】输入指针只在调用内借用，宿主保证它们有效
        let (previous, current) =
            unsafe { (previous.borrow()?.to_image(), current.borrow()?.to_image()) };
        let previous = previous.map_err(|e| e.to_string())?;
        let current = current.map_err(|e| e.to_string())?;
        // SAFETY: 【Rust接口】【固定栏观察】句柄已验证且宿主保证独占
        let detector = &mut unsafe { &mut *handle }.detector;
        let result = detector.observe(&previous, &current, offset);
        let result = ShotFixedObservation {
            header: result.header,
            footer: result.footer,
            valid: i32::from(result.valid),
            top: detector.top(),
            bottom: detector.bottom(),
        };
        // SAFETY: 【Rust接口】【固定栏观察】宿主输出对象有效且与输入不重叠
        unsafe { boundary::write(output, result) }
    })
}
