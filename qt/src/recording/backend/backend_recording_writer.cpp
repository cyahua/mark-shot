#include "recording/backend/backend_recording_writer.h"

#include "debug_log.h"
#include "recording/backend/recording_backend_loader.h"

#include <QScopeGuard>
#include <utility>

namespace markshot::recording {
namespace {

/**
 * 将 error 中可能引用动态库只读段的字符串复制到堆，返回调用结果 result。
 * @param result 后端调用结果。
 * @param error 后端输出的可选错误字符串。
 * @return result 原值。
 */
bool ownError(bool result, QString *error)
{
    if (error) error->detach();
    return result;
}

class BackendRecordingWriter final : public RecordingWriter {
public:
    /** 接管 library 加载句柄与 writer 写出器，无返回值。 */
    BackendRecordingWriter(std::unique_ptr<RecordingBackendLibrary> library, RecordingWriter *writer)
        : m_library(std::move(library)), m_writer(writer)
    {
    }

    /** 先在原模块销毁写出器和后台线程，再释放库，无参数和返回值。 */
    ~BackendRecordingWriter() override
    {
        m_library->api().destroy(m_writer);
    }

    /** 使用 size 帧尺寸和 fps 帧率启动，失败写入 error，返回启动结果。 */
    bool start(QSize size, int fps, QString *error) override
    {
        return ownError(m_writer->start(size, fps, error), error);
    }

    /** 写入 sample，失败写入 error，返回写入结果。 */
    bool writeFrame(const RecordingFrameSample &sample, QString *error) override
    {
        return ownError(m_writer->writeFrame(sample, error), error);
    }

    /** 完成输出，失败写入 error，返回完成结果。 */
    bool finish(QString *error) override
    {
        return ownError(m_writer->finish(error), error);
    }

    /** 按 paused 设置音频暂停状态，无返回值。 */
    void setPaused(bool paused) override
    {
        m_writer->setPaused(paused);
    }

    /** 取消写出并释放编码与音频资源，无参数和返回值。 */
    void cancel() override
    {
        m_writer->cancel();
    }

private:
    std::unique_ptr<RecordingBackendLibrary> m_library;
    RecordingWriter *m_writer;
};

}  // namespace

std::unique_ptr<RecordingWriter> createBackendRecordingWriter(const RecordingOptions &options,
                                                             QString *error, const QString &path)
{
    auto library = RecordingBackendLibrary::load(error, path);
    if (!library) return {};
    RecordingWriter *writer = library->api().create(options, &markshot::debugLogV, error);
    ownError(writer != nullptr, error);
    if (!writer) return {};
    const auto cleanup = qScopeGuard([&] {
        if (writer) library->api().destroy(writer);
    });
    auto result = std::make_unique<BackendRecordingWriter>(std::move(library), writer);
    writer = nullptr;
    return result;
}

}  // namespace markshot::recording
