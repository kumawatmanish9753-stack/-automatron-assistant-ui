#include "AgentWorker.h"
#include "MockSpeechToText.h"
#include "HarnessIPCClient.h"
#include <QDebug>
#include <QJsonObject>

AgentWorker::AgentWorker(QObject *parent)
    : QObject(parent)
{
    // Initialize audio manager
    m_audioManager = new AudioManager(this);
    m_audioManager->moveToThread(this->thread());
    connect(m_audioManager, &AudioManager::audioLevelChanged, this, &AgentWorker::onAudioLevelChanged);
    connect(m_audioManager, &AudioManager::captureStarted, this, [this]() {
        emit transcriptUpdated("System", "Microphone active");
    });
    connect(m_audioManager, &AudioManager::captureStopped, this, [this]() {
        if (!m_cancellationToken.has_value() || !m_cancellationToken->stop_requested())
        {
            emit transcriptUpdated("System", "Microphone stopped");
        }
    });
    connect(m_audioManager, &AudioManager::errorOccurred, this, &AgentWorker::errorOccurred);

    // Initialize STT (mock implementation for now)
    m_stt = new MockSpeechToText(this);
    m_stt->moveToThread(this->thread());
    connect(m_stt, &ISpeechToText::partialTranscript, this, &AgentWorker::onSTTPartialTranscript);
    connect(m_stt, &ISpeechToText::finalTranscript, this, &AgentWorker::onSTTFinalTranscript);
    connect(m_stt, &ISpeechToText::completed, this, &AgentWorker::onSTTCompleted);
    connect(m_stt, &ISpeechToText::error, this, &AgentWorker::errorOccurred);

    // Initialize LLM Client / Harness IPC Client
    HarnessIPCClient *harnessClient = new HarnessIPCClient(this);
    m_llmClient = harnessClient;
    m_llmClient->moveToThread(this->thread());
    connect(m_llmClient, &ILLMClient::responseChunk, this, &AgentWorker::onLLMResponseChunk);
    connect(m_llmClient, &ILLMClient::responseComplete, this, &AgentWorker::onLLMResponseComplete);
    connect(m_llmClient, &ILLMClient::cancelled, this, &AgentWorker::onLLMCancelled);
    connect(m_llmClient, &ILLMClient::error, this, &AgentWorker::onLLMError);

    connect(harnessClient, &HarnessIPCClient::harnessStateChanged, this, [this](int stateInt) {
        AgentState st = static_cast<AgentState>(stateInt);
        changeState(st);
    });
    connect(harnessClient, &HarnessIPCClient::transcriptMessage, this, &AgentWorker::transcriptUpdated);
    connect(harnessClient, &HarnessIPCClient::consentRequested, this, &AgentWorker::consentRequested);
    connect(harnessClient, &HarnessIPCClient::toolEvent, this, [this](
        const QString &kind, const QString &name, const QJsonObject &args,
        const QString &output, const QString &error, const QString &risk) {
        QString msg = QString("Tool [%1] %2 (%3)").arg(name, kind, risk);
        if (!output.isEmpty()) msg += " — " + output;
        if (!error.isEmpty()) msg += " — Error: " + error;
        emit transcriptUpdated("Harness Security Gate", msg);
    });

    // Initialize Tool Executor
    m_toolExecutor = new ToolExecutor(this);
    m_toolExecutor->moveToThread(this->thread());

    m_listeningTimer = new QTimer(this);
    m_listeningTimer->setSingleShot(true);
    connect(m_listeningTimer, &QTimer::timeout, this, &AgentWorker::simulateListeningEnd);
}

AgentWorker::~AgentWorker()
{
    // Ensure clean shutdown
    requestKill();
    if (m_audioManager)    m_audioManager->deleteLater();
    if (m_stt)             m_stt->deleteLater();
    if (m_llmClient)       m_llmClient->deleteLater();
    if (m_toolExecutor)    m_toolExecutor->deleteLater();
}

void AgentWorker::changeState(AgentState newState)
{
    if (m_currentState.exchange(newState) == newState)
        return;

    emit stateChanged(newState);
}

void AgentWorker::startListening()
{
    if (m_currentState != AgentState::Idle)
    {
        emit errorOccurred("Cannot start listening unless Idle.");
        return;
    }

    // Clear previous cancellation state (if any)
    m_cancellationToken.reset();

    changeState(AgentState::Listening);

    // Start audio capture
    m_audioManager->startCapture();

    // Start STT
    m_stt->start();
}

void AgentWorker::submitPrompt(const QString &text)
{
    if (m_currentState == AgentState::Terminated)
    {
        emit errorOccurred("Cannot accept input while terminated.");
        return;
    }

    // Clear previous cancellation state (if any)
    m_cancellationToken.reset();

    // Stop any ongoing mockup steps
    m_listeningTimer->stop();

    m_currentPrompt = text;
    m_accumulatedResponse.clear();

    changeState(AgentState::Thinking);

    // Send prompt to LLM client
    m_llmClient->sendPrompt(text);
}

void AgentWorker::executeTool(const QString &toolName, const QVariantMap &args)
{
    if (m_currentState == AgentState::Terminated) return;

    if (!m_toolExecutor->hasTool(toolName)) {
        emit transcriptUpdated("System", "Error: Tool not found - " + toolName);
        return;
    }

    changeState(AgentState::Executing);

    // Perform execution safely
    if (!m_cancellationToken.has_value())
        m_cancellationToken.emplace();

    ToolCall call;
    call.name = toolName;
    call.arguments = args;

    ToolResult result = m_toolExecutor->execute(call, *m_cancellationToken);

    if (result.cancelled) {
         emit transcriptUpdated("System", "Tool execution cancelled");
    } else if (result.success) {
         emit transcriptUpdated("Tool Output", result.output);
         changeState(AgentState::Idle);
    } else {
         emit transcriptUpdated("System", "Tool error: " + result.error);
         changeState(AgentState::Idle);
    }
}

void AgentWorker::requestKill()
{
    // 1. Atomically request cancellation
    if (!m_cancellationToken.has_value())
        m_cancellationToken.emplace();

    m_cancellationToken->request_stop();

    // 2. Immediately update the UI to Terminated
    changeState(AgentState::Terminated);

    // 3. Stop timers
    m_listeningTimer->stop();

    // 4. Stop microphone
    m_audioManager->stopCapture();

    // 5. Cancel STT
    if (m_stt) m_stt->cancel();

    // 6. Cancel active LLM request and send kill to harness
    if (m_llmClient) {
        if (auto *hClient = qobject_cast<HarnessIPCClient*>(m_llmClient)) {
            hClient->sendKill();
        }
        m_llmClient->cancel();
    }

    // 7. Flush buffers
    m_currentPrompt.clear();
    m_accumulatedResponse.clear();

    // 8. Emit terminated()
    emit terminated();
}

void AgentWorker::submitConsentResponse(
    const QString &requestId,
    bool approved,
    int scopeExpansion,
    const QString &passwordProvided,
    const QString &operatorNotes)
{
    if (m_llmClient) {
        if (auto *hClient = qobject_cast<HarnessIPCClient*>(m_llmClient)) {
            hClient->submitConsentResponse(requestId, approved, scopeExpansion, passwordProvided, operatorNotes);
        }
    }
}

void AgentWorker::resumeAgent()
{
    if (m_currentState == AgentState::Terminated)
    {
        // Clear cancellation state
        m_cancellationToken.reset();

        // Reset worker state
        m_currentPrompt.clear();
        m_accumulatedResponse.clear();

        // Stop any lingering timers
        m_listeningTimer->stop();

        // Reset to Idle
        changeState(AgentState::Idle);
    }
}

// ---------------------------------------------------------------------------
// Audio and STT Handlers
// ---------------------------------------------------------------------------

void AgentWorker::onAudioLevelChanged(float level)
{
    emit audioLevelChanged(level);
}

void AgentWorker::onSTTPartialTranscript(const QString &text)
{
    emit transcriptUpdated("STT (Partial)", text);
}

void AgentWorker::onSTTFinalTranscript(const QString &text)
{
    m_currentPrompt = text;
    emit transcriptUpdated("User", text);
    changeState(AgentState::Thinking);
    m_llmClient->sendPrompt(text);
}

void AgentWorker::onSTTCompleted()
{
    emit transcriptUpdated("System", "Speech recognition completed");
}

// ---------------------------------------------------------------------------
// LLM Handlers
// ---------------------------------------------------------------------------

void AgentWorker::onLLMResponseChunk(const QString &chunk)
{
    if (m_cancellationToken.has_value() && m_cancellationToken->stop_requested())
        return;

    if (m_currentState == AgentState::Thinking)
        changeState(AgentState::Executing);

    m_accumulatedResponse += chunk;
    emit responseChunk(chunk);
}

void AgentWorker::onLLMResponseComplete()
{
    if (m_cancellationToken.has_value() && m_cancellationToken->stop_requested())
        return;

    emit transcriptUpdated("Agent", m_accumulatedResponse);
    changeState(AgentState::Idle);
}

void AgentWorker::onLLMCancelled()
{
    emit transcriptUpdated("System", "LLM request cancelled");
}

void AgentWorker::onLLMError(const QString &message)
{
    emit errorOccurred(QString("LLM Error: %1").arg(message));
    changeState(AgentState::Idle);
}

// ---------------------------------------------------------------------------
// Mock Pipeline Implementation
// ---------------------------------------------------------------------------

void AgentWorker::simulateListeningEnd()
{
    if (m_cancellationToken.has_value() && m_cancellationToken->stop_requested())
        return;

    if (m_currentState != AgentState::Listening)
        return;

    changeState(AgentState::Thinking);
}
