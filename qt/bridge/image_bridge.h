#pragma once
#include "shot_ffi.h"
#include <QDebug>
#include <QImage>
namespace markshot::rust
{
/**
 * 【Rust接口】【图像借用】为已转换的 32 位 image 创建同步借用视图
 * @param image 直通或预乘 ARGB 图像，必须在调用期间保持存活
 * @return 无所有权的像素视图
 */
inline ShotImageView imageView(const QImage &image)
{
    return {reinterpret_cast<const uint32_t *>(image.constBits()),
            static_cast<size_t>(image.sizeInBytes() / 4), image.width(), image.height(),
            static_cast<size_t>(image.bytesPerLine() / 4)};
}
/**
 * 【Rust接口】【错误报告】将 status 对应的线程内错误交给 Qt 日志
 * @param status Rust 接口状态码
 * @return 成功时为 true，失败时记录 error 级别日志并返回 false
 */
inline bool succeeded(int status)
{
    if (status == 0)
    {
        return true;
    }
    uint8_t error[512] = {};
    shot_last_error(error, sizeof(error));
    qCritical().noquote() << QStringLiteral("【Rust核心】【接口调用】")
                          << QString::fromUtf8(reinterpret_cast<const char *>(error));
    return false;
}
} // namespace markshot::rust
