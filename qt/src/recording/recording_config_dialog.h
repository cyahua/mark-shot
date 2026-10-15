#pragma once

#include "recording/recording_options.h"

#include <QDialog>
#include <QVector>

class QCheckBox;
class QComboBox;
class QLabel;
class QFormLayout;
class QPushButton;
class QLineEdit;
class QScrollArea;
class QShowEvent;
class QToolButton;
class QWidget;

namespace markshot::ui {
class DisclosureSection;
}

namespace markshot::recording {

struct RecordingDialogConfig;

class RecordingConfigDialog final : public QDialog {
public:
    /**
     * 创建录制配置对话框。
     * @param mode 录制模式。
     * @param parent 父窗口。
     */
    explicit RecordingConfigDialog(RecordingMode mode, QWidget *parent = nullptr);

    /**
     * 读取用户确认后的录制配置。
     * @return 录制配置。
     */
    RecordingOptions options() const;

protected:
    /// @brief 显示录制配置时按当前选项调整窗口高度
    /// @param event 窗口显示事件
    /// @return 无返回值
    void showEvent(QShowEvent *event) override;

private:
    /// @brief 构建以录制范围为中心的准备界面，低频参数按需展开
    /// @param persisted 最近保存的录制参数
    /// @return 无返回值
    void buildLayout(const RecordingDialogConfig &persisted);

    /// @brief 将选项当前值汇总到折叠入口，避免重复平铺参数
    /// @return 无返回值
    void updateSummary();

    /// @brief 按范围显示下一步操作和对应显示器信息
    /// @return 无返回值
    void updateScopeControls();

    /// @brief 合并选项显隐变化，等待布局更新后调整窗口
    /// @param reveal 需要滚动到可见区域的展开内容，可为空
    /// @return 无返回值
    void scheduleContentResize(QWidget *reveal = nullptr);

    /// @brief 在当前屏幕范围内容纳选项，屏幕不足时定位展开内容
    /// @return 无返回值
    void fitExpandedContent();

    /**
     * 打开输出文件选择对话框。
     * @return 无返回值。
     */
    void browseOutputPath();

    /**
     * 按录制模式更新音频控件可用状态。
     * @return 无返回值。
     */
    void updateAudioControls();

    /**
     * 按录制模式更新容器与质量控件可用状态。
     * @return 无返回值。
     */
    void updateVideoOnlyControls();

    /**
     * 按当前模式与容器刷新输出路径扩展名。
     * @param preserveCurrentPath 为 true 时保留用户已输入的路径主体。
     * @return 无返回值。
     */
    void refreshOutputExtension(bool preserveCurrentPath);

    /**
     * 读取指定录制模式的帧率状态。
     * @param mode 录制模式。
     * @return 帧率。
     */
    int fpsForMode(RecordingMode mode) const;

    /**
     * 保存当前帧率下拉框状态到指定录制模式。
     * @param mode 录制模式。
     * @return 无返回值。
     */
    void storeCurrentFpsForMode(RecordingMode mode);

    /**
     * 应用与设置界面一致的主题样式。
     * @return 无返回值。
     */
    void applyDialogTheme();

    /**
     * 填充音频输入设备下拉框。
     * @param persistedDevice 上次保存的设备标识。
     * @return 无返回值。
     */
    void populateAudioDevices(const QString &persistedDevice);

    RecordingMode m_mode = RecordingMode::Gif;
    int m_videoFps = 30;
    int m_gifFps = 12;
    QVector<DisplaySource> m_sources;
    QLabel *m_title = nullptr;
    QComboBox *m_modeSelector = nullptr;
    QComboBox *m_fps = nullptr;
    QCheckBox *m_audio = nullptr;
    QComboBox *m_audioDevice = nullptr;
    QComboBox *m_display = nullptr;
    QComboBox *m_backend = nullptr;
    QComboBox *m_scope = nullptr;
    QComboBox *m_countdown = nullptr;
    QComboBox *m_container = nullptr;
    QComboBox *m_quality = nullptr;
    QLineEdit *m_outputPath = nullptr;
    QLabel *m_scopeHint = nullptr;
    QPushButton *m_startButton = nullptr;
    QWidget *m_audioRow = nullptr;
    QWidget *m_audioDeviceRow = nullptr;
    QScrollArea *m_contentScroll = nullptr;
    QWidget *m_pendingReveal = nullptr;
    QFormLayout *m_optionsForm = nullptr;
    markshot::ui::DisclosureSection *m_optionsSection = nullptr;
    markshot::ui::DisclosureSection *m_outputSection = nullptr;
    bool m_outputPathTouched = false;
    bool m_contentResizePending = false;
};

}  // namespace markshot::recording
