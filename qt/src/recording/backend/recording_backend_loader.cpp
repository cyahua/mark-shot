#include "recording/backend/recording_backend_loader.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

namespace markshot::recording {
namespace {

/** 返回当前平台随应用部署的后端路径，无参数；未安装时返回空。 */
QString bundledBackendPath()
{
#ifdef Q_OS_WIN
    const QString name = QStringLiteral("mark-shot-recording-libav.dll");
#elif defined(Q_OS_MACOS)
    const QString name = QStringLiteral("libmark-shot-recording-libav.dylib");
#else
    const QString name = QStringLiteral("libmark-shot-recording-libav.so");
#endif
    const QDir app(QCoreApplication::applicationDirPath());
    const QStringList directories = {
        app.filePath(QStringLiteral("recording")),
#ifdef MARK_SHOT_RECORDING_INSTALL_DIR
        app.filePath(QStringLiteral(MARK_SHOT_RECORDING_INSTALL_DIR)),
#endif
    };
    for (const QString &directory : directories) {
        const QFileInfo file(QDir(directory).filePath(name));
        if (file.isFile()) {
            return file.absoluteFilePath();
        }
    }
    return {};
}

}  // namespace

RecordingBackendLibrary::RecordingBackendLibrary(const QString &path)
    : m_library(path)
{
    m_library.setLoadHints(QLibrary::ResolveAllSymbolsHint);
}

RecordingBackendLibrary::~RecordingBackendLibrary()
{
    m_library.unload();
}

std::unique_ptr<RecordingBackendLibrary> RecordingBackendLibrary::load(QString *error,
                                                                      const QString &path)
{
    if (error) error->clear();
    // 1. 【录制】【按需加载】仅查找明确的构建或安装位置，不使用当前工作目录
    const QString resolved = path.isEmpty() ? bundledBackendPath() : QFileInfo(path).absoluteFilePath();
    if (resolved.isEmpty()) {
        if (error) *error = QStringLiteral("Recording backend is not installed");
        return {};
    }
    auto library = std::unique_ptr<RecordingBackendLibrary>(new RecordingBackendLibrary(resolved));
    // 2. 【录制】【按需加载】立即解析全部导入，缺失依赖在录制开始前返回错误
    if (!library->m_library.load()) {
        if (error) *error = library->m_library.errorString();
        return {};
    }
    const auto entry = reinterpret_cast<RecordingBackendEntry>(
        library->m_library.resolve("markshot_recording_backend_v1"));
    library->m_api = entry ? entry() : nullptr;
    const auto *api = library->m_api;
    // 3. 【录制】【接口校验】调用函数表前检查版本、尺寸和必需入口
    if (!api || api->version != RecordingBackendVersion || api->size != sizeof(RecordingBackendApi)
        || !api->create || !api->destroy) {
        if (error) *error = QStringLiteral("Recording backend version does not match the application");
        return {};
    }
    return library;
}

const RecordingBackendApi &RecordingBackendLibrary::api() const
{
    return *m_api;
}

}  // namespace markshot::recording
