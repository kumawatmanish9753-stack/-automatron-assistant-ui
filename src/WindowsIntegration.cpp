#include "WindowsIntegration.h"
#include <QProcess>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <winuser.h>
#include <winreg.h>
#include <powrprof.h>
#include <tlhelp32.h>
#else
#include <QDir>
#include <QFile>
#include <QTextStream>
#endif

struct WindowsIntegration::Impl {
    bool hotkeyRegistered = false;
    bool onBattery = false;

    // Helper for checking battery status
    void updateBatteryStatus() {
#ifdef _WIN32
        SYSTEM_POWER_STATUS status;
        if (GetSystemPowerStatus(&status)) {
            onBattery = (status.ACLineStatus == 0);
        } else {
            onBattery = false;
        }
#else
        // Linux battery status check via /sys/class/power_supply/
        onBattery = false;
        QDir powerDir("/sys/class/power_supply");
        QStringList entries = powerDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &entry : entries) {
            if (entry.startsWith("BAT")) {
                QFile typeFile(powerDir.filePath(entry + "/status"));
                if (typeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream in(&typeFile);
                    QString statusStr = in.readLine().trimmed();
                    if (statusStr.compare("Discharging", Qt::CaseInsensitive) == 0) {
                        onBattery = true;
                        break;
                    }
                }
            }
        }
#endif
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

#ifdef _WIN32
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
        std::cout << "Global hotkey Ctrl+Shift+K registered successfully." << std::endl;
    } else {
        DWORD err = GetLastError();
        std::cerr << "Failed to register global hotkey. Error code: " << err << std::endl;
    }

    return success;
#else
    // Linux stub — global hotkeys handled by HotkeyMonitor via nativeEventFilter
    m_impl->hotkeyRegistered = true;
    std::cout << "Global hotkey registered (Linux stub)." << std::endl;
    return true;
#endif
}

void WindowsIntegration::unregisterGlobalHotkey()
{
    if (m_impl->hotkeyRegistered) {
#ifdef _WIN32
        UnregisterHotKey(nullptr, 1234);
#endif
        m_impl->hotkeyRegistered = false;
        std::cout << "Global hotkey unregistered." << std::endl;
    }
}

void WindowsIntegration::showDesktopNotification(const QString &title, const QString &message, int durationMs)
{
    Q_UNUSED(durationMs);
    std::cout << "[Desktop Notification] " << title.toStdString() << ": " << message.toStdString() << std::endl;
}

bool WindowsIntegration::isOnBatteryPower() const
{
#ifdef _WIN32
    SYSTEM_POWER_STATUS status;
    if (GetSystemPowerStatus(&status)) {
        return (status.ACLineStatus == 0);
    }
    return false;
#else
    return m_impl->onBattery;
#endif
}

bool WindowsIntegration::terminateProcessByName(const QString &processName)
{
    bool foundAndKilled = false;

#ifdef _WIN32
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

    // Fallback: taskkill to handle child processes / permission edge cases
    QProcess::startDetached("taskkill", QStringList() << "/F" << "/IM" << processName);
#else
    // Linux / POSIX: strip .exe suffix if present, then use killall
    QString cleanName = processName;
    if (cleanName.endsWith(".exe", Qt::CaseInsensitive)) {
        cleanName.chop(4);
    }
    int exitCode = QProcess::execute("killall", QStringList() << cleanName);
    foundAndKilled = (exitCode == 0);
#endif

    return foundAndKilled;
}
