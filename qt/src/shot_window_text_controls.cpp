#include "shot_window_module.h"

#include <QLineEdit>

namespace {

constexpr int kMinTextPointSize = 8;
constexpr int kMaxTextPointSize = 300;
constexpr int kTextPointSizeOffset = 19;
// 字体弹层底部字号输入框与 B/I 按钮的统一高度
constexpr int kFontPanelControlHeight = 26;

}  // namespace

/**
 * 创建文字字体面板，并连接字体、字号、粗体和斜体控件。
 * @return 无返回值。
 */
void ShotWindow::initializePropertyFontPanel()
{
    m_controls.m_propertyFontPanel = new QWidget(this);
    m_controls.m_propertyFontPanel->setObjectName(QStringLiteral("propertyFontPanel"));
    m_controls.m_propertyFontPanel->setCursor(Qt::ArrowCursor);
    m_controls.m_propertyFontPanel->setStyleSheet(markshot::theme::openWithPanelStyleSheet());
    auto *fontPanelLayout = new QVBoxLayout(m_controls.m_propertyFontPanel);
    fontPanelLayout->setContentsMargins(6, 6, 6, 6);
    // 列表与底部控件行之间保留固定间距，高度由 updatePropertyFontPanelGeometry 统一分配
    fontPanelLayout->setSpacing(6);

    // 1. 搜索框按名称过滤，避免在整份字体列表里滚动查找
    m_controls.m_propertyFontSearch = new QLineEdit(m_controls.m_propertyFontPanel);
    m_controls.m_propertyFontSearch->setFixedHeight(kFontPanelControlHeight);
    m_controls.m_propertyFontSearch->setClearButtonEnabled(true);
    m_controls.m_propertyFontSearch->setPlaceholderText(MS_TR("Search fonts"));
    m_controls.m_propertyFontSearch->setAccessibleName(MS_TR("Search fonts"));
    fontPanelLayout->addWidget(m_controls.m_propertyFontSearch);

    // 2. 创建字体系列列表
    m_controls.m_propertyFontList = new QListWidget(m_controls.m_propertyFontPanel);
    m_controls.m_propertyFontList->setFocusPolicy(Qt::NoFocus);
    m_controls.m_propertyFontList->setUniformItemSizes(true);
    m_controls.m_propertyFontList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_controls.m_propertyFontList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    for (const QString &family : QFontDatabase::families()) {
        auto *item = new QListWidgetItem(family, m_controls.m_propertyFontList);
        item->setData(Qt::UserRole, family);
        item->setToolTip(family);
        item->setFont(QFont(family, 11));
    }
    connect(m_controls.m_propertyFontSearch, &QLineEdit::textChanged, this, [this](const QString &query) {
        const QString needle = query.trimmed();
        for (int index = 0; index < m_controls.m_propertyFontList->count(); ++index) {
            QListWidgetItem *item = m_controls.m_propertyFontList->item(index);
            const QString family = item->data(Qt::UserRole).toString();
            item->setHidden(!needle.isEmpty() && !family.contains(needle, Qt::CaseInsensitive));
        }
    });
    connect(m_controls.m_propertyFontList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        if (!item) {
            return;
        }
        setSelectedTextFontFamily(item->data(Qt::UserRole).toString());
        m_controls.m_propertyFontPanel->hide();
    });
    fontPanelLayout->addWidget(m_controls.m_propertyFontList, 1);

    // 3. 底部控件行：字号输入框与 B/I 按钮同排，固定高度，不参与列表伸缩
    auto *fontStyleLayout = new QHBoxLayout;
    fontStyleLayout->setContentsMargins(0, 0, 0, 0);
    fontStyleLayout->setSpacing(4);
    m_controls.m_propertyFontSizeSpin = new QSpinBox(m_controls.m_propertyFontPanel);
    m_controls.m_propertyFontSizeSpin->setFixedHeight(kFontPanelControlHeight);
    m_controls.m_propertyFontSizeSpin->setRange(kMinTextPointSize, kMaxTextPointSize);
    m_controls.m_propertyFontSizeSpin->setSingleStep(1);
    m_controls.m_propertyFontSizeSpin->setSuffix(QStringLiteral(" pt"));
    m_controls.m_propertyFontSizeSpin->setFocusPolicy(Qt::StrongFocus);
    m_controls.m_propertyFontSizeSpin->setToolTip(MS_TR("Text font size in points"));
    connect(m_controls.m_propertyFontSizeSpin, &QSpinBox::valueChanged, this, [this](int value) {
        setSelectedTextFontSize(value);
    });
    fontStyleLayout->addWidget(m_controls.m_propertyFontSizeSpin, 1);

    // 4. 创建粗体和斜体切换控件
    m_controls.m_propertyFontBoldButton = new QToolButton(m_controls.m_propertyFontPanel);
    m_controls.m_propertyFontBoldButton->setFixedSize(kFontPanelControlHeight, kFontPanelControlHeight);
    m_controls.m_propertyFontBoldButton->setCheckable(true);
    m_controls.m_propertyFontBoldButton->setText(QStringLiteral("B"));
    m_controls.m_propertyFontBoldButton->setToolTip(MS_TR("Bold"));
    m_controls.m_propertyFontBoldButton->setFocusPolicy(Qt::NoFocus);
    QFont boldButtonFont = m_controls.m_propertyFontBoldButton->font();
    boldButtonFont.setBold(true);
    m_controls.m_propertyFontBoldButton->setFont(boldButtonFont);
    connect(m_controls.m_propertyFontBoldButton, &QToolButton::toggled, this, [this](bool checked) {
        setSelectedTextBold(checked);
    });
    fontStyleLayout->addWidget(m_controls.m_propertyFontBoldButton);

    m_controls.m_propertyFontItalicButton = new QToolButton(m_controls.m_propertyFontPanel);
    m_controls.m_propertyFontItalicButton->setFixedSize(kFontPanelControlHeight, kFontPanelControlHeight);
    m_controls.m_propertyFontItalicButton->setCheckable(true);
    m_controls.m_propertyFontItalicButton->setText(QStringLiteral("I"));
    m_controls.m_propertyFontItalicButton->setToolTip(MS_TR("Italic"));
    m_controls.m_propertyFontItalicButton->setFocusPolicy(Qt::NoFocus);
    QFont italicButtonFont = m_controls.m_propertyFontItalicButton->font();
    italicButtonFont.setItalic(true);
    m_controls.m_propertyFontItalicButton->setFont(italicButtonFont);
    connect(m_controls.m_propertyFontItalicButton, &QToolButton::toggled, this, [this](bool checked) {
        setSelectedTextItalic(checked);
    });
    fontStyleLayout->addWidget(m_controls.m_propertyFontItalicButton);
    fontPanelLayout->addLayout(fontStyleLayout, 0);
    m_controls.m_propertyFontPanel->hide();
}

/**
 * 设置选中文字标注或文字工具默认使用的字体系列。
 * @param fontFamily 字体系列名称。
 * @return 无返回值。
 */
void ShotWindow::setSelectedTextFontFamily(const QString &fontFamily)
{
    if (fontFamily.isEmpty()) {
        return;
    }

    if (m_selectedAnnotationId.has_value()) {
        Annotation *annotation = annotationById(*m_selectedAnnotationId);
        if (!annotation || annotation->tool != Tool::Text || annotation->fontFamily == fontFamily) {
            return;
        }
        pushHistorySnapshot();
        annotation->fontFamily = fontFamily;
    } else {
        if (m_tool != Tool::Text || m_textFontFamily == fontFamily) {
            return;
        }
        m_textFontFamily = fontFamily;
        if (m_textEditor && m_textEditor->isVisible() && !m_editingTextAnnotationId.has_value()) {
            QFont font = m_textEditor->font();
            font.setFamily(m_textFontFamily);
            m_textEditor->setFont(font);
        }
    }
    updateAnnotationPropertyPanel();
    update();
    persistAnnotationState();
}

/**
 * 设置选中文字标注或文字工具默认使用的精确字号。
 * @param pointSize 最终渲染字号，范围为 8 至 300 点。
 * @return 无返回值。
 */
void ShotWindow::setSelectedTextFontSize(int pointSize)
{
    pointSize = std::clamp(pointSize, kMinTextPointSize, kMaxTextPointSize);
    if (m_controls.m_propertyFontSizeSpin) {
        const QSignalBlocker blocker(m_controls.m_propertyFontSizeSpin);
        m_controls.m_propertyFontSizeSpin->setValue(pointSize);
    }

    // 1. 标注宽度字段保存相对于 19 点的字号偏移
    const int targetWidth = pointSize - kTextPointSizeOffset;
    setSelectedAnnotationWidth(targetWidth);

    // 2. 内联编辑器立即使用相同字号
    if (m_textEditor && m_textEditor->isVisible()) {
        QFont font = m_textEditor->font();
        font.setPointSize(pointSize);
        m_textEditor->setFont(font);
    }
    update();
}

/**
 * 切换选中文字标注或文字工具默认样式的粗体状态。
 * @param bold 是否使用半粗体字重。
 * @return 无返回值。
 */
void ShotWindow::setSelectedTextBold(bool bold)
{
    const QFont::Weight targetWeight = bold ? QFont::DemiBold : QFont::Normal;
    const QVector<int> selectedIds = selectedAnnotationIds();
    if (!selectedIds.isEmpty()) {
        bool changed = false;
        for (int id : selectedIds) {
            const Annotation *annotation = annotationById(id);
            if (annotation && annotation->tool == Tool::Text && annotation->fontWeight != targetWeight) {
                changed = true;
                break;
            }
        }
        if (!changed) {
            return;
        }
        pushHistorySnapshot();
        for (int id : selectedIds) {
            if (Annotation *annotation = annotationById(id);
                annotation && annotation->tool == Tool::Text) {
                annotation->fontWeight = targetWeight;
            }
        }
    } else {
        if (m_textWeight == targetWeight) {
            return;
        }
        m_textWeight = targetWeight;
    }
    if (m_textEditor && m_textEditor->isVisible()) {
        QFont font = m_textEditor->font();
        if (m_editingTextAnnotationId.has_value()) {
            if (const Annotation *annotation = annotationById(*m_editingTextAnnotationId)) {
                font.setWeight(annotation->fontWeight);
                font.setItalic(annotation->textItalic);
            }
        } else {
            font.setWeight(m_textWeight);
            font.setItalic(m_textItalic);
        }
        m_textEditor->setFont(font);
    }
    updateAnnotationPropertyPanel();
    update();
    persistAnnotationState();
}

/**
 * 切换选中文字标注或文字工具默认样式的斜体状态。
 * @param italic 是否使用斜体。
 * @return 无返回值。
 */
void ShotWindow::setSelectedTextItalic(bool italic)
{
    const QVector<int> selectedIds = selectedAnnotationIds();
    if (!selectedIds.isEmpty()) {
        bool changed = false;
        for (int id : selectedIds) {
            const Annotation *annotation = annotationById(id);
            if (annotation && annotation->tool == Tool::Text && annotation->textItalic != italic) {
                changed = true;
                break;
            }
        }
        if (!changed) {
            return;
        }
        pushHistorySnapshot();
        for (int id : selectedIds) {
            if (Annotation *annotation = annotationById(id);
                annotation && annotation->tool == Tool::Text) {
                annotation->textItalic = italic;
            }
        }
    } else {
        if (m_textItalic == italic) {
            return;
        }
        m_textItalic = italic;
    }
    if (m_textEditor && m_textEditor->isVisible()) {
        QFont font = m_textEditor->font();
        if (m_editingTextAnnotationId.has_value()) {
            if (const Annotation *annotation = annotationById(*m_editingTextAnnotationId)) {
                font.setWeight(annotation->fontWeight);
                font.setItalic(annotation->textItalic);
            }
        } else {
            font.setWeight(m_textWeight);
            font.setItalic(m_textItalic);
        }
        m_textEditor->setFont(font);
    }
    updateAnnotationPropertyPanel();
    update();
    persistAnnotationState();
}
