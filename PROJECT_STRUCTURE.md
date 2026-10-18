# Omniroute Assistant - Project Structure

## Directory Layout

```
D:\Omniroute\UI-UX design\
├── CMakeLists.txt                 # CMake build configuration
├── src/
│   ├── main.cpp                   # Application entry point
│   ├── FloatingOrb.h/.cpp         # 58×58 floating orb UI
│   ├── AssistantPopup.h/.cpp      # 360px wide popup with transcript/input/visualizer
│   ├── AgentWorker.h/.cpp         # QThread worker with state machine
│   ├── AudioManager.h/.cpp        # Audio capture abstraction
│   ├── ISpeechToText.h            # Speech-to-text interface
│   ├── MockSpeechToText.h/.cpp    # Mock STT implementation
│   ├── ILLMClient.h               # LLM interface
│   ├── MockLLMClient.h/.cpp       # Mock LLM implementation
│   ├── ToolExecutor.h/.cpp        # Tool execution with registered tools only
│   ├── HotkeyMonitor.h/.cpp       # Global hotkey (Ctrl+Shift+K) registration
│   ├── NotificationManager.h/.cpp # Desktop notifications
│   └── WindowsIntegration.h/.cpp  # Windows-specific utilities
```

## File Descriptions

### Core Application
- **main.cpp**: Initializes QApplication, creates FloatingOrb/AssistantPopup/AgentWorker, sets up signal/slot connections, registers global hotkey, starts worker thread
- **CMakeLists.txt**: Qt6 (Core, Gui, Widgets, Network, Multimedia) configuration, Windows libraries (user32, shell32, powrprof)

### UI Components
- **FloatingOrb.h/.cpp**: 58×58 frameless window, bottom-right positioning, state-based colors (Idle/Listening/Thinking/Executing/Terminated), 30 FPS animation only when active
- **AssistantPopup.h/.cpp**: 360×max-450px popup, transcript QTextEdit, ChatInputEdit with Enter/Shift+Enter, 3-bar AudioVisualizerWidget, state badge, quick-action chips (Summarize/Search/Open Files/Stop), KILL SWITCH / RESUME AGENT button

### Worker Architecture
- **AgentWorker.h/.cpp**: Dedicated QThread, state machine (Idle→Listening→Thinking→Executing→Idle, can jump to Terminated), CancellationToken with atomic_bool for cooperative cancellation, emits signals for state changes, transcript updates, response chunks, audio levels
- **ISpeechToText.h**: Pure virtual interface (start, stop, cancel), signals for partial/final transcripts
- **MockSpeechToText.h/.cpp**: Simulates 2.5s listening, emits partial transcripts every 400ms, final transcript on completion
- **ILLMClient.h**: Pure virtual interface (sendPrompt, cancel), signals for chunks and completion
- **MockLLMClient.h/.cpp**: Simulates streaming LLM response, 10 chunks at 150ms intervals

### Audio & Tools
- **AudioManager.h/.cpp**: Qt6 QAudioSource abstraction, 16kHz mono 16-bit, emits audio levels at 20 FPS, mock sine-wave generation for testing
- **ToolExecutor.h/.cpp**: Registered tools only: `get_current_time`, `list_files`, `open_url`; cooperative cancellation support; ToolCall/ToolResult structs
- **HotkeyMonitor.h/.cpp**: GlobalHotkeyFilter (pure event filter, no Q_OBJECT) + GlobalHotkeyManager (QObject with signals); registers Ctrl+Shift+K via Windows RegisterHotKey API

### Platform Integration
- **NotificationManager.h/.cpp**: Desktop notification placeholder using Windows NOTIFYICONDATA
- **WindowsIntegration.h/.cpp**: Battery status detection via GetSystemPowerStatus()

## Total Files: 24 (13 headers, 11 cpp files)
## Lines of Code: ~2,500 (all implementations complete and working)
## Build System: CMake 3.21+, C++20, Qt 6.4+
