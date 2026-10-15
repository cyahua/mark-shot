#include "rapid_model_paths.h"
#include "markshot/ocr_model_directory.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class RapidModelPathsTest final : public QObject {
    Q_OBJECT
private:
    QStringList m_createdFiles;

    /// @brief 在指定目录创建三份模型路径测试文件
    /// @param directory 测试模型目录
    /// @return 无返回值，已有文件时拒绝覆盖
    void createModels(const QString &directory)
    {
        QVERIFY(QDir().mkpath(directory));
        for (const auto *name : {"ch_PP-OCRv5_det_mobile.onnx", "ch_PP-OCRv5_rec_mobile.onnx", "ppocrv5_dict.txt"}) {
            QFile file(QDir(directory).filePath(QLatin1String(name)));
            QVERIFY2(!file.exists(), "Refusing to overwrite an existing model fixture");
            QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::NewOnly));
            m_createdFiles.append(file.fileName());
            QCOMPARE(file.write("fixture"), qint64(7));
        }
    }

private slots:
    /// @brief 只删除当前测试创建的文件，无参数和返回值
    void cleanup()
    {
        for (const QString &path : m_createdFiles) {
            QVERIFY(QFile::remove(path));
        }
        m_createdFiles.clear();
        qunsetenv("MARK_SHOT_OCR_MODEL_DIR");
    }

    /// @brief 验证下载器所用的平台标准目录能够直接被插件识别，无参数和返回值
    void findsDownloadedModels()
    {
        const QString directory = markshot::plugin::ocrModelDirectory();
        QCOMPARE(directory, QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
                                .filePath(QStringLiteral("mark-shot/models")));
        createModels(directory);
        const auto paths = markshot::ocr_rapid::locateRapidModels();
        QVERIFY(paths.isComplete());
        QCOMPARE(paths.detModel, QDir(directory).filePath(QStringLiteral("ch_PP-OCRv5_det_mobile.onnx")));
        QCOMPARE(paths.recModel, QDir(directory).filePath(QStringLiteral("ch_PP-OCRv5_rec_mobile.onnx")));
        QCOMPARE(paths.recDictionary, QDir(directory).filePath(QStringLiteral("ppocrv5_dict.txt")));
    }

    /// @brief 用户指定目录同时控制下载位置与插件搜索优先级，无参数和返回值
    void honorsConfiguredDirectory()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        qputenv("MARK_SHOT_OCR_MODEL_DIR", directory.path().toUtf8());
        createModels(directory.path());
        QCOMPARE(markshot::plugin::ocrModelDirectory(), directory.path());
        const auto paths = markshot::ocr_rapid::locateRapidModels();
        QCOMPARE(paths.detModel, directory.filePath(QStringLiteral("ch_PP-OCRv5_det_mobile.onnx")));
        cleanup();
    }
};

/**
 * 【OCR测试】【目录隔离】在 Qt 测试数据目录中检查模型发现
 * @param argc 参数数量
 * @param argv 参数列表
 * @return 测试退出码
 */
int main(int argc, char **argv)
{
    QStandardPaths::setTestModeEnabled(true);
    for (const auto *name : {"MARK_SHOT_OCR_MODEL_DIR", "MARK_SHOT_RAPID_DET_MODEL",
                            "MARK_SHOT_RAPID_REC_MODEL", "MARK_SHOT_RAPID_REC_DICT"}) {
        qunsetenv(name);
    }
    QCoreApplication app(argc, argv);
    RapidModelPathsTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "rapid_model_paths_test.moc"
