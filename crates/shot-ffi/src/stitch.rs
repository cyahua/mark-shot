use crate::boundary::{self, Result};
use crate::image::{ShotImageView, copy_image};
use shot_stitch::{Axis, Config, Edge, Status, Stitcher};

pub struct ShotStitch {
    state: Stitcher,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct ShotStitchConfig {
    pub min_overlap: i32,
    pub accept_diff: f32,
    pub min_append: i32,
    pub approx_diff: f32,
}
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct ShotStitchResult {
    pub status: i32,
    pub added: i32,
    pub edge: i32,
    pub position: i32,
    pub frame_length: i32,
}
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct ShotStitchInfo {
    pub width: i32,
    pub height: i32,
    pub frame_count: i32,
    pub total_height: i32,
    pub last_append: i32,
    pub axis: i32,
    pub axis_locked: i32,
    pub revision: u64,
}

/// 验证 handle 并借用独占状态，返回可修改会话。
/// # Safety
/// handle 必须来自 shot_stitch_create 且未释放；同一句柄不得并发调用。
unsafe fn state<'a>(handle: *mut ShotStitch) -> Result<&'a mut Stitcher> {
    boundary::validate(handle)?;
    // SAFETY: 【Rust接口】【会话借用】宿主保证句柄存活且独占
    Ok(&mut unsafe { &mut *handle }.state)
}

/// 根据 config 创建会话，返回 Rust 所有的句柄；失败返回空指针。
#[unsafe(no_mangle)]
pub extern "C" fn shot_stitch_create(config: ShotStitchConfig) -> *mut ShotStitch {
    boundary::value(std::ptr::null_mut(), || {
        let state = Stitcher::new(Config {
            min_overlap: config.min_overlap,
            accept_diff: config.accept_diff,
            min_append: config.min_append,
            approx_diff: config.approx_diff,
        })
        .map_err(|e| e.to_string())?;
        Ok(Box::into_raw(Box::new(ShotStitch { state })))
    })
}

/// 释放 handle；空指针无操作、无返回值。
/// # Safety
/// 非空 handle 必须由 create 返回，且只能释放一次、释放时不存在并发访问。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_stitch_destroy(handle: *mut ShotStitch) {
    boundary::run(|| {
        if handle.is_null() {
            return Ok(());
        }
        boundary::validate(handle)?;
        // SAFETY: 【Rust接口】【会话释放】所有权由创建函数交给宿主，此处回收一次
        unsafe {
            drop(Box::from_raw(handle));
        }
        Ok(())
    });
}

/// 把 frame 提交到 handle，并写入 output 业务结果；返回接口状态码，与业务 NoMatch 不同。
/// # Safety
/// handle 必须存活且独占，frame 必须有效，output 必须指向独立可写结果对象。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_stitch_push(
    handle: *mut ShotStitch,
    frame: ShotImageView,
    output: *mut ShotStitchResult,
) -> i32 {
    boundary::run(|| {
        boundary::validate(output)?;
        // SAFETY: 【Rust接口】【帧提交】输入有效且句柄独占，复制后不保留宿主指针
        let (state, image) = unsafe {
            (
                state(handle)?,
                frame.borrow()?.to_image().map_err(|e| e.to_string())?,
            )
        };
        let result = state.push(image).map_err(|e| e.to_string())?;
        let result = ShotStitchResult {
            status: match result.status {
                Status::FirstFrame => 0,
                Status::Appended => 1,
                Status::NoProgress => 2,
                Status::NoMatch => 3,
            },
            edge: match result.edge {
                Edge::None => 0,
                Edge::Start => 1,
                Edge::End => 2,
            },
            added: result.added,
            position: result.position,
            frame_length: result.frame_length,
        };
        // SAFETY: 【Rust接口】【帧提交】宿主结果对象已完成非空与对齐校验
        unsafe { boundary::write(output, result) }
    })
}

/// 查询 handle 的尺寸、统计和修订号到 output，返回接口状态码。
/// # Safety
/// handle 必须存活且独占，output 必须是独立可写对象。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_stitch_info(
    handle: *mut ShotStitch,
    output: *mut ShotStitchInfo,
) -> i32 {
    boundary::run(|| {
        // SAFETY: 【Rust接口】【会话查询】宿主保证句柄的独占生命周期
        let state = unsafe { state(handle)? };
        let stats = state.stats();
        let (width, height) = state.dimensions();
        let info = ShotStitchInfo {
            width,
            height,
            frame_count: stats.frame_count,
            total_height: stats.total_height,
            last_append: stats.last_append,
            axis: i32::from(state.axis() == Axis::Horizontal),
            axis_locked: i32::from(state.axis_locked()),
            revision: state.revision(),
        };
        // SAFETY: 【Rust接口】【会话查询】宿主保证输出对象可写
        unsafe { boundary::write(output, info) }
    })
}

/// 为 handle 设置 axis，0 为垂直、1 为水平；锁定后保持原轴，返回接口状态码。
/// # Safety
/// handle 必须存活且独占。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_stitch_set_axis(handle: *mut ShotStitch, axis: i32) -> i32 {
    boundary::run(|| {
        let axis = match axis {
            0 => Axis::Vertical,
            1 => Axis::Horizontal,
            _ => return Err("unknown scroll axis".into()),
        };
        // SAFETY: 【Rust接口】【滚动方向】宿主保证句柄的独占生命周期
        unsafe { state(handle)? }
            .set_axis(axis)
            .map_err(|e| e.to_string())?;
        Ok(())
    })
}

/// 把 handle 的完整图像复制到 output，len 为像素容量；返回接口状态码。
/// # Safety
/// handle 必须存活且独占，output 必须拥有 len 个可写 u32。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_stitch_copy_image(
    handle: *mut ShotStitch,
    output: *mut u32,
    len: usize,
) -> i32 {
    boundary::run(|| {
        // SAFETY: 【Rust接口】【长图导出】宿主保证句柄有效，图像快照独立于宿主输出
        let state = unsafe { state(handle)? };
        let (width, height) = state.dimensions();
        if len < width as usize * height as usize {
            return Err("output pixel buffer is too small".into());
        }
        let image = state
            .full_image()
            .map_err(|e| e.to_string())?
            .ok_or("stitch session has no image")?;
        // SAFETY: 【Rust接口】【长图导出】宿主提供足够可写空间，结果复制不暴露 Rust 分配器
        unsafe { copy_image(&image, output, len) }
    })
}
