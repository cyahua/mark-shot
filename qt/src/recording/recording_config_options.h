#pragma once

#include "recording/recording_options.h"
#include <QVector>

class QComboBox;

namespace markshot::recording::dialog {

/// @brief 根据录制模式返回窗口标题
/// @param mode 录制模式
/// @return 标题文本
QString titleForMode(RecordingMode mode);

/// @brief 查找光标所在显示器
/// @param sources 可用的显示器来源
/// @return 来源索引，无来源时为 -1
int currentDisplaySourceIndex(const QVector<DisplaySource> &sources);

/// @brief 按持久化标识查找显示器
/// @param sources 可用显示器列表
/// @param key 保存的显示器标识
/// @return 匹配索引，未匹配时为 -1
int displaySourceIndexForKey(const QVector<DisplaySource> &sources, const QString &key);

/// @brief 填充对应模式的帧率选项
/// @param combo 帧率控件
/// @param mode 录制模式
/// @param requestedFps 期望帧率，负数时使用默认值
/// @return 无返回值
void populateFrameRateOptions(QComboBox *combo, RecordingMode mode, int requestedFps = -1);

/// @brief 填充采集后端选项
/// @param combo 选项控件
/// @param requested 保存的后端
/// @return 无返回值
void populateBackendOptions(QComboBox *combo, RecordingCaptureBackend requested);

/// @brief 填充视频容器选项
/// @param combo 选项控件
/// @param requested 保存的容器
/// @return 无返回值
void populateContainerOptions(QComboBox *combo, RecordingContainerFormat requested);

/// @brief 填充质量档位
/// @param combo 选项控件
/// @param requested 保存的质量档位
/// @return 无返回值
void populateQualityOptions(QComboBox *combo, RecordingQuality requested);

/// @brief 填充录制倒计时选项
/// @param combo 选项控件
/// @param requestedSeconds 保存的倒计时秒数
/// @return 无返回值
void populateCountdownOptions(QComboBox *combo, int requestedSeconds);

/// @brief 读取视频容器选项
/// @param combo 选项控件
/// @return 容器格式
RecordingContainerFormat containerFromCombo(const QComboBox *combo);

/// @brief 读取录制质量选项
/// @param combo 选项控件
/// @return 质量档位
RecordingQuality qualityFromCombo(const QComboBox *combo);

/// @brief 读取录制模式选项
/// @param combo 选项控件
/// @param fallback 选项缺失时的模式
/// @return 录制模式
RecordingMode modeFromCombo(const QComboBox *combo, RecordingMode fallback);

/// @brief 读取采集后端选项
/// @param combo 选项控件
/// @return 采集后端
RecordingCaptureBackend backendFromCombo(const QComboBox *combo);

}
