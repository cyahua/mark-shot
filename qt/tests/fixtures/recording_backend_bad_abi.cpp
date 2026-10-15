#include "recording/backend/recording_backend_api.h"

/** 返回不兼容的函数表以验证加载边界，无参数和返回值所有权转移。 */
extern "C" Q_DECL_EXPORT const markshot::recording::RecordingBackendApi *markshot_recording_backend_v1()
{
    using namespace markshot::recording;
    static const RecordingBackendApi api{
        RecordingBackendVersion + 1, sizeof(RecordingBackendApi), nullptr, nullptr};
    return &api;
}
