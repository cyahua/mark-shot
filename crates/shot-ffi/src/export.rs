use serde_json::Value;
use shot_config::{MAX_JSON_BYTES, export::FrameSettings};
use shot_geometry::Size;
use shot_image::export::{self, FrameOptions, Layout};

use crate::boundary;
use crate::bytes::{self, ShotBytesView};
use crate::image::{ShotImageView, copy_image};

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct ShotExportConfig {
    pub enabled: i32,
    pub padding: i32,
    pub shadow_radius: i32,
    pub shadow_offset_y: i32,
    pub corner_radius: f64,
    pub shadow_opacity: f64,
    pub shadow_color: u32,
    pub reserved: u32,
}

impl From<ShotExportConfig> for FrameOptions {
    /// 把 value 的 ABI 字段转换为领域参数，返回显式外框选项。
    fn from(value: ShotExportConfig) -> Self {
        Self {
            enabled: value.enabled != 0,
            padding: value.padding,
            corner_radius: value.corner_radius,
            shadow_radius: value.shadow_radius,
            shadow_offset_y: value.shadow_offset_y,
            shadow_opacity: value.shadow_opacity,
            shadow_color: value.shadow_color,
        }
    }
}

/// 从 root 应用配置 JSON 解析外框参数到 output，返回状态码。
/// # Safety
/// root 必须是有效字节视图，output 必须指向独立可写结构。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_export_config(
    root: ShotBytesView,
    output: *mut ShotExportConfig,
) -> i32 {
    boundary::run(|| {
        // SAFETY: 【Rust接口】【外框配置】输入同步解码，业务解析集中在配置核心
        unsafe {
            let root: Value = bytes::json(root, MAX_JSON_BYTES)?;
            let settings = FrameSettings::from_root(&root);
            boundary::write(
                output,
                ShotExportConfig {
                    enabled: i32::from(settings.enabled),
                    padding: settings.padding,
                    corner_radius: settings.corner_radius,
                    shadow_radius: settings.shadow_radius,
                    shadow_offset_y: settings.shadow_offset_y,
                    shadow_opacity: settings.shadow_opacity,
                    shadow_color: 0xff000000,
                    reserved: 0,
                },
            )
        }
    })
}

/// 根据 source 物理尺寸与 config 计算导出尺寸到 output，返回状态码，不分配图像。
/// # Safety
/// output 必须指向独立可写尺寸结构。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_export_size(
    source: Size,
    config: ShotExportConfig,
    output: *mut Size,
) -> i32 {
    boundary::run(|| {
        let layout = Layout::new(source.width, source.height, &config.into())
            .map_err(|error| error.to_string())?;
        // SAFETY: 【Rust接口】【导出尺寸】宿主提供独立可写结构，尺寸已由核心校验
        unsafe {
            boundary::write(
                output,
                Size {
                    width: layout.width,
                    height: layout.height,
                },
            )
        }
    })
}

/// 对预乘 source 应用 config，将结果写入 output 的 len 个像素空间，返回状态码。
/// # Safety
/// source 必须是有效只读视图，output 必须拥有 len 个可写像素且不得与输入重叠。
#[unsafe(no_mangle)]
pub unsafe extern "C" fn shot_image_export(
    source: ShotImageView,
    config: ShotExportConfig,
    output: *mut u32,
    len: usize,
) -> i32 {
    boundary::run(|| {
        boundary::validate(output)?;
        let options = config.into();
        let layout = Layout::new(source.width, source.height, &options)
            .map_err(|error| error.to_string())?;
        if len < layout.width as usize * layout.height as usize {
            return Err("export pixel buffer is too small".into());
        }
        // SAFETY: 【Rust接口】【外框绘制】输入同步借用，结果由 Rust 独立持有
        let image = export::render(unsafe { source.borrow()? }, &options)
            .map_err(|error| error.to_string())?;
        // SAFETY: 【Rust接口】【导出输出】输出容量已检查，像素计算全部成功后才写入宿主缓冲
        unsafe { copy_image(&image, output, len) }
    })
}
