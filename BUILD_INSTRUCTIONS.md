# Build Instructions

## Prerequisites

### Windows Environment
- Windows 10 or later (Visual Studio Build Tools 2019 or later)
- CMake 3.21 or later
- Qt 6.4 or later (with development libraries)

### Required Tools
```
Visual Studio Build Tools 2019+  (for MSVC compiler and Windows SDK)
CMake 3.21+                       (from https://cmake.org/download/)
Qt 6.4+                           (from https://www.qt.io/download-open-source)
```

### Windows Libraries
The following are linked automatically via CMake for Windows:
- `user32.lib`      - Windows API (RegisterHotKey, message handling)
- `shell32.lib`     - Shell integration (notifications)
- `powrprof.lib`    - Power status queries

## Build Steps

### 1. Configure CMake
```bash
cd "D:\Omniroute\UI-UX design"
mkdir build
cd build
cmake -G "Visual Studio 16 2019" -DCMAKE_PREFIX_PATH="C:/Qt/6.4.0/msvc2019_64" ..
```

**Note**: Adjust the Qt path to match your Qt 6 installation directory.

### 2. Build the Project
```bash
cmake --build . --config Release
```

Or use Visual Studio directly:
```bash
start OmnirouteAssistant.sln
# Then build Release configuration from Visual Studio GUI
```

### 3. Verify Output
The executable will be in:
```
build/bin/OmnirouteAssistant.exe
```

## CMake Configuration Details

**C++ Standard**: C++20 (required for std::atomic, std::optional, and structured bindings)

**Qt6 Components**:
- Qt6::Core      - Threading, signals/slots, core utilities
- Qt6::Gui       - Native event filters, high-DPI support
- Qt6::Widgets   - QApplication, QWidget derivatives
- Qt6::Network   - Network utilities (reserved for future LLM APIs)
- Qt6::Multimedia - Audio input, QAudioSource

**Compiler Definitions** (Windows):
- `UNICODE`  - Unicode API usage
- `_UNICODE` - Wide-character support

## Troubleshooting Build Issues

### CMake Not Finding Qt6
```bash
# Set Qt path explicitly
cmake -DCMAKE_PREFIX_PATH="C:/Qt/6.4.0/msvc2019_64" ..
```

### Linker Errors for Windows Libraries
These should link automatically on Windows. If not, verify:
1. Visual Studio Build Tools installed with Windows SDK
2. CMake version is 3.21 or later
3. Windows 10 SDK is available

### C++20 Compiler Errors
Verify MSVC is C++20 capable (VS2019 Update 10 or later):
```bash
cmake --system-information | grep "CXX_COMPILER"
```

### Q_OBJECT / Meta-Object Compiler Errors
The project uses `CMAKE_AUTOMOC ON` which automatically runs the Qt meta-object compiler. If you see errors:
1. Delete the `build` directory
2. Run `cmake ..` again to regenerate build files

## Clean Build

```bash
cd "D:\Omniroute\UI-UX design"
rm -r build              # or: rmdir build /s /q (Windows cmd)
mkdir build
cd build
cmake -G "Visual Studio 16 2019" -DCMAKE_PREFIX_PATH="C:/Qt/6.4.0/msvc2019_64" ..
cmake --build . --config Release
```

## Incremental Build

After making code changes:
```bash
cd "D:\Omniroute\UI-UX design\build"
cmake --build . --config Release
```

This automatically re-runs MOC, RCC, and UIC as needed.

## IDE Integration

### Visual Studio
1. Open the generated `OmnirouteAssistant.sln` in Visual Studio
2. Set startup project to `OmnirouteAssistant`
3. Build → Build Solution (or F7)

### Qt Creator
1. Open `CMakeLists.txt` as a project
2. Configure kit with MSVC 2019 and Qt 6.4
3. Build project with Ctrl+B

## Build Output

**Release build**:
- Executable: `build/bin/OmnirouteAssistant.exe` (~3-5 MB)
- All dependencies must be copied or linked at runtime

**To run from Visual Studio**:
- Executable is automatically found in build output directory
- Qt libraries must be in PATH or in the same directory

## Deployment

To distribute the application:
1. Copy `OmnirouteAssistant.exe` to a folder
2. Copy required Qt6 DLLs from Qt installation (Qt6Core.dll, Qt6Gui.dll, Qt6Widgets.dll, Qt6Multimedia.dll, Qt6Network.dll)
3. Copy Qt plugins directory (platforms, imageformats, etc.)
4. Optionally create an installer using Qt Installer Framework or NSIS
