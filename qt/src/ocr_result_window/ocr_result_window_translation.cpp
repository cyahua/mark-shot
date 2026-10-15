#include "ocr_result_window/ocr_result_window.h"

#include "debug_log.h"
#include "ocr_result_window/ocr_text_pane.h"
#include "providers/provider_task.h"
#include "providers/translate/translate_provider_factory.h"
#include "shell_command.h"
#include "translation_language_options.h"
#include "ui/i18n.h"
#include "ui/theme.h"

#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QPushButton>
#include <QTemporaryFile>
#include <QTextEdit>

namespace markshot::shot {

void OcrResultWindow::setupTargetLanguageCombo()
{
    m_targetLanguageCombo->setObjectName(QStringLiteral("ocrLanguageCombo"));
    m_targetLanguageCombo->setFont(markshot::theme::uiFont(10));
    m_targetLanguageCombo->setEditable(true);
    m_targetLanguageCombo->setInsertPolicy(QComboBox::NoInsert);
    m_targetLanguageCombo->setMinimumWidth(130);
    m_targetLanguageCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_targetLanguageCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_targetLanguageCombo->setMinimumContentsLength(16);
    m_targetLanguageCombo->setToolTip(MS_TR("Target Language"));
    m_targetLanguageCombo->setAccessibleName(MS_TR("Target Language"));

    // 1. 【OCR】【目标语言】内置语言和手动输入共用规范化及持久化逻辑
    for (const markshot::TranslationLanguageOption &language : markshot::translationLanguageOptions()) {
        m_targetLanguageCombo->addItem(language.label, language.value);
    }
    setTargetLanguageComboValue(m_config.translationTargetLanguage);
    connect(m_targetLanguageCombo, QOverload<int>::of(&QComboBox::activated), this,
            [this] { applyTargetLanguageFromCombo(); });
    connect(m_targetLanguageCombo->lineEdit(), &QLineEdit::editingFinished, this,
            [this] { applyTargetLanguageFromCombo(); });
    connect(m_targetLanguageCombo, &QComboBox::currentTextChanged, this,
            [this] { updateSourceState(); });
}

void OcrResultWindow::setTargetLanguageComboValue(const QString &targetLanguage)
{
    const QString normalized = markshot::translationLanguageValueFromText(targetLanguage);
    for (int index = 0; index < m_targetLanguageCombo->count(); ++index) {
        if (m_targetLanguageCombo->itemData(index).toString() == normalized) {
            m_targetLanguageCombo->setCurrentIndex(index);
            return;
        }
    }
    m_targetLanguageCombo->setEditText(normalized);
}

QString OcrResultWindow::currentTargetLanguage() const
{
    const QString text = m_targetLanguageCombo->currentText().trimmed();
    const int index = m_targetLanguageCombo->findText(text, Qt::MatchFixedString);
    if (index >= 0) {
        const QString value = m_targetLanguageCombo->itemData(index).toString().trimmed();
        if (!value.isEmpty()) {
            return value;
        }
    }
    return text.isEmpty() ? m_config.translationTargetLanguage
                         : markshot::translationLanguageValueFromText(text);
}

void OcrResultWindow::applyTargetLanguageFromCombo()
{
    const QString targetLanguage = currentTargetLanguage().trimmed();
    if (targetLanguage.isEmpty() || targetLanguage == m_config.translationTargetLanguage) {
        return;
    }
    QString error;
    if (markshot::saveTranslationTargetLanguage(targetLanguage, &error)) {
        m_config.translationTargetLanguage = targetLanguage;
        setTargetLanguageComboValue(targetLanguage);
    } else {
        setTargetLanguageComboValue(m_config.translationTargetLanguage);
        showToast(MS_TR("Failed to save settings"));
        markshot::debugLog("config", "【OCR】【目标语言】保存失败: %s", error.toUtf8().constData());
    }
    updateSourceState();
}

void OcrResultWindow::startTranslation()
{
    if (m_translationTask) {
        return;
    }
    applyTargetLanguageFromCombo();
    const QString targetLanguage = m_config.translationTargetLanguage.trimmed();
    const QString text = m_sourcePane->text().trimmed();
    if (text.isEmpty() || targetLanguage.isEmpty()) {
        showToast(MS_TR("No text to translate"));
        return;
    }

    // 1. 【OCR】【翻译输入】记录原文快照，按已有协议生成行级 token
    QJsonArray tokens;
    int lineIndex = 0;
    for (const QString &rawLine : text.split(QLatin1Char('\n'))) {
        const QString line = rawLine.trimmed();
        if (!line.isEmpty()) {
            tokens.append(QJsonObject{
                {QStringLiteral("text"), line},
                {QStringLiteral("box"), QJsonArray{0, lineIndex * 24.0, 1000.0, 20.0}},
                {QStringLiteral("line"), lineIndex},
                {QStringLiteral("index"), 0},
                {QStringLiteral("confidence"), 1.0}});
        }
        ++lineIndex;
    }
    const QByteArray inputJson = QJsonDocument(QJsonObject{
        {QStringLiteral("targetLanguage"), targetLanguage},
        {QStringLiteral("tokens"), tokens}}).toJson(QJsonDocument::Compact);
    QTemporaryFile inputFile(QDir::temp().filePath(QStringLiteral("mark-shot-ocr-result-translate-XXXXXX.json")));
    if (!inputFile.open() || inputFile.write(inputJson) != inputJson.size() || !inputFile.flush()) {
        showTranslationPane();
        m_translationFailed = true;
        m_translationPane->setNotice(MS_TR("Translation failed"), true);
        return;
    }
    inputFile.setAutoRemove(false);
    m_translationInputPath = inputFile.fileName();
    inputFile.close();

    // 2. 【OCR】【翻译请求】沿用现有 provider 优先链与命令占位符
    markshot::providers::TranslateTaskRequest request;
    request.inputJson = inputJson;
    request.inputPath = m_translationInputPath;
    request.targetLanguage = targetLanguage;
    request.configPath = appConfigPath();
    request.provider = m_config.translationProvider;
    if (!m_config.translationCommand.isEmpty()) {
        QString commandLine = m_config.translationCommand;
        bool replaced = false;
        replaceShellPlaceholder(&commandLine, QStringLiteral("{input}"), m_translationInputPath, &replaced);
        replaceShellPlaceholder(&commandLine, QStringLiteral("{inputPath}"), m_translationInputPath, &replaced);
        replaceShellPlaceholder(&commandLine, QStringLiteral("{targetLanguage}"), targetLanguage, &replaced);
        replaceShellPlaceholder(&commandLine, QStringLiteral("{config}"), appConfigPath(), &replaced);
        if (!replaced) {
            commandLine += QLatin1Char(' ') + shellQuote(m_translationInputPath);
        }
        request.commandLine = commandLine;
    } else {
        request.helperProgram = helperProgramPath(QStringLiteral("mark-shot-translate"));
    }

    // 3. 【OCR】【翻译执行】原文保持可编辑，译文单独显示进度并支持主动取消
    showTranslationPane();
    m_pendingSource = text;
    m_pendingTarget = targetLanguage;
    m_translationFailed = false;
    m_translationPane->setBusy(true);
    m_translationPane->setNotice(MS_TR("Translating..."));
    m_translateButton->setText(MS_TR("Cancel"));
    m_translateButton->setToolTip(MS_TR("Cancel translation"));
    m_targetLanguageCombo->setEnabled(false);
    markshot::providers::ProviderTask *task = markshot::providers::createTranslateTask(request, this);
    m_translationTask = task;
    connect(task, &markshot::providers::ProviderTask::finished, this,
            [this, task](const markshot::providers::TaskResult &result) { finishTranslation(task, result); });
    task->start(m_config.translationTimeoutMs);
}

void OcrResultWindow::finishTranslation(markshot::providers::ProviderTask *task,
                                       const markshot::providers::TaskResult &result)
{
    if (task != m_translationTask) {
        return;
    }

    // 1. 【OCR】【翻译结果】只有成功任务的有效文本可以更新译文，原文始终不变
    QStringList translatedLines;
    QString detail;
    const QJsonDocument document = QJsonDocument::fromJson(result.output);
    if (result.ok && document.isObject()) {
        const QJsonObject root = document.object();
        for (const QJsonValue &value : root.value(QStringLiteral("tokens")).toArray()) {
            if (value.isObject()) {
                translatedLines.append(value.toObject().value(QStringLiteral("text")).toString().trimmed());
            }
        }
        const QJsonArray errors = root.value(QStringLiteral("errors")).toArray();
        if (!errors.isEmpty()) {
            detail = errors.first().toString();
        }
    }
    const QString translatedText = translatedLines.join(QLatin1Char('\n')).trimmed();
    if (!translatedText.isEmpty()) {
        m_translatedSource = m_pendingSource;
        m_translatedTarget = m_pendingTarget;
        m_translationPane->setText(translatedText);
        m_translationPane->setNotice(QString());
    } else {
        // 2. 【OCR】【翻译失败】错误留在译文区，长内容可通过工具提示完整查看
        m_translationFailed = true;
        if (detail.isEmpty()) {
            detail = QString::fromUtf8(result.errorOutput).trimmed();
        }
        if (detail.isEmpty()) {
            switch (result.error) {
            case markshot::providers::TaskError::Timeout:
                detail = MS_TR("timed out");
                break;
            case markshot::providers::TaskError::StartFailed:
                detail = MS_TR("provider failed to start");
                break;
            case markshot::providers::TaskError::Failed:
                detail = MS_TR("provider failed");
                break;
            case markshot::providers::TaskError::None:
                detail = MS_TR("no translation result");
                break;
            }
        }
        const QString provider = result.providerName.isEmpty() ? MS_TR("unknown provider") : result.providerName;
        m_translationPane->setNotice(MS_TR("Translation failed (%1): %2").arg(provider, detail), true);
    }
    finishTranslationCleanup(task);
}

void OcrResultWindow::cancelTranslation()
{
    if (m_translationTask) {
        disconnect(m_translationTask, nullptr, this, nullptr);
        m_translationTask->cancel();
        m_translationTask->deleteLater();
        m_translationTask = nullptr;
    }
    if (!m_translationInputPath.isEmpty()) {
        QFile::remove(m_translationInputPath);
        m_translationInputPath.clear();
    }
    resetTranslationUi();
}

void OcrResultWindow::finishTranslationCleanup(markshot::providers::ProviderTask *task)
{
    m_translationTask = nullptr;
    if (!m_translationInputPath.isEmpty()) {
        QFile::remove(m_translationInputPath);
        m_translationInputPath.clear();
    }
    resetTranslationUi();
    task->deleteLater();
}

void OcrResultWindow::resetTranslationUi()
{
    m_pendingSource.clear();
    m_pendingTarget.clear();
    if (m_translationPane) {
        m_translationPane->setBusy(false);
    }
    if (m_translateButton) {
        m_translateButton->setText(MS_TR("Translate"));
        m_translateButton->setToolTip(MS_TR("Translate") + QStringLiteral(" (Ctrl+Enter)"));
    }
    if (m_targetLanguageCombo) {
        m_targetLanguageCombo->setEnabled(true);
    }
    updateSourceState();
}

}
