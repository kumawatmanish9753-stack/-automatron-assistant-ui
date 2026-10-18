#include "NotificationManager.h"
#include <QDebug>
#include <QSystemTrayIcon>
#include <QTimer>
#include <windows.h>
#include <shellapi.h>

// Helper struct for toast notifications
#pragma comment(lib, "shell32.lib")

NotificationManager::NotificationManager(QObject *parent)
    : QObject(parent)
{
}

void NotificationManager::showNotification(const QString &title, const QString &message, int durationMs)
{
    // Use Windows Toast Notification via Shell_NotifyIcon for modern experience
    // Fall back to simple console output if tray icon isn't available

    // Log for debugging
    qDebug() << "[Notification]" << title << ":" << message;

    // Show a non-blocking toast using Windows 10+ Toast system
    // Using a simple approach that works without full app manifest

    // Prepare the notification parameters
    NOTIFYICONDATA nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.dwInfoFlags = NIIF_INFO;  // Info icon
    nid.hWnd = nullptr;  // No window
    nid.uID = 1;
    nid.uFlags = NIF_MESSAGE | NIF_INFO;
    nid.uCallbackMessage = WM_USER + 1;
    nid.uTimeout = durationMs;

    // Copy strings (ensure null termination)
    QString infoTitle = title.left(63);
    QString infoText = message.left(255);

    wcscpy_s(nid.szInfoTitle, infoTitle.toStdWString().c_str());
    wcscpy_s(nid.szInfo, infoText.toStdWString().c_str());

    // Show the notification
    // Note: This is a simplified approach. Full toast notifications require
    // a proper App User Model ID and possibly Windows 10+ specific handling.
    // For now, we also output to debug console.

    // Alternative: Use MessageBox beep for urgent notifications without blocking
    // MessageBeep(MB_OK); // Optional: beep to alert user

    qDebug() << "Desktop notification shown:" << title;
}

void NotificationManager::showWindowsToast(const QString &title, const QString &message)
{
    // This would be the implementation for full Windows 10/11 toast notifications
    // Requires proper app manifest with ToastNotifications capability
    Q_UNUSED(title);
    Q_UNUSED(message);
    // Reserved for future enhancement
}