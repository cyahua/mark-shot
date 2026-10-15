#pragma once

#include "recording/backend/recording_backend_api.h"

#include <QLibrary>
#include <memory>

namespace markshot::recording {

class RecordingBackendLibrary final {
public:
    /** 销毁加载句柄并释放本次加载引用，无参数和返回值。 */
    ~RecordingBackendLibrary();

    /**
     * 【录制】【按需加载】从应用或安装目录加载后端，不搜索工作目录。
     * @param error 输出诊断信息。
     * @param path 显式库路径，留空时使用随应用安装的后端。
     * @return 已校验版本的加载句柄，失败返回空。
     */
    static std::unique_ptr<RecordingBackendLibrary> load(QString *error, const QString &path = {});

    /** 返回后端函数表；仅在本加载句柄存活期间有效，无参数。 */
    const RecordingBackendApi &api() const;

private:
    /** 使用 path 创建尚未加载的库句柄，无返回值。 */
    explicit RecordingBackendLibrary(const QString &path);

    QLibrary m_library;
    const RecordingBackendApi *m_api = nullptr;
};

}  // namespace markshot::recording
