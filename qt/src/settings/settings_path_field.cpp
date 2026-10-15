#include "settings/settings_path_field.h"

#include "settings/settings_command_path.h"
#include "settings/settings_field_texts.h"
#include "settings/settings_inline_icons.h"
#include "settings/settings_text_context_menu.h"

#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>

namespace markshot::settings {

SettingsPathField::SettingsPathField(Kind kind, const QString &label, QWidget *parent)
    : QWidget(parent)
    , m_kind(kind)
    , m_label(label)
{
    setObjectName(QStringLiteral("settingsPathField"));
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    // 1. 【设置】【路径字段】输入框占满剩余宽度，常见长路径不再被截断
    m_edit = new QLineEdit(this);
    m_edit->setAccessibleName(label);
    m_edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    installLineEditContextMenu(m_edit);
    layout->addWidget(m_edit, 1);

    // 2. 【设置】【路径字段】浏览按钮使用文件夹图标与短文案，宽度随内容
    m_browse = new QPushButton(texts::browse(), this);
    m_browse->setObjectName(QStringLiteral("settingsPathBrowse"));
    m_browse->setAccessibleName(texts::browse() + QLatin1Char(' ') + label);
    m_browse->setIcon(settingsGlyphIcon(SettingsGlyph::Folder, palette().color(QPalette::ButtonText)));
    m_browse->setIconSize(QSize(16, 16));
    m_browse->setCursor(Qt::PointingHandCursor);
    m_browse->setAutoDefault(false);
    m_browse->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(m_browse);

    // 3. 【设置】【路径字段】点击标签区时焦点转交输入框
    setFocusProxy(m_edit);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(m_browse, &QPushButton::clicked, this, &SettingsPathField::browse);
}

SettingsPathField::Kind SettingsPathField::kind() const
{
    return m_kind;
}

QLineEdit *SettingsPathField::lineEdit() const
{
    return m_edit;
}

QPushButton *SettingsPathField::browseButton() const
{
    return m_browse;
}

void SettingsPathField::applySelectedPath(const QString &path)
{
    if (path.isEmpty()) {
        return;
    }
    // 1. 【设置】【路径字段】命令字段只替换程序路径，保留 {image} 等占位符与参数
    if (m_kind == Kind::Command) {
        m_edit->setText(replaceCommandProgram(m_edit->text(), QDir::toNativeSeparators(path)));
        return;
    }
    m_edit->setText(QDir::toNativeSeparators(path));
}

QString SettingsPathField::dialogStartPath() const
{
    const QString text = m_edit->text().trimmed();
    // 1. 【设置】【路径字段】命令字段以程序所在目录作为起点
    const QString path = expandHomePath(m_kind == Kind::Command ? splitCommandProgram(text).program : text);
    if (path.isEmpty()) {
        return QDir::homePath();
    }
    if (m_kind == Kind::Command) {
        const QFileInfo info(path);
        return info.isAbsolute() ? info.absolutePath() : QDir::homePath();
    }
    return path;
}

void SettingsPathField::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange) {
        m_browse->setIcon(settingsGlyphIcon(SettingsGlyph::Folder, palette().color(QPalette::ButtonText)));
    }
}

void SettingsPathField::browse()
{
    // 1. 【设置】【路径字段】按字段类型选择目录、保存文件或可执行程序
    QString selected;
    switch (m_kind) {
    case Kind::Directory:
        selected = QFileDialog::getExistingDirectory(this, texts::selectFolder(), dialogStartPath());
        break;
    case Kind::SaveFile:
        selected = QFileDialog::getSaveFileName(this, texts::selectFile(), dialogStartPath(), {}, nullptr,
                                                QFileDialog::DontConfirmOverwrite);
        break;
    case Kind::Command:
        selected = QFileDialog::getOpenFileName(this, texts::selectProgram(), dialogStartPath());
        break;
    }
    // 2. 【设置】【路径字段】取消选择时保持原文本
    applySelectedPath(selected);
}

}  // namespace markshot::settings
