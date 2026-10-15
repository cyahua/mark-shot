#pragma once

#include <QWidget>

class QLineEdit;
class QPushButton;

namespace markshot::settings {

/// @brief 路径类字段：输入框按卡片宽度自适应拉伸，右侧带文件夹图标的浏览按钮
class SettingsPathField final : public QWidget {
    Q_OBJECT

public:
    /// @brief 浏览按钮选择的目标类型
    enum class Kind {
        /// @brief 选择已有目录，整体替换文本
        Directory,
        /// @brief 选择保存文件路径，整体替换文本
        SaveFile,
        /// @brief 选择可执行程序，只替换命令首段并保留参数
        Command,
    };

    /// @brief 创建路径字段
    /// @param kind 浏览按钮的选择类型
    /// @param label 字段标题，用于可访问名称与对话框标题
    /// @param parent 所属控件
    SettingsPathField(Kind kind, const QString &label, QWidget *parent = nullptr);

    /// @brief 读取字段类型
    /// @return 浏览选择类型
    Kind kind() const;

    /// @brief 获取路径输入框，配置读写仍通过该输入框完成
    /// @return 输入框
    QLineEdit *lineEdit() const;

    /// @brief 获取浏览按钮
    /// @return 浏览按钮
    QPushButton *browseButton() const;

    /// @brief 写入浏览得到的路径，命令类型只替换程序部分
    /// @param path 选中的路径，为空时不修改
    /// @return 无返回值
    void applySelectedPath(const QString &path);

    /// @brief 计算文件对话框的起始位置
    /// @return 展开 ~ 后的起始目录或文件
    QString dialogStartPath() const;

protected:
    /// @brief 主题切换后刷新文件夹图标颜色
    /// @param event 变化事件
    /// @return 无返回值
    void changeEvent(QEvent *event) override;

private:
    /// @brief 弹出与字段类型对应的文件对话框
    /// @return 无返回值
    void browse();

    Kind m_kind;
    QString m_label;
    QLineEdit *m_edit = nullptr;
    QPushButton *m_browse = nullptr;
};

}  // namespace markshot::settings
