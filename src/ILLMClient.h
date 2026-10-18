#pragma once

#include <QObject>
#include <QString>

// ---------------------------------------------------------------------------
// ILLMClient Interface
// ---------------------------------------------------------------------------
// Abstraction for Large Language Model services (local or cloud).
// All implementations must be non-blocking and communicate via signals.
// Supports streaming responses and cooperative cancellation.
class ILLMClient : public QObject
{
    Q_OBJECT

public:
    explicit ILLMClient(QObject *parent = nullptr) : QObject(parent) {}
    ~ILLMClient() override = default;

public slots:
    // Send a prompt to the LLM and start streaming the response
    virtual void sendPrompt(const QString &prompt) = 0;

    // Cancel the current request (if any)
    virtual void cancel() = 0;

signals:
    // Emitted when a chunk of the response is received
    // Chunks are meant to be concatenated for the full response
    void responseChunk(const QString &chunk);

    // Emitted when the response is complete
    void responseComplete();

    // Emitted if an error occurs
    void error(const QString &message);

    // Emitted when the request is cancelled
    void cancelled();
};