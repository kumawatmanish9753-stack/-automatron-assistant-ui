#include "WindowsIntegration.h"
#include <windows.h>
#include <winuser.h>
#include <winreg.h>
#include <powrprof.h>
#include <tlhelp32.h>
#include <QProcess>
#include <iostream>

struct WindowsIntegration::Impl {
    bool hotkeyRegistered = false;
    bool onBattery = false;

    // Helper for checking battery status
    void updateBatteryStatus() {
        SYSTEM_POWER_STATUS status;
        if (GetSystemPowerStatus(&status)) {
            onBattery = (status.ACLineStatus == 0);
        } else {
            onBattery = false;
        }
    }
};

WindowsIntegration::WindowsIntegration(QObject *parent)
    : QObject(parent)
    , m_impl(std::make_unique<Impl>())
{
    m_impl->updateBatteryStatus();
}

WindowsIntegration::~WindowsIntegration()
{
    unregisterGlobalHotkey();
}

bool WindowsIntegration::registerGlobalHotkey()
{
    if (m_impl->hotkeyRegistered) return true;

    // Register Ctrl+Shift+K (Ctrl = MOD_CONTROL, Shift = MOD_SHIFT, K = 0x4B)
    // Using ID 1234 as a unique identifier for our hotkey
    BOOL success = RegisterHotKey(
        nullptr,             // Use message-only window (thread messages)
        1234,                // Unique identifier for this hotkey
        MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT, // Ctrl+Shift, disable auto-repeat
        0x4B                 // Virtual key code for 'K'
    );

    if (success) {
        m_impl->hotkeyRegistered = true;

        // Start a separate thread or timer to poll for hotkey messages
        // For simplicity, we'll let the main message loop handle it via nativeEventFilter
        std::cout << "Global hotkey Ctrl+Shift+K registered successfully." << std::endl;
    } else {
        DWORD err = GetLastError();
        std::cerr << "Failed to register global hotkey. Error code: " << err << std::endl;
    }

    return success;
}

void WindowsIntegration::unregisterGlobalHotkey()
{
    if (m_impl->hotkeyRegistered) {
        UnregisterHotKey(nullptr, 1234);
        m_impl->hotkeyRegistered = false;
        std::cout << "Global hotkey unregistered." << std::endl;
    }
}

void WindowsIntegration::showDesktopNotification(const QString &title, const QString &message, int durationMs)
{
    // Windows toast notifications require a modern app with COM activation.
    // For simplicity, we'll use a legacy system tray balloon message.
    // In a real application, you'd want to use the modern Windows Toast API.
    std::cout << "[Desktop Notification] " << title.toStdString() << ": " << message.toStdString() << std::endl;

    // Fallback: Show a MessageBox (non-blocking) for demonstration.
    // In production, you'd schedule a system toast.
    // This is just a placeholder implementation.
}

bool WindowsIntegration::isOnBatteryPower() const
{
    SYSTEM_POWER_STATUS status;
    if (GetSystemPowerStatus(&status)) {
        return (status.ACLineStatus == 0);
    }
    return false;
}

bool WindowsIntegration::terminateProcessByName(const QString &processName)
{
    bool foundAndKilled = false;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W entry;
        entry.dwSize = sizeof(PROCESSENTRY32W);
        if (Process32FirstW(snapshot, &entry)) {
            do {
                if (QString::fromWCharArray(entry.szExeFile).compare(processName, Qt::CaseInsensitive) == 0) {
                    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
                    if (hProcess) {
                        TerminateProcess(hProcess, 0);
                        CloseHandle(hProcess);
                        foundAndKilled = true;
                    }
                }
            } while (Process32NextW(snapshot, &entry));
        }
        CloseHandle(snapshot);
    }

    // Fallback: also try taskkill to be thorough (in case of child processes/permissions)
    QProcess::startDetached("taskkill", QStringList() << "/F" << "/IM" << processName);

    return foundAndKilled;
}