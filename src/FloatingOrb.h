#pragma once

#include <QWidget>
#include <QPoint>
#include <QColor>
#include <QTimer>
#include <QRect>

enum class OrbState {
    Idle,
    Listening,
    Thinking,
    Executing,
    Terminated
};

enum class OrbAnchor {
    TopRight,
    BottomRight
};

class FloatingOrb : public QWidget
{
    Q_OBJECT

public:
    explicit FloatingOrb(QWidget *parent = nullptr);
    ~FloatingOrb() override = default;

    void setState(OrbState state);
    OrbState state() const { return m_state; }

    OrbAnchor anchor() const { return m_anchor; }
    void setAnchor(OrbAnchor anchor);

    // Fade animation
    void fadeOut();
    void fadeIn();

signals:
    void clicked();
    void cornerChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;

private:
    OrbState m_state = OrbState::Idle;
    OrbAnchor m_anchor = OrbAnchor::TopRight;
    QTimer *m_animTimer = nullptr;
    QTimer *m_proximityTimer = nullptr;
    float m_pulsePhase = 0.0f;
    float m_rotationAngle = 0.0f;
    float m_glowPhase = 0.0f;  // Independent glow pulse for enhanced effect

    void checkProximity();
    void animateToAnchor(OrbAnchor anchor);

    QPoint m_dragPosition;
    bool m_isDragging = false;

    QColor stateColor() const;
};