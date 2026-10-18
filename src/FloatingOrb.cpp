#include "FloatingOrb.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QScreen>
#include <QGuiApplication>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QGraphicsDropShadowEffect>
#include <QCursor>
#include <cmath>

FloatingOrb::FloatingOrb(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    // 80x80 allows space for multi-layer quantum rings with enhanced glow
    setFixedSize(80, 80);

    m_animTimer = new QTimer(this);
    connect(m_animTimer, &QTimer::timeout, this, [this]() {
        m_pulsePhase += 0.10f;
        m_rotationAngle += 2.5f; // Outer ring rotation
        if (m_rotationAngle >= 360.0f) m_rotationAngle -= 360.0f;
        m_glowPhase += 0.08f; // Independent glow pulse
        if (m_glowPhase >= 6.28f) m_glowPhase -= 6.28f;
        update();
    });
    // Start timer only when state changes to active (not Idle)
    // Idle state = zero CPU usage from animation

    // Initial positioning in the top-right work area
    if (auto screen = QGuiApplication::primaryScreen()) {
        QRect workArea = screen->availableGeometry();
        move(workArea.right() - width() - 24, workArea.top() + 24);
    }

    m_proximityTimer = new QTimer(this);
    connect(m_proximityTimer, &QTimer::timeout, this, &FloatingOrb::checkProximity);
    // Don't start proximity timer - let popup handle dodge behavior
    // m_proximityTimer->start(50); // 20Hz polling
}

void FloatingOrb::checkProximity()
{
    if (!isVisible() || m_anchor != OrbAnchor::TopRight) return;

    QPoint cursor = QCursor::pos();
    QPoint orbCenter = geometry().center();

    // Within 120px radius from orb center
    if (std::hypot(cursor.x() - orbCenter.x(), cursor.y() - orbCenter.y()) < 120.0) {
        animateToAnchor(OrbAnchor::BottomRight);
    }
}

void FloatingOrb::animateToAnchor(OrbAnchor anchor)
{
    if (m_anchor == anchor) return;

    m_anchor = anchor;
    m_proximityTimer->stop(); // Stop polling once moved

    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) return;
    QRect workArea = screen->availableGeometry();

    QPoint target;
    if (anchor == OrbAnchor::TopRight) {
        target = QPoint(workArea.right() - width() - 24, workArea.top() + 24);
    } else {
        target = QPoint(workArea.right() - width() - 24, workArea.bottom() - height() - 24);
    }

    QPropertyAnimation *anim = new QPropertyAnimation(this, "pos");
    anim->setDuration(400);
    anim->setStartValue(pos());
    anim->setEndValue(target);
    anim->setEasingCurve(QEasingCurve::InOutCubic);
    connect(anim, &QPropertyAnimation::finished, this, &FloatingOrb::cornerChanged);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void FloatingOrb::setAnchor(OrbAnchor anchor)
{
    if (m_anchor == anchor) return;
    m_anchor = anchor;
    // Immediate move for non-proximity shifts
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) return;
    QRect workArea = screen->availableGeometry();
    if (anchor == OrbAnchor::TopRight) {
        move(workArea.right() - width() - 24, workArea.top() + 24);
    } else {
        move(workArea.right() - width() - 24, workArea.bottom() - height() - 24);
    }
    emit cornerChanged();
}

void FloatingOrb::setState(OrbState state)
{
    m_state = state;
    raise(); // Ensure Orb stays on top

    // Stop animation when terminated to visually "freeze" the orb
    if (state == OrbState::Terminated) {
        m_animTimer->stop();
    } else if (!m_animTimer->isActive() && state != OrbState::Idle) {
        // Restart animation for active states
        m_animTimer->start(33);
    }

    update();
}

QColor FloatingOrb::stateColor() const
{
    switch (m_state) {
        case OrbState::Listening:  return QColor(56, 189, 248);  // Electric Cyan
        case OrbState::Thinking:   return QColor(245, 158, 11);  // Solar Amber
        case OrbState::Executing:  return QColor(168, 85, 247);  // Cyber Violet
        case OrbState::Terminated: return QColor(239, 68, 68);   // Alert Red
        case OrbState::Idle:
        default:                   return QColor(124, 58, 237);  // AI Purple (idle glow)
    }
}

void FloatingOrb::fadeOut()
{
    QPropertyAnimation *fadeAnim = new QPropertyAnimation(this, "windowOpacity");
    fadeAnim->setDuration(250);
    fadeAnim->setStartValue(1.0);
    fadeAnim->setEndValue(0.0);
    fadeAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(fadeAnim, &QPropertyAnimation::finished, this, [this]() {
        hide();
        setWindowOpacity(1.0); // Reset for next show
    });
    fadeAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

void FloatingOrb::fadeIn()
{
    setWindowOpacity(0.0);
    show();
    QPropertyAnimation *fadeAnim = new QPropertyAnimation(this, "windowOpacity");
    fadeAnim->setDuration(250);
    fadeAnim->setStartValue(0.0);
    fadeAnim->setEndValue(1.0);
    fadeAnim->setEasingCurve(QEasingCurve::InCubic);
    fadeAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

void FloatingOrb::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QPointF center(width() / 2.0, height() / 2.0);
    QColor theme = stateColor();

    // Pulse computation (subtle breathing: 0.0 -> 1.0)
    float pulseFactor = 0.5f * (1.0f + std::sin(m_pulsePhase));
    float glowIntensity = 0.5f * (1.0f + std::sin(m_glowPhase));

    // Refined subtle active boost
    float activeGlow = (m_state != OrbState::Idle) ? 1.5f : 0.8f;

    // === Layer 0: Ultra-Wide Ambient Glow (Outer Ring) ===
    if (m_state != OrbState::Idle) {
        qreal outerGlowRadius = 40.0 + (glowIntensity * 5.0);
        QRadialGradient outerGlow(center, outerGlowRadius);
        outerGlow.setColorAt(0.3, QColor(theme.red(), theme.green(), theme.blue(), static_cast<int>(20 * activeGlow * glowIntensity)));
        outerGlow.setColorAt(1.0, QColor(theme.red(), theme.green(), theme.blue(), 0));
        painter.setBrush(outerGlow);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(center, outerGlowRadius, outerGlowRadius);
    }

    // === Layer 1: Mid-Range Ambient Halo ===
    qreal ambientRadius = 28.0 + (pulseFactor * 3.0 * activeGlow);
    QRadialGradient ambientGlow(center, ambientRadius);
    ambientGlow.setColorAt(0.4, QColor(theme.red(), theme.green(), theme.blue(), static_cast<int>(80 * activeGlow)));
    ambientGlow.setColorAt(1.0, QColor(theme.red(), theme.green(), theme.blue(), 0));
    painter.setBrush(ambientGlow);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(center, ambientRadius, ambientRadius);

    // === Layer 2: Precision Glowing Orbit Ring ===
    painter.save();
    painter.translate(center);
    painter.rotate(m_rotationAngle);

    QPen quantumPen;
    quantumPen.setWidthF(2.0);
    quantumPen.setColor(QColor(theme.red(), theme.green(), theme.blue(), static_cast<int>(180 * activeGlow)));
    quantumPen.setStyle(Qt::DashLine);
    quantumPen.setDashPattern({5.0, 3.0});
    painter.setPen(quantumPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QPointF(0, 0), 24.5, 24.5);

    // Secondary orbit ring for depth
    if (m_state != OrbState::Idle) {
        QPen secondaryPen;
        secondaryPen.setWidthF(1.0);
        secondaryPen.setColor(QColor(theme.red(), theme.green(), theme.blue(), static_cast<int>(80 * activeGlow * pulseFactor)));
        secondaryPen.setStyle(Qt::DashLine);
        secondaryPen.setDashPattern({3.0, 4.0});
        painter.setPen(secondaryPen);
        painter.drawEllipse(QPointF(0, 0), 18.0, 18.0);
    }

    // Dynamic bright photon particles on the orbit (2 particles for visual interest)
    painter.setPen(Qt::NoPen);
    QPointF photonPos1(24.5 * std::cos(m_rotationAngle * 0.01745f), 24.5 * std::sin(m_rotationAngle * 0.01745f));
    QPointF photonPos2(24.5 * std::cos((m_rotationAngle + 180.0f) * 0.01745f), 24.5 * std::sin((m_rotationAngle + 180.0f) * 0.01745f));

    // First photon with glow
    painter.setBrush(QColor(255, 255, 255, static_cast<int>(230 * activeGlow * glowIntensity)));
    painter.drawEllipse(photonPos1, 2.5, 2.5);

    // Second photon (dimmer)
    painter.setBrush(QColor(255, 255, 255, static_cast<int>(120 * activeGlow * (1.0f - glowIntensity))));
    painter.drawEllipse(photonPos2, 1.5, 1.5);

    painter.restore();

    // === Layer 3: Inner Accent Halo ===
    QPen innerPen;
    innerPen.setWidthF(1.4);
    innerPen.setColor(QColor(theme.red(), theme.green(), theme.blue(), static_cast<int>(140 + 80 * pulseFactor * activeGlow)));
    painter.setPen(innerPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(center, 20.5, 20.5);

    // === Layer 4: Glass Orb Core Sphere with Enhanced 3D Effect ===
    QRadialGradient orbGrad(center.x() - 3.5, center.y() - 5, 18.5);
    orbGrad.setColorAt(0.0, QColor(theme.red(), theme.green(), theme.blue(), 240));
    orbGrad.setColorAt(0.5, QColor(theme.red() * 0.7, theme.green() * 0.7, theme.blue() * 0.7, 230));
    orbGrad.setColorAt(0.8, QColor(15, 23, 42, 225)); // Deep slate glass
    orbGrad.setColorAt(1.0, QColor(2, 6, 23, 245));

    painter.setPen(QPen(QColor(255, 255, 255, 70), 1.2));
    painter.setBrush(orbGrad);
    painter.drawEllipse(center, 17.5, 17.5);

    // === Layer 5: Dynamic Inner Glow (Breathing) ===
    if (m_state != OrbState::Idle) {
        QRadialGradient innerGlow(center, 15.0);
        innerGlow.setColorAt(0.0, QColor(theme.red(), theme.green(), theme.blue(), static_cast<int>(80 * pulseFactor * activeGlow)));
        innerGlow.setColorAt(1.0, QColor(theme.red(), theme.green(), theme.blue(), 0));
        painter.setPen(Qt::NoPen);
        painter.setBrush(innerGlow);
        painter.drawEllipse(center, 15.0, 15.0);
    }

    // === Layer 6: Specular Reflection Highlight (Enhanced) ===
    painter.setPen(Qt::NoPen);
    QRadialGradient highlight(center.x() - 5, center.y() - 6, 7.0);
    highlight.setColorAt(0.0, QColor(255, 255, 255, 150));
    highlight.setColorAt(0.6, QColor(255, 255, 255, 30));
    highlight.setColorAt(1.0, QColor(255, 255, 255, 0));
    painter.setBrush(highlight);
    painter.drawEllipse(QPointF(center.x() - 5, center.y() - 6), 5.5, 3.5);

    // === Layer 7: Secondary Highlight (subtle rim light) ===
    if (m_state != OrbState::Idle) {
        painter.setPen(QPen(QColor(255, 255, 255, static_cast<int>(40 * activeGlow * glowIntensity)), 0.8));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(center, 18.0, 18.0);
    }
}

void FloatingOrb::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        m_isDragging = false;
        event->accept();
    }
}

void FloatingOrb::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        m_isDragging = true;
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}

void FloatingOrb::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (!m_isDragging) {
            emit clicked(); // Triggers toggle visibility
        }
        m_isDragging = false;
        event->accept();
    }
}

void FloatingOrb::enterEvent(QEnterEvent *event)
{
    // Optionally trigger dodge on mouse entry directly
    // Not strictly needed with proximity timer
    QWidget::enterEvent(event);
}
