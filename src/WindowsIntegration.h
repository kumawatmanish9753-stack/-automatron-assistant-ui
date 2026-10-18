#pragma once

#include <QObject>
#include <QString>
#include <memory>

// ---------------------------------------------------------------------------
// WindowsIntegration
// ---------------------------------------------------------------------------
// Platform-specific Windows features:
//   - Global hotkey registration
//   - Desktop notifications (toast)
//   - Power/battery status detection
//   - Multi-monitor awareness beyond Qt default
class WindowsIntegration : public QObject
{
    Q_OBJECT

public:
    explicit WindowsIntegration(QObject *parent = nullptr);
    ~WindowsIntegration() override;

    bool registerGlobalHotkey();
    void unregisterGlobalHotkey();

    void showDesktopNotification(const QString &title, const QString &message, int durationMs = 3000);

    bool isOnBatteryPower() const;

    // Terminate a process by name (e.g., "voice.exe") using Windows API
    static bool terminateProcessByName(const QString &processName);

signals:
    void globalHotkeyTriggered();
    void notificationActivated();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};