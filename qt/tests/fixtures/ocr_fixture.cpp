#include <QCoreApplication>
#include <QFileInfo>
#include <QTextStream>

/**
 * 【OCR测试】【固定结果】通过控制台进程输出可预测的识别结果
 * @param argc 参数数量
 * @param argv 参数列表，依次包含结果模式和图像文件路径
 * @return 参数或文件无效时返回非零状态
 */
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const QStringList args = app.arguments();
    if (args.size() != 3 || !QFileInfo::exists(args.at(2))) {
        QTextStream(stderr) << "Expected OCR mode and an existing image path\n";
        return 1;
    }
    if (args.at(1) == QStringLiteral("--ocr-empty")) {
        QTextStream(stdout) << "[]";
        return 0;
    }
    if (args.at(1) != QStringLiteral("--ocr-fixture")) {
        return 2;
    }
    QTextStream(stdout) << R"({"tokens":[{"text":"Hello","box":[40,40,60,40],"line":0,"index":0},{"text":"world","box":[120,40,60,40],"line":0,"index":1}]})";
    return 0;
}
