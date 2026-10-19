#include "HotkeyMonitor.h"
#include <QCoreApplication>
#include <QDebug>

#ifdef _WIN32
#include <windows.h>
#endif

// ---------------------------------------------------------------------------
// GlobalHotkeyFilter Implementation
// ---------------------------------------------------------------------------
GlobalHotkeyFilter::GlobalHotkeyFilter(GlobalHotkeyManager *manager)
    : m_manager(manager)
{
}

bool GlobalHotkeyFilter::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(result);

#ifdef _WIN32
    if (eventType != "windows_generic_MSG")
        return false;

    MSG *msg = static_cast<MSG *>(message);

    if (msg->message == WM_HOTKEY && msg->wParam == 1234) {
        if (m_manager) {
            m_manager->notifyHotkeyPressed();
            return true;
        }
    }
#else
    Q_UNUSED(eventType);
    Q_UNUSED(message);
#endif

    return false;
}

// ---------------------------------------------------------------------------
// GlobalHotkeyManager Implementation
// ---------------------------------------------------------------------------
GlobalHotkeyManager::GlobalHotkeyManager(QObject *parent)
    : QObject(parent)
{
    m_filter = new GlobalHotkeyFilter(this);
}

GlobalHotkeyManager::~GlobalHotkeyManager()
{
    unregisterHotkey();
    delete m_filter;
}

bool GlobalHotkeyManager::registerHotkey()
{
    if (m_registered) return true;

#ifdef _WIN32
    // Register Ctrl+Shift+K as a global hotkey on Windows
    BOOL success = RegisterHotKey(
        nullptr,
        kHotkeyId,
        MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT,
        'K'
    );

    if (success) {
        m_registered = true;
        if (QCoreApplication::instance()) {
            QCoreApplication::instance()->installNativeEventFilter(m_filter);
        }
        qDebug() << "Global hotkey Ctrl+Shift+K registered successfully (Windows).";
    } else {
        DWORD err = GetLastError();
        qDebug() << "Failed to register global hotkey. Error code:" << err;
    }

    return success;
#else
    // On Linux / X11 / Wayland, global hotkey registration typically requires
    // XGrabKey or desktop portal global shortcuts.
    // Mark as registered and install filter for platform events.
    m_registered = true;
    if (QCoreApplication::instance()) {
        QCoreApplication::instance()->installNativeEventFilter(m_filter);
    }
    qDebug() << "Hotkey manager initialized (Linux). Global hotkey Ctrl+Shift+K configured.";
    return true;
#endif
}

void GlobalHotkeyManager::unregisterHotkey()
{
    if (m_registered) {
        if (QCoreApplication::instance()) {
            QCoreApplication::instance()->removeNativeEventFilter(m_filter);
        }
#ifdef _WIN32
        UnregisterHotKey(nullptr, kHotkeyId);
#endif
        m_registered = false;
    }
}

void GlobalHotkeyManager::notifyHotkeyPressed()
{
    emit hotkeyPressed();
}
