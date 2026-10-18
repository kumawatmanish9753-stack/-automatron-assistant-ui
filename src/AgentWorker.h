#pragma once

#include <QObject>
#include <QThread>
#include <QString>
#include <QTimer>
#include <atomic>
#include <optional>
#include "ISpeechToText.h"
#include "AudioManager.h"
#include "ILLMClient.h"
#include "ToolExecutor.h"

class HarnessIPCClient;

// ---------------------------------------------------------------------------
// Agent State Definition
// ---------------------------------------------------------------------------
enum class AgentState
{
    Idle,
    Listening,
    Thinking,
    Executing,
    Terminated
};

Q_DECLARE_METATYPE(AgentState)

// ---------------------------------------------------------------------------
// Lightweight cancellation token for C++20
// ---------------------------------------------------------------------------
class CancellationToken
{
public:
    bool stop_requested() const { return m_cancelled.load(std::memory_order_acquire); }
    void request_stop() { m_cancelled.store(true, std::memory_order_release); }
    void reset() { m_cancelled.store(false, std::memory_order_release); }

private:
    std::atomic_bool m_cancelled{false};
};

// ---------------------------------------------------------------------------
// Agent Worker
// ---------------------------------------------------------------------------
// Operates on a dedicated background thread. Contains the core state machine.
// All interaction must be via Signals and Slots to ensure thread safety.
class AgentWorker : public QObject
{
    Q_OBJECT

public:
    explicit AgentWorker(QObject *parent = nullptr);
    ~AgentWorker() override;

    CancellationToken& cancellationToken() { return *m_cancellationToken; }

public slots:
    void startListening();
    void submitPrompt(const QString &text);
    void requestKill();
    void resumeAgent();
    void executeTool(const QString &toolName, const QVariantMap &args);
    void submitConsentResponse(
        const QString &requestId,
        bool approved,
        int scopeExpansion,
        const QString &passwordProvided,
        const QString &operatorNotes
    );

signals:
    void stateChanged(AgentState newState);
    void responseChunk(const QString &chunk);
    void transcriptUpdated(const QString &sender, const QString &text);
    void audioLevelChanged(float level);
    void errorOccurred(const QString &errorMsg);
    void terminated();
    void consentRequested(
        const QString &requestId,
        const QString &path,
        const QString &operation,
        const QString &reason,
        bool isHighRisk,
        bool requiresPassword,
        int tier
    );

private slots:
    void changeState(AgentState newState);

    // Audio and STT handlers
    void onAudioLevelChanged(float level);
    void onSTTPartialTranscript(const QString &text);
    void onSTTFinalTranscript(const QString &text);
    void onSTTCompleted();

    // LLM handlers
    void onLLMResponseChunk(const QString &chunk);
    void onLLMResponseComplete();
    void onLLMCancelled();
    void onLLMError(const QString &message);

    // Mock simulation steps
    void simulateListeningEnd();

private:
    std::atomic<AgentState> m_currentState{AgentState::Idle};

    // Cooperative cancellation token
    std::optional<CancellationToken> m_cancellationToken;

    // Audio, STT, LLM, and Tool components
    AudioManager *m_audioManager = nullptr;
    ISpeechToText *m_stt = nullptr;
    ILLMClient *m_llmClient = nullptr;
    ToolExecutor *m_toolExecutor = nullptr;

    // Timers for the mock pipeline (must only be started/stopped on the worker thread)
    QTimer *m_listeningTimer = nullptr;

    QString m_currentPrompt;
    QString m_accumulatedResponse;
};