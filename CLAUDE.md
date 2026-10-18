# CLAUDE.md - Automatron Assistant

Windows desktop Agentic AI Voice Assistant built with **C++20**, **Qt 6**, and **CMake**.

---

## 🛠️ Build & Development Commands

### Prerequisites
- **OS**: Windows 10/11 (x64)
- **Compiler**: MSVC (Visual Studio 2019/2022) with C++20 support
- **Build System**: CMake 3.21+
- **Qt Version**: Qt 6.4+ (Components: `Core`, `Gui`, `Widgets`, `Network`, `Multimedia`)

### Configure & Build
```cmd
# Create build directory and configure (adjust Qt path if needed)
mkdir build && cd build
cmake -G "Visual Studio 16 2019" -DCMAKE_PREFIX_PATH="C:/Qt/6.4.0/msvc2019_64" ..

# Build Release binary
cmake --build . --config Release

# Build Debug binary
cmake --build . --config Debug
```

### Run Executable
```cmd
# Release binary output location
.\bin\Voice-AI.exe
```

---

## 🏛️ Architecture & Core Rules

### 1. Threading Model
- **Main Thread (GUI)**: Owns and manages all widgets (`FloatingOrb`, `AssistantPopup`, `NotificationManager`). **NEVER** instantiate, modify, or invoke GUI widgets directly from background threads.
- **Worker Thread (`QThread`)**: `AgentWorker` runs on a dedicated worker thread via `worker->moveToThread(&workerThread)`.
- **Cross-Thread Communication**: Exclusively via Qt queued signals and slots (`Qt::QueuedConnection`) or `QMetaObject::invokeMethod(worker, "method", Qt::QueuedConnection, ...)`.

### 2. State Machine (`AgentState`)
- States: `Idle` ➔ `Listening` ➔ `Thinking` ➔ `Executing` ➔ `Idle`
- Emergency State: `Terminated` (reachable from any state via Kill Switch / `requestKill()`).
- All state changes must be broadcast via `AgentWorker::stateChanged(AgentState)` to keep `FloatingOrb` and `AssistantPopup` synchronized.

### 3. Cooperative Cancellation
- Implemented with C++20 atomic primitives via `CancellationToken` (`std::atomic_bool` with `std::memory_order_acquire` / `release`).
- Components (`AudioManager`, `ISpeechToText`, `ILLMClient`, `ToolExecutor`) must accept and periodically check `token.is_cancellation_requested()` to bail out cleanly without hanging threads.

### 4. UI Guidelines & Metrics

#### FloatingOrb (Enhanced 3D Glass Orb)
- **Size**: 80×80 px frameless window with `Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint`.
- **Position**: Bottom-right within `QScreen::availableGeometry()` with a 24px margin.
- **Visual Layers**:
  - **Layer 0**: Ultra-wide ambient glow (40px radius, active states only)
  - **Layer 1**: Mid-range breathing halo (28px base radius)
  - **Layer 2**: Dual-orbit quantum rings (24.5px & 18px) with dash patterns
  - **Layer 3**: Dual photon particles orbiting at 0° and 180° offsets
  - **Layer 4**: Inner accent halo (20.5px)
  - **Layer 5**: 3D glass core sphere with radial gradient
  - **Layer 6**: Dynamic inner glow (breathing effect)
  - **Layer 7**: Dual specular highlights (main + rim light)
- **Colors by State**:
  - **Idle**: Deep AI Purple (#7c3aed) with subtle glow
  - **Listening**: Electric Cyan (#38bdf8)
  - **Thinking**: Solar Amber (#fbbf24)
  - **Executing**: Cyber Violet (#a855f7)
  - **Terminated**: Alert Red (#ef4444)
- **Animation**: 30 FPS (33ms intervals) only when active; **zero CPU usage when Idle** (timer stops completely).
- **Physics**: Independent `m_pulsePhase`, `m_rotationAngle`, and `m_glowPhase` for multi-layered animation complexity.

#### AssistantPopup (Glassmorphic Chat Interface)
- **Size**: 440px wide, max 580px height, 22px rounded border with `Qt::WA_TranslucentBackground`.
- **Focus Behavior**: Uses `Qt::WA_ShowWithoutActivating` to avoid stealing focus; focus is gained only when user clicks into `ChatInputEdit`.
- **Dynamic Glowing Border** (Active States Only):
  - **3-Layer Glow System**: Outer halo (40% opacity) → Mid glow (80% opacity) → Inner core (120% opacity)
  - **Animation**: 25 FPS (40ms intervals) with sine wave modulation (`m_glowPhase`)
  - **State Colors**: Matches orb state colors (Cyan/Amber/Violet)
  - **Idle State**: Static cyan border at 60 opacity (no animation)
  
- **Header Components**:
  - Status indicator dot (● cyan)
  - Title: **"Automatron"**
  - State badge with emoji indicators:
    - 🎤 Listening (Cyan gradient)
    - 🧠 Thinking (Amber gradient)
    - ⚡ Executing (Violet gradient)
    - ⛔ Terminated (Red)
  - Minimize button (−) — frosted glass style
  - Kill Switch button (■) — solid red gradient with 2px red border, tooltip: "Kill Switch — Terminate Automatron"

- **Chat Transcript** (Native Qt Text Formatting):
  - **User Messages**: Right-aligned with `Qt::AlignRight`
    - Avatar: 👤 (cyan gradient badge)
    - Label: "You" in cyan
    - Background: Dark slate (`rgba(30, 41, 59, 180)`)
    - Text: White (#f1f5f9), 12pt
  - **AI Messages**: Left-aligned with `Qt::AlignLeft`
    - Avatar: 🤖 (purple gradient badge)
    - Label: "Automatron" in light purple
    - Background: Cyber violet (`rgba(88, 28, 135, 120)`)
    - Text: Light gray (#e2e8f0), 12pt
  - **System Messages**: Center-aligned, italic, muted gray (#64748b), 10pt
  - **Streaming Support**: Real-time chunk appending with proper cursor positioning

- **Audio Visualizer** (5-Bar Gradient Display):
  - Active: Cyan-to-Indigo gradient bars with dynamic height modulation
  - Idle: Static gray bars with subtle gradient
  - Animation: 25 FPS (40ms intervals)
  - Bar dimensions: 4px width, 8px spacing, rounded corners (2px)
  - Glow effect: Subtle outer halo on active bars

- **Action Chips** (`Summarize`, `Code`, `Search`, `Stop`):
  - Glassmorphic frosted background (`rgba(51, 65, 85, 0.3)`)
  - Hover: Cyan glow border + text color shift to cyan
  - Pressed: Deeper cyan background with white text
  - Border radius: 8px

- **Input Area**:
  - Chat input: Frosted dark glass with cyan focus border
  - Send button (➤): Large gradient icon button (42×42px)
    - Background: Cyan-to-Indigo gradient
    - Hover: Full opacity bright gradient
    - Pressed: Deep blue state

### 5. Windows Integration
- **Global Hotkey (`Ctrl+Shift+K`)**: Handled by `GlobalHotkeyManager` + `GlobalHotkeyFilter` (`QAbstractNativeEventFilter`) listening for `WM_HOTKEY` with `MOD_NOREPEAT`.
  - Note: Keep `GlobalHotkeyFilter` as a pure C++ class inheriting from `QAbstractNativeEventFilter` (no `Q_OBJECT` macro) to prevent MOC multiple-inheritance conflicts.
- **Linked Windows Libraries**: `user32`, `shell32`, `powrprof`.

---

## 📁 Source Tree Structure

```
src/
├── main.cpp                   # Application lifecycle, thread wiring & signal routing
├── FloatingOrb.h/.cpp         # 80x80 enhanced 3D glass orb with multi-layer glow
├── AssistantPopup.h/.cpp      # Glassmorphic chat UI with dynamic glow border
├── AgentWorker.h/.cpp         # Background QThread worker, cancellation & state machine
├── AudioManager.h/.cpp        # Audio capture abstraction & level metering (20 FPS)
├── ISpeechToText.h            # Speech-to-text pure interface
├── MockSpeechToText.h/.cpp    # Mock STT implementation (simulates 2.5s transcription)
├── ILLMClient.h               # LLM client interface with streaming support
├── MockLLMClient.h/.cpp       # Mock streaming LLM client (10 chunks @ 150ms intervals)
├── ToolExecutor.h/.cpp        # Safe tool executor (get_current_time, list_files, open_url)
├── HotkeyMonitor.h/.cpp       # Windows RegisterHotKey & native event filter
├── NotificationManager.h/.cpp # Desktop toast/notification manager
└── WindowsIntegration.h/.cpp  # Windows battery and power status helpers
```

---

## 💻 Code Style & Conventions

- **Standard**: C++20 (`set(CMAKE_CXX_STANDARD 20)`).
- **Naming**:
  - Types/Classes: `PascalCase` (e.g., `AgentWorker`, `FloatingOrb`)
  - Methods/Functions: `camelCase` (e.g., `requestKill`, `updatePositionAboveOrb`)
  - Member Variables: `m_camelCase` (e.g., `m_cancellationToken`, `m_currentState`, `m_glowPhase`)
  - Constants/Enums: `PascalCase` or `kCamelCase` (e.g., `AgentState::Listening`, `kHotkeyId`)
- **Qt Memory Management**: Use Qt parent-child hierarchy for `QObject` lifecycles where possible. For top-level widgets, explicitly manage lifecycle and thread shutdowns in `main.cpp` using `QApplication::aboutToQuit`.
- **Safety**:
  - No arbitrary shell/command execution in `ToolExecutor` (strictly allowlisted tools).
  - Explicit string bounds checks before Win32 buffer copies (e.g., `wcscpy_s`).
  - Always register custom Qt meta types before launching `QApplication` (`qRegisterMetaType<AgentState>("AgentState")`).

---

## 🎨 UI Design Enhancements (2026-10-01)

### Visual Identity
- **App Name**: **Automatron** (all UI references updated)
- **Color Palette**:
  - Listening: Electric Cyan (#38bdf8)
  - Thinking: Solar Amber (#fbbf24)
  - Executing: Cyber Violet (#a855f7)
  - Idle: AI Purple (#7c3aed)
  - Terminate: Alert Red (#ef4444)

### Key Features
1. **Multi-Layer Orb Glow**: 7 rendering layers with independent animation phases
2. **Dynamic Window Border**: 3-layer pulsing glow synchronized with AI state
3. **Native Text Formatting**: Proper right/left alignment using `QTextBlockFormat`
4. **Compact Message Bubbles**: Emoji avatars with sender labels
5. **5-Bar Audio Visualizer**: Gradient-filled bars with glow effects
6. **Kill Switch Clarity**: Red stop icon (■) with gradient background, clearly distinct from minimize

### Performance
- **Idle CPU**: 0% (all animation timers stopped)
- **Active Animation**: ~0.1-0.3% CPU (30 FPS orb + 25 FPS window glow + 25 FPS visualizer)

---

**Last Updated**: 2026-10-01
**Version**: 1.1.0 (Enhanced UI with Dynamic Glow Effects)
