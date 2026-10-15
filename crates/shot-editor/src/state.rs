use crate::AnnotationStyle;
use shot_config::{Result, store};
use std::path::Path;

/// 从显式 path 读取工具状态；缺失、无法读取或损坏时返回内置默认值，不创建文件。
pub fn load(path: &Path) -> AnnotationStyle {
    store::read(path).map_or_else(
        |_| AnnotationStyle::default(),
        |root| AnnotationStyle::from_legacy(&root),
    )
}

/// 把 style 原子写入显式 path，返回保存结果；沿用原状态字段与颜色格式。
pub fn save(path: &Path, style: &AnnotationStyle) -> Result<()> {
    let root = serde_json::to_value(style)?;
    store::write(path, &root)
}
