#pragma once

#include <QObject>
#include <QString>

// ---------------------------------------------------------------------------
// NotificationManager
// ---------------------------------------------------------------------------
// Manages Windows desktop notifications using toast API or fallback mechanisms.
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