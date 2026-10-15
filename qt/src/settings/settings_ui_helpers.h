#pragma once

#include "settings/settings_path_field.h"

#include <QString>

class QCheckBox;
class QComboBox;
class QFormLayout;
class QFrame;
class QKeySequenceEdit;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QDoubleSpinBox;
class QSpinBox;
class QVBoxLayout;
class QWidget;

namespace markshot::settings {

/// @brief 创建设置页根布局。
/// @param parent 父控件。
/// @return 垂直布局。
QVBoxLayout *createSettingsPageLayout(QWidget *parent);

/// @brief 创建常驻的设置分类卡片，内部附带统一对齐的表单。
/// @param title 分组标题。
/// @param description 分组描述，作为标题悬停提示。
/// @param parent 父控件。
/// @return 卡片控件，可通过 settingsCardForm 添加选项。
QFrame *createSettingsCard(const QString &title, const QString &description, QWidget *parent);

/// @brief 创建默认折叠的低频配置卡片，折叠不会丢失表单值
/// @param title 分组入口标题
/// @param description 展开后显示的说明
/// @param parent 所属页面
/// @return 可通过 settingsCardForm 添加选项的卡片
QFrame *createAdvancedSettingsCard(const QString &title, const QString &description, QWidget *parent);

/// @brief 获取卡片中的表单布局。
/// @param card 卡片控件。
/// @return 表单布局。
QFormLayout *settingsCardForm(QFrame *card);

/// @brief 添加开关表单项，标题与说明位于左侧标签区，开关位于字段列。
/// @param form 表单布局。
/// @param label 标签文本。
/// @param description 控件说明，显示在标题下方。
/// @return 复选框控件。
QCheckBox *addSwitchRow(QFormLayout *form, const QString &label, const QString &description = {});

/// @brief 添加文本输入表单项。
/// @param form 表单布局。
/// @param label 标签文本。
/// @param placeholder 占位文本。
/// @return 文本输入控件。
QLineEdit *addTextRow(QFormLayout *form, const QString &label, const QString &placeholder = {});

/// @brief 添加路径或命令输入表单项，输入框自适应拉伸并带浏览按钮
/// @param form 表单布局
/// @param label 标签文本
/// @param kind 浏览类型，命令类型只替换程序路径并保留参数
/// @param placeholder 占位文本
/// @return 路径输入框，配置读写方式与普通文本行一致
QLineEdit *addPathRow(QFormLayout *form, const QString &label, SettingsPathField::Kind kind,
                      const QString &placeholder = {});

/// @brief 添加环境变量多行输入项，默认约 4 行高度并使用等宽字体
/// @param form 表单布局
/// @param label 标签文本
/// @param placeholder 占位文本
/// @return 多行文本输入控件
QPlainTextEdit *addEnvironmentRow(QFormLayout *form, const QString &label, const QString &placeholder = {});

/// @brief 添加多行文本输入表单项。
/// @param form 表单布局。
/// @param label 标签文本。
/// @param placeholder 占位文本。
/// @return 多行文本输入控件。
QPlainTextEdit *addPlainTextRow(QFormLayout *form, const QString &label, const QString &placeholder = {});

/// @brief 添加数值输入表单项。
/// @param form 表单布局。
/// @param label 标签文本。
/// @param minimum 最小值。
/// @param maximum 最大值。
/// @param suffix 数值后缀。
/// @return 数值输入控件。
QSpinBox *addSpinRow(QFormLayout *form, const QString &label, int minimum, int maximum, const QString &suffix = {});

/// @brief 添加小数输入表单项。
/// @param form 表单布局。
/// @param label 标签文本。
/// @param minimum 最小值。
/// @param maximum 最大值。
/// @param decimals 小数位数。
/// @return 小数输入控件。
QDoubleSpinBox *addDoubleRow(QFormLayout *form, const QString &label, double minimum, double maximum, int decimals);

/// @brief 用自绘条目替换系统下拉列表，去掉单选圆点并统一行高与选中标记
/// @param combo 需要调整弹出列表的下拉框
/// @return 无返回值
void polishComboPopup(QComboBox *combo);

/// @brief 添加下拉选择表单项。
/// @param form 表单布局。
/// @param label 标签文本。
/// @param description 字段说明，为空时只显示标题。
/// @return 下拉框控件。
QComboBox *addComboRow(QFormLayout *form, const QString &label, const QString &description = {});

/// @brief 以统一标签区添加自定义字段控件，用于颜色按钮、目录选择、只读信息等。
/// @param form 表单布局。
/// @param label 标签文本。
/// @param field 字段控件。
/// @param description 字段说明，为空时只显示标题。
/// @return 传入的字段控件。
QWidget *addFieldRow(QFormLayout *form, const QString &label, QWidget *field, const QString &description = {});

/// @brief 添加快捷键输入表单项。
/// @param form 表单布局。
/// @param label 标签文本。
/// @return 快捷键输入控件，实际类型为胶囊样式的 SettingsShortcutEdit。
QKeySequenceEdit *addShortcutRow(QFormLayout *form, const QString &label);

}  // namespace markshot::settings
