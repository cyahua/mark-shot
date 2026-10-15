#include "settings/settings_dialog.h"

#include "settings/settings_page_about.h"
#include "settings/settings_page_advanced.h"
#include "settings/settings_page_annotation.h"
#include "settings/settings_page_capture.h"
#include "settings/settings_page_general.h"
#include "settings/settings_page_integrations.h"
#include "settings/settings_page_pinned.h"
#include "settings/settings_page_plugins.h"
#include "settings/settings_page_scroll.h"
#include "settings/settings_page_shortcuts.h"
#include "settings/settings_page_storage.h"

#include <QPushButton>
#include <QScopedValueRollback>
#include <QScrollArea>
#include <QStackedWidget>

namespace markshot::settings {
namespace {

/**
 * 【设置】【页面基线】创建并初始化一个分类，仅规范化该分类负责的保存值。
 * @param page 接收创建后的页面指针。
 * @param parent 页面所属滚动容器。
 * @param saved 已保存配置，不包含其他页面的未保存输入。
 * @return 新建页面，由父控件管理生命周期。
 */
template <typename Page>
QWidget *createConfiguredPage(Page *&page, QWidget *parent, SettingsConfig *saved)
{
    page = new Page(parent);
    page->setConfig(*saved);
    page->updateConfig(saved);
    return page;
}

}  // namespace

void SettingsDialog::applyConfigToPages(const SettingsConfig &config)
{
    const QScopedValueRollback<bool> applying(m_applyingConfig, true);
    m_config = config;
    if (m_generalPage) m_generalPage->setConfig(config);
    if (m_capturePage) m_capturePage->setConfig(config);
    if (m_shortcutsPage) m_shortcutsPage->setConfig(config);
    if (m_annotationPage) m_annotationPage->setConfig(config);
    if (m_pinnedPage) m_pinnedPage->setConfig(config);
    if (m_integrationsPage) m_integrationsPage->setConfig(config);
    if (m_pluginsPage) m_pluginsPage->setConfig(config);
    if (m_scrollPage) m_scrollPage->setConfig(config);
    if (m_storagePage) m_storagePage->setConfig(config);
    if (m_advancedPage) m_advancedPage->setConfig(config);

    // 1. 【设置】【保存基线】全部已创建页面恢复完成后，才收集控件规范化后的配置
    m_config = collectConfig();
    m_savedValues = settingsConfigToJson(m_config);
    m_revertButton->setEnabled(false);
    m_saveButton->setEnabled(false);
}

SettingsConfig SettingsDialog::collectConfig() const
{
    // 1. 【设置】【配置收集】从保存基线开始，只覆盖已创建页面，保留未访问分类的值
    SettingsConfig config = m_config;
    if (m_generalPage) m_generalPage->updateConfig(&config);
    if (m_capturePage) m_capturePage->updateConfig(&config);
    if (m_shortcutsPage) m_shortcutsPage->updateConfig(&config);
    if (m_annotationPage) m_annotationPage->updateConfig(&config);
    if (m_pinnedPage) m_pinnedPage->updateConfig(&config);
    if (m_integrationsPage) m_integrationsPage->updateConfig(&config);
    if (m_pluginsPage) m_pluginsPage->updateConfig(&config);
    if (m_scrollPage) m_scrollPage->updateConfig(&config);
    if (m_storagePage) m_storagePage->updateConfig(&config);
    if (m_advancedPage) m_advancedPage->updateConfig(&config);
    return config;
}

void SettingsDialog::ensureDeferredPage(int index)
{
    auto *area = qobject_cast<QScrollArea *>(m_stack->widget(index));
    if (!area || area->widget()) return;

    // 1. 【设置】【按需创建】初始化新页面时屏蔽变更通知，不读取其他页面的草稿
    const QScopedValueRollback<bool> applying(m_applyingConfig, true);
    QWidget *page = nullptr;
    switch (index) {
    case 0:
        page = createConfiguredPage(m_generalPage, area, &m_config);
        break;
    case 1:
        page = createConfiguredPage(m_capturePage, area, &m_config);
        break;
    case 2:
        page = createConfiguredPage(m_shortcutsPage, area, &m_config);
        break;
    case 3:
        page = createConfiguredPage(m_annotationPage, area, &m_config);
        break;
    case 4:
        page = createConfiguredPage(m_pinnedPage, area, &m_config);
        break;
    case 5:
        page = createConfiguredPage(m_integrationsPage, area, &m_config);
        break;
    case 6:
        page = createConfiguredPage(m_pluginsPage, area, &m_config);
        break;
    case 7:
        page = createConfiguredPage(m_scrollPage, area, &m_config);
        break;
    case 8:
        page = createConfiguredPage(m_storagePage, area, &m_config);
        break;
    case 9:
        page = createConfiguredPage(m_advancedPage, area, &m_config);
        break;
    case 10:
        m_aboutPage = new SettingsPageAbout(area);
        page = m_aboutPage;
        break;
    default:
        return;
    }
    area->setWidget(page);
    m_savedValues = settingsConfigToJson(m_config);
    // 2. 【设置】【变更跟踪】只连接新页面，重复访问复用页面和未保存输入
    trackSettingChanges(page);
}

}  // namespace markshot::settings
