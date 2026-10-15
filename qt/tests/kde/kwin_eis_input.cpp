#include "kwin_eis_input.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusUnixFileDescriptor>
#include <QDir>
#include <QtTest>

#include <libei.h>
#include <linux/input-event-codes.h>
#include <stdexcept>
#include <unistd.h>

KWinEisInput::KWinEisInput()
{
    const QDir root(qEnvironmentVariable("MARK_SHOT_KDE_TEST_RUNTIME"));
    if (!root.dirName().startsWith(QStringLiteral("mark-shot-kde-"))
        || qEnvironmentVariable("XDG_RUNTIME_DIR") != root.filePath(QStringLiteral("runtime"))) {
        throw std::runtime_error("Input check requires an isolated KWin runtime");
    }

    // 1. 【KDE测试】【鼠标设备】只连接测试会话的私有 EIS 接口
    QDBusInterface remote(QStringLiteral("org.kde.KWin"), QStringLiteral("/org/kde/KWin/EIS/RemoteDesktop"),
                          QStringLiteral("org.kde.KWin.EIS.RemoteDesktop"), QDBusConnection::sessionBus());
    const auto reply = remote.call(QStringLiteral("connectToEIS"), 2);
    if (reply.type() == QDBusMessage::ErrorMessage || reply.arguments().isEmpty()) {
        throw std::runtime_error(reply.errorMessage().toStdString());
    }
    const auto descriptor = reply.arguments().first().value<QDBusUnixFileDescriptor>();
    if (!descriptor.isValid()) {
        throw std::runtime_error("No EIS socket returned");
    }
    try {
        m_context = ei_new_sender(nullptr);
        ei_configure_name(m_context, "Mark Shot isolated pointer checks");
        if (ei_setup_backend_fd(m_context, dup(descriptor.fileDescriptor())) != 0) {
            throw std::runtime_error("Cannot initialize EIS sender");
        }
        for (int i = 0; i < 40 && (!m_pointer || !m_buttons); ++i) {
            pump(50);
        }
        if (!m_pointer || !m_buttons) {
            throw std::runtime_error("KWin EIS pointer did not become ready");
        }
    } catch (...) {
        release();
        throw;
    }
}

KWinEisInput::~KWinEisInput()
{
    release();
}

void KWinEisInput::release()
{
    if (m_pointer) {
        ei_device_unref(m_pointer);
        m_pointer = nullptr;
    }
    if (m_buttons) {
        ei_device_unref(m_buttons);
        m_buttons = nullptr;
    }
    if (m_context) {
        ei_unref(m_context);
        m_context = nullptr;
    }
}

void KWinEisInput::pump(int milliseconds)
{
    QElapsedTimer timer;
    timer.start();
    do {
        ei_dispatch(m_context);
        while (auto *event = ei_get_event(m_context)) {
            const auto type = ei_event_get_type(event);
            if (type == EI_EVENT_SEAT_ADDED) {
                ei_seat_bind_capabilities(ei_event_get_seat(event), EI_DEVICE_CAP_POINTER_ABSOLUTE,
                                           EI_DEVICE_CAP_BUTTON, nullptr);
            } else if (type == EI_EVENT_DEVICE_RESUMED) {
                auto *device = ei_event_get_device(event);
                ei_device_start_emulating(device, 1);
                if (!m_pointer && ei_device_has_capability(device, EI_DEVICE_CAP_POINTER_ABSOLUTE)) {
                    m_pointer = ei_device_ref(device);
                }
                if (!m_buttons && ei_device_has_capability(device, EI_DEVICE_CAP_BUTTON)) {
                    m_buttons = ei_device_ref(device);
                }
            }
            ei_event_unref(event);
        }
        QTest::qWait(10);
    } while (timer.elapsed() < milliseconds);
}

void KWinEisInput::move(QPointF position)
{
    ei_device_pointer_motion_absolute(m_pointer, position.x(), position.y());
    ei_device_frame(m_pointer, ei_now(m_context));
    pump(100);
}

void KWinEisInput::left(bool down)
{
    ei_device_button_button(m_buttons, BTN_LEFT, down);
    ei_device_frame(m_buttons, ei_now(m_context));
    pump(100);
}

void KWinEisInput::drag(QPointF from, QPointF to)
{
    move(from);
    left(true);
    move(from + (to - from) / 2);
    move(to);
    left(false);
}
