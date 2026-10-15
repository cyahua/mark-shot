#include "markshot/ocr_provider_plugin.h"

#include <QFileInfo>
#include <QObject>

class UpdateFixture final : public QObject, public markshot::plugin::OcrProviderPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID MARK_SHOT_OCR_PROVIDER_PLUGIN_IID)
    Q_INTERFACES(markshot::plugin::OcrProviderPlugin)
public:
    /// @brief 返回两个版本共用的 provider 标识，无参数
    /// @return 测试 provider 标识
    QString providerId() const override { return QStringLiteral("update-fixture"); }

    /// @brief 返回当前库版本，无参数
    /// @return 用于断言加载结果的版本名称
    QString displayName() const override { return QString::number(MARK_SHOT_TEST_PLUGIN_REVISION); }

    /// @brief 通过测试文件模拟模型安装前后的可用状态
    /// @param error 不可用原因
    /// @return 测试文件存在时返回 true
    bool isAvailable(QString *error) const override
    {
        const bool available = QFileInfo::exists(qEnvironmentVariable("MARK_SHOT_TEST_MODEL_MARKER"));
        if (error) *error = available ? QString() : QStringLiteral("Models not installed");
        return available;
    }

    /// @brief 测试库不提供实际识别
    /// @param image 输入图片
    /// @param tokens 输出文字列表
    /// @param error 输出错误
    /// @return false
    bool recognize(const QImage &image, QVector<markshot::plugin::OcrToken> *tokens, QString *error) override
    {
        Q_UNUSED(image);
        tokens->clear();
        if (error) *error = QStringLiteral("Fixture only");
        return false;
    }
};


#include "registry_update_plugin.moc"
