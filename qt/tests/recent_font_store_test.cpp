#include "ui/recent_font_store.h"

#include <QJsonArray>
#include <QtTest/QtTest>

/// @brief 最近使用字体状态测试。
class RecentFontStoreTest : public QObject
{
    Q_OBJECT

private slots:
    /// @brief 验证新字体会前置、去重并限制数量。
    void remembersFontAtFront()
    {
        const QStringList history = {
            QStringLiteral("Noto Sans"),
            QStringLiteral("Source Han Sans"),
            QStringLiteral("Noto Sans"),
        };

        const QStringList updated = markshot::ui::recentFontsWithRememberedFont(
            history, QStringLiteral("Source Han Sans"), 2);

        QCOMPARE(updated.size(), 2);
        QCOMPARE(updated.at(0), QStringLiteral("Source Han Sans"));
        QCOMPARE(updated.at(1), QStringLiteral("Noto Sans"));
    }

    /// @brief 验证空白名称不会清空已有记录。
    void keepsHistoryWhenFontIsBlank()
    {
        const QStringList history = {QStringLiteral("Noto Sans")};
        const QStringList updated =
            markshot::ui::recentFontsWithRememberedFont(history, QStringLiteral("  "));
        QCOMPARE(updated, history);
    }

    /// @brief 验证可以从应用配置根对象读取最近使用字体。
    void readsConfigRoot()
    {
        QJsonArray fonts;
        fonts.append(QStringLiteral("Noto Sans"));
        fonts.append(QStringLiteral("  "));
        fonts.append(QStringLiteral("DejaVu Sans"));
        fonts.append(42);

        QJsonObject text;
        text.insert(QStringLiteral("recentFonts"), fonts);

        QJsonObject root;
        root.insert(QStringLiteral("text"), text);

        const QStringList recent = markshot::ui::recentFontsFromConfigRoot(root);
        QCOMPARE(recent.size(), 2);
        QCOMPARE(recent.at(0), QStringLiteral("Noto Sans"));
        QCOMPARE(recent.at(1), QStringLiteral("DejaVu Sans"));
    }

    /// @brief 验证未安装的字体会从展示列表中去掉。
    void hidesMissingFamilies()
    {
        const QStringList history = {
            QStringLiteral("Missing Font"),
            QStringLiteral("Noto Sans"),
        };
        const QStringList available = {QStringLiteral("Noto Sans"), QStringLiteral("DejaVu Sans")};

        const QStringList visible = markshot::ui::visibleRecentFonts(history, available);
        QCOMPARE(visible, QStringList({QStringLiteral("Noto Sans")}));
    }
};

/// @brief 最近使用字体状态测试入口。
/// @param argc 参数数量。
/// @param argv 参数数组。
/// @return 进程退出码。
QTEST_APPLESS_MAIN(RecentFontStoreTest)

#include "recent_font_store_test.moc"
