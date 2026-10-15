#pragma once

#include "settings/settings_config.h"

#include <QDialog>

class QLabel;
class QBoxLayout;
class QPushButton;
class QStackedWidget;
class QWidget;

namespace markshot::settings {

class SettingsNavigation;
class SettingsPageAbout;
class SettingsPageAnnotation;
class SettingsPageAdvanced;
class SettingsPageCapture;
class SettingsPageGeneral;
class SettingsPageIntegrations;
class SettingsPagePinned;
class SettingsPagePlugins;
class SettingsPageScroll;
class SettingsPageShortcuts;
class SettingsPageStorage;

class SettingsDialog final : public QDialog {
public:
    /// @brief 创建设置窗口。
    /// @param parent 父控件。
    explicit SettingsDialog(QWidget *parent = nullptr);

protected:
    /// @brief 窄窗口使用顶部分类选择器，优先保留设置内容宽度
    /// @param event 窗口尺寸变化事件
    /// @return 无返回值
    void resizeEvent(QResizeEvent *event) override;

private:
    /// @brief 从配置文件加载设置并更新所有页面。
    void loadConfig();

    /// @brief 【设置】【保存基线】将配置应用到已创建页面，不创建未访问分类。
    /// @param config 需要应用的设置结构。
    /// @return 无返回值。
    void applyConfigToPages(const SettingsConfig &config);

    /// @brief 【设置】【配置收集】合并已创建页面的输入与未访问分类的保存值。
    /// @return 设置结构。
    SettingsConfig collectConfig() const;

    /// @brief 保存当前设置并保持窗口打开，在操作栏显示结果
    /// @return 无返回值
    void saveConfig();

    /// @brief 应用设置界面主题。
    /// @param mode 配置中的界面主题模式。
    void applyTheme(markshot::ui::UiThemeMode mode);

    /// @brief 连接可编辑配置的变更信号，展开分组不会标记配置修改
    /// @param root 需要连接的页面，为空时连接整个内容栈
    /// @return 无返回值
    void trackSettingChanges(QWidget *root = nullptr);

    /// @brief 【设置】【按需创建】首次访问分类时创建页面并应用保存配置
    /// @param index 内容栈页面索引
    /// @return 无返回值
    void ensureDeferredPage(int index);

    /// @brief 对比实际配置与保存基线，更新未保存状态及原位撤销入口
    /// @return 无返回值
    void markSettingsChanged();

    /// @brief 在底部操作区显示状态并同步错误配色
    /// @param text 状态文本
    /// @param error 是否为错误状态
    /// @return 无返回值
    void setStatus(const QString &text, bool error = false);

    /// @brief 恢复最近一次保存的配置并保留当前所在页面
    /// @return 无返回值
    void revertChanges();

    SettingsNavigation *m_navigation = nullptr;
    QStackedWidget *m_stack = nullptr;
    QLabel *m_statusLabel = nullptr;
    QBoxLayout *m_bodyLayout = nullptr;
    QPushButton *m_revertButton = nullptr;
    QPushButton *m_saveButton = nullptr;
    bool m_applyingConfig = false;
    SettingsConfig m_config;
    QJsonObject m_savedValues;
    SettingsPageGeneral *m_generalPage = nullptr;
    SettingsPageCapture *m_capturePage = nullptr;
    SettingsPageShortcuts *m_shortcutsPage = nullptr;
    SettingsPageAnnotation *m_annotationPage = nullptr;
    SettingsPagePinned *m_pinnedPage = nullptr;
    SettingsPageIntegrations *m_integrationsPage = nullptr;
    SettingsPagePlugins *m_pluginsPage = nullptr;
    SettingsPageScroll *m_scrollPage = nullptr;
    SettingsPageStorage *m_storagePage = nullptr;
    SettingsPageAdvanced *m_advancedPage = nullptr;
    SettingsPageAbout *m_aboutPage = nullptr;
};

/// @brief 显示全局设置窗口，重复调用会复用现有窗口。
/// @param parent 用于定位窗口的父控件。
void showSettingsDialog(QWidget *parent = nullptr);

}  // namespace markshot::settings
