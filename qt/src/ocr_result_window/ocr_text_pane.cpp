#include "ocr_result_window/ocr_text_pane.h"

#include "ocr_result_window/ocr_result_window_style.h"
#include "ui/i18n.h"
#include "ui/theme.h"

#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QClipboard>
#include <QLabel>
#include "ui/context_menu.h"
#include <QPaintEvent>
#include <QProgressBar>
#include <QPushButton>
#include <QStyle>
#include <QTextBoundaryFinder>
#include <QTextDocument>
#include <QTextEdit>
#include <QTimer>

namespace markshot::shot {

namespace {

/// @brief 标题行空间不足时收起辅助统计，避免显示截断文本
class StatisticsLabel final : public QLabel {
public:
    using QLabel::QLabel;

protected:
    /// @brief 【OCR】【文本统计】仅在统计内容能够完整显示时绘制文本
    /// @param event 当前重绘事件
    /// @return 无返回值
    void paintEvent(QPaintEvent *event) override
    {
        if (width() >= sizeHint().width()) {
            QLabel::paintEvent(event);
        }
    }
};

}

OcrTextPane::OcrTextPane(const QString &title, const QString &placeholder, QWidget *parent)
    : QFrame(parent)
{
    setProperty("ocrPane", true);
    setMinimumSize(180, 112);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 2, 0, 0);
    layout->setSpacing(6);
    // 1. 【OCR】【分区高度】根据进度和提示的实际高度更新最小尺寸，避免挤压编辑器
    layout->setSizeConstraint(QLayout::SetMinimumSize);

    // 2. 【OCR】【文本分区】将复制入口与所属文本放在同一分区
    auto *header = new QHBoxLayout;
    header->setSpacing(6);
    m_titleLabel = new QLabel(title, this);
    m_titleLabel->setProperty("role", QStringLiteral("sectionTitle"));
    m_titleLabel->setFont(markshot::theme::uiFont(10, QFont::DemiBold));
    header->addWidget(m_titleLabel);
    m_statistics = new StatisticsLabel(this);
    m_statistics->setObjectName(QStringLiteral("ocrTextStatistics"));
    m_statistics->setProperty("role", QStringLiteral("muted"));
    m_statistics->setFont(markshot::theme::uiFont(9));
    m_statistics->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    header->addStretch(1);
    m_statistics->hide();
    m_undoButton = new QPushButton(this);
    m_undoButton->setObjectName(QStringLiteral("ocrUndoButton"));
    m_undoButton->setProperty("role", QStringLiteral("quiet"));
    m_undoButton->setFixedSize(28, 28);
    m_undoButton->setAccessibleName(MS_TR("Undo edit"));
    m_undoButton->setToolTip(MS_TR("Undo edit") + QStringLiteral(" (Ctrl+Z)"));
    m_undoButton->hide();
    header->addWidget(m_undoButton);
    m_copyButton = new QPushButton(MS_TR("Copy"), this);
    m_copyButton->setObjectName(QStringLiteral("ocrCopyButton"));
    m_copyButton->setProperty("role", QStringLiteral("quiet"));
    m_copyButton->setFont(markshot::theme::uiFont(10));
    m_copyButton->setMinimumWidth(68);
    m_copyButton->setIconSize(QSize(16, 16));
    m_copyButton->setToolTip(MS_TR("Copy all text"));
    m_copyButton->setAccessibleName(MS_TR("Copy %1").arg(title));
    header->addWidget(m_copyButton);
    layout->addLayout(header);

    // 3. 【OCR】【文本编辑】保持纯文本和原有换行，长行在窗口内自动折行
    m_editor = new QTextEdit(this);
    m_editor->setAcceptRichText(false);
    m_editor->setAccessibleName(title);
    m_editor->setPlaceholderText(placeholder);
    m_editor->setFont(markshot::theme::uiFont(11));
    m_editor->setTabStopDistance(m_editor->fontMetrics().horizontalAdvance(QLatin1Char(' ')) * 4);
    m_editor->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    m_editor->setMinimumSize(0, 28);
    m_editor->document()->setDocumentMargin(4);
    m_editor->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_editor, &QTextEdit::customContextMenuRequested, this, &OcrTextPane::showEditorMenu);
    connect(m_editor, &QTextEdit::textChanged, this, &OcrTextPane::updateTextState);
    connect(m_editor, &QTextEdit::undoAvailable, m_undoButton, &QWidget::setVisible);
    connect(m_undoButton, &QPushButton::clicked, m_editor, &QTextEdit::undo);
    layout->addWidget(m_editor, 1);

    m_progress = new QProgressBar(this);
    m_progress->setObjectName(QStringLiteral("ocrTranslationProgress"));
    m_progress->setRange(0, 0);
    m_progress->setTextVisible(false);
    m_progress->setFixedHeight(3);
    m_progress->hide();
    layout->addWidget(m_progress);

    m_notice = new QLabel(this);
    m_notice->setObjectName(QStringLiteral("ocrPaneNotice"));
    m_notice->setTextFormat(Qt::PlainText);
    m_notice->setWordWrap(true);
    m_notice->setFont(markshot::theme::uiFont(9));
    m_notice->setMaximumHeight(m_notice->fontMetrics().lineSpacing() * 3);
    m_notice->hide();
    layout->addWidget(m_notice);

    m_copyTimer = new QTimer(this);
    m_copyTimer->setSingleShot(true);
    connect(m_copyTimer, &QTimer::timeout, this, [this] { m_copyButton->setText(MS_TR("Copy")); });
    connect(m_copyButton, &QPushButton::clicked, this, [this] { emit copyRequested(text()); });
    updateTextState();
}

QTextEdit *OcrTextPane::editor() const
{
    return m_editor;
}

QString OcrTextPane::text() const
{
    return m_editor->toPlainText();
}

void OcrTextPane::setText(const QString &text)
{
    m_editor->setPlainText(text);
}

void OcrTextPane::setNotice(const QString &text, bool error)
{
    m_notice->setText(text);
    m_notice->setToolTip(text);
    m_notice->setProperty("error", error);
    m_notice->style()->unpolish(m_notice);
    m_notice->style()->polish(m_notice);
    m_notice->setVisible(!text.isEmpty());
}

void OcrTextPane::setBusy(bool busy)
{
    m_progress->setVisible(busy);
    m_editor->setReadOnly(busy);
    m_undoButton->setEnabled(!busy);
}

void OcrTextPane::refreshTheme()
{
    m_copyButton->setIcon(ocrActionIcon(types::Action::Copy, palette().color(QPalette::ButtonText)));
    m_undoButton->setIcon(ocrActionIcon(types::Action::Undo, palette().color(QPalette::ButtonText)));
}

void OcrTextPane::showCopyFeedback(bool success)
{
    m_copyButton->setText(success ? MS_TR("Copied") : MS_TR("Copy failed"));
    m_copyTimer->start(1800);
}

void OcrTextPane::updateTextState()
{
    // 1. 【OCR】【文本统计】按字素统计，避免把代理对或组合字符拆成多个字符
    const QString current = text();
    QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, current);
    int characters = 0;
    while (finder.toNextBoundary() > 0) {
        ++characters;
    }
    const int lines = current.isEmpty() ? 0 : current.count(QLatin1Char('\n')) + 1;
    m_statistics->setText(MS_TR("%1 characters").arg(characters));
    m_statistics->setToolTip(MS_TR("%1 characters · %2 lines").arg(characters).arg(lines));
    m_titleLabel->setToolTip(m_statistics->toolTip());
    m_copyButton->setEnabled(!current.trimmed().isEmpty());
}

QLabel *OcrTextPane::takeStatisticsLabel(QWidget *parent)
{
    m_statistics->setParent(parent);
    m_statistics->setAccessibleName(m_titleLabel->text());
    m_statistics->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_statistics->show();
    return m_statistics;
}

void OcrTextPane::showEditorMenu(const QPoint &position)
{
    markshot::ui::ContextMenu menu(this);
    const bool editable = !m_editor->isReadOnly();
    const bool selected = m_editor->textCursor().hasSelection();

    // 1. 【OCR】【文本菜单】复用编辑器操作和应用翻译，显式呈现可用状态
    const auto add = [this, &menu](const QString &label, const QKeySequence &shortcut,
                                  bool enabled, auto callback) {
        QAction *action = menu.addAction(label, this, callback);
        action->setShortcut(shortcut);
        action->setShortcutVisibleInContextMenu(true);
        action->setEnabled(enabled);
    };
    add(MS_TR("Undo"), QKeySequence::Undo, editable && m_editor->document()->isUndoAvailable(),
        [this] { m_editor->undo(); });
    add(MS_TR("Redo"), QKeySequence::Redo, editable && m_editor->document()->isRedoAvailable(),
        [this] { m_editor->redo(); });
    menu.addSeparator();
    add(MS_TR("Cut"), QKeySequence::Cut, editable && selected, [this] { m_editor->cut(); });
    add(MS_TR("Copy"), QKeySequence::Copy, selected, [this] { m_editor->copy(); });
    add(MS_TR("Paste"), QKeySequence::Paste,
        editable && !QApplication::clipboard()->text().isEmpty(), [this] { m_editor->paste(); });
    add(MS_TR("Delete"), QKeySequence(Qt::Key_Delete), editable && selected, [this] {
        QTextCursor cursor = m_editor->textCursor();
        cursor.removeSelectedText();
    });
    menu.addSeparator();
    add(MS_TR("Select All"), QKeySequence::SelectAll, !m_editor->document()->isEmpty(),
        [this] { m_editor->selectAll(); });
    menu.exec(m_editor->mapToGlobal(position));
}

}
