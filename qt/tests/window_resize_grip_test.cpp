#include "ui/window_resize_grip.h"

#include <QMouseEvent>
#include <QtTest/QtTest>

namespace {

/// @brief 向尺寸控件发送保持左键按下的移动事件，允许指针移出控件范围
/// @param grip 接收事件的尺寸控件
/// @param globalPosition 指针全局位置
/// @return 无返回值
void movePointer(QWidget *grip, const QPoint &globalPosition)
{
    const QPoint localPosition = grip->mapFromGlobal(globalPosition);
    QMouseEvent event(QEvent::MouseMove, QPointF(localPosition), QPointF(globalPosition),
                      Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(grip, &event);
}

}

class WindowResizeGripTest final : public QObject {
    Q_OBJECT

private slots:
    /// @brief 验证应用内调整尺寸显式捕获指针，释放左键后归还捕获
    /// @return 无返回值
    void capturesPointerUntilRelease()
    {
        // 1. 创建采用应用内尺寸调整的可见窗口
        QWidget window;
        window.resize(320, 200);
        markshot::ui::WindowResizeGrip grip(&window, [] { return true; });
        grip.setGeometry(290, 170, 20, 20);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        // 2. 按下后验证捕获归属，释放后确认捕获结束
        QTest::mousePress(&grip, Qt::LeftButton);
        QCOMPARE(QWidget::mouseGrabber(), &grip);
        QTest::mouseRelease(&grip, Qt::LeftButton);
        QVERIFY(QWidget::mouseGrabber() != &grip);
    }

    /// @brief 验证浮层等待平台配置时仍能调整尺寸，避免标准控件提前返回
    /// @return 无返回值
    void resizesWhileConfigureIsPending()
    {
        // 1. 创建可见窗口并模拟平台仍在等待配置的状态
        QWidget window;
        window.resize(320, 200);
        markshot::ui::WindowResizeGrip grip(&window, [] { return true; });
        grip.setGeometry(290, 170, 20, 20);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        window.setAttribute(Qt::WA_WState_ConfigPending, true);
        // 2. 将指针移出控件并验证窗口仍能调整尺寸
        const QPoint start = grip.mapToGlobal(grip.rect().center());
        QTest::mousePress(&grip, Qt::LeftButton, Qt::NoModifier, grip.rect().center());
        movePointer(&grip, start + QPoint(80, -20));
        QCOMPARE(window.size(), QSize(400, 180));
        // 3. 结束拖动并恢复平台配置状态
        QTest::mouseRelease(&grip, Qt::LeftButton);
        window.setAttribute(Qt::WA_WState_ConfigPending, false);
    }

    /// @brief 验证尺寸约束生效且结束拖动后不会继续改变窗口
    /// @return 无返回值
    void respectsSizeLimitsAndRelease()
    {
        // 1. 为窗口设置明确的最小和最大尺寸
        QWidget window;
        window.setMinimumSize(200, 140);
        window.setMaximumSize(450, 300);
        window.resize(320, 200);
        markshot::ui::WindowResizeGrip grip(&window, [] { return true; });
        grip.setGeometry(290, 170, 20, 20);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        // 2. 向两个方向越界拖动，验证尺寸约束
        const QPoint start = grip.mapToGlobal(grip.rect().center());
        QTest::mousePress(&grip, Qt::LeftButton, Qt::NoModifier, grip.rect().center());
        movePointer(&grip, start + QPoint(500, 500));
        QCOMPARE(window.size(), QSize(450, 300));
        movePointer(&grip, start - QPoint(500, 500));
        QCOMPARE(window.size(), QSize(200, 140));
        // 3. 释放后继续发送移动事件，窗口尺寸应保持不变
        QTest::mouseRelease(&grip, Qt::LeftButton);
        movePointer(&grip, start);
        QCOMPARE(window.size(), QSize(200, 140));
    }

    /// @brief 验证隐藏窗口会终止捕获，重新显示后需要再次按下才能调整尺寸
    /// @return 无返回值
    void cancelsWhenHidden()
    {
        // 1. 创建可见窗口并开始拖动
        QWidget window;
        window.resize(320, 200);
        markshot::ui::WindowResizeGrip grip(&window, [] { return true; });
        grip.setGeometry(290, 170, 20, 20);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        const QPoint start = grip.mapToGlobal(grip.rect().center());
        QTest::mousePress(&grip, Qt::LeftButton);
        // 2. 隐藏窗口后确认鼠标捕获已经释放
        window.hide();
        QVERIFY(QWidget::mouseGrabber() != &grip);
        // 3. 重新显示并移动指针，验证旧拖动状态不会恢复
        window.show();
        movePointer(&grip, start + QPoint(80, 20));
        QCOMPARE(window.size(), QSize(320, 200));
    }
};

QTEST_MAIN(WindowResizeGripTest)

#include "window_resize_grip_test.moc"
