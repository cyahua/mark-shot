#include "ui/context_menu_config.h"

#include "app_config_store.h"
#include "ui/interface_theme_config.h"

namespace markshot::ui {

ContextMenu::ColorScheme configuredContextMenuScheme()
{
    // 每次弹出前读取配置，设置页切换主题后无需重启即可生效
    const UiThemeMode mode = effectiveUiThemeMode(uiThemeModeFromConfigRoot(markshot::readAppConfigRoot()));
    return mode == UiThemeMode::Light ? ContextMenu::ColorScheme::Light : ContextMenu::ColorScheme::Dark;
}

}  // namespace markshot::ui
