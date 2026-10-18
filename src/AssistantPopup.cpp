#include "AssistantPopup.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScreen>
#include <QApplication>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QParallelAnimationGroup>
#include <QGraphicsOpacityEffect>
#include <QScrollBar>
#include <cmath>

// ---------------------------------------------------------------------------
// AudioVisualizerWidget Implementation
// ---------------------------------------------------------------------------
AudioVisualizerWidget::AudioVisualizerWidget(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(80, 32);
    connect(&m_animTimer, &QTimer::timeout, this, &AudioVisualizerWidget::onTick);
}

void AudioVisualizerWidget::setActive(bool active)
{
    m_active = active;
    if (m_active) {
        m_animTimer.start(40); // 25 FPS - smoother
    } else {
        m_animTimer.stop();
        m_audioLevel = 0.0f;
        update();
    }
}

void AudioVisualizerWidget::setAudioLevel(float level)
{
    m_audioLevel = qBound(0.0f, level, 1.0f);
}

void AudioVisualizerWidget::onTick()
{
    m_phase += 0.12f;
    if (m_phase > 6.28f) m_phase -= 6.28f;
    update();
}

void AudioVisualizerWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int numBars = 5;
    const int barWidth = 4;
    const int spacing = 8;
    const int maxHeight = height() - 6;
    const int baseY = height() / 2;
    const int totalWidth = numBars * (barWidth + spacing) - spacing;
    const int startX = (width() - totalWidth) / 2;

    if (!m_active) {
        // Draw idle static bars with subtle gradient
        for (int i = 0; i < numBars; ++i) {
            int x = startX + i * (barWidth + spacing);
            QLinearGradient idleGrad(x, baseY - 3, x, baseY + 3);
            idleGrad.setColorAt(0.0, QColor(100, 116, 139, 80));
            idleGrad.setColorAt(1.0, QColor(100, 116, 139, 40));
            painter.setPen(Qt::NoPen);
            painter.setBrush(idleGrad);
            painter.drawRoundedRect(x, baseY - 3, barWidth, 6, 2, 2);
        }
        return;
    }

    // Dynamic 5-bar visualizer with gradient and glow
    QColor barColorTop(56, 189, 248);    // Cyan
    QColor barColorBottom(99, 102, 241); // Indigo

    for (int i = 0; i < numBars; ++i) {
        float offset = m_phase + i * 0.9f;
        float wave = std::sin(offset) * 0.5f + 0.5f;
        float barHeight = (wave * 0.65f + m_audioLevel * 0.35f) * maxHeight;
        barHeight = qMax(barHeight, 4.0f); // Minimum bar height

        int x = startX + i * (barWidth + spacing);
        int y = baseY - static_cast<int>(barHeight / 2);
        int h = static_cast<int>(barHeight);

        // Bar glow (subtle outer glow)
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(56, 189, 248, static_cast<int>(40 * wave)));
        painter.drawRoundedRect(x - 1, y - 1, barWidth + 2, h + 2, 3, 3);

        // Bar gradient fill
        QLinearGradient barGrad(x, y, x, y + h);
        barGrad.setColorAt(0.0, barColorTop);
        barGrad.setColorAt(1.0, barColorBottom);
        painter.setBrush(barGrad);
        painter.drawRoundedRect(x, y, barWidth, h, 2, 2);
    }
}

// ---------------------------------------------------------------------------
// ChatInputEdit Implementation
// ---------------------------------------------------------------------------
ChatInputEdit::ChatInputEdit(QWidget *parent)
    : QTextEdit(parent)
{
    setPlaceholderText("Ask anything...");
    setMaximumHeight(70);
}

void ChatInputEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (event->modifiers() & Qt::ShiftModifier) {
            QTextEdit::keyPressEvent(event);
        } else {
            emit sendRequested();
            event->accept();
            return;
        }
    }
    QTextEdit::keyPressEvent(event);
}

void ChatInputEdit::mousePressEvent(QMouseEvent *event)
{
    window()->activateWindow();
    QTextEdit::mousePressEvent(event);
}

// ---------------------------------------------------------------------------
// AssistantPopup Implementation
// ---------------------------------------------------------------------------
AssistantPopup::AssistantPopup(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    setFixedWidth(kPopupWidth);
    setMaximumHeight(kPopupMaxHeight);

    setupUi();

    // Setup glow animation timer
    m_glowAnimTimer = new QTimer(this);
    connect(m_glowAnimTimer, &QTimer::timeout, this, [this]() {
        m_glowPhase += 0.12f;
        if (m_glowPhase >= 6.28f) m_glowPhase -= 6.28f;
        update(); // Trigger paintEvent to redraw with new glow
    });

    m_proximityTimer = new QTimer(this);
    connect(m_proximityTimer, &QTimer::timeout, this, &AssistantPopup::checkProximity);

    m_autoReturnTimer = new QTimer(this);
    connect(m_autoReturnTimer, &QTimer::timeout, this, [this]() {
        // Only return if anchor is bottom and cursor is out of interference zone
        if (m_anchor == OrbAnchor::BottomRight) {
            glideToAnchor(OrbAnchor::TopRight);
        }
        m_autoReturnTimer->stop();
    });

    // Futuristic Frosted Obsidian Glassmorphism
    setStyleSheet(R"(
        QWidget#CentralWidget {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 rgba(8, 12, 28, 0.92),
                stop:0.5 rgba(15, 23, 42, 0.90),
                stop:1 rgba(8, 12, 28, 0.95));
            border: 1px solid rgba(56, 189, 248, 0.25);
            border-radius: 22px;
        }

        QLabel#TitleLabel {
            color: #f8fafc;
            font-family: 'Segoe UI Semibold', sans-serif;
            font-size: 18px;
            font-weight: 600;
            letter-spacing: 0.5px;
        }

        QLabel#StateBadge {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 rgba(71, 85, 105, 0.3),
                stop:1 rgba(71, 85, 105, 0.1));
            border: 1px solid rgba(148, 163, 184, 0.2);
            border-radius: 12px;
            padding: 4px 12px;
            color: #94a3b8;
            font-size: 12px;
            font-weight: 600;
        }

        QTextEdit#Transcript {
            background: rgba(15, 23, 42, 0.4);
            border: 1px solid rgba(71, 85, 105, 0.15);
            border-radius: 14px;
            color: #e2e8f0;
            font-family: 'Segoe UI', sans-serif;
            font-size: 13px;
            padding: 8px;
        }

        QScrollBar:vertical {
            background: rgba(15, 23, 42, 0.2);
            width: 6px;
            margin: 0px;
            border-radius: 3px;
        }
        QScrollBar::handle:vertical {
            background: rgba(148, 163, 184, 0.3);
            min-height: 20px;
            border-radius: 3px;
        }
        QScrollBar::handle:vertical:hover {
            background: rgba(56, 189, 248, 0.5);
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }

        ChatInputEdit {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 rgba(30, 41, 59, 0.6),
                stop:1 rgba(15, 23, 42, 0.7));
            border: 1px solid rgba(56, 189, 248, 0.2);
            border-radius: 14px;
            color: #f1f5f9;
            padding: 12px;
            font-size: 14px;
            selection-background-color: rgba(56, 189, 248, 0.3);
        }

        ChatInputEdit:focus {
            border: 1px solid rgba(56, 189, 248, 0.5);
        }

        QPushButton#PrimaryButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 rgba(56, 189, 248, 0.8),
                stop:1 rgba(99, 102, 241, 0.8));
            color: #f8fafc;
            border: 1px solid rgba(56, 189, 248, 0.5);
            border-radius: 12px;
            padding: 0px;
            font-weight: 700;
            font-size: 18px;
        }

        QPushButton#PrimaryButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 rgba(56, 189, 248, 1.0),
                stop:1 rgba(99, 102, 241, 1.0));
            border: 1px solid rgba(56, 189, 248, 0.8);
        }

        QPushButton#PrimaryButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 rgba(30, 144, 255, 0.9),
                stop:1 rgba(65, 105, 225, 0.9));
        }

        QPushButton#PowerButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 rgba(220, 38, 38, 0.6),
                stop:1 rgba(153, 27, 27, 0.7));
            color: #fca5a5;
            border: 2px solid rgba(239, 68, 68, 0.5);
            border-radius: 14px;
            padding: 0px;
            font-size: 10px;
            font-weight: bold;
            font-family: 'Segoe UI', sans-serif;
        }

        QPushButton#PowerButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 rgba(248, 113, 113, 0.85),
                stop:1 rgba(239, 68, 68, 0.9));
            border: 2px solid rgba(248, 113, 113, 0.8);
            color: #ffffff;
        }

        QPushButton#PowerButton:pressed {
            background: rgba(153, 27, 27, 0.95);
            border: 2px solid rgba(220, 38, 38, 0.9);
            color: #ffffff;
        }

        QPushButton#MinimizeButton {
            background: rgba(30, 41, 59, 0.3);
            color: #94a3b8;
            border: 1px solid rgba(71, 85, 105, 0.25);
            border-radius: 14px;
            padding: 0px;
            font-size: 14px;
            font-family: 'Segoe UI', sans-serif;
        }

        QPushButton#MinimizeButton:hover {
            background: rgba(51, 65, 85, 0.5);
            border: 1px solid rgba(148, 163, 184, 0.4);
            color: #e2e8f0;
        }

        QPushButton#GlassButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 rgba(51, 65, 85, 0.3),
                stop:1 rgba(30, 41, 59, 0.4));
            color: #cbd5e1;
            border: 1px solid rgba(71, 85, 105, 0.3);
            border-radius: 8px;
            padding: 6px 14px;
            font-size: 12px;
            font-weight: 500;
        }

        QPushButton#GlassButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 rgba(56, 189, 248, 0.3),
                stop:1 rgba(56, 189, 248, 0.15));
            border: 1px solid rgba(56, 189, 248, 0.5);
            color: #38bdf8;
        }

        QPushButton#GlassButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 rgba(56, 189, 248, 0.5),
                stop:1 rgba(56, 189, 248, 0.3));
            border: 1px solid rgba(56, 189, 248, 0.7);
            color: #e0f2fe;
        }

        QLabel#StatsLabel {
            color: rgba(148, 163, 184, 0.8);
            font-size: 11px;
            font-family: 'Consolas', monospace;
        }
    )");
}

void AssistantPopup::setupUi()
{
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setObjectName("CentralWidget");

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(20, 18, 20, 18);
    mainLayout->setSpacing(12);

    // === Header Row ===
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(10);

    // Title with dot indicator
    QLabel *dotIndicator = new QLabel("●", this);
    dotIndicator->setStyleSheet("color: #38bdf8; font-size: 10px; margin-right: 2px;");
    headerLayout->addWidget(dotIndicator);

    m_titleLabel = new QLabel("Automatron", this);
    m_titleLabel->setObjectName("TitleLabel");
    headerLayout->addWidget(m_titleLabel);

    m_stateBadgeLabel = new QLabel("Idle", this);
    m_stateBadgeLabel->setObjectName("StateBadge");
    headerLayout->addWidget(m_stateBadgeLabel);

    headerLayout->addStretch();

    // Minimize button (circular, icon-only)
    m_minimizeButton = new QPushButton("−", this);
    m_minimizeButton->setObjectName("MinimizeButton");
    m_minimizeButton->setFixedSize(28, 28);
    m_minimizeButton->setCursor(Qt::PointingHandCursor);
    headerLayout->addWidget(m_minimizeButton);

    // Kill Switch button (circular, skull/stop icon) - clearly distinct from window close
    m_killButton = new QPushButton(this);
    m_killButton->setObjectName("PowerButton");
    m_killButton->setText(QString::fromUtf8("■"));
    m_killButton->setFixedSize(28, 28);
    m_killButton->setToolTip("Kill Switch — Terminate Automatron");
    m_killButton->setCursor(Qt::PointingHandCursor);
    headerLayout->addWidget(m_killButton);

    mainLayout->addLayout(headerLayout);

    // === Thin Separator Line ===
    QFrame *separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("background: rgba(56, 189, 248, 0.12); max-height: 1px; border: none;");
    mainLayout->addWidget(separator);

    // === Stats Row ===
    QHBoxLayout *statsLayout = new QHBoxLayout();
    statsLayout->setSpacing(16);

    m_visualizer = new AudioVisualizerWidget(this);
    statsLayout->addWidget(m_visualizer);

    m_cpuLabel = new QLabel("CPU: 0%", this);
    m_cpuLabel->setObjectName("StatsLabel");
    statsLayout->addWidget(m_cpuLabel);

    m_ramLabel = new QLabel("RAM: 0 MB", this);
    m_ramLabel->setObjectName("StatsLabel");
    statsLayout->addWidget(m_ramLabel);

    statsLayout->addStretch();

    mainLayout->addLayout(statsLayout);

    // === Transcript Area ===
    m_transcript = new QTextEdit(this);
    m_transcript->setObjectName("Transcript");
    m_transcript->setReadOnly(true);
    m_transcript->setMinimumHeight(260);
    mainLayout->addWidget(m_transcript);

    // === Action Chips ===
    setupActionChips(mainLayout);

    // === Input Area ===
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->setSpacing(10);

    m_inputEdit = new ChatInputEdit(this);
    inputLayout->addWidget(m_inputEdit);

    m_sendButton = new QPushButton("➤", this);
    m_sendButton->setObjectName("PrimaryButton");
    m_sendButton->setFixedSize(42, 42);
    m_sendButton->setCursor(Qt::PointingHandCursor);
    m_sendButton->setToolTip("Send message");
    inputLayout->addWidget(m_sendButton);

    mainLayout->addLayout(inputLayout);

    // === Root Layout ===
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->addWidget(centralWidget);

    // Connect signals
    connect(m_sendButton, &QPushButton::clicked, this, &AssistantPopup::onSendClicked);
    connect(m_killButton, &QPushButton::clicked, this, &AssistantPopup::onKillClicked);
    connect(m_minimizeButton, &QPushButton::clicked, this, &AssistantPopup::onMinimizeClicked);
    connect(m_inputEdit, &ChatInputEdit::sendRequested, this, &AssistantPopup::onSendClicked);
}

void AssistantPopup::setupActionChips(QVBoxLayout *parentLayout)
{
    m_chipsContainer = new QWidget(this);
    QHBoxLayout *chipsLayout = new QHBoxLayout(m_chipsContainer);
    chipsLayout->setContentsMargins(0, 0, 0, 0);
    chipsLayout->setSpacing(10);

    QStringList actions = {"Summarize", "Code", "Search", "Stop"};
    for (const QString &action : actions) {
        QPushButton *chip = new QPushButton(action, this);
        chip->setObjectName("GlassButton");
        chipsLayout->addWidget(chip);
        connect(chip, &QPushButton::clicked, this, &AssistantPopup::onChipClicked);
    }

    chipsLayout->addStretch();
    parentLayout->addWidget(m_chipsContainer);
}

void AssistantPopup::updatePositionAboveOrb(const QPoint &orbGlobalPos, const QSize &orbSize)
{
    updatePositionRelativeToOrb(orbGlobalPos, orbSize, OrbAnchor::BottomRight);
}

void AssistantPopup::updatePositionRelativeToOrb(const QPoint &orbGlobalPos, const QSize &orbSize, OrbAnchor anchor)
{
    QScreen *screen = QApplication::screenAt(orbGlobalPos);
    if (!screen) {
        screen = QApplication::primaryScreen();
    }

    QRect availableGeometry = screen->availableGeometry();
    int popupX = orbGlobalPos.x() + orbSize.width() / 2 - kPopupWidth / 2;
    int popupY;

    if (anchor == OrbAnchor::TopRight) {
        // For top-right orb, place popup below
        popupY = orbGlobalPos.y() + orbSize.height() + kSpacingAboveOrb;
    } else {
        // For bottom-right orb, place popup above (default behavior)
        popupY = orbGlobalPos.y() - height() - kSpacingAboveOrb;
    }

    popupX = qBound(availableGeometry.left(), popupX, availableGeometry.right() - kPopupWidth);
    popupY = qBound(availableGeometry.top(), popupY, availableGeometry.bottom() - height());

    move(popupX, popupY);
}
void AssistantPopup::checkProximity()
{
    if (m_isDragging) {
        m_autoReturnTimer->stop();
        return;
    }

    QPoint cursor = QCursor::pos();
    QScreen *screen = QApplication::screenAt(cursor);
    if (!screen) screen = QApplication::primaryScreen();
    QRect workArea = screen->availableGeometry();

    // Top interference zone: anywhere in the top 150px of the screen (tabs, titlebars, close buttons)
    // or near the top of the popup
    bool inTopZone = (cursor.y() < workArea.top() + 150);
    bool overPopup = geometry().contains(cursor);

    if (m_anchor == OrbAnchor::TopRight) {
        // While docked at the top: if user approaches the top area, glide down to clear the space
        if (inTopZone) {
            glideToAnchor(OrbAnchor::BottomRight);
            m_autoReturnTimer->stop();
        }
    } else if (m_anchor == OrbAnchor::BottomRight) {
        // While docked at the bottom:
        // STAY at the bottom if:
        // 1. Cursor is still working in the top half of the screen (closing tabs, browsing)
        // 2. OR user is hovering/interacting with the popup at the bottom
        if (cursor.y() < workArea.center().y() || overPopup) {
            m_autoReturnTimer->stop();
        } else {
            // Cursor has left the top area and is not on the popup:
            // Grace period of 2.0 seconds before smoothly returning to top
            if (!m_autoReturnTimer->isActive()) {
                m_autoReturnTimer->start(2000);
            }
        }
    }
}

void AssistantPopup::glideToAnchor(OrbAnchor anchor)
{
    if (m_anchor == anchor) return;

    QScreen *screen = QApplication::primaryScreen();
    if (!screen) return;
    QRect workArea = screen->availableGeometry();

    int targetWidth = kPopupWidth;
    int targetHeight = kPopupMaxHeight;
    int targetX = workArea.right() - targetWidth - 24;
    int targetY;

    if (anchor == OrbAnchor::TopRight) {
        // Slide back to top-right docked position
        targetY = workArea.top() + 24 + 80 + 12; // below orb at top-right
    } else {
        // Slide down to bottom-right docked position
        targetY = workArea.bottom() - targetHeight - 24;
    }

    m_proximityTimer->stop(); // Pause polling during transition

    QRect endRect(targetX, targetY, targetWidth, targetHeight);

    QPropertyAnimation *anim = new QPropertyAnimation(this, "geometry");
    anim->setDuration(350);
    anim->setStartValue(geometry());
    anim->setEndValue(endRect);
    anim->setEasingCurve(QEasingCurve::InOutCubic);
    connect(anim, &QPropertyAnimation::finished, this, [this, anchor]() {
        m_anchor = anchor;
        emit anchorChanged(anchor);
        m_proximityTimer->start(50); // Resume polling after glide
    });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void AssistantPopup::setState(OrbState state)
{
    m_currentState = state;

    // Control glow animation based on state
    if (state == OrbState::Listening || state == OrbState::Thinking || state == OrbState::Executing) {
        if (!m_glowAnimTimer->isActive()) {
            m_glowAnimTimer->start(40); // 25 FPS for smooth glow
        }
        m_isGlowActive = true;
    } else {
        m_glowAnimTimer->stop();
        m_isGlowActive = false;
        update(); // Final update to show non-glowing state
    }

    switch (state) {
        case OrbState::Idle:
            m_stateBadgeLabel->setText("Idle");
            m_stateBadgeLabel->setStyleSheet("background: rgba(71, 85, 105, 0.3); border: 1px solid rgba(148, 163, 184, 0.2); border-radius: 12px; padding: 4px 12px; color: #94a3b8; font-size: 12px; font-weight: 600;");
            m_visualizer->setActive(false);
            m_isStreaming = false;
            break;
        case OrbState::Listening:
            m_stateBadgeLabel->setText("🎤 Listening...");
            m_stateBadgeLabel->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(56, 189, 248, 0.25), stop:1 rgba(56, 189, 248, 0.15)); border: 1px solid rgba(56, 189, 248, 0.5); border-radius: 12px; padding: 4px 12px; color: #38bdf8; font-size: 12px; font-weight: 600;");
            m_visualizer->setActive(true);
            break;
        case OrbState::Thinking:
            m_stateBadgeLabel->setText("🧠 Thinking...");
            m_stateBadgeLabel->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(245, 158, 11, 0.25), stop:1 rgba(245, 158, 11, 0.15)); border: 1px solid rgba(245, 158, 11, 0.5); border-radius: 12px; padding: 4px 12px; color: #fbbf24; font-size: 12px; font-weight: 600;");
            m_visualizer->setActive(false);
            m_isStreaming = true;
            break;
        case OrbState::Executing:
            m_stateBadgeLabel->setText("⚡ Executing...");
            m_stateBadgeLabel->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(168, 85, 247, 0.25), stop:1 rgba(168, 85, 247, 0.15)); border: 1px solid rgba(168, 85, 247, 0.5); border-radius: 12px; padding: 4px 12px; color: #a855f7; font-size: 12px; font-weight: 600;");
            m_visualizer->setActive(false);
            break;
        case OrbState::Terminated:
            m_stateBadgeLabel->setText("⛔ Terminated");
            m_stateBadgeLabel->setStyleSheet("background: rgba(239, 68, 68, 0.25); border: 1px solid rgba(239, 68, 68, 0.5); border-radius: 12px; padding: 4px 12px; color: #ef4444; font-size: 12px; font-weight: 600;");
            m_visualizer->setActive(false);
            m_isTerminated = true;
            m_isStreaming = false;
            break;
    }
}

void AssistantPopup::appendTranscriptMessage(const QString &sender, const QString &text)
{
    if (m_isStreaming && (sender == "Agent" || sender == "Voice AI" || sender == "System")) {
        m_isStreaming = false;
        QScrollBar *bar = m_transcript->verticalScrollBar();
        if (bar) bar->setValue(bar->maximum());
        if (sender != "System") return;
    }
    m_isStreaming = false;

    QString parsedText = text.toHtmlEscaped().replace("\n", "<br/>");
    QTextCursor cursor = m_transcript->textCursor();
    cursor.movePosition(QTextCursor::End);

    // Block format for alignment
    QTextBlockFormat blockFmt;
    blockFmt.setTopMargin(6);
    blockFmt.setBottomMargin(2);

    // Character format (reset to default)
    QTextCharFormat defaultFmt;
    defaultFmt.setFont(m_transcript->font());

    if (sender == "You" || sender == "User") {
        // --- User bubble: right-aligned ---
        blockFmt.setAlignment(Qt::AlignRight);
        cursor.insertBlock(blockFmt, defaultFmt);

        // Avatar + label inline
        QTextCharFormat avatarFmt;
        avatarFmt.setForeground(QColor("#38bdf8"));
        avatarFmt.setFontPointSize(11);
        avatarFmt.setFontWeight(QFont::Bold);
        cursor.insertText("  👤 You  ", avatarFmt);

        // Insert a new line for the message text, still right-aligned
        QTextBlockFormat msgBlockFmt;
        msgBlockFmt.setAlignment(Qt::AlignRight);
        msgBlockFmt.setTopMargin(2);
        msgBlockFmt.setBottomMargin(2);
        msgBlockFmt.setRightMargin(8);
        msgBlockFmt.setBackground(QColor(30, 41, 59, 180));
        cursor.insertBlock(msgBlockFmt);

        QTextCharFormat msgFmt;
        msgFmt.setForeground(QColor("#f1f5f9"));
        msgFmt.setFontPointSize(12);
        cursor.insertText(text, msgFmt);

        // Spacer line
        QTextBlockFormat spacerFmt;
        spacerFmt.setTopMargin(4);
        cursor.insertBlock(spacerFmt, defaultFmt);

    } else if (sender == "Agent" || sender == "Voice AI") {
        // --- AI bubble: left-aligned ---
        blockFmt.setAlignment(Qt::AlignLeft);
        cursor.insertBlock(blockFmt, defaultFmt);

        QTextCharFormat avatarFmt;
        avatarFmt.setForeground(QColor("#c084fc"));
        avatarFmt.setFontPointSize(11);
        avatarFmt.setFontWeight(QFont::Bold);
        cursor.insertText("🤖 Automatron  ", avatarFmt);

        QTextBlockFormat msgBlockFmt;
        msgBlockFmt.setAlignment(Qt::AlignLeft);
        msgBlockFmt.setTopMargin(2);
        msgBlockFmt.setBottomMargin(2);
        msgBlockFmt.setLeftMargin(8);
        msgBlockFmt.setBackground(QColor(88, 28, 135, 120));
        cursor.insertBlock(msgBlockFmt);

        QTextCharFormat msgFmt;
        msgFmt.setForeground(QColor("#e2e8f0"));
        msgFmt.setFontPointSize(12);
        cursor.insertText(text, msgFmt);

        QTextBlockFormat spacerFmt;
        spacerFmt.setTopMargin(4);
        cursor.insertBlock(spacerFmt, defaultFmt);

    } else {
        // System message: centered, italic, muted
        blockFmt.setAlignment(Qt::AlignHCenter);
        blockFmt.setTopMargin(4);
        blockFmt.setBottomMargin(4);
        cursor.insertBlock(blockFmt, defaultFmt);

        QTextCharFormat sysFmt;
        sysFmt.setForeground(QColor("#64748b"));
        sysFmt.setFontItalic(true);
        sysFmt.setFontPointSize(10);
        cursor.insertText(text, sysFmt);
    }

    m_transcript->setTextCursor(cursor);
    QScrollBar *bar = m_transcript->verticalScrollBar();
    if (bar) bar->setValue(bar->maximum());
}

void AssistantPopup::appendStreamingChunk(const QString &chunk)
{
    QTextCursor cursor = m_transcript->textCursor();
    cursor.movePosition(QTextCursor::End);

    if (!m_isStreaming) {
        m_isStreaming = true;

        QTextBlockFormat blockFmt;
        blockFmt.setAlignment(Qt::AlignLeft);
        blockFmt.setTopMargin(6);
        blockFmt.setBottomMargin(2);

        QTextCharFormat avatarFmt;
        avatarFmt.setForeground(QColor("#c084fc"));
        avatarFmt.setFontPointSize(11);
        avatarFmt.setFontWeight(QFont::Bold);

        cursor.insertBlock(blockFmt);
        cursor.insertText("🤖 Automatron  ", avatarFmt);

        QTextBlockFormat msgBlockFmt;
        msgBlockFmt.setAlignment(Qt::AlignLeft);
        msgBlockFmt.setTopMargin(2);
        msgBlockFmt.setBottomMargin(2);
        msgBlockFmt.setLeftMargin(8);
        msgBlockFmt.setBackground(QColor(88, 28, 135, 120));
        cursor.insertBlock(msgBlockFmt);
    }

    QTextCharFormat msgFmt;
    msgFmt.setForeground(QColor("#e2e8f0"));
    msgFmt.setFontPointSize(12);
    cursor.insertText(chunk, msgFmt);

    m_transcript->setTextCursor(cursor);
    QScrollBar *bar = m_transcript->verticalScrollBar();
    if (bar) bar->setValue(bar->maximum());
}

void AssistantPopup::setStats(float cpuPercent, float ramMb)
{
    m_cpuLabel->setText(QString("CPU: %1%").arg(static_cast<int>(cpuPercent)));
    m_ramLabel->setText(QString("RAM: %1 MB").arg(static_cast<int>(ramMb)));
}

void AssistantPopup::setAudioLevel(float level)
{
    m_visualizer->setAudioLevel(level);
}

void AssistantPopup::resetTermination()
{
    m_isTerminated = false;
    setState(OrbState::Idle);
}

void AssistantPopup::onSendClicked()
{
    QString text = m_inputEdit->toPlainText().trimmed();
    if (text.isEmpty() || m_isTerminated) {
        return;
    }

    // Show user prompt in transcript (like a chatbot)
    appendTranscriptMessage("You", text);

    emit promptSubmitted(text);
    m_inputEdit->clear();
}

void AssistantPopup::onKillClicked()
{
    m_isTerminated = true;
    emit killRequested();
}

void AssistantPopup::onChipClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (btn) {
        emit actionChipTriggered(btn->text());
    }
}

void AssistantPopup::onMinimizeClicked()
{
    emit minimizeRequested();
}

void AssistantPopup::expandFrom(const QPoint &orbCenter, const QSize &orbSize)
{
    expandFrom(orbCenter, orbSize, OrbAnchor::BottomRight);
}

void AssistantPopup::expandFrom(const QPoint &orbCenter, const QSize &orbSize, OrbAnchor anchor)
{
    // Calculate target relative to orb anchor
    QScreen *screen = QApplication::screenAt(orbCenter);
    if (!screen) screen = QApplication::primaryScreen();
    QRect availableGeometry = screen->availableGeometry();

    int targetWidth = kPopupWidth;
    int targetHeight = kPopupMaxHeight;
    int targetX = orbCenter.x() - targetWidth / 2;
    int targetY;

    if (anchor == OrbAnchor::TopRight) {
        // Expand downward from top-right orb
        targetY = orbCenter.y() + orbSize.height() / 2 + 12;
    } else {
        // Expand upward from bottom-right orb (original behavior)
        targetY = orbCenter.y() - orbSize.height() / 2 - targetHeight - 12;
    }

    // Clamp to screen bounds
    targetX = qBound(availableGeometry.left(), targetX, availableGeometry.right() - targetWidth);
    targetY = qBound(availableGeometry.top(), targetY, availableGeometry.bottom() - targetHeight);

    QRect endRect(targetX, targetY, targetWidth, targetHeight);
    QRect startRect;

    if (anchor == OrbAnchor::TopRight) {
        // Slide down from 24px above target
        startRect = QRect(targetX, targetY - 24, targetWidth, targetHeight);
    } else {
        // Slide up from 24px below target (original)
        startRect = QRect(targetX, targetY + 24, targetWidth, targetHeight);
    }

    setGeometry(startRect);
    setWindowOpacity(0.0);
    show();

    QParallelAnimationGroup *group = new QParallelAnimationGroup(this);

    // Smooth position slide (Cubic Ease Out)
    QPropertyAnimation *posAnim = new QPropertyAnimation(this, "geometry");
    posAnim->setDuration(240);
    posAnim->setStartValue(startRect);
    posAnim->setEndValue(endRect);
    posAnim->setEasingCurve(QEasingCurve::OutCubic);
    group->addAnimation(posAnim);

    // Smooth opacity fade
    QPropertyAnimation *opacityAnim = new QPropertyAnimation(this, "windowOpacity");
    opacityAnim->setDuration(240);
    opacityAnim->setStartValue(0.0);
    opacityAnim->setEndValue(1.0);
    opacityAnim->setEasingCurve(QEasingCurve::OutCubic);
    group->addAnimation(opacityAnim);

    connect(group, &QParallelAnimationGroup::finished, this, [this, anchor]() {
        m_anchor = anchor;
        emit expansionComplete();
        if (anchor == OrbAnchor::TopRight) {
            m_proximityTimer->start(50);
        }
    });

    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void AssistantPopup::collapseTo(const QPoint &orbCenter, const QSize &orbSize)
{
    collapseTo(orbCenter, orbSize, OrbAnchor::BottomRight);
}

void AssistantPopup::collapseTo(const QPoint &orbCenter, const QSize &orbSize, OrbAnchor anchor)
{
    Q_UNUSED(orbCenter);
    Q_UNUSED(orbSize);

    m_proximityTimer->stop();
    m_autoReturnTimer->stop();

    QRect currentRect = geometry();
    QRect endRect;

    if (anchor == OrbAnchor::TopRight) {
        // Descend downward 24px (toward orb below)
        endRect = currentRect.translated(0, 24);
    } else {
        // Ascend upward 24px (toward orb below, original behavior)
        endRect = currentRect.translated(0, 24);
    }

    QParallelAnimationGroup *group = new QParallelAnimationGroup(this);

    // Smooth position descent
    QPropertyAnimation *posAnim = new QPropertyAnimation(this, "geometry");
    posAnim->setDuration(200);
    posAnim->setStartValue(currentRect);
    posAnim->setEndValue(endRect);
    posAnim->setEasingCurve(QEasingCurve::InCubic);
    group->addAnimation(posAnim);

    // Smooth opacity fade out
    QPropertyAnimation *opacityAnim = new QPropertyAnimation(this, "windowOpacity");
    opacityAnim->setDuration(200);
    opacityAnim->setStartValue(1.0);
    opacityAnim->setEndValue(0.0);
    opacityAnim->setEasingCurve(QEasingCurve::InCubic);
    group->addAnimation(opacityAnim);

    connect(group, &QParallelAnimationGroup::finished, this, [this]() {
        emit collapseComplete();
    });

    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void AssistantPopup::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(rect(), kCornerRadius, kCornerRadius);

    // Base dark background
    painter.fillPath(path, QColor(8, 12, 28, 220));

    // Dynamic glowing border when active
    if (m_isGlowActive) {
        float glowIntensity = 0.5f * (1.0f + std::sin(m_glowPhase));

        QColor glowColor;
        switch (m_currentState) {
            case OrbState::Listening:
                glowColor = QColor(56, 189, 248); // Cyan
                break;
            case OrbState::Thinking:
                glowColor = QColor(245, 158, 11); // Amber
                break;
            case OrbState::Executing:
                glowColor = QColor(168, 85, 247); // Violet
                break;
            default:
                glowColor = QColor(124, 58, 237); // Purple
                break;
        }

        // Outer glow layer (wide, soft)
        QPen outerGlowPen;
        outerGlowPen.setWidthF(3.0);
        outerGlowPen.setColor(QColor(glowColor.red(), glowColor.green(), glowColor.blue(),
                                      static_cast<int>(40 + 30 * glowIntensity)));
        painter.setPen(outerGlowPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);

        // Mid glow layer
        QPen midGlowPen;
        midGlowPen.setWidthF(2.0);
        midGlowPen.setColor(QColor(glowColor.red(), glowColor.green(), glowColor.blue(),
                                    static_cast<int>(80 + 60 * glowIntensity)));
        painter.setPen(midGlowPen);
        painter.drawPath(path);

        // Inner bright border
        QPen innerBorderPen;
        innerBorderPen.setWidthF(1.2);
        innerBorderPen.setColor(QColor(glowColor.red(), glowColor.green(), glowColor.blue(),
                                        static_cast<int>(120 + 80 * glowIntensity)));
        painter.setPen(innerBorderPen);
        painter.drawPath(path);
    } else {
        // Static border when idle
        QPen staticBorder;
        staticBorder.setWidthF(1.0);
        staticBorder.setColor(QColor(56, 189, 248, 60));
        painter.setPen(staticBorder);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }
}

void AssistantPopup::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QWidget *child = childAt(event->pos());
        // Allow dragging if clicked on background, header, labels, stats, or central widget
        // (Block dragging only on interactive controls like buttons, text fields, chips)
        bool isInteractive = qobject_cast<QPushButton*>(child) != nullptr ||
                             qobject_cast<QTextEdit*>(child) != nullptr ||
                             qobject_cast<ChatInputEdit*>(child) != nullptr;

        if (!isInteractive) {
            m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
            m_isDragging = false;
            event->accept();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void AssistantPopup::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton && !m_dragPosition.isNull()) {
        m_isDragging = true;
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}

void AssistantPopup::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        m_dragPosition = QPoint();
        event->accept();
    }
}

void AssistantPopup::showConsentCard(
    const QString &requestId,
    const QString &path,
    const QString &operation,
    const QString &reason,
    bool isHighRisk,
    bool requiresPassword,
    int tier)
{
    if (m_consentCard) {
        m_consentCard->deleteLater();
        m_consentCard = nullptr;
    }

    m_consentRequestId = requestId;

    m_consentCard = new QFrame(this);
    m_consentCard->setObjectName("ConsentCard");
    m_consentCard->setStyleSheet(R"(
        QFrame#ConsentCard {
            background: rgba(15, 23, 42, 0.95);
            border: 2px solid #f59e0b;
            border-radius: 12px;
            padding: 8px;
        }
    )");

    QVBoxLayout *cardLayout = new QVBoxLayout(m_consentCard);
    cardLayout->setContentsMargins(12, 10, 12, 10);
    cardLayout->setSpacing(6);

    QString badgeText = (tier == 2 || requiresPassword || isHighRisk)
        ? QString::fromUtf8("🛡️ Tier 2: Credential Gate Challenge")
        : QString::fromUtf8("⚠️ Tier 1: General Consent Required");
    QLabel *headerLabel = new QLabel(badgeText, m_consentCard);
    headerLabel->setStyleSheet(tier == 2 ? "color: #f59e0b; font-weight: bold; font-size: 13px;" : "color: #38bdf8; font-weight: bold; font-size: 13px;");
    cardLayout->addWidget(headerLabel);

    QLabel *detailsLabel = new QLabel(QString("Target: <b>%1</b> (%2)").arg(path, operation), m_consentCard);
    detailsLabel->setStyleSheet("color: #cbd5e1; font-size: 12px;");
    cardLayout->addWidget(detailsLabel);

    if (!reason.isEmpty()) {
        QLabel *reasonLabel = new QLabel(QString("Reason: %1").arg(reason), m_consentCard);
        reasonLabel->setStyleSheet("color: #94a3b8; font-size: 11px; font-style: italic;");
        cardLayout->addWidget(reasonLabel);
    }

    QLineEdit *pwdEdit = nullptr;
    if (requiresPassword) {
        pwdEdit = new QLineEdit(m_consentCard);
        pwdEdit->setEchoMode(QLineEdit::Password);
        pwdEdit->setPlaceholderText("Enter operator password (e.g. secret123)...");
        pwdEdit->setStyleSheet(R"(
            QLineEdit {
                background: rgba(30, 41, 59, 0.8);
                border: 1px solid rgba(245, 158, 11, 0.5);
                border-radius: 6px;
                color: #f8fafc;
                padding: 4px 8px;
                font-size: 12px;
            }
        )");
        cardLayout->addWidget(pwdEdit);
    }

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);

    QPushButton *btnOnce = new QPushButton("Allow Once", m_consentCard);
    btnOnce->setCursor(Qt::PointingHandCursor);
    btnOnce->setStyleSheet("background: rgba(56, 189, 248, 0.2); border: 1px solid #38bdf8; border-radius: 6px; color: #38bdf8; font-weight: 600; padding: 4px 10px;");
    btnLayout->addWidget(btnOnce);

    QPushButton *btnSession = new QPushButton("Allow Session", m_consentCard);
    btnSession->setCursor(Qt::PointingHandCursor);
    btnSession->setStyleSheet("background: rgba(168, 85, 247, 0.2); border: 1px solid #a855f7; border-radius: 6px; color: #a855f7; font-weight: 600; padding: 4px 10px;");
    btnLayout->addWidget(btnSession);

    QPushButton *btnDeny = new QPushButton("Deny", m_consentCard);
    btnDeny->setCursor(Qt::PointingHandCursor);
    btnDeny->setStyleSheet("background: rgba(239, 68, 68, 0.2); border: 1px solid #ef4444; border-radius: 6px; color: #ef4444; font-weight: 600; padding: 4px 10px;");
    btnLayout->addWidget(btnDeny);

    cardLayout->addLayout(btnLayout);

    QWidget *centralWidget = findChild<QWidget*>("CentralWidget");
    if (centralWidget && centralWidget->layout()) {
        QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(centralWidget->layout());
        if (mainLayout) {
            mainLayout->insertWidget(mainLayout->count() - 2, m_consentCard);
        }
    }

    auto handleDecision = [this, requestId, pwdEdit](bool approved, int scope) {
        QString pwd = (pwdEdit ? pwdEdit->text() : QString());
        emit consentDecided(requestId, approved, scope, pwd, approved ? "Approved via UI" : "Denied via UI");
        if (m_consentCard) {
            m_consentCard->deleteLater();
            m_consentCard = nullptr;
        }
    };

    connect(btnOnce, &QPushButton::clicked, this, [handleDecision]() {
        handleDecision(true, 0); // 0 = Once
    });
    connect(btnSession, &QPushButton::clicked, this, [handleDecision]() {
        handleDecision(true, 1); // 1 = Session
    });
    connect(btnDeny, &QPushButton::clicked, this, [handleDecision]() {
        handleDecision(false, 0);
    });
}
