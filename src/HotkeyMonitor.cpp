#include "HotkeyMonitor.h"
#include <QCoreApplication>
#include <QDebug>
#include <windows.h>

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

    if (eventType != "windows_generic_MSG")
        return false;

    MSG *msg = static_cast<MSG *>(message);

    if (msg->message == WM_HOTKEY && msg->wParam == 1234) {
        if (m_manager) {
            m_manager->notifyHotkeyPressed();
            return true;
        }
    }

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

    // Register Ctrl+Shift+K as a global hotkey
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
        qDebug() << "Global hotkey Ctrl+Shift+K registered successfully.";
    } else {
        DWORD err = GetLastError();
        qDebug() << "Failed to register global hotkey. Error code:" << err;
    }

    return success;
}

void GlobalHotkeyManager::unregisterHotkey()
{
    if (m_registered) {
        if (QCoreApplication::instance()) {
            QCoreApplication::instance()->removeNativeEventFilter(m_filter);
        }
        UnregisterHotKey(nullptr, kHotkeyId);
        m_registered = false;
    }
}

void GlobalHotkeyManager::notifyHotkeyPressed()
{
    emit hotkeyPressed();
}
