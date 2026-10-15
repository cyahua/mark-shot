#include "settings/settings_field_texts.h"

#include "ui/i18n.h"

namespace markshot::settings::texts {
namespace {

/// @brief 根据当前界面语言选择文案
/// @param english 英文文案
/// @param chinese 中文文案
/// @return 当前语言对应的文案
QString pick(const char *english, const char *chinese)
{
    return markshot::i18n::language() == markshot::i18n::Language::Chinese ? QString::fromUtf8(chinese)
                                                                             : QString::fromUtf8(english);
}

}  // namespace

QString shortcutIdlePlaceholder()
{
    return pick("Click to set shortcut", "点击设置快捷键");
}

QString shortcutRecordingPlaceholder()
{
    return pick("Press shortcut...", "请按下快捷键...");
}

QString clearShortcut()
{
    return pick("Clear shortcut", "清除快捷键");
}

QString browse()
{
    return pick("Browse...", "浏览...");
}

QString selectFolder()
{
    return pick("Select Folder", "选择文件夹");
}

QString selectFile()
{
    return pick("Select File", "选择文件");
}

QString selectProgram()
{
    return pick("Select Program", "选择程序");
}

QString revertUnsaved()
{
    return pick("Revert unsaved changes", "恢复未保存修改");
}

}  // namespace markshot::settings::texts
