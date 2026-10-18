cmake_minimum_required(VERSION 3.21)

project(VoiceAI VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    Multimedia
)

# Source files
set(SOURCES
    src/main.cpp
    src/FloatingOrb.h
    src/FloatingOrb.cpp
    src/AssistantPopup.h
    src/AssistantPopup.cpp
    src/AgentWorker.h
    src/AgentWorker.cpp
    src/ISpeechToText.h
    src/MockSpeechToText.h
    src/MockSpeechToText.cpp
    src/AudioManager.h
    src/AudioManager.cpp
    src/ILLMClient.h
    src/HarnessIPCClient.h
    src/HarnessIPCClient.cpp
    src/MockLLMClient.h
    src/MockLLMClient.cpp
    src/ToolExecutor.h
    src/ToolExecutor.cpp
    src/WindowsIntegration.h
    src/WindowsIntegration.cpp
    src/HotkeyMonitor.h
    src/HotkeyMonitor.cpp
    src/NotificationManager.h
    src/NotificationManager.cpp
)

# Create executable
add_executable(${PROJECT_NAME} WIN32 ${SOURCES})

# Link Qt libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
)

# Set output directory and binary name
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    OUTPUT_NAME "Voice-AI"
)

# Windows specific settings
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        UNICODE
        _UNICODE
    )
    # Link against required Windows libraries for hotkeys and power status
    target_link_libraries(${PROJECT_NAME} PRIVATE
        user32
        shell32
        powrprof
    )
endif()