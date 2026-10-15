#include "recording/recording_config_dialog.h"

#include "recording/recording_config_options.h"
#include "recording/recording_dialog_config.h"
#include "recording/recording_file_naming.h"
#include "ui/disclosure_section.h"
#include "ui/i18n.h"
#include "ui/theme.h"

#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>

namespace markshot::recording {

void RecordingConfigDialog::buildLayout(const RecordingDialogConfig &persisted)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 14, 16, 14);
    root->setSpacing(14);

    // 1. 【录制】【准备界面】标题旁切换类型，首要区域只呈现录制范围
    auto *header = new QHBoxLayout;
    m_title = new QLabel(dialog::titleForMode(m_mode), this);
    m_title->setFont(markshot::theme::uiFont(12, QFont::DemiBold));
    header->addWidget(m_title, 1);
    m_modeSelector = new QComboBox(this);
    m_modeSelector->setObjectName(QStringLiteral("recordingMode"));
    m_modeSelector->setAccessibleName(MS_TR("Recording Type"));
    m_modeSelector->addItem(MS_TR("Video"), static_cast<int>(RecordingMode::Video));
    m_modeSelector->addItem(QStringLiteral("GIF"), static_cast<int>(RecordingMode::Gif));
    m_modeSelector->setCurrentIndex(m_modeSelector->findData(static_cast<int>(m_mode)));
    header->addWidget(m_modeSelector);
    root->addLayout(header);

    auto *scroll = new QScrollArea(this);
    m_contentScroll = scroll;
    scroll->setObjectName(QStringLiteral("recordingContentScroll"));
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *content = new QWidget(scroll);
    auto *body = new QVBoxLayout(content);
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(16);
    body->setSizeConstraint(QLayout::SetMinAndMaxSize);
    scroll->setWidget(content);
    content->setAutoFillBackground(false);
    scroll->viewport()->setAutoFillBackground(false);
    root->addWidget(scroll, 1);

    auto *source = new QVBoxLayout;
    source->setSpacing(8);
    auto *sourceTitle = new QLabel(MS_TR("Capture Area"), content);
    sourceTitle->setFont(markshot::theme::uiFont(11, QFont::DemiBold));
    source->addWidget(sourceTitle);
    auto *sourceRow = new QHBoxLayout;
    sourceRow->setSpacing(8);
    m_scope = new QComboBox(content);
    m_scope->setObjectName(QStringLiteral("recordingScope"));
    m_scope->setAccessibleName(MS_TR("Capture Area"));
    m_scope->addItem(MS_TR("Display"), static_cast<int>(RecordingScope::Display));
    m_scope->addItem(MS_TR("Region"), static_cast<int>(RecordingScope::Region));
    m_scope->setCurrentIndex(m_scope->findData(static_cast<int>(persisted.scope)));
    sourceRow->addWidget(m_scope);
    m_display = new QComboBox(content);
    m_display->setObjectName(QStringLiteral("recordingDisplay"));
    m_display->setAccessibleName(MS_TR("Display"));
    m_display->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_display->setMinimumContentsLength(8);
    for (int index = 0; index < m_sources.size(); ++index) {
        m_display->addItem(m_sources.at(index).title, index);
    }
    const int savedSource = dialog::displaySourceIndexForKey(m_sources, persisted.displayKey);
    const int selectedSource = savedSource >= 0 ? savedSource : dialog::currentDisplaySourceIndex(m_sources);
    m_display->setCurrentIndex(m_display->findData(selectedSource));
    sourceRow->addWidget(m_display, 1);
    source->addLayout(sourceRow);
    m_scopeHint = new QLabel(content);
    m_scopeHint->setObjectName(QStringLiteral("recordingScopeHint"));
    m_scopeHint->setProperty("role", QStringLiteral("muted"));
    m_scopeHint->setFont(markshot::theme::uiFont(9));
    m_scopeHint->setWordWrap(true);
    source->addWidget(m_scopeHint);
    body->addLayout(source);

    // 2. 【录制】【音频】仅在视频模式显示开关，开启后才呈现输入设备
    m_audioRow = new QWidget(content);
    auto *audioLayout = new QVBoxLayout(m_audioRow);
    audioLayout->setContentsMargins(0, 0, 0, 0);
    audioLayout->setSpacing(8);
    m_audio = new QCheckBox(MS_TR("Record audio"), m_audioRow);
    m_audio->setObjectName(QStringLiteral("recordingAudio"));
    m_audio->setChecked(persisted.includeAudio);
    audioLayout->addWidget(m_audio);
    m_audioDeviceRow = new QWidget(m_audioRow);
    auto *deviceLayout = new QFormLayout(m_audioDeviceRow);
    deviceLayout->setContentsMargins(0, 0, 0, 0);
    deviceLayout->setRowWrapPolicy(QFormLayout::WrapLongRows);
    m_audioDevice = new QComboBox(m_audioDeviceRow);
    m_audioDevice->setAccessibleName(MS_TR("Audio input"));
    m_audioDevice->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    populateAudioDevices(persisted.audioDevice);
    deviceLayout->addRow(MS_TR("Audio input"), m_audioDevice);
    audioLayout->addWidget(m_audioDeviceRow);
    body->addWidget(m_audioRow);

    // 3. 【录制】【默认参数】默认位置与质量只显示摘要，需要更改时原位展开
    m_outputSection = new markshot::ui::DisclosureSection(MS_TR("Save to"), content);
    auto *outputLayout = new QHBoxLayout(m_outputSection->content());
    outputLayout->setContentsMargins(0, 0, 0, 0);
    m_outputPath = new QLineEdit(persisted.outputPath.isEmpty()
        ? defaultRecordingPath(m_mode, persisted.container)
        : normalizedRecordingPath(persisted.outputPath, m_mode, persisted.container), m_outputSection->content());
    m_outputPath->setObjectName(QStringLiteral("recordingOutput"));
    m_outputPath->setAccessibleName(MS_TR("Output"));
    outputLayout->addWidget(m_outputPath, 1);
    auto *browse = new QPushButton(MS_TR("Browse"), m_outputSection->content());
    outputLayout->addWidget(browse);
    body->addWidget(m_outputSection);

    m_optionsSection = new markshot::ui::DisclosureSection(MS_TR("Recording options"), content);
    m_optionsForm = new QFormLayout(m_optionsSection->content());
    m_optionsForm->setContentsMargins(0, 0, 0, 0);
    m_optionsForm->setHorizontalSpacing(16);
    m_optionsForm->setVerticalSpacing(10);
    m_optionsForm->setRowWrapPolicy(QFormLayout::WrapLongRows);
    m_fps = new QComboBox(m_optionsSection->content());
    m_fps->setAccessibleName(MS_TR("Frame Rate"));
    dialog::populateFrameRateOptions(m_fps, m_mode, fpsForMode(m_mode));
    m_optionsForm->addRow(MS_TR("Frame Rate"), m_fps);
    m_container = new QComboBox(m_optionsSection->content());
    m_container->setAccessibleName(MS_TR("Container"));
    dialog::populateContainerOptions(m_container, persisted.container);
    m_optionsForm->addRow(MS_TR("Container"), m_container);
    m_quality = new QComboBox(m_optionsSection->content());
    m_quality->setAccessibleName(MS_TR("Quality"));
    dialog::populateQualityOptions(m_quality, persisted.quality);
    m_optionsForm->addRow(MS_TR("Quality"), m_quality);
    m_countdown = new QComboBox(m_optionsSection->content());
    m_countdown->setAccessibleName(MS_TR("Countdown"));
    dialog::populateCountdownOptions(m_countdown, persisted.countdownSeconds);
    m_optionsForm->addRow(MS_TR("Countdown"), m_countdown);
    m_backend = new QComboBox(m_optionsSection->content());
    m_backend->setAccessibleName(MS_TR("Recording Backend"));
    dialog::populateBackendOptions(m_backend, persisted.backend);
    m_optionsForm->addRow(MS_TR("Recording Backend"), m_backend);
    body->addWidget(m_optionsSection);
    body->addStretch();
    for (auto *section : {m_outputSection, m_optionsSection}) {
        connect(section, &markshot::ui::DisclosureSection::expandedChanged, this,
            [this, section](bool expanded) {
                scheduleContentResize(expanded ? section->content() : nullptr);
            });
    }

    auto *footer = new QHBoxLayout;
    footer->addStretch();
    auto *cancel = new QPushButton(MS_TR("Cancel"), this);
    cancel->setProperty("role", QStringLiteral("quiet"));
    footer->addWidget(cancel);
    m_startButton = new QPushButton(this);
    m_startButton->setObjectName(QStringLiteral("recordingStart"));
    m_startButton->setProperty("role", QStringLiteral("primary"));
    m_startButton->setDefault(true);
    footer->addWidget(m_startButton);
    root->addLayout(footer);

    // 4. 【录制】【情境更新】模式变化保留手动路径与各自帧率，隐藏项继续参与配置读取
    connect(m_modeSelector, &QComboBox::currentIndexChanged, this, [this] {
        const RecordingMode next = dialog::modeFromCombo(m_modeSelector, m_mode);
        if (next == m_mode) {
            return;
        }
        storeCurrentFpsForMode(m_mode);
        m_mode = next;
        setWindowTitle(dialog::titleForMode(m_mode));
        m_title->setText(dialog::titleForMode(m_mode));
        dialog::populateFrameRateOptions(m_fps, m_mode, fpsForMode(m_mode));
        updateAudioControls();
        updateVideoOnlyControls();
        refreshOutputExtension(m_outputPathTouched);
    });
    connect(m_container, &QComboBox::currentIndexChanged, this, [this] { refreshOutputExtension(m_outputPathTouched); });
    connect(m_outputPath, &QLineEdit::textEdited, this, [this] { m_outputPathTouched = true; updateSummary(); });
    connect(m_fps, &QComboBox::currentIndexChanged, this, &RecordingConfigDialog::updateSummary);
    connect(m_quality, &QComboBox::currentIndexChanged, this, &RecordingConfigDialog::updateSummary);
    connect(m_scope, &QComboBox::currentIndexChanged, this, &RecordingConfigDialog::updateScopeControls);
    connect(m_display, &QComboBox::currentIndexChanged, this, &RecordingConfigDialog::updateScopeControls);
    connect(m_audio, &QCheckBox::toggled, this, &RecordingConfigDialog::updateAudioControls);
    connect(browse, &QPushButton::clicked, this, &RecordingConfigDialog::browseOutputPath);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_startButton, &QPushButton::clicked, this, &QDialog::accept);
    updateAudioControls();
    updateVideoOnlyControls();
    updateScopeControls();
    updateSummary();
}

void RecordingConfigDialog::updateSummary()
{
    const QString format = m_mode == RecordingMode::Gif ? QStringLiteral("GIF") : m_container->currentText();
    QString summary = m_fps->currentText() + QStringLiteral(" · ") + format;
    if (m_mode == RecordingMode::Video) {
        summary += QStringLiteral(" · ") + m_quality->currentText();
    }
    m_optionsSection->setSummary(summary);
    m_outputSection->setSummary(QFileInfo(m_outputPath->text()).fileName());
    m_outputSection->setToolTip(m_outputPath->text());
}

void RecordingConfigDialog::updateScopeControls()
{
    const bool region = m_scope->currentData().toInt() == static_cast<int>(RecordingScope::Region);
    const int index = m_display->currentIndex() >= 0 ? m_display->currentData().toInt() : -1;
    const bool available = index >= 0 && index < m_sources.size() && !m_sources.at(index).geometry.isEmpty();
    m_startButton->setEnabled(available);
    m_startButton->setText(region ? MS_TR("Select region") : MS_TR("Start recording"));
    if (!available) {
        m_scopeHint->setText(MS_TR("No display is available for recording."));
        return;
    }
    const QRect bounds = m_sources.at(index).geometry;
    m_scopeHint->setText(QStringLiteral("%1 × %2 · %3").arg(bounds.width()).arg(bounds.height())
        .arg(region ? MS_TR("Choose a region on the next screen.") : MS_TR("Record the selected display.")));
}

}
