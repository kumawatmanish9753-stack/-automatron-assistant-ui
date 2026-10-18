# Runtime Instructions

## Starting the Application

### Standard Launch
```bash
cd "D:\Omniroute\UI-UX design\build\bin"
OmnirouteAssistant.exe
### From Visual Studio
1. Set `OmnirouteAssistant` as startup project
2. Press F5 or Debug → Start Debugging
3. The floating orb will appear in the bottom-right corner

### First Launch
On first run:
1. A 58×58 pixel cyan/gray orb appears in bottom-right corner
2. Press **Ctrl+Shift+K** to open the AssistantPopup
3. Or click the orb directly to activate

## User Interactions

### Hotkey Control
- **Ctrl+Shift+K**: Toggle AssistantPopup visibility
  - If popup is hidden: shows popup and starts listening (if idle)
  - If popup is visible: hides popup

### Floating Orb
- **Left-click**: Toggle popup visibility and start listening
- **Right-click**: Context menu (reserved for future use)
- **Drag**: Reposition orb anywhere on screen (respects multi-monitor boundaries)

### Assistant Popup

#### Layout
```
┌─────────────────────────────────────┐  ← 360px wide, max 450px height
│ [State Badge]  CPU:XX% RAM:XX%  [◆◆◆] │  ← State label, system stats, 3-bar visualizer
├─────────────────────────────────────┤
│ User: Hello, what time is it?        │
│ Assistant: The time is 3:45 PM...    │  ← Transcript with auto-scroll
│                                      │
│                                      │
├─────────────────────────────────────┤
│ [Type message here...]               │  ← ChatInputEdit
│ Enter=send, Shift+Enter=newline      │
│                                      │
├─────────────────────────────────────┤
│ [Summarize] [Search] [Open Files]  │  ← Quick-action chips
│           [KILL SWITCH] [Resume]    │  ← Control buttons
└─────────────────────────────────────┘
```

#### Chat Input
- **Type message**: Any question or command
- **Enter**: Submit prompt to agent
- **Shift+Enter**: Insert newline in message
- **Click input field**: Activates popup window (brings to focus)

#### Quick-Action Chips
- **Summarize**: Submits "Summarize" to the agent
- **Search**: Submits "Search" to the agent
- **Open Files**: Submits "Open Files" to the agent
- **Stop**: Resume agent operation (see Kill Switch below)

#### Kill Switch / Resume Button
- **Red "KILL SWITCH"**: When agent is active, press to:
  - Stop all audio capture
  - Cancel speech-to-text mid-stream
  - Terminate LLM response streaming
  - Cancel in-flight tool execution
  - Show desktop notification "Agent stopped"

- **Green "RESUME AGENT"**: When agent is killed, press to:
  - Reset to Idle state
  - Allow new prompts to be submitted

## State Machine

### Orb State Indicators

| State | Color | Animation | Meaning |
|-------|-------|-----------|---------|
| Idle | Slate Gray | Stopped | Ready for input |
| Listening | Cyan | Pulsing | Capturing audio / waiting for speech |
| Thinking | Amber | Rotating | Processing prompt with LLM |
| Executing | Amber | Rotating | Running a tool (get_current_time, list_files, etc.) |
| Terminated | Red | Stopped | Kill switch pressed, waiting to resume |

### State Transitions
```
Idle ──→ Listening ──→ Thinking ──→ Executing ──→ Idle
          ↓            ↓           ↓           ↓
      (Terminated from any state via Kill Switch)
          ←────────────←─────────────←───────────
```

## Audio Visualization

The 3-bar AudioVisualizerWidget shows:
- **Visual feedback**: Real-time audio levels while listening
- **Phase animation**: Smooth scaling effect even when audio is silent
- **Updates**: 20 FPS (50ms interval) for smooth appearance
- **Test mode**: Generates synthetic sine-wave pattern when microphone unavailable

## System Integration

### Global Hotkey
- **Ctrl+Shift+K**: Registered globally with Windows
  - Works even when other applications have focus
  - Toggles popup visibility without stealing focus
  - Does not auto-repeat on key hold (MOD_NOREPEAT)

### Desktop Notifications
- **Kill Switch pressed**: Shows "Agent stopped" notification
- Appears as a system toast (Windows 10/11)

### Multi-Monitor Support
- Orb positioned on screen with work area boundaries respected
- Popup positioned above orb, adjusted if it would go off-screen
- Uses QScreen::availableGeometry() for accurate monitor detection

### High-DPI Support
- Qt 6 automatic high-DPI scaling
- All UI elements render crisply on 125%, 150%, 200% displays
- Coordinates in logical pixels (Qt handles physical pixel conversion)

## Mock Implementations (Current State)

### Audio Capture
- **Current**: Generates synthetic sine-wave audio levels
- **Real**: Connects to QAudioSource from system microphone
- **Behavior**: Emits audio levels at 20 FPS, scales 0.0–1.0

### Speech-to-Text
- **Current**: Simulates 2.5-second listening session
  - Emits partial transcripts: "Listening.", "Listening..", "Listening..."
  - Final: "Mock transcription output"
- **Real**: Would connect to Whisper API, Google Cloud Speech-to-Text, Azure Speech Services, etc.

### LLM Client
- **Current**: Simulates streaming response in 10 chunks
  - Response: "This is the Omniroute AI Assistant. I'm designed to help you..." (predefined)
  - Chunks emit at 150ms intervals
- **Real**: Would connect to OpenAI GPT-4, Anthropic Claude, local LLM server, etc.

### Registered Tools
- **get_current_time**: Returns system datetime
- **list_files**: Lists directory contents with cancellation support
- **open_url**: Opens HTTP/HTTPS URLs via QDesktopServices

All tools cooperate with cancellation tokens, so Kill Switch cleanly terminates file listings and other long operations.

## Debugging

### Enable Debug Output
The application outputs to Qt debug console. To capture:

**Visual Studio**:
- Debug → Windows → Output (or Alt+2)
- Shows all `qDebug() <<` statements

**Qt Creator**:
- View → Panels → Application Output
- Shows debug messages automatically

### Common Debug Messages
```
"Global hotkey Ctrl+Shift+K registered successfully."
"[Notification] Agent stopped : All cancellable agent operations..."
"Listening started"
"STT partial transcript: Listening.."
"LLM streaming: chunk 1/10"
```

### Verifying Thread Safety
The application is designed for thread safety:
- GUI widgets (Orb, Popup) only accessed from main thread
- Worker runs on separate QThread
- All communication via queued signals/slots
- CancellationToken uses atomic_bool with acquire/release semantics

To verify: Press Ctrl+Shift+K rapidly while worker is active. The UI should remain responsive and not crash.

## Stopping the Application

### Normal Exit
- Click window close button (X)
- Or press Alt+F4
- Application cleanup:
  1. All signals stop propagating
  2. Worker thread receives quit() signal
  3. Waits for worker to finish (wait())
  4. Global hotkey unregistered
  5. Application terminates

### Force Exit
- If the application hangs (unlikely with cooperative cancellation):
  - Press Ctrl+C in the terminal
  - Or use Task Manager → End Task

## Performance Notes

### CPU Usage
- **Idle state**: ~0% (animation timer stopped)
- **Active listening**: ~2-5% (audio levels at 20 FPS, phase animation at 30 FPS)
- **LLM streaming**: ~5-10% (receiving chunks, updating UI)

### Memory Usage
- **Initial**: ~60-80 MB (Qt runtime + application)
- **After streaming LLM**: ~90-110 MB (transcript buffer grows)
- **Multi-hour sessions**: Transcript buffer may grow to 200+ MB (clear via UI in future versions)

### Battery Impact
- **Idle (orb hidden)**: Minimal (hotkey monitoring only, ~1% drain)
- **Active**: ~5-15% drain (audio capture, UI updates, network if using real LLM)

### Multi-Monitor Considerations
- Orb repositions correctly when monitor configuration changes
- On 4K displays with high-DPI: UI scales proportionally, remains readable
- On ultra-wide monitors: Positioned in bottom-right work area of primary monitor

## Troubleshooting Runtime Issues

### Orb Not Appearing
- Check Windows taskbar: might be minimized
- Try Ctrl+Shift+K to toggle popup (orb is always active)
- Verify: Window is created with Qt::Tool | Qt::FramelessWindowHint flags

### Hotkey Not Working
- Verify: Ctrl+Shift+K is not already registered by another app
- Check: Application is running with appropriate privileges
- Debug: Look for "Failed to register global hotkey" in debug output

### Popup Cuts Off Screen
- The application auto-adjusts popup position if it would go off-screen
- Try dragging the orb closer to center of screen
- Multi-monitor: Ensure primary monitor is set correctly in Windows

### No Audio Visualization
- Currently using mock audio (synthetic sine wave)
- Real microphone implementation will show actual levels
- Verify: Mock is generating levels by watching 3-bar visualizer pulse

### LLM Response Not Appearing
- Currently using mock response (predefined text)
- Real LLM integration will stream actual responses
- Verify: Transcript shows mock response appearing in 10 chunks

### Kill Switch Not Responding
- If agent is not actively running, button toggles to RESUME AGENT (green)
- Press green button to reset state, then try again
- If truly stuck: Close and restart application

## Next Steps for Real Integration

1. **Audio**: Replace MockAudioManager with real QAudioSource
2. **STT**: Integrate Whisper API or cloud STT service
3. **LLM**: Connect to OpenAI API or local model server
4. **Tools**: Add real implementations (file operations, URL launching, etc.)
5. **Notifications**: Use Windows Toast API for proper notifications
6. **Persistence**: Add settings, history, user preferences storage
