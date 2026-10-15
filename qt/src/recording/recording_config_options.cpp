#include "recording/recording_config_options.h"

#include "recording/recording_dialog_config.h"
#include "ui/i18n.h"

#include <QComboBox>
#include <QCursor>
#include <QGuiApplication>
#include <QScreen>

namespace markshot::recording::dialog {

/**
 * 返回录制模式标题。
 * @param mode 录制模式。
 * @return 标题文本。
 */
QString titleForMode(RecordingMode mode)
{
    return mode == RecordingMode::Gif ? MS_TR("GIF Recording") : MS_TR("Video Recording");
}

/**
 * 返回当前显示器。
 * @return 当前显示器，无法判断时返回主显示器。
 */
QScreen *currentScreen()
{
    QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
    return screen ? screen : QGuiApplication::primaryScreen();
}

/**
 * 查找当前显示器在来源列表中的下标。
 * @param sources 显示器来源列表。
 * @return 当前显示器来源下标。
 */
int currentDisplaySourceIndex(const QVector<DisplaySource> &sources)
{
    QScreen *screen = currentScreen();
    if (!screen) {
        return sources.isEmpty() ? -1 : 0;
    }

    for (int i = 0; i < sources.size(); ++i) {
        const DisplaySource &source = sources.at(i);
        if (!source.allOutputs && source.screenName == screen->name()) {
            return i;
        }
    }
    for (int i = 0; i < sources.size(); ++i) {
        const DisplaySource &source = sources.at(i);
        if (!source.allOutputs && source.geometry == screen->geometry()) {
            return i;
        }
    }
    return sources.isEmpty() ? -1 : 0;
}

/**
 * 按持久化键查找显示器来源下标。
 * @param sources 显示器来源列表。
 * @param key 持久化键。
 * @return 匹配下标，找不到时返回 -1。
 */
int displaySourceIndexForKey(const QVector<DisplaySource> &sources, const QString &key)
{
    if (key.trimmed().isEmpty()) {
        return -1;
    }
    for (int i = 0; i < sources.size(); ++i) {
        if (recordingDisplayPersistenceKey(sources.at(i)) == key) {
            return i;
        }
    }
    return -1;
}

/**
 * 给帧率下拉框写入阶梯选项。
 * @param combo 帧率下拉框。
 * @param mode 录制模式。
 * @param requestedFps 保存的帧率，负数时使用默认值。
 * @return 无返回值。
 */
void populateFrameRateOptions(QComboBox *combo, RecordingMode mode, int requestedFps)
{
    if (!combo) {
        return;
    }
    combo->clear();
    const QVector<int> values = mode == RecordingMode::Gif
        ? QVector<int>{6, 8, 10, 12, 15, 20, 24, 30}
        : QVector<int>{15, 24, 30, 48, 60};
    const int fallback = mode == RecordingMode::Gif ? 12 : 30;
    for (int fps : values) {
        combo->addItem(MS_TR("%1 fps").arg(fps), fps);
    }
    const int requestedIndex = combo->findData(requestedFps);
    const int fallbackIndex = combo->findData(fallback);
    combo->setCurrentIndex(requestedIndex >= 0 ? requestedIndex : (fallbackIndex >= 0 ? fallbackIndex : 0));
}

/**
 * 填充采集后端下拉框。
 * @param combo 采集后端下拉框。
 * @param requested 请求后端。
 * @return 无返回值。
 */
void populateBackendOptions(QComboBox *combo, RecordingCaptureBackend requested)
{
    if (!combo) {
        return;
    }
    combo->clear();
    combo->addItem(QStringLiteral("Auto"), static_cast<int>(RecordingCaptureBackend::Auto));
    combo->addItem(QStringLiteral("wlroots screencopy"), static_cast<int>(RecordingCaptureBackend::Wlroots));
    combo->addItem(QStringLiteral("PipeWire"), static_cast<int>(RecordingCaptureBackend::PipeWire));
    combo->addItem(QStringLiteral("Windows Graphics Capture"), static_cast<int>(RecordingCaptureBackend::WindowsWgc));
    combo->addItem(QStringLiteral("Polling"), static_cast<int>(RecordingCaptureBackend::Polling));
    const int index = combo->findData(static_cast<int>(requested));
    combo->setCurrentIndex(index >= 0 ? index : 0);
}

/**
 * 填充容器格式下拉框。
 * @param combo 容器格式下拉框。
 * @param requested 请求的容器格式。
 * @return 无返回值。
 */
void populateContainerOptions(QComboBox *combo, RecordingContainerFormat requested)
{
    if (!combo) {
        return;
    }
    combo->clear();
    combo->addItem(QStringLiteral("MP4"), static_cast<int>(RecordingContainerFormat::Mp4));
    combo->addItem(QStringLiteral("MKV"), static_cast<int>(RecordingContainerFormat::Mkv));
    const int index = combo->findData(static_cast<int>(requested));
    combo->setCurrentIndex(index >= 0 ? index : 0);
    combo->setToolTip(MS_TR("MKV keeps a playable file if the recording is interrupted."));
}

/**
 * 填充质量档位下拉框。
 * @param combo 质量档位下拉框。
 * @param requested 请求的质量档位。
 * @return 无返回值。
 */
void populateQualityOptions(QComboBox *combo, RecordingQuality requested)
{
    if (!combo) {
        return;
    }
    combo->clear();
    combo->addItem(MS_TR("Balanced"), static_cast<int>(RecordingQuality::Balanced));
    combo->addItem(MS_TR("Higher quality"), static_cast<int>(RecordingQuality::High));
    combo->addItem(MS_TR("Smaller file"), static_cast<int>(RecordingQuality::Efficient));
    const int index = combo->findData(static_cast<int>(requested));
    combo->setCurrentIndex(index >= 0 ? index : 0);
}

/**
 * 填充起录倒计时下拉框。
 * @param combo 倒计时下拉框。
 * @param requestedSeconds 请求的倒计时秒数。
 * @return 无返回值。
 */
void populateCountdownOptions(QComboBox *combo, int requestedSeconds)
{
    if (!combo) {
        return;
    }
    combo->clear();
    combo->addItem(MS_TR("Off"), 0);
    for (int seconds : {3, 5}) {
        combo->addItem(MS_TR("%1 seconds").arg(seconds), seconds);
    }
    const int index = combo->findData(requestedSeconds);
    combo->setCurrentIndex(index >= 0 ? index : 0);
}

/**
 * 从下拉框数据读取容器格式。
 * @param combo 容器格式下拉框。
 * @return 容器格式。
 */
RecordingContainerFormat containerFromCombo(const QComboBox *combo)
{
    bool ok = false;
    const int value = combo ? combo->currentData().toInt(&ok) : 0;
    if (ok && static_cast<RecordingContainerFormat>(value) == RecordingContainerFormat::Mkv) {
        return RecordingContainerFormat::Mkv;
    }
    return RecordingContainerFormat::Mp4;
}

/**
 * 从下拉框数据读取质量档位。
 * @param combo 质量档位下拉框。
 * @return 质量档位。
 */
RecordingQuality qualityFromCombo(const QComboBox *combo)
{
    bool ok = false;
    const int value = combo ? combo->currentData().toInt(&ok) : 0;
    if (!ok) {
        return RecordingQuality::Balanced;
    }
    switch (static_cast<RecordingQuality>(value)) {
    case RecordingQuality::Efficient:
        return RecordingQuality::Efficient;
    case RecordingQuality::High:
        return RecordingQuality::High;
    case RecordingQuality::Balanced:
        break;
    }
    return RecordingQuality::Balanced;
}

/**
 * 从下拉框数据读取录制模式。
 * @param combo 录制模式下拉框。
 * @param fallback 默认录制模式。
 * @return 录制模式。
 */
RecordingMode modeFromCombo(const QComboBox *combo, RecordingMode fallback)
{
    bool ok = false;
    const int value = combo ? combo->currentData().toInt(&ok) : 0;
    if (!ok) {
        return fallback;
    }
    return value == static_cast<int>(RecordingMode::Video)
        ? RecordingMode::Video
        : RecordingMode::Gif;
}

/**
 * 从下拉框数据读取采集后端。
 * @param combo 采集后端下拉框。
 * @return 采集后端。
 */
RecordingCaptureBackend backendFromCombo(const QComboBox *combo)
{
    bool ok = false;
    const int value = combo ? combo->currentData().toInt(&ok) : 0;
    if (!ok) {
        return RecordingCaptureBackend::Auto;
    }
    switch (static_cast<RecordingCaptureBackend>(value)) {
    case RecordingCaptureBackend::Wlroots:
    case RecordingCaptureBackend::PipeWire:
    case RecordingCaptureBackend::WindowsWgc:
    case RecordingCaptureBackend::Polling:
        return static_cast<RecordingCaptureBackend>(value);
    case RecordingCaptureBackend::Auto:
        break;
    }
    return RecordingCaptureBackend::Auto;
}


}
