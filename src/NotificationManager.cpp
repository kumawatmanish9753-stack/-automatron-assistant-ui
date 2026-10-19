#include "NotificationManager.h"
#include <QDebug>
#include <QSystemTrayIcon>
#include <QTimer>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")
#endif

NotificationManager::NotificationManager(QObject *parent)
    : QObject(parent)
{
}

void NotificationManager::showNotification(const QString &title, const QString &message, int durationMs)
{
    // Log for debugging
    qDebug() << "[Notification]" << title << ":" << message;

#ifdef _WIN32
    // Windows Toast Notification via Shell_NotifyIcon
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

    Shell_NotifyIcon(NIM_ADD, &nid);
#else
    // Linux / Cross-platform fallback: QSystemTrayIcon if available or system notification
    Q_UNUSED(durationMs);
#endif
}

void NotificationManager::showWindowsToast(const QString &title, const QString &message)
{
    // This would be the implementation for full Windows 10/11 toast notifications
    // Requires proper app manifest with ToastNotifications capability
    Q_UNUSED(title);
    Q_UNUSED(message);
    // Reserved for future enhancement
}