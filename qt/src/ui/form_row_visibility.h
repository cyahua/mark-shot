#pragma once

#include <QFormLayout>
#include <QWidget>

namespace markshot::ui {

/// @brief 同步设置表单字段及标签的可见性，兼容 Qt 6.2 并保留字段内容
/// @param form 字段所属表单
/// @param field 需要显示或隐藏的字段控件
/// @param visible 是否显示该表单行
/// @return 无返回值
inline void setFormRowVisible(QFormLayout *form, QWidget *field, bool visible)
{
    field->setVisible(visible);
    if (QWidget *label = form->labelForField(field)) {
        label->setVisible(visible);
    }
}

}
