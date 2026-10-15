#pragma once

#include "recording/backend/recording_backend_api.h"

namespace markshot::recording {

/** 设置宿主日志接收函数 sink，空指针表示不输出，无返回值。 */
void setRecordingBackendLog(RecordingBackendLog sink);

}  // namespace markshot::recording
