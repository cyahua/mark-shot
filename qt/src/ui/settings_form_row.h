#pragma once

#include <QPointer>
#include <QWidget>

class QFormLayout;
class QLabel;

namespace markshot::ui {

/// @brief 设置表单行的标签区，上方为字段标题、下方为可选说明，并与右侧控件关联
class SettingsRowLabel final : public QWidget {
    Q_OBJECT

public:
    /// @brief 创建设置行标签
    /// @param title 字段标题
    /// @param description 字段说明，为空时不显示说明行
    /// @param parent 所属控件
    SettingsRowLabel(const QString &title, const QString &description, QWidget *parent = nullptr);

    /// @brief 读取字段标题文本
    /// @return 字段标题
    QString title() const;

    /// @brief 读取字段说明文本
    /// @return 字段说明，未设置时为空
    QString description() const;

    /// @brief 获取标题标签，供表单布局统一换行与高度对齐
    /// @return 标题标签
    QLabel *titleLabel() const;

    /// @brief 获取说明标签
    /// @return 说明标签，未设置说明时为 nullptr
    QLabel *descriptionLabel() const;

    /// @brief 关联右侧控件，点击标签时聚焦控件，开关类控件直接切换，并同步禁用状态
    /// @param buddy 右侧字段控件
    /// @return 无返回值
    void setBuddy(QWidget *buddy);

    /// @brief 读取关联的字段控件
    /// @return 字段控件，未关联时为 nullptr
    QWidget *buddy() const;

protected:
    /// @brief 接收按下事件，保证释放事件回到标签区
    /// @param event 鼠标事件
    /// @return 无返回值
    void mousePressEvent(QMouseEvent *event) override;

    /// @brief 在标签区内释放左键时转交给关联控件
    /// @param event 鼠标事件
    /// @return 无返回值
    void mouseReleaseEvent(QMouseEvent *event) override;

    /// @brief 监听关联控件的启用状态变化
    /// @param watched 事件目标
    /// @param event 事件
    /// @return 始终返回 false，不拦截事件
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QLabel *m_title = nullptr;
    QLabel *m_description = nullptr;
    QPointer<QWidget> m_buddy;
};

/// @brief 以统一的标题、说明和控件三段结构向设置表单追加一行
/// @param form 目标表单布局
/// @param title 字段标题，同时作为控件的可访问名称
/// @param description 字段说明，为空时只显示标题
/// @param field 右侧字段控件
/// @return 新建的标签区，可用于后续的可见性控制
SettingsRowLabel *addSettingsFormRow(QFormLayout *form, const QString &title,
                                     const QString &description, QWidget *field);

}  // namespace markshot::ui
