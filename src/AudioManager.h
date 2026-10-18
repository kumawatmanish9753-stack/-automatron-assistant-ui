#pragma once

#include <QObject>
#include <QAudioSource>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QByteArray>
#include <QTimer>
#include <atomic>

// ---------------------------------------------------------------------------
// AudioManager
// ---------------------------------------------------------------------------
// Manages microphone lifecycle, audio capture, level reporting, and cancellation.
// All operations are non-blocking and designed to run on a background thread.
class AudioManager : public QObject
{
    Q_OBJECT

public:
    explicit AudioManager(QObject *parent = nullptr);
    ~AudioManager() override;

    // Start capturing audio from the default microphone
    void startCapture();

    // Stop capturing and flush buffers
    void stopCapture();

    // Cancel immediately (for kill switch)
    void cancel();

    // Check if currently capturing
    bool isCapturing() const { return m_capturing.load(std::memory_order_acquire); }

signals:
    // Emitted periodically with current audio level (0.0 to 1.0)
    void audioLevelChanged(float level);

    // Emitted when capture starts
    void captureStarted();

    // Emitted when capture stops normally
    void captureStopped();

    // Emitted on error
    void errorOccurred(const QString &message);

private slots:
    void onAudioReady();
    void onLevelTimerTick();

private:
    void cleanup();

    QAudioSource *m_audioSource = nullptr;
    QIODevice *m_audioIO = nullptr;
    QTimer *m_levelTimer = nullptr;

    std::atomic_bool m_capturing{false};
    std::atomic_bool m_cancelled{false};

    // Audio format constants
    static constexpr int kSampleRate = 16000;
    static constexpr int kChannelCount = 1;
    static constexpr int kSampleSize = 16;
    static constexpr int kLevelUpdateIntervalMs = 50; // 20 FPS for level updates
};