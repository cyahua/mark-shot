#pragma once

class QFormLayout;
class QVBoxLayout;

namespace markshot::settings {

/// @brief 为设置分组创建统一字段起点、长标签换行和窄窗口上下排列的表单
/// @param parentLayout 接收表单的分组布局
/// @return 已添加到分组内的表单布局
QFormLayout *createSettingsFormLayout(QVBoxLayout *parentLayout);

/// @brief 计算设置表单的标签列宽度，同一视区内的所有分组得到相同结果
/// @param lineHeight 表单字体行高
/// @param availableWidth 视区可用宽度
/// @return 标签列像素宽度
int settingsLabelColumnWidth(int lineHeight, int availableWidth);

}
