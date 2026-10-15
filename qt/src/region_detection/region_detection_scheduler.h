#pragma once

#include <QFuture>
#include <QImage>

#include <memory>

namespace markshot::region_detection {

class RegionDetector;

/**
 * 【选区识别】【任务调度】在有界专用队列分析图像，复用少量工作线程的临时分配。
 * @param image 只读共享图像，队列持有它直到分析完成。
 * @return 异步检测结果，不访问界面控件。
 */
QFuture<std::shared_ptr<RegionDetector>> scheduleRegionDetection(QImage image);

/** 【选区识别】【忙碌状态】返回队列是否仍在处理图像，供空闲回收和诊断等待使用，无参数。 */
bool regionDetectionBusy();

}  // namespace markshot::region_detection
