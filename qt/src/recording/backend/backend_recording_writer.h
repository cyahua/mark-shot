#pragma once

#include "recording/recording_options.h"
#include "recording/recording_writer.h"

#include <memory>

namespace markshot::recording {

/**
 * 【录制】【按需加载】创建持有后端库的写出器，最后一个写出器销毁后释放库引用。
 * @param options 录制配置。
 * @param error 输出加载或创建错误。
 * @param path 显式后端路径，留空时使用安装目录。
 * @return 写出器；缺少库或版本不匹配时返回空。
 */
std::unique_ptr<RecordingWriter> createBackendRecordingWriter(const RecordingOptions &options,
                                                             QString *error,
                                                             const QString &path = {});

}  // namespace markshot::recording
