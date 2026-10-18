#include <QApplication>
#include <QThread>
#include <QDebug>
#include "FloatingOrb.h"
#include "AssistantPopup.h"
#include "AgentWorker.h"
#include "HotkeyMonitor.h"
#include "NotificationManager.h"
#include "WindowsIntegration.h"

// Helper function to map AgentState to OrbState
static OrbState toOrbState(AgentState state)
{
    switch (state) {
        case AgentState::Idle:       return OrbState::Idle;
        case AgentState::Listening:  return OrbState::Listening;
        case AgentState::Thinking:   return OrbState::Thinking;
        case AgentState::Executing:  return OrbState::Executing;
        case AgentState::Terminated: return OrbState::Terminated;
        default:                     return OrbState::Idle;
    }
}

int main(int argc, char *argv[])
{
    // Need to register AgentState so it can be passed across threads via queued signals
    qRegisterMetaType<AgentState>("AgentState");

    QApplication app(argc, argv);
    app.setApplicationName("Voice-AI");
    app.setOrganizationName("Ai-Voice");

    FloatingOrb orb;
    AssistantPopup popup;
    NotificationManager notificationManager;

    // --- Worker Architecture Setup ---
    QThread workerThread;
    AgentWorker* worker = new AgentWorker();
    worker->moveToThread(&workerThread);

    // Make sure thread shuts down cleanly
    QObject::connect(&app, &QApplication::aboutToQuit, [&workerThread]() {
        workerThread.quit();
        workerThread.wait();
    });
    QObject::connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);

    // Start background thread
    workerThread.start();

    // --- Register Global Hotkey (Ctrl+Shift+K) ---
    GlobalHotkeyManager hotkeyManager;
    hotkeyManager.registerHotkey();

    // Connect hotkey to toggle orb/popup or trigger kill
    QObject::connect(&hotkeyManager, &GlobalHotkeyManager::hotkeyPressed, [&orb, &popup, worker]() {
        if (popup.isVisible()) {
            // Update orb position to match popup before collapsing
            if (orb.anchor() == OrbAnchor::TopRight) {
                orb.move(popup.geometry().center().x() - orb.width() / 2, popup.geometry().top() - orb.height() / 2);
            } else {
                orb.move(popup.geometry().center().x() - orb.width() / 2, popup.geometry().bottom() - orb.height() / 2);
            }
            popup.collapseTo(orb.pos() + QPoint(orb.width() / 2, orb.height() / 2), orb.size(), orb.anchor());
        } else {
            popup.expandFrom(orb.pos() + QPoint(orb.width() / 2, orb.height() / 2), orb.size(), orb.anchor());
            popup.show();
            orb.hide(); // Hide orb only after expansion starts
            if (orb.state() == OrbState::Idle) {
                QMetaObject::invokeMethod(worker, "startListening", Qt::QueuedConnection);
            }
        }
    });

    // --- Connect Worker to UI (Queued Connections by default when cross-thread) ---

    // State changes update both Orb and Popup
    QObject::connect(worker, &AgentWorker::stateChanged, &orb, [&orb, &popup](AgentState state) {
        OrbState oState = toOrbState(state);
        orb.setState(oState);
        popup.setState(oState);
    });

    // Worker provides transcript updates to the UI
    QObject::connect(worker, &AgentWorker::transcriptUpdated, &popup, [&popup](const QString &sender, const QString &text) {
        popup.appendTranscriptMessage(sender, text);
    });

    // Worker streaming chunks (simulate live generation)
    QObject::connect(worker, &AgentWorker::responseChunk, &popup, [&popup](const QString &chunk) {
        popup.appendStreamingChunk(chunk);
    });

    // Audio level updates for the 3-bar visualizer
    QObject::connect(worker, &AgentWorker::audioLevelChanged, &popup, [&popup](float level) {
        popup.setAudioLevel(level);
    });

    // --- Connect UI to Worker (Cross-thread calls) ---

    // Toggle popup visibility on orb left-click
    QObject::connect(&orb, &FloatingOrb::clicked, [&orb, &popup, worker]() {
        if (popup.isVisible()) {
            if (orb.anchor() == OrbAnchor::TopRight) {
                orb.move(popup.geometry().center().x() - orb.width() / 2, popup.geometry().top() - orb.height() / 2);
            } else {
                orb.move(popup.geometry().center().x() - orb.width() / 2, popup.geometry().bottom() - orb.height() / 2);
            }
            popup.collapseTo(orb.pos() + QPoint(orb.width() / 2, orb.height() / 2), orb.size(), orb.anchor());
        } else {
            popup.expandFrom(orb.pos() + QPoint(orb.width() / 2, orb.height() / 2), orb.size(), orb.anchor());
            popup.show();
            orb.hide(); // Hide orb only after expansion starts
            if (orb.state() == OrbState::Idle) {
                QMetaObject::invokeMethod(worker, "startListening", Qt::QueuedConnection);
            }
        }
    });

    // Handle prompt submission
    QObject::connect(&popup, &AssistantPopup::promptSubmitted, worker, [worker](const QString &text) {
        QMetaObject::invokeMethod(worker, "submitPrompt", Qt::QueuedConnection, Q_ARG(QString, text));
    });

    // Handle Consent Requests (Harness -> Worker -> Popup)
    QObject::connect(worker, &AgentWorker::consentRequested, &popup,
        [&popup](const QString &requestId, const QString &path, const QString &operation,
                 const QString &reason, bool isHighRisk, bool requiresPassword, int tier) {
            popup.showConsentCard(requestId, path, operation, reason, isHighRisk, requiresPassword, tier);
        });

    // Handle Consent Decisions (Popup -> Worker -> Harness)
    QObject::connect(&popup, &AssistantPopup::consentDecided, worker,
        [worker](const QString &requestId, bool approved, int scope, const QString &pwd, const QString &notes) {
            QMetaObject::invokeMethod(worker, "submitConsentResponse", Qt::QueuedConnection,
                Q_ARG(QString, requestId),
                Q_ARG(bool, approved),
                Q_ARG(int, scope),
                Q_ARG(QString, pwd),
                Q_ARG(QString, notes));
        });

    // Handle quick-action chips
    QObject::connect(&popup, &AssistantPopup::actionChipTriggered, worker, [worker](const QString &action) {
        if (action == "Stop") {
            QMetaObject::invokeMethod(worker, "resumeAgent", Qt::QueuedConnection);
        } else {
            QMetaObject::invokeMethod(worker, "submitPrompt", Qt::QueuedConnection, Q_ARG(QString, action));
        }
    });

    // Handle Kill Switch / Abort - Terminate entire Voice-AI.exe process and notify harness
    QObject::connect(&popup, &AssistantPopup::killRequested, [worker, &app, &notificationManager]() {
        // Request worker to send kill signal to harness
        QMetaObject::invokeMethod(worker, "requestKill", Qt::QueuedConnection);

        // Show notification before terminating
        notificationManager.showNotification(
            "Voice-AI Terminated",
            "Application shutting down..."
        );

        // Terminate the entire application
        QApplication::quit();
    });

    // Handle Resume (signature takes no parameters)
    QObject::connect(&popup, &AssistantPopup::resumeRequested, worker, [worker]() {
        QMetaObject::invokeMethod(worker, "resumeAgent", Qt::QueuedConnection);
    });

    // Connect collapse completion
    QObject::connect(&popup, &AssistantPopup::collapseComplete, [&popup, &orb]() {
        popup.hide();
        orb.show(); // Show orb when window collapses
    });

    // Connect minimize request - collapses to orb
    QObject::connect(&popup, &AssistantPopup::minimizeRequested, [&orb, &popup]() {
        if (popup.isVisible()) {
            // Update orb position to match window's current position
            if (orb.anchor() == OrbAnchor::TopRight) {
                orb.move(popup.geometry().center().x() - orb.width() / 2, popup.geometry().top() - orb.height() / 2);
            } else {
                orb.move(popup.geometry().center().x() - orb.width() / 2, popup.geometry().bottom() - orb.height() / 2);
            }
            popup.collapseTo(orb.pos() + QPoint(orb.width() / 2, orb.height() / 2), orb.size(), orb.anchor());
        }
    });

    // When the orb moves due to corner switch, synchronize the popup if visible
    QObject::connect(&orb, &FloatingOrb::cornerChanged, [&orb, &popup]() {
        if (popup.isVisible()) {
            popup.updatePositionRelativeToOrb(orb.pos(), orb.size(), orb.anchor());
        }
    });

    // When the popup dodges or returns, synchronize the orb anchor
    QObject::connect(&popup, &AssistantPopup::anchorChanged, [&orb](OrbAnchor anchor) {
        orb.setAnchor(anchor);
    });

    // Handle kill complete - disable both orb and popup
    QObject::connect(worker, &AgentWorker::stateChanged, [&orb, &popup](AgentState state) {
        if (state == AgentState::Terminated) {
            // Disable orb by stopping animation (visual freeze)
            // Disable popup by showing "Terminated" state
            orb.update();
            popup.update();
        } else if (state == AgentState::Idle) {
            // Reset termination state when returning to idle
            popup.resetTermination();
        }
    });

    orb.show();

    int exitCode = app.exec();

    return exitCode;
}