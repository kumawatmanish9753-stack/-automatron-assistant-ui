#include "MockSpeechToText.h"
#include <QDebug>

MockSpeechToText::MockSpeechToText(QObject *parent)
    : ISpeechToText(parent)
{
    m_timer = new QTimer(this);
    m_timer->setSingleShot(false);
    m_timer->setInterval(kPartialIntervalMs);
    connect(m_timer, &QTimer::timeout, this, &MockSpeechToText::onTick);
}

void MockSpeechToText::start()
{
    if (m_isRunning)
        return;

    resetState();
    m_isRunning = true;

    // Start the simulation timer
    m_timer->start();
}

void MockSpeechToText::stop()
{
    if (!m_isRunning)
        return;

    m_timer->stop();

    // Emit final mock transcript
    if (!m_cancelled)
    {
        emit finalTranscript("Mock transcription output");
        emit completed();
    }

    m_isRunning = false;
}

void MockSpeechToText::cancel()
{
    if (!m_isRunning)
        return;

    m_timer->stop();
    m_cancelled = true;

    // Discard any pending output
    m_isRunning = false;
}

void MockSpeechToText::onTick()
{
    if (!m_isRunning || m_cancelled)
        return;

    m_elapsedMs += kPartialIntervalMs;

    // Emit partial transcripts as if we're "recognizing" speech
    if (m_elapsedMs <= kTotalDurationMs)
    {
        QString partial;
        for (int i = 0; i < m_elapsedMs / kPartialIntervalMs; ++i)
        {
            partial += ".";
        }
        emit partialTranscript(QString("Listening%1").arg(partial));
    }
    else
    {
        // Time's up - finalize
        stop();
    }
}

void MockSpeechToText::resetState()
{
    m_elapsedMs = 0;
    m_cancelled = false;
}