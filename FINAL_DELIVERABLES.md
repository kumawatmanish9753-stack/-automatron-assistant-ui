# Final Project Deliverables

## 1. Final Project Tree

```
D:\Omniroute\UI-UX design\
├── CMakeLists.txt                 # CMake build configuration
├── PROJECT_STRUCTURE.md           # Detailed file directory breakdown
├── BUILD_INSTRUCTIONS.md          # Compilation and setup steps
├── RUNTIME_INSTRUCTIONS.md        # Operation and user manual
├── FINAL_DELIVERABLES.md          # Architectural summary and audit report
└── src/
    ├── main.cpp                   # Application lifecycle & thread coordination
    ├── FloatingOrb.h/.cpp         # 58×58 frameless floating orb with state animations
    ├── AssistantPopup.h/.cpp      # 360px popup with transcript, input & visualizer
    ├── AgentWorker.h/.cpp         # Background QThread worker & state machine
    ├── AudioManager.h/.cpp        # Audio capture abstraction & level metering
    ├── ISpeechToText.h            # Speech-to-text interface
    ├── MockSpeechToText.h/.cpp    # Mock STT simulator
    ├── ILLMClient.h               # Streaming LLM client interface
    ├── MockLLMClient.h/.cpp       # Mock streaming LLM simulator
    ├── ToolExecutor.h/.cpp        # Cooperative cancellation-safe tool execution
    ├── HotkeyMonitor.h/.cpp       # Native Windows global hotkey handling (Ctrl+Shift+K)
    ├── NotificationManager.h/.cpp # Desktop notification interface
    └── WindowsIntegration.h/.cpp  # Windows platform power & display utilities
```

---

## 2. Build Instructions

### Prerequisites
- **OS**: Windows 10/11 (64-bit)
- **Compiler**: MSVC from Visual Studio 2019/2022 (with C++20 support)
- **Build System**: CMake 3.21 or newer
- **Framework**: Qt 6.4+ (Qt6::Core, Qt6::Gui, Qt6::Widgets, Qt6::Network, Qt6::Multimedia)

### Build Commands
```cmd
cd "D:\Omniroute\UI-UX design"
mkdir build
cd build
cmake -G "Visual Studio 16 2019" -DCMAKE_PREFIX_PATH="C:/Qt/6.4.0/msvc2019_64" ..
cmake --build . --config Release
```
*Note: The resulting binary is generated at `build/bin/Release/OmnirouteAssistant.exe`.*

---

## 3. Runtime Instructions

- **Launch**: Run `OmnirouteAssistant.exe`. The floating orb will anchor to the bottom-right screen work area.
- **Global Hotkey**: Press `Ctrl+Shift+K` from any active application to toggle the popup and start listening.
- **Orb Interaction**: Left-click to open/close popup; drag to reposition across screens.
- **Chat & Actions**: Type queries into the popup text area (Enter to send, Shift+Enter for newline) or click quick action chips.
- **Kill Switch**: Press the red `KILL SWITCH / ABORT` button to immediately abort speech listening, LLM streaming, or tool execution. Click the green `RESUME AGENT` button to reset to the Idle state.

---

## 4. Architecture Summary

```
┌─────────────────────────────────────────────────────────────┐
│                       MAIN UI THREAD                        │
│                                                             │
│   ┌──────────────────┐           ┌──────────────────────┐   │
│   │   FloatingOrb    │           │    AssistantPopup    │   │
│   │  (58x58 Widget)  │           │ (Chat/Transcript UI) │   │
│   └────────▲─────────┘           └──────────▲───────────┘   │
│            │                                │               │
│            └───────────────┬────────────────┘               │
│                            │ (Queued Signal / Slot)         │
├────────────────────────────┼────────────────────────────────┤
│                            │ (Cross-Thread Boundary)        │
│                       WORKER THREAD                         │
│                            ▼                                │
│                   ┌──────────────────┐                      │
│                   │   AgentWorker    │                      │
│                   │ (State Machine)  │                      │
│                   └────────┬─────────┘                      │
│          ┌─────────────────┼─────────────────┐              │
│          ▼                 ▼                 ▼              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │
│  │ AudioManager │  │ ISpeechToText│  │ ILLMClient   │       │
│  └──────────────┘  └──────────────┘  └──────────────┘       │
│                            │                                │
│                            ▼                                │
│                    ┌──────────────┐                         │
│                    │ ToolExecutor │                         │
│                    └──────────────┘                         │
└─────────────────────────────────────────────────────────────┘
```

- **Threading Model**: Strictly non-blocking UI. The GUI remains on the primary thread while `AgentWorker` and all operational logic run inside a dedicated `QThread` via `moveToThread()`. Cross-thread data exchange uses thread-safe Qt queued connections (`Qt::QueuedConnection`).
- **State Machine**: States cycle cleanly across `Idle`, `Listening`, `Thinking`, and `Executing`. Any state can transition instantly to `Terminated` upon a kill switch event.
- **Cancellation Propagation**: Uses a C++20 `CancellationToken` (built on `std::atomic_bool` with acquire/release semantics). Cancellation signals propagate directly across audio capture, speech transcription, LLM token streaming, and directory/tool loops.
- **Resource Efficiency**: Active rendering (orb rotation, pulsing, audio visualizer) runs at 30 FPS / 20 FPS, and the animation timer is explicitly stopped during `Idle` to ensure zero idle CPU usage.

---

## 5. Current Mock Components

1. **`MockSpeechToText`**: Simulates speech transcription over 2.5 seconds, emitting incremental partial transcriptions every 400ms before sending a final transcript.
2. **`MockLLMClient`**: Emits simulated streaming token chunks every 150ms over 10 intervals.
3. **`AudioManager` (Mock Mode)**: Emits synthetic sine-wave audio level floats at 20 FPS to drive the 3-bar audio visualizer when no live microphone input is selected.

---

## 6. Components That Need a Real Provider

1. **Speech-to-Text (`ISpeechToText`)**:
   - Needs integration with a live engine such as OpenAI Whisper (local whisper.cpp or cloud API), Azure Speech Service, or Google Cloud STT.
2. **LLM Provider (`ILLMClient`)**:
   - Needs integration with an OpenAI, Anthropic Claude, or local Ollama/vLLM streaming endpoint via `QNetworkAccessManager` with Server-Sent Events (SSE) parsing.
3. **Audio Capture (`AudioManager`)**:
   - Live microphone buffer streaming using `QAudioSource` and `QMediaDevices` routed into the active STT client.
4. **Desktop Notifications (`NotificationManager`)**:
   - Integration with the Windows 10/11 WinRT Toast Notification APIs or `Shell_NotifyIconW` with an AppUserModelID for interactive toasts.

---

## 7. Known Limitations

- **System Power Hooks**: Battery status query via `GetSystemPowerStatus()` is implemented as a utility function; automatic low-power mode throttling logic can be hooked into future background timers.
- **Tool Sandbox**: `ToolExecutor` is strictly restricted to registered tools (`get_current_time`, `list_files`, `open_url`) for safety. Arbitrary shell or CLI execution is intentionally not supported.
- **Transcript Retention**: Chat transcript is maintained in-memory in the UI text edit; persistence across application restarts requires an SQLite/JSON storage backend.

---

## 8. Windows-Specific Requirements

- **Global Hotkey Registration**: Uses Windows native `RegisterHotKey()` with `MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT` and `VK_K`. A custom `QAbstractNativeEventFilter` intercepts `WM_HOTKEY` without stealing application focus.
- **Translucent Top-Level Windows**: Popup and Orb use `Qt::FramelessWindowHint`, `Qt::Tool`, and `Qt::WA_TranslucentBackground` combined with `Qt::WA_ShowWithoutActivating` to avoid window focus conflicts.
- **Linked Windows Libraries**: `user32.lib` (hotkeys and window management), `shell32.lib` (shell execution & notification tray), and `powrprof.lib` (power status queries).
