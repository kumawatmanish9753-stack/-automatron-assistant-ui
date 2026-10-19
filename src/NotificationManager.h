#pragma once

#include <QObject>
#include <QString>

// ---------------------------------------------------------------------------
// NotificationManager
// ---------------------------------------------------------------------------
// Manages desktop notifications using platform-appropriate mechanisms.
// On Windows: Shell_NotifyIcon (system tray balloon / toast).
// On Linux: falls back to QSystemTrayIcon or DBus.
// Designed to be non-blocking.
class NotificationManager : public QObject
{
    Q_OBJECT

public:
    explicit NotificationManager(QObject *parent = nullptr);
    ~NotificationManager() override = default;

    void showNotification(const QString &title, const QString &message, int durationMs = 3000);

signals:
    void notificationActivated();

private:
    // Internal helper for Windows toast
    void showWindowsToast(const QString &title, const QString &message);
};