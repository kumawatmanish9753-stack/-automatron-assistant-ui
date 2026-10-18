#include "MockLLMClient.h"
#include <QDebug>

MockLLMClient::MockLLMClient(QObject *parent)
    : ILLMClient(parent)
{
    m_chunkTimer = new QTimer(this);
    m_chunkTimer->setSingleShot(false);
    m_chunkTimer->setInterval(kChunkIntervalMs);
    connect(m_chunkTimer, &QTimer::timeout, this, &MockLLMClient::onChunkTimer);

    // Pre-generate mock response chunks
    m_responseChunks = {
        "I understand ",
        "you're asking about the ",
        "Omniroute Assistant. ",
        "This is a mock LLM response ",
        "being streamed to test ",
        "the UI responsiveness ",
        "and cancellation logic. ",
        "Each chunk arrives independently, ",
        "allowing for a realistic streaming ",
        "experience without a real backend."
    };
}

void MockLLMClient::sendPrompt(const QString &prompt)
{
    if (m_chunkTimer->isActive())
    {
        emit error("LLM client already processing a request");
        return;
    }

    resetState();
    m_currentPrompt = prompt;

    // Start the streaming simulation
    m_chunkTimer->start();
}

void MockLLMClient::cancel()
{
    if (!m_chunkTimer->isActive())
        return;

    m_chunkTimer->stop();
    m_cancelled = true;

    // Discard pending chunks
    m_currentChunkIndex = 0;

    emit cancelled();
}

void MockLLMClient::onChunkTimer()
{
    if (m_cancelled)
    {
        m_chunkTimer->stop();
        return;
    }

    if (m_currentChunkIndex >= kTotalChunks)
    {
        m_chunkTimer->stop();
        emit responseComplete();
        return;
    }

    // Emit the next chunk
    if (m_currentChunkIndex < m_responseChunks.size())
    {
        const QString &chunk = m_responseChunks.at(m_currentChunkIndex);
        emit responseChunk(chunk);
    }

    m_currentChunkIndex++;
}

void MockLLMClient::resetState()
{
    m_currentChunkIndex = 0;
    m_cancelled = false;
}