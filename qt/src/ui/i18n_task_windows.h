#pragma once

#include <QHash>
#include <QString>

namespace markshot::i18n {

/// @brief 提供文本识别、设置和录制界面的中文文案
/// @return 以英文源文本为键的只读中文词典
const QHash<QString, QString> &taskWindowChineseTable();

}
