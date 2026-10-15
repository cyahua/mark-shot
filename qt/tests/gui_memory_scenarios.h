#pragma once

#include <QSize>
#include <QString>

#include <functional>

namespace markshot::test {

using GuiMemoryCheckpoint = std::function<void(const QString &)>;

/** 按 scenario 执行真实窗口操作，frameSize/viewport 是源图与窗口尺寸，strokes/zoomPasses 控制笔画与连续缩放次数；checkpoint 采样，失败写入 error，返回是否成功。 */
bool runGuiMemoryScenario(const QString &scenario, QSize frameSize, QSize viewport, int strokes, int zoomPasses,
                          const GuiMemoryCheckpoint &checkpoint, QString *error);

}  // namespace markshot::test
