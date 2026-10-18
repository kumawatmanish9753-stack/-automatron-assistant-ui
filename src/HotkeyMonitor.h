#pragma once

#include <QAbstractNativeEventFilter>
#include <QObject>

// ---------------------------------------------------------------------------
// GlobalHotkeyFilter
// ---------------------------------------------------------------------------
// Pure native event filter (no Q_OBJECT to avoid multiple inheritance issues).
// Signals are delegated to a separate QObject if needed.
class GlobalHotkeyFilter : public QAbstractNativeEventFilter
{
public:
    explicit GlobalHotkeyFilter(class GlobalHotkeyManager *manager);
    ~GlobalHotkeyFilter() override = default;

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

private:
    class GlobalHotkeyManager *m_manager = nullptr;
};

// ---------------------------------------------------------------------------
// GlobalHotkeyManager
// ---------------------------------------------------------------------------
// Manages hotkey registration and emits Qt signals.
class GlobalHotkeyManager : public QObject
{
    Q_OBJECT

public:
    explicit GlobalHotkeyManager(QObject *parent = nullptr);
    ~GlobalHotkeyManager() override;

    bool registerHotkey();
    void unregisterHotkey();

    void notifyHotkeyPressed();

signals:
    void hotkeyPressed();

private:
    bool m_registered = false;
    GlobalHotkeyFilter *m_filter = nullptr;
    static constexpr int kHotkeyId = 1234;
};
