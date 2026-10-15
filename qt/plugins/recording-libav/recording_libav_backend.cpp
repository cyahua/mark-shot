#include "recording/backend/recording_backend_api.h"

#include "recording_backend_log.h"
#include "recording/gif_recording_writer.h"
#include "recording/video_recording_writer.h"

#include <exception>

namespace markshot::recording {
namespace {

/** 使用 options 创建写出器并连接 sink 日志，失败写入 error 并返回空。 */
RecordingWriter *createWriter(const RecordingOptions &options, RecordingBackendLog sink, QString *error)
{
    setRecordingBackendLog(sink);
    try {
        if (options.mode == RecordingMode::Gif) return new GifRecordingWriter(options);
        return new VideoRecordingWriter(options);
    } catch (const std::exception &exception) {
        if (error) *error = QString::fromUtf8(exception.what());
    } catch (...) {
        if (error) *error = QStringLiteral("Cannot allocate recording writer");
    }
    return nullptr;
}

/** 在分配模块内销毁 writer，无返回值。 */
void destroyWriter(RecordingWriter *writer)
{
    delete writer;
}

}  // namespace
}  // namespace markshot::recording

/** 返回内部录制后端的版本化函数表，无参数；调用者须保持动态库存活。 */
extern "C" Q_DECL_EXPORT const markshot::recording::RecordingBackendApi *markshot_recording_backend_v1()
{
    using namespace markshot::recording;
    static const RecordingBackendApi api{
        RecordingBackendVersion, sizeof(RecordingBackendApi), &createWriter, &destroyWriter};
    return &api;
}
