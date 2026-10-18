#pragma once

#include "ILLMClient.h"
#include <QTimer>
#include <QString>
#include <QStringList>

// ---------------------------------------------------------------------------
// MockLLMClient
// ---------------------------------------------------------------------------
// Simulates streaming LLM responses for testing without a real backend.
// Generates mock response chunks at configurable intervals.
class MockLLMClient : public ILLMClient
{
    Q_OBJECT

public:
    explicit MockLLMClient(QObject *parent = nullptr);

    void sendPrompt(const QString &prompt) override;
    void cancel() override;

private slots:
    void onChunkTimer();

private:
    void resetState();

    QTimer *m_chunkTimer = nullptr;
    int m_currentChunkIndex = 0;
    bool m_cancelled = false;

    QString m_currentPrompt;
    QStringList m_responseChunks;

    // Configuration
    static constexpr int kChunkIntervalMs = 150;  // Emit chunks every 150ms
    static constexpr int kTotalChunks = 10;       // Total number of chunks to emit
};