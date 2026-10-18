#pragma once

#include <QObject>

// ---------------------------------------------------------------------------
// ISpeechToText Interface
// ---------------------------------------------------------------------------
// Abstraction for speech-to-text services (local or cloud).
// All implementations must be non-blocking and communicate via signals.
class ISpeechToText : public QObject
{
    Q_OBJECT

public:
    explicit ISpeechToText(QObject *parent = nullptr) : QObject(parent) {}
    ~ISpeechToText() override = default;

public slots:
    // Start listening and transcription
    virtual void start() = 0;

    // Stop and finalize current transcription
    virtual void stop() = 0;

    // Cancel and discard current transcription
    virtual void cancel() = 0;

signals:
    // Partial transcript (streaming)
    void partialTranscript(const QString &text);

    // Finalized transcript
    void finalTranscript(const QString &text);

    // Error occurred
    void error(const QString &message);

    // Transcription completed (stop() was called)
    void completed();
};