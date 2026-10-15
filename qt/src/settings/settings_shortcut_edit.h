#pragma once

#include <QKeySequenceEdit>

class QAction;
class QLineEdit;

namespace markshot::settings {

/// @brief 胶囊样式的快捷键录入框：常态显示浅色底与圆角边界，录入时高亮青色边框，
///        悬停或聚焦且已有快捷键时在右侧显示清除图标
class SettingsShortcutEdit final : public QKeySequenceEdit {
    Q_OBJECT

public:
    /// @brief 创建快捷键录入框
    /// @param parent 所属控件
    explicit SettingsShortcutEdit(QWidget *parent = nullptr);

    /// @brief 是否处于等待按键的录入状态
    /// @return 已聚焦录入时返回 true
    bool isRecording() const;

    /// @brief 清除图标当前是否可见
    /// @return 可见时返回 true
    bool isClearVisible() const;

    /// @brief 获取内部显示文本的输入框，供测试与样式检查
    /// @return 内部输入框
    QLineEdit *displayEdit() const;

    /// @brief 获取清除动作
    /// @return 清除动作
    QAction *clearAction() const;

protected:
    /// @brief 进入录入状态并切换提示文案
    /// @param event 焦点事件
    /// @return 无返回值
    void focusInEvent(QFocusEvent *event) override;

    /// @brief 退出录入状态并恢复常态提示
    /// @param event 焦点事件
    /// @return 无返回值
    void focusOutEvent(QFocusEvent *event) override;

    /// @brief 记录悬停状态以显示清除图标
    /// @param event 进入事件
    /// @return 无返回值
    void enterEvent(QEnterEvent *event) override;

    /// @brief 清除悬停状态并隐藏清除图标
    /// @param event 离开事件
    /// @return 无返回值
    void leaveEvent(QEvent *event) override;

    /// @brief 调色板或启用状态变化时刷新图标
    /// @param event 变化事件
    /// @return 无返回值
    void changeEvent(QEvent *event) override;

    /// @brief 在内部输入框绘制前校正提示文案，覆盖 Qt 内部重置的英文提示
    /// @param watched 事件目标
    /// @param event 事件
    /// @return 交由基类处理的结果
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    /// @brief 切换录入状态并刷新样式
    /// @param recording 是否录入中
    /// @return 无返回值
    void setRecording(bool recording);

    /// @brief 根据悬停、焦点与内容决定清除图标可见性
    /// @return 无返回值
    void refreshClearAction();

    /// @brief 按当前录入状态写入提示文案
    /// @return 无返回值
    void applyPlaceholder();

    QLineEdit *m_display = nullptr;
    QAction *m_clear = nullptr;
    bool m_hovered = false;
    bool m_recording = false;
};

}  // namespace markshot::settings
