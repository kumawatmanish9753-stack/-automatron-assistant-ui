#pragma once

#include "ISpeechToText.h"
#include <QTimer>
#include <QString>

// ---------------------------------------------------------------------------
// MockSpeechToText
// ---------------------------------------------------------------------------
// Simulates speech-to-text for testing the UI without a real STT backend.
// Generates a mock transcript after a configurable delay.
class MockSpeechToText : public ISpeechToText
{
    Q_OBJECT

public:
    explicit MockSpeechToText(QObject *parent = nullptr);

    void start() override;
    void stop() override;
    void cancel() override;

private slots:
    void onTick();

private:
    void resetState();

    QTimer *m_timer = nullptr;
    int m_elapsedMs = 0;
    bool m_isRunning = false;
    bool m_cancelled = false;

    // Configuration
    static constexpr int kTotalDurationMs = 2500;  // 2.5 seconds of "listening"
    static constexpr int kPartialIntervalMs = 400; // Emit partials every 400ms
};