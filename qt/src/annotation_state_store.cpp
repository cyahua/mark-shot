#include "annotation_state_store.h"

#include "annotation_style_codec.h"
#include "json_bridge.h"
#include "shot_editor_ffi.h"
#include "window_detection.h"

#include <QDir>

namespace markshot {

/** 返回当前显式配置目录内的标注状态路径，无参数。 */
QString annotationStateFilePath()
{
    return QDir(markShotConfigDir()).filePath(QStringLiteral("annotation-state.json"));
}

/** 读取并兼容旧版工具状态，无参数，返回完整样式；缺失或损坏时使用默认值。 */
AnnotationState loadAnnotationState()
{
    ShotBytes *output = nullptr;
    if (!rust::configSucceeded(shot_annotation_state_load(
            rust::byteView(annotationStateFilePath().toUtf8()), &output))) {
        return {};
    }
    const auto root = rust::takeObject(output);
    return root ? rust::decodeAnnotationStyle(*root) : AnnotationState{};
}

/** 把 state 转为跨语言协议并交给 Rust 原子保存，返回保存结果。 */
bool saveAnnotationState(const AnnotationState &state)
{
    const QJsonDocument document(rust::encodeAnnotationStyle(state));
    return rust::configSucceeded(shot_annotation_state_save(
        rust::byteView(annotationStateFilePath().toUtf8()),
        rust::byteView(document.toJson(QJsonDocument::Compact))));
}

} // namespace markshot
