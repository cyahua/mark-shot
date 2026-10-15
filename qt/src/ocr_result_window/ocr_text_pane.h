#pragma once

#include <QFrame>
#include <QString>

class QLabel;
class QProgressBar;
class QPushButton;
class QTextEdit;
class QTimer;

namespace markshot::shot {

/// @brief 组合文本编辑、字符统计和复制入口的 OCR 文本区
class OcrTextPane final : public QFrame {
    Q_OBJECT

public:
    /// @brief 创建可编辑的文本区
    /// @param title 分区标题，同时用作编辑器的无障碍名称
    /// @param placeholder 空文本提示
    /// @param parent 所属窗口
    explicit OcrTextPane(const QString &title, const QString &placeholder, QWidget *parent = nullptr);

    /// @brief 获取文本编辑器，供窗口连接编辑事件和设置焦点
    /// @return 当前文本编辑器
    QTextEdit *editor() const;

    /// @brief 获取完整纯文本，保留空格和换行
    /// @return 当前文本
    QString text() const;

    /// @brief 替换文本并同步统计与复制按钮状态
    /// @param text 新的纯文本
    /// @return 无返回值
    void setText(const QString &text);

    /// @brief 显示持久提示，完整内容同时保留在工具提示中
    /// @param text 提示内容，为空时隐藏
    /// @param error 是否采用错误状态配色
    /// @return 无返回值
    void setNotice(const QString &text, bool error = false);

    /// @brief 显示翻译进度并在任务期间暂停译文编辑
    /// @param busy 是否正在生成文本
    /// @return 无返回值
    void setBusy(bool busy);

    /// @brief 根据当前调色板更新复制图标
    /// @return 无返回值
    void refreshTheme();

    /// @brief 在所属复制按钮就地显示结果并自动恢复操作文案
    /// @param success 剪贴板写入是否成功
    /// @return 无返回值
    void showCopyFeedback(bool success);

    /// @brief 将字符统计移入窗口状态栏，保留统计文本、行数提示与所属分区信息
    /// @param parent 窗口状态栏所属控件
    /// @return 重新归属后的统计标签，由调用方加入状态栏布局
    QLabel *takeStatisticsLabel(QWidget *parent);

    /// @brief 控制分区标题是否占用正文上方的一行
    /// @param visible 为 false 时隐藏标题，复制按钮移走且没有撤销时整行收起
    /// @return 无返回值
    void setSectionTitleVisible(bool visible);

    /// @brief 把复制按钮交给外部布局，原文页用来放到窗口标题栏
    /// @param parent 新的父控件
    /// @return 复制按钮，调用方负责加入布局
    QPushButton *takeCopyButton(QWidget *parent);

signals:
    /// @brief 请求窗口通过应用的剪贴板服务复制完整文本
    /// @param text 保留原始空白的完整文本
    void copyRequested(const QString &text);

private:
    /// @brief 按用户可见字符统计文本，并同步复制入口
    /// @return 无返回值
    void updateTextState();

    /// @brief 标题、复制和撤销都离开后收起标题行，把高度还给编辑器
    /// @return 无返回值
    void updateHeaderVisibility();

    /// @brief 显示采用窗口主题的编辑菜单
    /// @param position 编辑器内部的菜单请求位置
    /// @return 无返回值
    void showEditorMenu(const QPoint &position);

    QWidget *m_header = nullptr;
    QTextEdit *m_editor = nullptr;
    QPushButton *m_copyButton = nullptr;
    QPushButton *m_undoButton = nullptr;
    QTimer *m_copyTimer = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_statistics = nullptr;
    QLabel *m_notice = nullptr;
    QProgressBar *m_progress = nullptr;
};

}
