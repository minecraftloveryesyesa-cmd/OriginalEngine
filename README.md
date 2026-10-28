# OriginalEngine

A small cross-platform game-engine project, starting with a C++20 foundation.

## Current status

- CMake-based C++20 project
- SDL3 sample application with a resizable window
- Close the window or press **Esc** to exit
- GitHub Actions workflow for Windows builds

Rendering, scene management, input abstraction, resources, audio, and physics are not implemented yet.

## Requirements

- CMake 3.24+
- A C++20 compiler
- Git (CMake downloads SDL3 through FetchContent)

## Build on Windows

From a Developer PowerShell or a terminal with CMake and MSVC available:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
.`buildReleaseoriginalengine_app.exe
```

The first configure downloads SDL3, so an internet connection is required.

## Project layout

```text
include/engine/   Public engine headers
src/engine/       Engine implementation
src/main.cpp      Sample application entry point
.github/workflows/ Automated Windows build
```

## Roadmap

1. Establish the build and application lifecycle
2. Add a renderer abstraction and a first 3D rendering backend
3. Add camera, mesh, texture, and shader resources
4. Add scenes/entities and a basic input system
5. Add asset loading, audio, and physics
6. Expand platform builds after the core architecture stabilizes
