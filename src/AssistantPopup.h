
#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QPainter>
#include <QVector>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "FloatingOrb.h"
#include <QFrame>
#include <QLineEdit>

// ---------------------------------------------------------------------------
// Modern Multi-Bar Lightweight Audio Visualizer
// ---------------------------------------------------------------------------
class AudioVisualizerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AudioVisualizerWidget(QWidget *parent = nullptr);
    void setActive(bool active);
    void setAudioLevel(float level);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onTick();

private:
    bool m_active = false;
    float m_audioLevel = 0.0f;
    QTimer m_animTimer;
    float m_phase = 0.0f;
};

// ---------------------------------------------------------------------------
// Custom TextEdit: Enter sends, Shift+Enter newlines, activates window on click
// ---------------------------------------------------------------------------
class ChatInputEdit : public QTextEdit
{
    Q_OBJECT

public:
    explicit ChatInputEdit(QWidget *parent = nullptr);

signals:
    void sendRequested();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
};

// ---------------------------------------------------------------------------
// AssistantPopup: Futuristic Glassmorphic AI Interface
// ---------------------------------------------------------------------------
class AssistantPopup : public QWidget
{
    Q_OBJECT

public:
    explicit AssistantPopup(QWidget *parent = nullptr);
    ~AssistantPopup() override = default;

    void updatePositionAboveOrb(const QPoint &orbGlobalPos, const QSize &orbSize);
    void updatePositionRelativeToOrb(const QPoint &orbGlobalPos, const QSize &orbSize, OrbAnchor anchor);
    void setState(OrbState state);
    void appendTranscriptMessage(const QString &sender, const QString &text);
    void appendStreamingChunk(const QString &chunk);
    void setStats(float cpuPercent, float ramMb);
    void setAudioLevel(float level);

    // Expansion & Collapse Morphing Animations
    void expandFrom(const QPoint &orbCenter, const QSize &orbSize);
    void expandFrom(const QPoint &orbCenter, const QSize &orbSize, OrbAnchor anchor);
    void collapseTo(const QPoint &orbCenter, const QSize &orbSize);
    void collapseTo(const QPoint &orbCenter, const QSize &orbSize, OrbAnchor anchor);

    bool isTerminated() const { return m_isTerminated; }
    void resetTermination();

signals:
    void promptSubmitted(const QString &text);
    void actionChipTriggered(const QString &action);
    void killRequested();
    void resumeRequested();
    void minimizeRequested();
    void expansionComplete();
    void collapseComplete();
    void anchorChanged(OrbAnchor newAnchor);
    void consentDecided(
        const QString &requestId,
        bool approved,
        int scopeExpansion,
        const QString &passwordProvided,
        const QString &operatorNotes
    );

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onSendClicked();
    void onKillClicked();
    void onChipClicked();
    void onMinimizeClicked();

public slots:
    void showConsentCard(
        const QString &requestId,
        const QString &path,
        const QString &operation,
        const QString &reason,
        bool isHighRisk,
        bool requiresPassword,
        int tier
    );

private:
    void setupUi();
    void setupActionChips(QVBoxLayout *parentLayout);

    // UI elements
    QLabel *m_titleLabel = nullptr;
    QLabel *m_stateBadgeLabel = nullptr;
    QLabel *m_cpuLabel = nullptr;
    QLabel *m_ramLabel = nullptr;
    AudioVisualizerWidget *m_visualizer = nullptr;

    QTextEdit *m_transcript = nullptr;
    ChatInputEdit *m_inputEdit = nullptr;
    QPushButton *m_sendButton = nullptr;
    QPushButton *m_killButton = nullptr;
    QPushButton *m_minimizeButton = nullptr;
    QWidget *m_chipsContainer = nullptr;
    QWidget *m_headerWidget = nullptr;

    // Glowing effect animation
    QTimer *m_glowAnimTimer = nullptr;
    float m_glowPhase = 0.0f;
    bool m_isGlowActive = false;

    bool m_isTerminated = false;
    bool m_isStreaming = false;
    OrbState m_currentState = OrbState::Idle;
    OrbAnchor m_anchor = OrbAnchor::TopRight;

    QTimer *m_proximityTimer = nullptr;
    QTimer *m_autoReturnTimer = nullptr;
    void checkProximity();
    void glideToAnchor(OrbAnchor anchor);

    QPoint m_dragPosition;
    bool m_isDragging = false;

    // Consent dialog card (inline in chat, not a popup)
    QFrame *m_consentCard = nullptr;
    QString m_consentRequestId;

    static constexpr int kPopupWidth = 440;
    static constexpr int kPopupMaxHeight = 580;
    static constexpr int kCornerRadius = 22;
    static constexpr int kSpacingAboveOrb = 12;
};