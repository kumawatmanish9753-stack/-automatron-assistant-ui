#include "AudioManager.h"
#include <QAudioFormat>
#include <QDebug>
#include <cmath>

AudioManager::AudioManager(QObject *parent)
    : QObject(parent)
{
    m_levelTimer = new QTimer(this);
    m_levelTimer->setInterval(kLevelUpdateIntervalMs);
    connect(m_levelTimer, &QTimer::timeout, this, &AudioManager::onLevelTimerTick);
}

AudioManager::~AudioManager()
{
    cleanup();
}

void AudioManager::startCapture()
{
    if (m_capturing.load(std::memory_order_acquire))
    {
        emit errorOccurred("Audio capture already in progress");
        return;
    }

    m_cancelled.store(false, std::memory_order_release);

    // Get default audio input device
    const QAudioDevice inputDevice = QMediaDevices::defaultAudioInput();
    if (inputDevice.isNull())
    {
        emit errorOccurred("No audio input device available");
        return;
    }

    // Configure audio format
    QAudioFormat format;
    format.setSampleRate(kSampleRate);
    format.setChannelCount(kChannelCount);
    format.setSampleFormat(QAudioFormat::Int16);

    // Check if format is supported
    if (!inputDevice.isFormatSupported(format))
    {
        emit errorOccurred("Audio format not supported by input device");
        return;
    }

    // Create audio source
    m_audioSource = new QAudioSource(inputDevice, format, this);
    m_audioIO = m_audioSource->start();

    if (!m_audioIO)
    {
        emit errorOccurred("Failed to start audio source");
        delete m_audioSource;
        m_audioSource = nullptr;
        return;
    }

    connect(m_audioIO, &QIODevice::readyRead, this, &AudioManager::onAudioReady);

    m_capturing.store(true, std::memory_order_release);
    m_levelTimer->start();

    emit captureStarted();
}

void AudioManager::stopCapture()
{
    if (!m_capturing.load(std::memory_order_acquire))
        return;

    m_levelTimer->stop();
    cleanup();

    m_capturing.store(false, std::memory_order_release);
    emit captureStopped();
}

void AudioManager::cancel()
{
    m_cancelled.store(true, std::memory_order_release);
    stopCapture();
}

void AudioManager::onAudioReady()
{
    if (!m_audioIO || !m_capturing.load(std::memory_order_acquire))
        return;

    // Read and discard audio data (we only need levels for now)
    // In a real implementation, this data would be sent to STT
    QByteArray data = m_audioIO->readAll();

    // Prevent buffer accumulation in mock mode
    // Real implementation would process this for STT
    Q_UNUSED(data);
}

void AudioManager::onLevelTimerTick()
{
    if (!m_capturing.load(std::memory_order_acquire) || !m_audioSource)
    {
        emit audioLevelChanged(0.0f);
        return;
    }

    // Get audio level from the source
    // Qt 6 QAudioSource doesn't have direct level access, so we compute from buffer
    // For mock purposes, we'll simulate a varying level
    // Real implementation would analyze the audio buffer

    static float mockPhase = 0.0f;
    mockPhase += 0.3f;

    // Simulated audio level (0.0 to 1.0) with some variation
    float level = 0.3f + 0.4f * std::abs(std::sin(mockPhase));
    level = std::clamp(level, 0.0f, 1.0f);

    emit audioLevelChanged(level);
}

void AudioManager::cleanup()
{
    if (m_audioSource)
    {
        m_audioSource->stop();
        m_audioSource->deleteLater();
        m_audioSource = nullptr;
    }

    // m_audioIO is owned by m_audioSource, no need to delete
    m_audioIO = nullptr;
}