#include "recording/recording_config_dialog.h"

#include "app_config_store.h"
#include "recording/audio/audio_capture_reader_factory.h"
#include "recording/audio/audio_input_device_list.h"
#include "recording/recording_dialog_config.h"
#include "recording/recording_config_options.h"
#include "ui/disclosure_section.h"
#include "ui/form_row_visibility.h"
#include "recording/recording_display_source.h"
#include "recording/recording_file_naming.h"
#include "settings/settings_design_tokens.h"
#include "ui/i18n.h"
#include "ui/interface_theme_config.h"
#include "ui/theme.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QLineEdit>

namespace markshot::recording {
RecordingConfigDialog::RecordingConfigDialog(RecordingMode mode, QWidget *parent)
    : QDialog(parent)
    , m_mode(mode)
    , m_sources(availableDisplaySources())
{
    const RecordingDialogConfig persisted = configuredRecordingDialogConfig(m_mode);
    m_videoFps = persisted.videoFps;
    m_gifFps = persisted.gifFps;
    setWindowTitle(dialog::titleForMode(m_mode));
    setObjectName(QStringLiteral("recordingConfigDialog"));
    setFont(markshot::theme::uiFont(10));
    setModal(true);
    setMinimumSize(380, 320);
    resize(480, 400);
    applyDialogTheme();
    buildLayout(persisted);
}

void RecordingConfigDialog::applyDialogTheme()
{
    // 与设置界面共用同一套主题：深/浅色跟随应用配置，避免录制入口
    // 出现与主界面割裂的系统原生灰色样式。
    bool ok = false;
    const QJsonObject root = markshot::readAppConfigRoot(&ok);
    const markshot::ui::UiThemeMode configuredMode = ok
        ? markshot::ui::uiThemeModeFromConfigRoot(root)
        : markshot::ui::UiThemeMode::System;
    const markshot::ui::UiThemeMode effectiveMode =
        markshot::ui::effectiveUiThemeMode(configuredMode);
    setPalette(markshot::settings::tokens::settingsPalette(effectiveMode));
    setStyleSheet(markshot::settings::tokens::settingsStyleSheet(effectiveMode));
}

void RecordingConfigDialog::populateAudioDevices(const QString &persistedDevice)
{
    if (!m_audioDevice) {
        return;
    }
    m_audioDevice->clear();
    m_audioDevice->addItem(MS_TR("System default input"), QString());
    const QVector<AudioInputDevice> devices = listAudioInputDevices();
    for (const AudioInputDevice &device : devices) {
        const QString label = device.isMonitor
            ? MS_TR("%1 (system audio)").arg(device.description)
            : device.description;
        m_audioDevice->addItem(label, device.name);
        m_audioDevice->setItemData(m_audioDevice->count() - 1, device.name, Qt::ToolTipRole);
    }
    const int persistedIndex = m_audioDevice->findData(persistedDevice.trimmed());
    m_audioDevice->setCurrentIndex(persistedIndex >= 0 ? persistedIndex : 0);
}

RecordingOptions RecordingConfigDialog::options() const
{
    RecordingOptions result;
    result.mode = m_mode;
    bool fpsOk = false;
    const int fallbackFps = m_mode == RecordingMode::Gif ? 12 : 30;
    const int selectedFps = m_fps ? m_fps->currentData().toInt(&fpsOk) : fallbackFps;
    result.fps = fpsOk ? selectedFps : fallbackFps;
    result.includeAudio = m_mode == RecordingMode::Video && m_audio && m_audio->isEnabled() && m_audio->isChecked();
    result.audioDevice = m_audioDevice ? m_audioDevice->currentData().toString() : QString();
    result.captureBackend = dialog::backendFromCombo(m_backend);
    bool countdownOk = false;
    const int countdown = m_countdown ? m_countdown->currentData().toInt(&countdownOk) : 0;
    result.countdownSeconds = countdownOk ? countdown : 0;
    result.container = dialog::containerFromCombo(m_container);
    result.quality = dialog::qualityFromCombo(m_quality);
    result.scope = static_cast<RecordingScope>(m_scope ? m_scope->currentData().toInt() : static_cast<int>(RecordingScope::Region));
    result.outputPath = normalizedRecordingPath(m_outputPath ? m_outputPath->text() : QString(),
                                                m_mode,
                                                result.container);

    const int sourceIndex = m_display ? m_display->currentData().toInt() : -1;
    if (sourceIndex >= 0 && sourceIndex < m_sources.size()) {
        result.display = m_sources.at(sourceIndex);
    }
    if (result.scope == RecordingScope::Display) {
        result.captureGeometry = result.display.geometry;
    }
    return result;
}

void RecordingConfigDialog::browseOutputPath()
{
    const RecordingContainerFormat container = dialog::containerFromCombo(m_container);
    const QString extension = recordingContainerExtension(container);
    const QString filter = m_mode == RecordingMode::Gif
        ? MS_TR("GIF Images (*.gif)")
        : MS_TR("Videos (*.%1)").arg(extension);
    const QString path = QFileDialog::getSaveFileName(
        this,
        MS_TR("Save Recording"),
        m_outputPath ? m_outputPath->text() : defaultRecordingPath(m_mode, container),
        filter);
    if (!path.isEmpty() && m_outputPath) {
        m_outputPathTouched = true;
        m_outputPath->setText(normalizedRecordingPath(path, m_mode, container));
        updateSummary();
    }
}

void RecordingConfigDialog::refreshOutputExtension(bool preserveCurrentPath)
{
    if (!m_outputPath) {
        return;
    }
    const RecordingContainerFormat container = dialog::containerFromCombo(m_container);
    const bool reusable = preserveCurrentPath && !m_outputPath->text().trimmed().isEmpty();
    m_outputPath->setText(reusable
                              ? normalizedRecordingPath(m_outputPath->text(), m_mode, container)
                              : defaultRecordingPath(m_mode, container));
    updateSummary();
}

void RecordingConfigDialog::updateVideoOnlyControls()
{
    // 1. 【录制】【模式选项】GIF 使用自身容器与逐帧调色板，容器与质量档位仅对视频有效
    const bool videoMode = m_mode == RecordingMode::Video;
    markshot::ui::setFormRowVisible(m_optionsForm, m_container, videoMode);
    markshot::ui::setFormRowVisible(m_optionsForm, m_quality, videoMode);
    scheduleContentResize();
}

void RecordingConfigDialog::updateAudioControls()
{
    if (!m_audio) {
        return;
    }
    const bool videoMode = m_mode == RecordingMode::Video;
    const bool audioAvailable = recordingAudioCaptureAvailable();
    m_audio->setEnabled(videoMode && audioAvailable);
    if (!videoMode) {
        m_audio->setToolTip(MS_TR("GIF recording does not include audio."));
    } else if (!audioAvailable) {
        m_audio->setToolTip(recordingAudioUnavailableText());
    } else {
        m_audio->setToolTip(MS_TR("Record audio"));
    }
    m_audioRow->setVisible(videoMode);
    m_audioDeviceRow->setVisible(videoMode && m_audio->isEnabled() && m_audio->isChecked());
    m_audioDevice->setEnabled(m_audio->isEnabled() && m_audio->isChecked());
    scheduleContentResize(m_audioDeviceRow->isVisible() ? m_audioDeviceRow : nullptr);
}

/**
 * 读取指定录制模式的帧率状态。
 * @param mode 录制模式。
 * @return 帧率。
 */
int RecordingConfigDialog::fpsForMode(RecordingMode mode) const
{
    return mode == RecordingMode::Gif ? m_gifFps : m_videoFps;
}

/**
 * 保存当前帧率下拉框状态到指定录制模式。
 * @param mode 录制模式。
 * @return 无返回值。
 */
void RecordingConfigDialog::storeCurrentFpsForMode(RecordingMode mode)
{
    if (!m_fps) {
        return;
    }
    bool ok = false;
    const int value = m_fps->currentData().toInt(&ok);
    if (!ok) {
        return;
    }
    if (mode == RecordingMode::Gif) {
        m_gifFps = value;
        return;
    }
    m_videoFps = value;
}

}  // namespace markshot::recording
