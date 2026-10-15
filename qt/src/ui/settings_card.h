#pragma once

#include <QFrame>

class QLabel;
class QVBoxLayout;

namespace markshot::ui {

class DisclosureSection;

/// @brief 设置分类卡片，提供统一的表面、标题区与内容区；低频分组可折叠且保留输入状态
class SettingsCard final : public QFrame {
    Q_OBJECT

public:
    /// @brief 卡片呈现方式
    enum class Mode {
        Static,       ///< 标题常驻，内容始终可见
        Collapsible,  ///< 标题作为折叠入口，内容默认收起
    };

    /// @brief 创建设置分类卡片
    /// @param title 分类标题
    /// @param description 分类说明，常驻卡片用于提示，折叠卡片展开后显示
    /// @param mode 卡片呈现方式
    /// @param parent 所属控件
    SettingsCard(const QString &title, const QString &description, Mode mode, QWidget *parent = nullptr);

    /// @brief 读取卡片呈现方式
    /// @return 呈现方式
    Mode mode() const;

    /// @brief 读取分类标题
    /// @return 分类标题
    QString title() const;

    /// @brief 读取分类说明
    /// @return 分类说明
    QString description() const;

    /// @brief 获取常驻标题标签
    /// @return 标题标签，折叠卡片返回 nullptr
    QLabel *titleLabel() const;

    /// @brief 获取折叠分组
    /// @return 折叠分组，常驻卡片返回 nullptr
    DisclosureSection *disclosure() const;

    /// @brief 获取承载表单的内容布局
    /// @return 内容区垂直布局
    QVBoxLayout *bodyLayout() const;

    /// @brief 读取当前是否处于紧凑内边距
    /// @return 窄宽度下返回 true
    bool isCompact() const;

    /// @brief 紧凑内边距的切换宽度
    static constexpr int kCompactWidth = 440;

protected:
    /// @brief 按卡片宽度切换常规或紧凑内边距
    /// @param event 尺寸事件
    /// @return 无返回值
    void resizeEvent(QResizeEvent *event) override;

private:
    /// @brief 应用指定密度的内边距，状态未变化时不触发重新布局
    /// @param compact 是否使用紧凑内边距
    /// @return 无返回值
    void applyDensity(bool compact);

    Mode m_mode;
    QString m_title;
    QString m_description;
    QLabel *m_titleLabel = nullptr;
    DisclosureSection *m_disclosure = nullptr;
    QVBoxLayout *m_body = nullptr;
    bool m_compact = false;
};

}  // namespace markshot::ui
