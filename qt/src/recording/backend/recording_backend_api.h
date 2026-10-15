#pragma once

#include "recording/recording_options.h"
#include "recording/recording_writer.h"

#include <cstdarg>

namespace markshot::recording {

// 【录制】【后端边界】仅供同一版本、同一 Qt 和编译器构建的内部模块使用
using RecordingBackendLog = void (*)(const char *category, const char *format, va_list args);

struct RecordingBackendApi {
    quint32 version;
    quint32 size;
    /** 使用 options 和宿主 log 创建写出器；失败写入 error 并返回空。 */
    RecordingWriter *(*create)(const RecordingOptions &options, RecordingBackendLog log, QString *error);
    /** 在分配模块销毁 writer，无返回值。 */
    void (*destroy)(RecordingWriter *writer);
};

inline constexpr quint32 RecordingBackendVersion = 1;
using RecordingBackendEntry = const RecordingBackendApi *(*)();

}  // namespace markshot::recording
