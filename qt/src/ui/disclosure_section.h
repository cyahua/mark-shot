#pragma once

#include <QFrame>

class QLabel;
class QToolButton;

namespace markshot::ui {

/// @brief 将低频选项收纳到原位展开区，折叠时保留控件及其输入状态
class DisclosureSection final : public QFrame {
    Q_OBJECT

public:
    /// @brief 创建默认折叠的选项分组
    /// @param title 轻量入口显示的标题
    /// @param parent 所属窗口
    explicit DisclosureSection(const QString &title, QWidget *parent = nullptr);

    /// @brief 获取用于承载选项的内容控件
    /// @return 可安装布局的内容控件
    QWidget *content() const;

    /// @brief 读取当前展开状态
    /// @return 内容可见时返回 true
    bool isExpanded() const;

    /// @brief 更新折叠入口旁的当前值摘要
    /// @param summary 简短的参数或状态摘要
    /// @return 无返回值
    void setSummary(const QString &summary);

    /// @brief 展开或折叠分组，收起时将内部焦点返回入口
    /// @param expanded 是否展开内容
    /// @return 无返回值
    void setExpanded(bool expanded);

signals:
    /// @brief 通知内容展开状态已经变化
    /// @param expanded 是否展开内容
    void expandedChanged(bool expanded);

private:
    QToolButton *m_toggle = nullptr;
    QLabel *m_summary = nullptr;
    QWidget *m_content = nullptr;
};

}
