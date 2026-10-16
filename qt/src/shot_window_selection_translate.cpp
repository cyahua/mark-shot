#include "shot_window_module.h"

#include "providers/ocr/ocr_provider_factory.h"
#include "providers/provider_task.h"
#include "providers/translate/translate_provider_factory.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QScopeGuard>
#include <QScopedValueRollback>
#include <QTemporaryFile>

#include <algorithm>

using namespace markshot::shot;

namespace {

/// 关闭时框选页不提供一键翻译
inline constexpr bool FEATURE_SELECTION_TRANSLATE_ENABLED = true;

/**
 * 把译文画进冻结截图的选区。
 * @param frame 冻结截图，就地修改。
 * @param selectionRect 选区在冻结图中的像素矩形。
 * @param tokens 相对选区左上角的译文框。
 * @return 无返回值。
 */
void bakeTranslationTokens(QImage *frame, const QRect &selectionRect, const QVector<markshot::ocr::Token> &tokens)
{
    if (!frame || selectionRect.isEmpty() || tokens.isEmpty()) {
        return;
    }
    QPainter painter(frame);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    for (const markshot::ocr::Token &token : tokens) {
        if (token.text.trimmed().isEmpty() || token.imageRect.isEmpty()) {
            continue;
        }
        // 1. OCR 坐标相对裁剪图，平移到冻结图后再限制在选区内
        const QRectF textRect = token.imageRect.translated(selectionRect.topLeft())
                                    .adjusted(-2.0, -1.0, 2.0, 1.0)
                                    .intersected(QRectF(selectionRect));
        if (textRect.isEmpty()) {
            continue;
        }
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, 235));
        painter.drawRoundedRect(textRect, 2.0, 2.0);
        QFont font = painter.font();
        font.setPixelSize(std::clamp(qRound(textRect.height() * 0.62), 8, 48));
        painter.setFont(font);
        painter.setPen(QColor(17, 24, 39));
        painter.drawText(textRect.adjusted(2.0, 0.0, -2.0, 0.0),
                         Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap,
                         token.text);
    }
}

/**
 * 组装翻译插件使用的 token JSON。
 * @param tokens OCR token。
 * @param targetLanguage 目标语言。
 * @return 紧凑 JSON。
 */
QByteArray translationInputJson(const QVector<markshot::ocr::Token> &tokens, const QString &targetLanguage)
{
    QJsonArray tokenArray;
    for (const markshot::ocr::Token &token : tokens) {
        QJsonObject object;
        object.insert(QStringLiteral("text"), token.text);
        object.insert(QStringLiteral("box"), markshot::ocr::rectToJson(token.imageRect));
        object.insert(QStringLiteral("line"), token.line);
        object.insert(QStringLiteral("index"), token.index);
        object.insert(QStringLiteral("confidence"), token.confidence);
        tokenArray.append(object);
    }
    QJsonObject root;
    root.insert(QStringLiteral("targetLanguage"), targetLanguage);
    root.insert(QStringLiteral("tokens"), tokenArray);
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

}  // namespace

void ShotWindow::translateSelection()
{
    if (!FEATURE_SELECTION_TRANSLATE_ENABLED) {
        return;
    }
    commitTextEditor();
    if (!hasUsableSelection() || m_mode != Mode::Editing) {
        return;
    }

    const PinnedWindowConfig config = pinnedWindowConfig();
    if (!config.ocrEnabled) {
        showToast(MS_TR("OCR helper not found"));
        return;
    }

    const QString tempPath = saveSelectionToTempFile();
    if (tempPath.isEmpty()) {
        return;
    }
    const auto removeTemp = qScopeGuard([&tempPath] { QFile::remove(tempPath); });
    const auto restoreCursor = qScopeGuard([this] { updateCursor(); });
    const QScopedValueRollback<bool> busy(m_operationBusy, true);
    updateCursor();

    // 1. 先识别选区文字，坐标保持在裁剪图内
    markshot::providers::OcrTaskRequest ocrRequest;
    ocrRequest.imagePath = tempPath;
    ocrRequest.backend = config.ocrBackend;
    ocrRequest.provider = config.ocrProvider;
    if (!config.ocrCommand.isEmpty()) {
        QString commandLine = config.ocrCommand;
        const bool replaced = replaceExtensionImagePlaceholders(&commandLine, tempPath);
        if (!replaced) {
            commandLine += QLatin1Char(' ');
            commandLine += shellQuote(tempPath);
        }
        ocrRequest.commandLine = commandLine;
    } else {
        ocrRequest.helperProgram = helperProgramPath(QStringLiteral("mark-shot-ocr"));
    }
    markshot::providers::ProviderTask *ocrTask = markshot::providers::createOcrTask(ocrRequest, this);
    ocrTask->start(config.ocrTimeoutMs);
    const markshot::providers::TaskResult ocrResult = ocrTask->waitForResult();
    ocrTask->deleteLater();
    if (!ocrResult.ok) {
        showToast(ocrResult.error == markshot::providers::TaskError::Timeout
                      ? MS_TR("OCR timed out")
                      : MS_TR("OCR failed"));
        return;
    }
    const QImage selectionImage(tempPath);
    const QVector<markshot::ocr::Token> sourceTokens =
        markshot::ocr::tokensFromJsonOutput(ocrResult.output, selectionImage.size());
    if (sourceTokens.isEmpty()) {
        showToast(MS_TR("No text to translate"));
        return;
    }

    // 2. 翻译识别结果，失败时保留原图
    QTemporaryFile inputFile(QDir::temp().filePath(QStringLiteral("mark-shot-selection-translate-XXXXXX.json")));
    inputFile.setAutoRemove(true);
    if (!inputFile.open()) {
        showToast(MS_TR("Translation failed"));
        return;
    }
    const QByteArray inputJson = translationInputJson(sourceTokens, config.translationTargetLanguage);
    inputFile.write(inputJson);
    inputFile.close();

    markshot::providers::TranslateTaskRequest request;
    request.inputJson = inputJson;
    request.inputPath = inputFile.fileName();
    request.targetLanguage = config.translationTargetLanguage;
    request.configPath = appConfigPath();
    request.provider = config.translationProvider;
    if (!config.translationCommand.isEmpty()) {
        QString commandLine = config.translationCommand;
        bool replaced = false;
        replaceShellPlaceholder(&commandLine, QStringLiteral("{input}"), inputFile.fileName(), &replaced);
        replaceShellPlaceholder(&commandLine, QStringLiteral("{inputPath}"), inputFile.fileName(), &replaced);
        replaceShellPlaceholder(&commandLine, QStringLiteral("{targetLanguage}"), config.translationTargetLanguage, &replaced);
        replaceShellPlaceholder(&commandLine, QStringLiteral("{config}"), appConfigPath(), &replaced);
        if (!replaced) {
            commandLine += QLatin1Char(' ');
            commandLine += shellQuote(inputFile.fileName());
        }
        request.commandLine = commandLine;
    } else {
        request.helperProgram = helperProgramPath(QStringLiteral("mark-shot-translate"));
    }
    markshot::providers::ProviderTask *task = markshot::providers::createTranslateTask(request, this);
    task->start(config.translationTimeoutMs);
    const markshot::providers::TaskResult result = task->waitForResult();
    task->deleteLater();
    if (!result.ok) {
        showToast(result.error == markshot::providers::TaskError::Timeout
                      ? MS_TR("Translation timed out")
                      : MS_TR("Translation failed"));
        return;
    }
    const QVector<markshot::ocr::Token> translated =
        markshot::ocr::tokensFromJsonOutput(result.output, selectionImage.size());
    if (translated.isEmpty()) {
        showToast(MS_TR("No text to translate"));
        return;
    }

    // 3. 把译文写回选区，复制和保存会带上这块预览
    const QRect selectionRect = normalizedSelection().toAlignedRect().intersected(QRect(QPoint(0, 0), m_frozenFrame.size()));
    bakeTranslationTokens(&m_frozenFrame, selectionRect, translated);
    showToast(MS_TR("Translation applied"));
    update();
}
